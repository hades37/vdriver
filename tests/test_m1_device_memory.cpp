/**
 * test_m1_device_memory —— M1 验收:设备/内存强语义符号契约测试
 *
 * 直接链接 libascend_hal.so(vdriver),按调研报告1 详单逐项断言:
 *  - 决策 D4: SYSTEM/INFO_TYPE_ADDR_MODE=UNIFIED、INFO_TYPE_RUN_MACH=VIRTUAL
 *  - 决策 D5: halGetSocVersion → Ascend910B1(存在对应 ini)
 *  - 决策 D7: MODULE_TYPE_AICPU/INFO_TYPE_CORE_NUM = 0(fail-fast)
 *  - 内存:512B 对齐、真拷贝(memmove)、头部记账、坏指针拒绝
 */
#include "ascend_hal.h"
#include "vdriver_internal.h"

#include <cstdio>
#include <cstring>
#include <cstdint>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("[PASS] %s\n", msg); } \
    else { printf("[FAIL] %s\n", msg); g_fail++; } \
} while (0)

int main()
{
    /* ---- 驱动级枚举 ---- */
    uint32_t num = 99;
    CHECK(drvGetDevNum(&num) == DRV_ERROR_NONE && num == 1, "drvGetDevNum=1");
    CHECK(drvGetDevNum(nullptr) == DRV_ERROR_INVALID_VALUE, "drvGetDevNum 空参拒绝");

    uint32_t ids[1] = {0xEE};
    CHECK(drvGetDevIDs(ids, 1) == DRV_ERROR_NONE && ids[0] == 0, "drvGetDevIDs={0}");

    /* ---- 设备打开/关闭 ---- */
    halDevOpenIn in = {};
    halDevOpenOut out = {};
    CHECK(halDeviceOpen(0, &in, &out) == DRV_ERROR_NONE, "halDeviceOpen(0)");
    CHECK(halDeviceOpen(1, &in, &out) == DRV_ERROR_INVALID_VALUE, "halDeviceOpen 非法设备号拒绝");
    CHECK(vdriver_device_is_open(0) == 1, "设备打开状态可查");

    /* ---- SoC 版本(D5) ---- */
    char soc[64] = {0};
    CHECK(halGetSocVersion(0, soc, sizeof(soc)) == DRV_ERROR_NONE &&
          strcmp(soc, VDRIVER_DEF_SOC_NAME) == 0, "halGetSocVersion=Ascend910B1");
    CHECK(halGetSocVersion(0, soc, 4) == DRV_ERROR_INVALID_VALUE, "halGetSocVersion 小缓冲拒绝");

    /* ---- 设备信息(D4/D5/D7 口径) ---- */
    int64_t v = -1;
    CHECK(halGetDeviceInfo(0, MODULE_TYPE_SYSTEM, INFO_TYPE_RUN_MACH, &v) == DRV_ERROR_NONE &&
          v == (int64_t)RUN_MACHINE_VIRTUAL, "RUN_MACH=VIRTUAL(D4)");
    v = -1;
    CHECK(halGetDeviceInfo(0, MODULE_TYPE_SYSTEM, INFO_TYPE_ADDR_MODE, &v) == DRV_ERROR_NONE &&
          v == (int64_t)ADDR_MODE_UNIFIED, "ADDR_MODE=UNIFIED(D4)");
    v = -1;
    CHECK(halGetDeviceInfo(0, MODULE_TYPE_AICORE, INFO_TYPE_CORE_NUM, &v) == DRV_ERROR_NONE &&
          v == VDRIVER_DEF_AICORE_NUM, "AICORE 核数=平台规格");
    v = 99;
    CHECK(halGetDeviceInfo(0, MODULE_TYPE_AICPU, INFO_TYPE_CORE_NUM, &v) == DRV_ERROR_NONE &&
          v == 0, "AICPU 核数=0(D7 fail-fast)");
    v = -1;
    CHECK(halGetDeviceInfo(0, MODULE_TYPE_SYSTEM, INFO_TYPE_VERSION, &v) == DRV_ERROR_NONE &&
          v == 0, "VERSION=0(SoC 名走 halGetSocVersion)");
    CHECK(halGetDeviceInfo(0, MODULE_TYPE_SYSTEM, INFO_TYPE_ENV, nullptr) == DRV_ERROR_INVALID_VALUE,
          "halGetDeviceInfo 空出参拒绝");

    /* ---- 能力/SMMU/平台模式 ---- */
    struct halCapabilityInfo cap = {};
    cap.sdma_reduce_support = 0xFFFF;
    CHECK(halGetChipCapability(0, &cap) == DRV_ERROR_NONE && cap.sdma_reduce_support == 0 &&
          cap.memory_support == 0, "halGetChipCapability 全零");
    uint32_t ssid = 0xEE;
    CHECK(drvMemSmmuQuery(0, &ssid) == (DVresult)DRV_ERROR_NONE && ssid == 0, "drvMemSmmuQuery SSID=0");
    uint32_t runMode = 0xEE;
    CHECK(drvGetPlatformInfo(&runMode) == DRV_ERROR_NONE && runMode == VDRIVER_RUN_MODE_ONLINE,
          "drvGetPlatformInfo=ONLINE");

    /* ---- 内存:分配/对齐/真拷贝/记账 ---- */
    void *dev = nullptr;
    CHECK(halMemAlloc(&dev, 4096, 0) == DRV_ERROR_NONE && dev != nullptr,
          "halMemAlloc 4096");
    CHECK(((uintptr_t)dev & 511ULL) == 0, "设备指针 512B 对齐");

    const char pattern[256] = "vdriver-memcpy-pattern";
    uint8_t host_back[256] = {0};
    struct memcpy_info info = {};
    info.dir = DRV_MEMCPY_HOST_TO_DEVICE;
    CHECK(halMemcpy(dev, 4096, (void *)pattern, sizeof(pattern), &info) == DRV_ERROR_NONE,
          "halMemcpy H2D");

    info.dir = DRV_MEMCPY_DEVICE_TO_HOST;
    CHECK(halMemcpy(host_back, sizeof(host_back), dev, sizeof(pattern), &info) == DRV_ERROR_NONE &&
          memcmp(host_back, pattern, sizeof(pattern)) == 0, "halMemcpy D2H 真拷贝回读一致");

    info.dir = DRV_MEMCPY_DEVICE_TO_DEVICE;
    void *dev2 = nullptr;
    CHECK(halMemAlloc(&dev2, 1024, 0) == DRV_ERROR_NONE, "halMemAlloc 1024");
    uint8_t back2[256] = {0};
    CHECK(halMemcpy(dev2, 1024, dev, 256, &info) == DRV_ERROR_NONE, "halMemcpy D2D");
    info.dir = DRV_MEMCPY_DEVICE_TO_HOST;
    CHECK(halMemcpy(back2, sizeof(back2), dev2, 256, &info) == DRV_ERROR_NONE &&
          memcmp(back2, pattern, sizeof(pattern)) == 0, "D2D 拷贝内容一致");

    info.dir = DRV_MEMCPY_HOST_TO_DEVICE;
    CHECK(halMemcpy(dev, 16, (void *)pattern, sizeof(pattern), &info) == DRV_ERROR_INVALID_VALUE,
          "count>dst_size 拒绝");
    CHECK(halMemcpy(nullptr, 0, (void *)pattern, 1, &info) == DRV_ERROR_INVALID_VALUE,
          "halMemcpy 空指针拒绝");

    CHECK(halMemFree(dev) == DRV_ERROR_NONE, "halMemFree");
    CHECK(halMemFree(dev2) == DRV_ERROR_NONE, "halMemFree(第二块)");
    CHECK(halMemFree(nullptr) == DRV_ERROR_NONE, "halMemFree 空指针幂等");
    {
        int stack_var = 0;
        CHECK(halMemFree(&stack_var) == DRV_ERROR_INVALID_VALUE, "halMemFree 非 vdriver 指针拒绝");
    }
    void *zero = (void *)0x1;
    CHECK(halMemAlloc(&zero, 0, 0) == DRV_ERROR_INVALID_VALUE, "size=0 拒绝");

    /* ---- Host 注册 ---- */
    void *mapped = nullptr;
    CHECK(halHostRegister((void *)host_back, sizeof(host_back), 0, 0, &mapped) == DRV_ERROR_NONE &&
          mapped == (void *)host_back, "halHostRegister 身份映射");

    /* ---- 关闭 ---- */
    halDevCloseIn close_in = {};
    CHECK(halDeviceClose(0, &close_in) == DRV_ERROR_NONE, "halDeviceClose(0)");

    printf("---- M1 %s ----\n", g_fail == 0 ? "全部通过" : "存在失败项");
    return g_fail == 0 ? 0 : 1;
}
