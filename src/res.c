/**
 * res.c —— vdriver 资源 ID 分配(stream/sq/cq/notify 等,M2+ 强语义,覆盖桩)
 *
 * 背景(M3 L1 实测):runtime 的 StreamIdAlloc(npu_driver_res.cc:1164-1177)从
 * halResourceIdAlloc 的出参取流 id——桩不写出参导致所有流都拿 id 0,CtrlSQ 流与
 * 主流冲突(SetDevice 回滚)。本模块提供按类型的真分配:递增位图 + 精确释放;
 * res[1] 含 TSDRV_RES_SPECIFIED_ID(ascend_hal_define.h:1150)时尊重指定 id。
 */
#include "vdriver_internal.h"

#include "ascend_hal.h"

#include <pthread.h>
#include <string.h>

#define VD_ID_PER_TYPE 256U /* 每类型资源 id 上限(流/notify 规模足够) */
#define VD_ID_TYPE_NUM (uint32_t)DRV_INVALID_ID

static uint8_t g_res_used[VD_ID_TYPE_NUM][VD_ID_PER_TYPE];
static uint32_t g_res_hint[VD_ID_TYPE_NUM]; /* 分配起点提示,避免 O(n) 扫描开头 */
static pthread_mutex_t g_res_lock = PTHREAD_MUTEX_INITIALIZER;

static int ResTypeValid(uint32_t type)
{
    return type < VD_ID_TYPE_NUM;
}

DLLEXPORT drvError_t halResourceIdAlloc(uint32_t devId, struct halResourceIdInputInfo *in,
                                        struct halResourceIdOutputInfo *out)
{
    (void)devId;
    if (in == NULL || out == NULL || !ResTypeValid((uint32_t)in->type)) {
        return DRV_ERROR_INVALID_VALUE;
    }
    (void)memset(out, 0, sizeof(*out));

    pthread_mutex_lock(&g_res_lock);
    const uint32_t type = (uint32_t)in->type;
    const int specified = ((in->res[1U] & TSDRV_RES_SPECIFIED_ID) != 0U);

    uint32_t id = VD_ID_PER_TYPE;
    if (specified) {
        /* runtime 指定 id(如 SQ/CQ 复用场景):校验后占用 */
        if (in->resourceId < VD_ID_PER_TYPE && g_res_used[type][in->resourceId] == 0U) {
            id = in->resourceId;
        }
    } else {
        for (uint32_t probe = 0U; probe < VD_ID_PER_TYPE; probe++) {
            const uint32_t cand = (g_res_hint[type] + probe) % VD_ID_PER_TYPE;
            if (g_res_used[type][cand] == 0U) {
                id = cand;
                break;
            }
        }
    }
    if (id == VD_ID_PER_TYPE) {
        pthread_mutex_unlock(&g_res_lock);
        vdriver_debug_log("halResourceIdAlloc: type=%u 资源耗尽", type);
        return DRV_ERROR_INVALID_VALUE;
    }
    g_res_used[type][id] = 1U;
    g_res_hint[type] = (id + 1U) % VD_ID_PER_TYPE;
    pthread_mutex_unlock(&g_res_lock);

    out->resourceId = id;
    vdriver_debug_log("halResourceIdAlloc: type=%u -> id=%u (specified=%d)", type, id, specified);
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halResourceIdFree(uint32_t devId, struct halResourceIdInputInfo *in)
{
    (void)devId;
    if (in == NULL || !ResTypeValid((uint32_t)in->type)) {
        return DRV_ERROR_INVALID_VALUE;
    }
    if (in->resourceId >= VD_ID_PER_TYPE) {
        return DRV_ERROR_INVALID_VALUE;
    }
    pthread_mutex_lock(&g_res_lock);
    g_res_used[(uint32_t)in->type][in->resourceId] = 0U;
    pthread_mutex_unlock(&g_res_lock);
    vdriver_debug_log("halResourceIdFree: type=%u id=%u", (uint32_t)in->type, in->resourceId);
    return DRV_ERROR_NONE;
}
