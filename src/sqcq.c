/**
 * sqcq.c —— vdriver SQ/CQ 记账与任务下发(强语义,覆盖桩)
 *
 * 模型(实施方案.md D6/§4.4):
 *  - halSqTaskSend:逐条解释执行 SQE(瞬间完成),tail += n 且 head = tail;
 *  - halSqCqQuery:SQ_HEAD 回 head,SQ_STATUS 恒"正常"(非 0),SQ_CQE_STATUS=0(无 CQE);
 *    注意结果直接写回入参 info->value[](npu_driver_res.cc:780-856 的用法);
 *  - halCqReportGet:恒 count=0 —— 任务回收走 FinishedTaskReclaim 的 sqHead 路径,
 *    完全不产 CQE(避开 phase 翻转/布局坑);
 *  - halCqReportIrqWait:立即返回、bitmap 清零(无中断源)。
 * 消费点:npu_driver_res.cc:780-856/2004/2096-2367、stars_engine.cc:395/709。
 */
#include "vdriver_internal.h"

#include "ascend_hal.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define VD_MAX_SQCQ 1024U /* 同时存在的 SQ/CQ 数上限(torch_npu 初始化会建大量内部流,M3 L4 实测)*/

typedef struct {
    int in_use;
    drvSqCqType_t type;
    uint32_t ts_id;
    uint32_t sqe_size;
    uint32_t sqe_depth;
    uint32_t head; /* 已消费(TS 执行完) */
    uint32_t tail; /* 已生产(runtime 写入) */
    uint8_t *ring; /* host SQ 环(SQ_REG_BASE 应答用;某些路径 runtime 直接写环) */
    uint32_t ring_len;
} vd_sq_t;

typedef struct {
    int in_use;
    drvSqCqType_t type;
    uint32_t ts_id;
} vd_cq_t;

static vd_sq_t g_sq_table[VD_MAX_SQCQ];
static vd_cq_t g_cq_table[VD_MAX_SQCQ];
static pthread_mutex_t g_sqcq_lock = PTHREAD_MUTEX_INITIALIZER;

/* 槽位下标即 id(Allocate 回填);按 type/tsId 校验归属,调用方持锁 */
static vd_sq_t *GetSqLocked(uint32_t sq_id, uint32_t type, uint32_t ts_id)
{
    if (sq_id >= VD_MAX_SQCQ || !g_sq_table[sq_id].in_use) {
        return NULL;
    }
    vd_sq_t *sq = &g_sq_table[sq_id];
    if ((uint32_t)sq->type != type || sq->ts_id != ts_id) {
        return NULL;
    }
    return sq;
}

static uint32_t AllocSlotLocked(int is_sq)
{
    for (uint32_t i = 0; i < VD_MAX_SQCQ; i++) {
        const int used = is_sq ? g_sq_table[i].in_use : g_cq_table[i].in_use;
        if (!used) {
            return i;
        }
    }
    return VD_MAX_SQCQ; /* 满 */
}

DLLEXPORT drvError_t halSqCqAllocate(uint32_t devId, struct halSqCqInputInfo *in,
                                     struct halSqCqOutputInfo *out)
{
    if (vdriver_device_is_open(devId) != 1) {
        vdriver_debug_log("halSqCqAllocate: devId=%u 未打开", devId);
        return DRV_ERROR_INVALID_VALUE;
    }
    if (in == NULL || out == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    (void)memset(out, 0, sizeof(*out));

    pthread_mutex_lock(&g_sqcq_lock);
    /* flag 语义按 ascend_hal_define.h:1128-1136(M3 L4 实测修正:trs_pkg.h 注释
     * 的 bit0/bit1 指 REUSE_CQ/REUSE_SQ;指定 id 是 SPECIFIED_SQ_ID(bit7)/
     * SPECIFIED_CQ_ID(bit8))。指定时占用该槽位,否则分配空槽;REUSE 位在
     * mock 下按普通分配处理(每流独立环,不影响流程)。 */
    uint32_t sq_id = ((in->flag & TSDRV_FLAG_SPECIFIED_SQ_ID) != 0U) ? in->sqId : AllocSlotLocked(1);
    uint32_t cq_id = ((in->flag & TSDRV_FLAG_SPECIFIED_CQ_ID) != 0U) ? in->cqId : AllocSlotLocked(0);
    if (sq_id >= VD_MAX_SQCQ || cq_id >= VD_MAX_SQCQ ||
        g_sq_table[sq_id].in_use || g_cq_table[cq_id].in_use) {
        pthread_mutex_unlock(&g_sqcq_lock);
        vdriver_debug_log("halSqCqAllocate: 资源耗尽或 id 冲突 sq=%u cq=%u", sq_id, cq_id);
        return DRV_ERROR_INVALID_VALUE; /* 上层有失败重试路径(npu_driver_res.cc:1577) */
    }

    vd_sq_t *sq = &g_sq_table[sq_id];
    (void)memset(sq, 0, sizeof(*sq));
    sq->in_use = 1;
    sq->type = in->type;
    sq->ts_id = in->tsId;
    sq->sqe_size = (in->sqeSize != 0U) ? in->sqeSize : 64U; /* normal: 64B(trs_pkg.h) */
    sq->sqe_depth = (in->sqeDepth != 0U) ? in->sqeDepth : 1024U;
    /* SQ 环内存:GetSqRegVirtualAddrBySqid(npu_driver_res.cc:1104)要求非 0 地址 */
    sq->ring_len = sq->sqe_depth * sq->sqe_size;
    sq->ring = (uint8_t *)malloc(sq->ring_len);
    if (sq->ring == NULL) {
        sq->in_use = 0;
        pthread_mutex_unlock(&g_sqcq_lock);
        return DRV_ERROR_INVALID_VALUE;
    }
    (void)memset(sq->ring, 0, sq->ring_len);

    vd_cq_t *cq = &g_cq_table[cq_id];
    (void)memset(cq, 0, sizeof(*cq));
    cq->in_use = 1;
    cq->type = in->type;
    cq->ts_id = in->tsId;
    pthread_mutex_unlock(&g_sqcq_lock);

    out->sqId = sq_id;
    out->cqId = cq_id;
    out->queueVAddr = 0ULL; /* 无 shm 环,SQE 经指针直传 */
    vdriver_debug_log("halSqCqAllocate: type=%d tsId=%u sqId=%u cqId=%u sqeDepth=%u",
                      (int)in->type, in->tsId, out->sqId, out->cqId, sq->sqe_depth);
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halSqCqFree(uint32_t devId, struct halSqCqFreeInfo *info)
{
    (void)devId;
    if (info == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    pthread_mutex_lock(&g_sqcq_lock);
    vd_sq_t *sq = GetSqLocked(info->sqId, (uint32_t)info->type, info->tsId);
    if (sq != NULL) {
        sq->in_use = 0;
        free(sq->ring); /* 释放 SQ 环 */
        sq->ring = NULL;
        sq->ring_len = 0U;
    }
    /* ONLY_SQCQ_ID(ascend_hal_define.h:1132)置位时仅释放 SQ,不级联释放 CQ */
    if ((info->flag & TSDRV_FLAG_ONLY_SQCQ_ID) == 0U && info->cqId < VD_MAX_SQCQ) {
        g_cq_table[info->cqId].in_use = 0;
    }
    pthread_mutex_unlock(&g_sqcq_lock);
    vdriver_debug_log("halSqCqFree: sqId=%u cqId=%u", info->sqId, info->cqId);
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halSqCqQuery(uint32_t devId, struct halSqCqQueryInfo *info)
{
    (void)devId;
    if (info == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    pthread_mutex_lock(&g_sqcq_lock);
    const vd_sq_t *sq = GetSqLocked(info->sqId, (uint32_t)info->type, info->tsId);
    const uint32_t head = (sq != NULL) ? sq->head : 0U;
    const uint32_t tail = (sq != NULL) ? sq->tail : 0U;

    switch (info->prop) {
        case DRV_SQCQ_PROP_SQ_HEAD:
            info->value[0] = head;
            break;
        case DRV_SQCQ_PROP_SQ_TAIL:
            info->value[0] = tail;
            break;
        case DRV_SQCQ_PROP_SQ_STATUS:
            info->value[0] = 1U; /* 非 0 = 正常;同步路径据 pending 判完成 */
            break;
        case DRV_SQCQ_PROP_SQ_CQE_STATUS: /* read clear:无 CQE,恒 0 */
            info->value[0] = 0U;
            break;
        case DRV_SQCQ_PROP_SQ_REG_BASE:
            /* GetSqRegVirtualAddrBySqid(npu_driver_res.cc:1096-1120):
             * value[0]=高 32 位, value[1]=低 32 位, value[2]=长度 */
            if (sq != NULL && sq->ring != NULL) {
                const uint64_t addr = (uint64_t)(uintptr_t)sq->ring;
                info->value[0] = (uint32_t)(addr >> 32U);
                info->value[1] = (uint32_t)(addr & 0xFFFFFFFFULL);
                info->value[2] = sq->ring_len;
            } else {
                info->value[0] = 0U;
                info->value[1] = 0U;
                info->value[2] = 0U;
            }
            break;
        case DRV_SQCQ_PROP_SQ_DEPTH:
            info->value[0] = (sq != NULL) ? sq->sqe_depth : 0U;
            break;
        case DRV_SQCQ_PROP_SQE_SIZE:
            info->value[0] = (sq != NULL) ? sq->sqe_size : 0U;
            break;
        default:
            info->value[0] = 0U;
            break;
    }
    pthread_mutex_unlock(&g_sqcq_lock);
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halSqCqConfig(uint32_t devId, struct halSqCqConfigInfo *info)
{
    (void)devId;
    if (info == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    pthread_mutex_lock(&g_sqcq_lock);
    if (info->prop == DRV_SQCQ_PROP_SQ_HEAD) {
        /* SetSqHead(npu_driver_res.cc:830):销毁/复用时把 head 拉回指定位置;
         * M2 评审建议④:mock 下 head=tail 同步回拉,避免 head<tail 的未定语义 */
        vd_sq_t *sq = GetSqLocked(info->sqId, (uint32_t)info->type, info->tsId);
        if (sq != NULL) {
            sq->head = info->value[0] & 0xFFFFU;
            sq->tail = sq->head;
        }
    } else if (info->prop != DRV_SQCQ_PROP_SQ_PAUSE &&
               info->prop != DRV_SQCQ_PROP_SQ_RESUME &&
               info->prop != DRV_SQCQ_PROP_SQ_DISABLE_TO_ENABLE) {
        vdriver_debug_log("halSqCqConfig: 未实现的 prop=%d 已忽略", (int)info->prop);
    }
    pthread_mutex_unlock(&g_sqcq_lock);
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halSqTaskSend(uint32_t devId, struct halTaskSendInfo *info)
{
    if (vdriver_device_is_open(devId) != 1) {
        vdriver_debug_log("halSqTaskSend: devId=%u 未打开", devId);
        return DRV_ERROR_INVALID_VALUE;
    }
    if (info == NULL || info->sqe_addr == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    if (info->sqe_num == 0U || info->sqe_num > 1024U) {
        vdriver_debug_log("halSqTaskSend: 非法 sqe_num=%u", info->sqe_num);
        return DRV_ERROR_INVALID_VALUE; /* M2 评审建议⑤:0/上限校验 */
    }

    pthread_mutex_lock(&g_sqcq_lock);
    vd_sq_t *sq = GetSqLocked(info->sqId, (uint32_t)info->type, info->tsId);
    if (sq == NULL) {
        pthread_mutex_unlock(&g_sqcq_lock);
        vdriver_debug_log("halSqTaskSend: 未知 sqId=%u(type=%d tsId=%u)",
                          info->sqId, (int)info->type, info->tsId);
        return DRV_ERROR_INVALID_VALUE;
    }
    if (sq->tail + info->sqe_num - sq->head > sq->sqe_depth) {
        pthread_mutex_unlock(&g_sqcq_lock);
        vdriver_debug_log("halSqTaskSend: 超过环深度 pending=%u num=%u depth=%u",
                          sq->tail - sq->head, info->sqe_num, sq->sqe_depth);
        return DRV_ERROR_INVALID_VALUE;
    }
    const uint32_t first_pos = sq->tail % sq->sqe_depth;
    const uint32_t stride = (sq->sqe_size != 0U) ? sq->sqe_size : 64U; /* M2 评审建议⑤ */
    sq->tail += info->sqe_num;
    pthread_mutex_unlock(&g_sqcq_lock);

    /* M2 评审严重①:先解释执行,再发布 head=tail——异步 waiter 等到效果落地才算完成 */
    if (vdriver_debug_level() >= 2) {
        /* 布局取证:hex-dump 真实下发的 SQE(评审严重②的 M3 门禁证据) */
        for (uint32_t i = 0; i < info->sqe_num && i < 4U; i++) {
            const uint8_t *s = info->sqe_addr + (size_t)i * stride;
            fprintf(stderr, "[vdriver][SQE %u/%u] %02x %02x %02x %02x %02x %02x %02x %02x | "
                    "%02x %02x %02x %02x %02x %02x %02x %02x | %02x %02x %02x %02x %02x %02x %02x %02x | "
                    "%02x %02x %02x %02x %02x %02x %02x %02x | "
                    "%02x %02x %02x %02x %02x %02x %02x %02x | %02x %02x %02x %02x %02x %02x %02x %02x | "
                    "%02x %02x %02x %02x %02x %02x %02x %02x | %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    i, info->sqe_num, s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
                    s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15],
                    s[16], s[17], s[18], s[19], s[20], s[21], s[22], s[23],
                    s[24], s[25], s[26], s[27], s[28], s[29], s[30], s[31],
                    s[32], s[33], s[34], s[35], s[36], s[37], s[38], s[39],
                    s[40], s[41], s[42], s[43], s[44], s[45], s[46], s[47],
                    s[48], s[49], s[50], s[51], s[52], s[53], s[54], s[55],
                    s[56], s[57], s[58], s[59], s[60], s[61], s[62], s[63]);
        }
    }
    for (uint32_t i = 0; i < info->sqe_num; i++) {
        sqe_interp_execute(info->sqe_addr + (size_t)i * stride);
    }

    pthread_mutex_lock(&g_sqcq_lock);
    sq->head = sq->tail;
    pthread_mutex_unlock(&g_sqcq_lock);

    info->pos = first_pos; /* 出参:首 SQE 位置(trs_pkg.h:153) */
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halCqReportIrqWait(uint32_t devId, struct halReportInfoInput *in,
                                        struct halReportInfoOutput *out)
{
    (void)devId;
    (void)in;
    if (out == NULL || out->cqIdBitmap == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    for (uint32_t i = 0; i < out->cqIdBitmapSize; i++) {
        out->cqIdBitmap[i] = 0ULL; /* 无任何 CQ 有完成中断 */
    }
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halCqReportGet(uint32_t devId, struct halReportGetInput *in,
                                    struct halReportGetOutput *out)
{
    (void)devId;
    (void)in;
    if (out == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    out->count = 0U;    /* D6:不产 CQE,上层 continue 后走 sqHead 回收 */
    out->reportPtr = NULL;
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halReportRelease(uint32_t devId, struct halReportReleaseInfo *info)
{
    (void)devId;
    (void)info;
    return DRV_ERROR_NONE;
}
