/**
 * vdriver.c —— 虚拟驱动全局态入口(M0:仅版本标记)
 *
 * 后续里程碑在此扩展:句柄表/锁/pthread_atfork(实施方案.md §4.1 vdriver.c)。
 * 版本标记用途:test_m0_load 通过 dlsym 确认加载的是 vdriver 而非官方桩。
 */
#include <stddef.h>

/* NpuDriver 等消费方仅依赖 hal、drv 系列弱符号直链,此符号仅测试用 */
const char *vdriver_version(void)
{
    return "vdriver 0.1.0 (M0 skeleton)";
}
