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
#include <string.h>

#define VD_MAX_SQCQ 64U /* 同时存在的 SQ/CQ 数上限 */

typedef struct {
    int in_use;
    drvSqCqType_t type;
    uint32_t ts_id;
    uint32_t sqe_size;
    uint32_t sqe_depth;
    uint32_t head; /* 已消费(TS 执行完) */
    uint32_t tail; /* 已生产(runtime 写入) */
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
    /* flag bit1=1:runtime 指定 sqId;bit0=1:指定 cqId(trs_pkg.h halSqCqInputInfo 注释);
       指定 id 时直接占用该槽位,未指定则分配空槽 */
    uint32_t sq_id = (in->flag & 0x2U) ? in->sqId : AllocSlotLocked(1);
    uint32_t cq_id = (in->flag & 0x1U) ? in->cqId : AllocSlotLocked(0);
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
    }
    /* flag bit0=0 表示要释放 cq(trs_pkg.h halSqCqFreeInfo 注释) */
    if ((info->flag & 0x1U) == 0U && info->cqId < VD_MAX_SQCQ) {
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
        /* SetSqHead(npu_driver_res.cc:830):销毁/复用时把 head 拉回指定位置 */
        vd_sq_t *sq = GetSqLocked(info->sqId, (uint32_t)info->type, info->tsId);
        if (sq != NULL) {
            sq->head = info->value[0] & 0xFFFFU;
        }
    }
    /* SQ_PAUSE/SQ_RESUME/SQ_DISABLE_TO_ENABLE:mock 下无实际效果 */
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

    pthread_mutex_lock(&g_sqcq_lock);
    vd_sq_t *sq = GetSqLocked(info->sqId, (uint32_t)info->type, info->tsId);
    if (sq == NULL) {
        pthread_mutex_unlock(&g_sqcq_lock);
        vdriver_debug_log("halSqTaskSend: 未知 sqId=%u(type=%d tsId=%u)",
                          info->sqId, (int)info->type, info->tsId);
        return DRV_ERROR_INVALID_VALUE;
    }
    const uint32_t first_pos = sq->tail % sq->sqe_depth;
    sq->tail += info->sqe_num;
    sq->head = sq->tail; /* D6:解释执行瞬时完成 */
    pthread_mutex_unlock(&g_sqcq_lock);

    for (uint32_t i = 0; i < info->sqe_num; i++) {
        sqe_interp_execute(info->sqe_addr + (size_t)i * 64U);
    }
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
