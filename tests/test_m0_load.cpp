/**
 * test_m0_load —— M0 验收:dlopen 同名命中 vdriver、SONAME、符号契约抽查
 *
 * 用法:test_m0_load <vdriver_lib_dir>
 * 前提:进程启动前 LD_LIBRARY_PATH 已前置 vdriver 目录(glibc 启动时固化搜索路径,
 *       进程内 setenv 对 dlopen 无效;ctest 通过 ENVIRONMENT 属性注入)。
 *
 * 验收点(实施方案.md M0 / 评审修复版):
 *  1) dlopen("libascend_hal.so", RTLD_LOCAL) 命中 vdriver 而非真包/官方桩(realpath 判定);
 *  2) 符号契约抽查(必须实现集 + dlsym 旁路消费者符号);
 *  3) 桩语义:halGetAPIVersion/halGetDeviceInfo 返回 DRV_ERROR_NONE(M0 桩不写出参,
 *     出参写入断言由 M1 强语义测试覆盖);
 *  4) vdriver_version 身份识别。
 */
#include "ascend_hal.h"

#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <cstring>
#include <cstdint>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("[PASS] %s\n", msg); } \
    else { printf("[FAIL] %s\n", msg); g_fail++; } \
} while (0)

static void Realpath(const char *path, char *resolved, size_t len)
{
    if (realpath(path, resolved) == nullptr) {
        /* 路径尚不存在等场景,退化为原样 */
        snprintf(resolved, len, "%s", path);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s <vdriver_lib_dir>\n", argv[0]);
        return 2;
    }
    char want[PATH_MAX] = {0};
    Realpath(argv[1], want, sizeof(want));

    /* 1. 环境说明:LD_LIBRARY_PATH 由启动环境前置(见文件头注释) */
    const char *ldp = getenv("LD_LIBRARY_PATH");
    printf("[INFO] 启动时 LD_LIBRARY_PATH: %s\n", ldp ? ldp : "(空)");

    /* 2. 按 DT_NEEDED 同名方式加载;RTLD_LOCAL:不并入全局作用域(评审建议项) */
    void *h = dlopen("libascend_hal.so", RTLD_NOW | RTLD_LOCAL);
    if (h == nullptr) {
        printf("[FAIL] dlopen libascend_hal.so: %s\n", dlerror());
        return 1;
    }
    printf("[PASS] dlopen libascend_hal.so\n");

    /* 3. 命中的必须是 vdriver(realpath 比较,免疫 symlink/相对路径) */
    void *self = dlsym(h, "vdriver_version");
    CHECK(self != nullptr, "vdriver_version 可解析(证明加载的是 vdriver)");
    if (self != nullptr) {
        Dl_info info = {};
        if (dladdr(self, &info) != 0 && info.dli_fname != nullptr) {
            char got[PATH_MAX] = {0};
            Realpath(info.dli_fname, got, sizeof(got));
            printf("[INFO] so 路径: %s\n", got);
            /* got 是 .so 文件路径,want 是目录:做目录前缀比较 */
            const size_t want_len = strlen(want);
            CHECK(strncmp(want, got, want_len) == 0 &&
                  (got[want_len] == '/' || got[want_len] == '\0'),
                  "命中路径位于 vdriver 目录");
        } else {
            CHECK(false, "dladdr 获取 so 路径失败");
        }
        printf("[INFO] 版本: %s\n", ((const char *(*)(void))self)());
    }

    /* 4. 符号契约抽查(必须实现集 + dlsym 旁路消费者,见调研报告1/3) */
    CHECK(dlsym(h, "halGetDeviceInfo") != nullptr, "halGetDeviceInfo");
    CHECK(dlsym(h, "halGetAPIVersion") != nullptr, "halGetAPIVersion(无判空调用,缺席必崩)");
    CHECK(dlsym(h, "halSqTaskSend") != nullptr, "halSqTaskSend");
    CHECK(dlsym(h, "halSqCqQuery") != nullptr, "halSqCqQuery");
    CHECK(dlsym(h, "halMemcpy") != nullptr, "halMemcpy");
    CHECK(dlsym(h, "drvGetPlatformInfo") != nullptr, "drvGetPlatformInfo(atrace/awatchdog/runtime)");
    CHECK(dlsym(h, "halHdcSend") != nullptr, "halHdcSend(plog dlsym)");
    CHECK(dlsym(h, "halEschedCreateGrpEx") != nullptr, "halEschedCreateGrpEx(msprof dlsym)");

    /* 5. 桩语义:返回 DRV_ERROR_NONE(M0 桩不写任何出参) */
    auto *apiVer = (drvError_t(*)(int *))dlsym(h, "halGetAPIVersion");
    if (apiVer != nullptr) {
        int v = -1;
        CHECK(apiVer(&v) == DRV_ERROR_NONE, "halGetAPIVersion 返回 DRV_ERROR_NONE");
    }
    auto *devInfo = (drvError_t(*)(uint32_t, int32_t, int32_t, int64_t *))dlsym(h, "halGetDeviceInfo");
    if (devInfo != nullptr) {
        int64_t v = -1;
        /* MODULE_TYPE_SYSTEM=0, INFO_TYPE_ENV=0(ascend_hal_base.h DEV_MODULE_TYPE/INFO_TYPE 段) */
        CHECK(devInfo(0, 0, 0, &v) == DRV_ERROR_NONE, "halGetDeviceInfo(SYSTEM,ENV) 返回 DRV_ERROR_NONE");
    }

    printf("---- M0 %s ----\n", g_fail == 0 ? "全部通过" : "存在失败项");
    dlclose(h);
    return g_fail == 0 ? 0 : 1;
}
