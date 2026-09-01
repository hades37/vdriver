/**
 * vdriver_internal.h —— vdriver 内部共享声明(仅 src/ 模块与测试使用)
 *
 * 决策引用见 实施方案.md:
 *  D4: RUN_MACHINE_VIRTUAL + ADDR_MODE_UNIFIED(短路 HostAddrRegister,CQ 深 256)
 *  D5: SoC = Ascend910B1(必须存在对应 platform_config ini,tiling 才能初始化)
 *  D7: AICPU 核数 0 → 上层 fail-fast(RT_ERROR_FEATURE_NOT_SUPPORT),免 tsd 桩
 */
#ifndef VDRIVER_INTERNAL_H
#define VDRIVER_INTERNAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 最多模拟设备数(与 cmodel_driver 的 MAX_DEV_NUM=1 同口径) */
#define VDRIVER_MAX_DEV_NUM 1U

/* 设备默认参数(可用环境变量覆盖,便于不同 SoC 复用) */
#define VDRIVER_DEF_SOC_NAME    "Ascend910B1"
#define VDRIVER_DEF_AICORE_NUM  20  /* Ascend910B1 平台规格 */

/* RT_RUN_MODE(runtime/pkg_inc/runtime/runtime/dev.h):ONLINE=真实在线运行时 */
#define VDRIVER_RUN_MODE_ONLINE 1U

/* 错误输出:仅在 VDRIVER_LOG=1 时打印,避免污染上层日志流 */
void vdriver_debug_log(const char *fmt, ...);

/* 设备模块( device.c):记录设备打开状态,供内存/后续 SQCQ 模块校验 */
int vdriver_device_is_open(uint32_t dev_id);

#ifdef __cplusplus
}
#endif

#endif /* VDRIVER_INTERNAL_H */
