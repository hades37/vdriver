/**
 * device.c —— vdriver 设备/能力/SoC 信息(强语义,覆盖桩)
 *
 * 决策(实施方案.md):D4 VIRTUAL+UNIFIED;D5 Ascend910B1;D7 AICPU 核数 0。
 * 回答口径依据 runtime 消费点:
 *  - halGetDeviceInfo:runtime.cc:655-731(SoC 推导)、npu_driver.cc:60-90/488(枚举与模式)、
 *    runtime.cc:978/1043(drvGetPlatformInfo → runMode)
 *  - halGetSocVersion:runtime.cc:655-696 直接将字符串交给 GetChipTypeFromPlatform 匹配
 *    platform_config/<SoC名>.ini
 */
#include "vdriver_internal.h"

#include "ascend_hal.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/* ---------------- 日志 ---------------- */
/* VDRIVER_LOG: 1=关键决策日志; 2=追加 SQE hex-dump 等详单 */
static atomic_int log_level_cached = -1;

static int LogLevel(void)
{
    if (atomic_load(&log_level_cached) < 0) {
        const char *env = getenv("VDRIVER_LOG");
        atomic_store(&log_level_cached, (env != NULL) ? atoi(env) : 0);
    }
    return atomic_load(&log_level_cached);
}

void vdriver_debug_log(const char *fmt, ...)
{
    if (LogLevel() < 1) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    (void)fprintf(stderr, "[vdriver] ");
    (void)vfprintf(stderr, fmt, ap);
    (void)fprintf(stderr, "\n");
    va_end(ap);
}

int vdriver_debug_level(void)
{
    return LogLevel();
}

/* ---------------- 设备状态 ---------------- */
static atomic_int g_dev_open[VDRIVER_MAX_DEV_NUM];

int vdriver_device_is_open(uint32_t dev_id)
{
    if (dev_id >= VDRIVER_MAX_DEV_NUM) {
        return 0;
    }
    return atomic_load(&g_dev_open[dev_id]);
}

/* ---------------- 设备打开/关闭 ---------------- */
DLLEXPORT drvError_t halDeviceOpen(uint32_t devid, halDevOpenIn *in, halDevOpenOut *out)
{
    (void)in;
    (void)out; /* 上下文全 reserve,无必须出参(base.h hal_dev_open_out 段) */
    if (devid >= VDRIVER_MAX_DEV_NUM) {
        vdriver_debug_log("halDeviceOpen: invalid devid=%u", devid);
        return DRV_ERROR_INVALID_VALUE;
    }
    atomic_store(&g_dev_open[devid], 1);
    vdriver_debug_log("halDeviceOpen: devid=%u", devid);
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halDeviceClose(uint32_t devid, halDevCloseIn *in)
{
    (void)in;
    if (devid >= VDRIVER_MAX_DEV_NUM) {
        return DRV_ERROR_INVALID_VALUE;
    }
    atomic_store(&g_dev_open[devid], 0);
    vdriver_debug_log("halDeviceClose: devid=%u", devid);
    return DRV_ERROR_NONE;
}

/* ---------------- 设备信息(决策 D4/D5/D7 的回答口径) ---------------- */
DLLEXPORT drvError_t halGetDeviceInfo(uint32_t devId, int32_t moduleType,
                                      int32_t infoType, int64_t *value)
{
    if (value == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    *value = 0;

    switch (moduleType) {
        case MODULE_TYPE_SYSTEM:
            switch (infoType) {
                case INFO_TYPE_ENV:          /* 环境类型,cmodel 桩同答 0 */
                    break;
                case INFO_TYPE_VERSION:      /* 硬件版本字:SoC 名走 halGetSocVersion,此处 0 即可 */
                    break;
                case INFO_TYPE_CORE_NUM:
                    /* 语义陷阱(M3 实测抓到):SYSTEM/CORE_NUM 被 runtime 当 tsNumber
                     * (TS 调度组数,合法 [1,2],runtime.cc:774-784),不是 AI 核数;
                     * AI 核数走 MODULE_TYPE_AICORE/INFO_TYPE_CORE_NUM */
                    *value = 2;
                    break;
                case INFO_TYPE_ADDR_MODE:    /* D4: UNIFIED(flat),命中 RUNTIME_WHEN_NO_VIRTUAL_MODEL_RETURN */
                    *value = (int64_t)ADDR_MODE_UNIFIED;
                    break;
                case INFO_TYPE_RUN_MACH:     /* D4: 虚拟机模式 */
                    *value = (int64_t)RUN_MACHINE_VIRTUAL;
                    break;
                default:                     /* FREQUE/FREQUE_LEVEL/SYS_COUNT 等一律 0 */
                    break;
            }
            break;
        case MODULE_TYPE_AICORE:
            if (infoType == INFO_TYPE_CORE_NUM) {
                *value = VDRIVER_DEF_AICORE_NUM; /* runtime 亦把 CUBE_NUM 重映射到此(api_impl.cc:3621) */
            }
            break;
        case MODULE_TYPE_VECTOR_CORE:
            if (infoType == INFO_TYPE_CORE_NUM) {
                *value = VDRIVER_DEF_VECTOR_CORE_NUM; /* 自动 tiling 需要向量核数(启动日志 vecCoreCnt) */
            }
            break;
        case MODULE_TYPE_AICPU:
            /* D7: 核数 0 → AiCpuTaskSupportCheck 报 FEATURE_NOT_SUPPORT,fail-fast 不崩 */
            *value = 0;
            break;
        default:
            break;
    }
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halGetChipCapability(uint32_t devId, struct halCapabilityInfo *info)
{
    if (info == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    (void)devId;
    (void)memset(info, 0, sizeof(*info));
    return DRV_ERROR_NONE;
}

/* SoC 名首读缓存(M1 评审建议⑤):中途 setenv 不再漂移,变更 SoC 需重启进程 */
DLLEXPORT drvError_t halGetSocVersion(uint32_t devId, char *socVersion, uint32_t len)
{
    static char cached_soc[sizeof(VDRIVER_DEF_SOC_NAME) + 32];
    static atomic_int cached = -1; /* -1 未初始化;ATOMIC_VAR_INIT 在 C17 弃用,直写 */
    if (socVersion == NULL || len == 0U) {
        return DRV_ERROR_INVALID_VALUE;
    }
    (void)devId;
    if (atomic_load(&cached) < 0) {
        const char *env = getenv("VDRIVER_SOC");
        const char *soc = (env != NULL && env[0] != '\0') ? env : VDRIVER_DEF_SOC_NAME;
        (void)snprintf(cached_soc, sizeof(cached_soc), "%s", soc);
        atomic_store(&cached, 1);
        vdriver_debug_log("halGetSocVersion: cache=%s", cached_soc);
    }
    if ((uint32_t)strlen(cached_soc) >= len) {
        vdriver_debug_log("halGetSocVersion: buffer too small len=%u", len);
        return DRV_ERROR_INVALID_VALUE;
    }
    (void)strcpy(socVersion, cached_soc);
    return DRV_ERROR_NONE;
}

/* ---------------- 驱动级信息(dlsym 消费者:atrace/awatchdog/plog/runtime) ---------------- */
DLLEXPORT drvError_t drvGetDevNum(uint32_t *num_dev)
{
    if (num_dev == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    *num_dev = VDRIVER_MAX_DEV_NUM;
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t drvGetDevIDs(uint32_t *devices, uint32_t len)
{
    if (devices == NULL || len < VDRIVER_MAX_DEV_NUM) {
        return DRV_ERROR_INVALID_VALUE;
    }
    devices[0] = 0;
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t drvGetPlatformInfo(uint32_t *info)
{
    if (info == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    /* ONLINE(runtime/pkg_inc/runtime/runtime/dev.h):maxProgramNum 取 4 千万、
       不触发 OFFLINE 的 StreamSyncMode ini 读取(runtime.cc:978/1043) */
    *info = VDRIVER_RUN_MODE_ONLINE;
    return DRV_ERROR_NONE;
}

/* SMMU 查询:独立地址模式下 SSID=0 即可(npu_driver.cc:457 DeviceOpen 内) */
DLLEXPORT DV_OFFLINE DVresult drvMemSmmuQuery(DVdevice device, UINT32 *SSID)
{
    (void)device;
    if (SSID == NULL) {
        return (DVresult)DRV_ERROR_INVALID_VALUE;
    }
    *SSID = 0U;
    return (DVresult)DRV_ERROR_NONE;
}
