/**
 * vdriver_internal.h —— vdriver 内部共享声明(仅 src/ 模块与测试使用)
 *
 * 决策引用见 实施方案.md:
 *  D4: RUN_MACHINE_VIRTUAL + ADDR_MODE_UNIFIED(短路 HostAddrRegister,CQ 深 256)
 *  D5: SoC = Ascend910B1(安装包 data/platform_config/Ascend910B1.ini 已确认存在)
 *  D7: AICPU 核数 0 → 上层 fail-fast(RT_ERROR_FEATURE_NOT_SUPPORT),免 tsd 桩
 *  D8: 每进程独立全局态,进程内加锁
 *
 * 可见性:全库 -fvisibility=hidden,仅 DLLEXPORT 契约符号与 VDRIVER_API 内部符号导出
 * (M1 评审风格项:消除 vdrv_stub_* 泄漏)。
 */
#ifndef VDRIVER_INTERNAL_H
#define VDRIVER_INTERNAL_H

#include <stdint.h>

#ifdef __cplusplus
#define VDRIVER_API extern "C" __attribute__((visibility("default")))
#else
#define VDRIVER_API __attribute__((visibility("default")))
#endif

/* 最多模拟设备数(与 cmodel_driver 的 MAX_DEV_NUM=1 同口径) */
#define VDRIVER_MAX_DEV_NUM 1U

/* 设备默认参数(VDRIVER_SOC 环境变量可覆盖,首读缓存;变更 SoC 需重启进程) */
#define VDRIVER_DEF_SOC_NAME    "Ascend910B1" /* data/platform_config/Ascend910B1.ini 已确认存在 */
#define VDRIVER_DEF_AICORE_NUM  20            /* Ascend910B1 平台规格 */

/* RT_RUN_MODE(runtime/pkg_inc/runtime/runtime/dev.h):ONLINE=真实在线运行时 */
#define VDRIVER_RUN_MODE_ONLINE 1U

VDRIVER_API void vdriver_debug_log(const char *fmt, ...);

/* 设备模块(device.c):设备打开状态;sqcq.c 在 Allocate/TaskSend 时校验 */
VDRIVER_API int vdriver_device_is_open(uint32_t dev_id);

/* 内存模块(memory.c):按地址反查归属与长度(M2 SQE 代执行/诊断用) */
VDRIVER_API int vdriver_mem_query(const void *user_ptr, uint64_t *size);

/*
 * SQE 解释器(sqe_interp.cpp):执行一条 64B STARS SQE(仅内存效果类真执行)。
 * 导出经链接脚本 src/vdriver.map 的 sqe_interp* 通配,无需可见性属性。
 */
VDRIVER_API void sqe_interp_execute(const uint8_t *sqe);
VDRIVER_API uint64_t sqe_interp_stat_kernel(void);
VDRIVER_API uint64_t sqe_interp_stat_memcpy(void);
VDRIVER_API uint64_t sqe_interp_stat_ignored(void);
VDRIVER_API void sqe_interp_stat_reset(void);

#endif /* VDRIVER_INTERNAL_H */
