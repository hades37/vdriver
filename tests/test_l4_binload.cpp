/**
 * test_l4_binload —— L4 验收:算子二进制加载路径(真 opp 内核 .o)
 *
 * 验证链(M3 报告 §二/1):aclrtBinaryLoadFromData → runtime BinaryLoader
 * (host 侧 ELF 解析)→ Runtime::BinaryLoad → halMemAlloc + halMemcpy 落
 * "设备"内存 → aclrtBinaryGetFunction 取函数句柄(Kernel::GetFunctionDevAddr
 * = BinAlignBaseAddr + offset)。
 * 内核文件用安装包 opp 真实 Add 内核(ascend910b/ops_legacy/add)。
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "acl/acl.h"

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("[PASS] %s\n", msg); } \
    else { printf("[FAIL] %s\n", msg); g_fail++; } \
} while (0)

static std::vector<uint8_t> ReadAll(const char *path)
{
    std::vector<uint8_t> data;
    FILE *fp = fopen(path, "rb");
    if (fp == nullptr) {
        return data;
    }
    (void)fseek(fp, 0, SEEK_END);
    const long len = ftell(fp);
    (void)fseek(fp, 0, SEEK_SET);
    data.resize(static_cast<size_t>(len));
    (void)fread(data.data(), 1, data.size(), fp);
    (void)fclose(fp);
    return data;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s <kernel.o>\n", argv[0]);
        return 2;
    }
    const std::vector<uint8_t> bin = ReadAll(argv[1]);
    CHECK(!bin.empty(), "读取内核 .o 文件");

    CHECK(aclInit(nullptr) == ACL_SUCCESS, "aclInit");
    CHECK(aclrtSetDevice(0) == ACL_SUCCESS, "aclrtSetDevice");

    /* 直接以 Data 形式加载(与 opbase indv_bininfo.cpp:90 同路径) */
    aclrtBinHandle bin_handle = nullptr;
    aclrtBinaryLoadOptions *opts = nullptr; // 与 opbase 相同:无额外选项时传 nullptr
    aclError ret = aclrtBinaryLoadFromData(bin.data(), bin.size(), opts, &bin_handle);
    CHECK(ret == ACL_SUCCESS && bin_handle != nullptr, "aclrtBinaryLoadFromData(真内核 .o)");

    /* 函数句柄:从 magic/entry 派生,任意名字走 GetFunction(接口打通即可) */
    aclrtFuncHandle func = nullptr;
    ret = aclrtBinaryGetFunction(bin_handle, "Add_28b8f2963b2205f1d66cfc7a1da60151_high_performance_2147483647", &func);
    CHECK(ret == ACL_SUCCESS, "aclrtBinaryGetFunction");

    /* 重复加载/释放稳定性 */
    aclrtBinHandle h2 = nullptr;
    CHECK(aclrtBinaryLoadFromData(bin.data(), bin.size(), opts, &h2) == ACL_SUCCESS,
          "二次 LoadFromData 稳定");
    CHECK(aclrtBinaryUnLoad(h2) == ACL_SUCCESS, "aclrtBinaryUnLoad");
    CHECK(aclrtBinaryUnLoad(bin_handle) == ACL_SUCCESS, "aclrtBinaryUnLoad(首块)");

    CHECK(aclrtResetDeviceForce(0) == ACL_SUCCESS, "aclrtResetDeviceForce");
    (void)aclFinalize();

    printf("---- L4 binload %s ----\n", g_fail == 0 ? "全部通过" : "存在失败项");
    return g_fail == 0 ? 0 : 1;
}
