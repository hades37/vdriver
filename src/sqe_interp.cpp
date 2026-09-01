/**
 * sqe_interp.cpp —— STARS SQE 解释器(vdriver 的"设备侧执行"替代)
 *
 * 原则(实施方案.md D6/§4.3):
 *  - 所有 SQE 视为"已完成"(head 推进由 sqcq.c 在解释执行后发布);
 *  - 只有内存效果类 SQE 需要真执行:SDMA 拷贝(内联/间接)、
 *    PLACE_HOLDER 承载的异步拷贝(task_type=90)、WRITE_VALUE 小块写;
 *  - kernel/AICPU/事件类忽略(runtime 侧 D7 已让 AICPU fail-fast);
 *  - 9.1.0 的大块 memset 在 runtime 侧已分解为 host memset + memcpy SQE
 *    (memcpy_starsv2.cc:246 DevMemSetAsync),无需 memset 解释。
 *  - M2 评审严重③:所有待写地址必须落在注册表已分配块内,否则丢弃并计数
 *    (防御畸形/布局不符的 SQE 造成野指针写)。
 *
 * ⚠️ 布局警告(M2 评审严重②,M3 门禁):当前按 Stars v100 布局解析
 * (stars_dma.hpp:125/172);910B1 若走 David 路径(task_david.cc ToConstructDavidSqe)
 * 则布局不同——已通过 halSqTaskSend 的 hex-dump(VDRIVER_LOG=2)取证,
 * 真实流量对照结果记录于 进展.md M3 节。
 */
#include "sqe_layout.hpp"
#include "vdriver_internal.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace vdriver::sqe;

extern "C" {

/* 统计计数(M2 评审风格⑩:原子化) */
static std::atomic<uint64_t> g_interp_kernel_cnt{0};
static std::atomic<uint64_t> g_interp_memcpy_cnt{0};
static std::atomic<uint64_t> g_interp_ignored_cnt{0};
static std::atomic<uint64_t> g_interp_dropped_cnt{0};

uint64_t sqe_interp_stat_kernel(void) { return g_interp_kernel_cnt.load(); }
uint64_t sqe_interp_stat_memcpy(void) { return g_interp_memcpy_cnt.load(); }
uint64_t sqe_interp_stat_ignored(void) { return g_interp_ignored_cnt.load(); }
uint64_t sqe_interp_stat_dropped(void) { return g_interp_dropped_cnt.load(); }
void sqe_interp_stat_reset(void)
{
    g_interp_kernel_cnt.store(0);
    g_interp_memcpy_cnt.store(0);
    g_interp_ignored_cnt.store(0);
    g_interp_dropped_cnt.store(0);
}

/* 地址防护(M2 评审严重③):[addr, addr+len) 必须整体落于一个已注册块内;
 * 已注册块涵盖设备内存与 aclrtMallocHost 的 host 内存(均经 halMemAlloc) */
static bool MemRangeValid(uint64_t addr, uint32_t len)
{
    if (addr == 0ULL) {
        return false;
    }
    uint64_t size = 0U;
    if (vdriver_mem_query(reinterpret_cast<const void *>(addr), &size) != 0) {
        return false;
    }
    return (len <= size);
}

static void DoMemcpy(uint64_t dst, uint64_t src, uint32_t len)
{
    if (len == 0U) {
        return;
    }
    if (!MemRangeValid(src, len) || !MemRangeValid(dst, len)) {
        vdriver_debug_log("DoMemcpy 丢弃: 地址越界或未注册 src=%#llx dst=%#llx len=%u",
                          (unsigned long long)src, (unsigned long long)dst, len);
        g_interp_dropped_cnt++;
        return;
    }
    (void)memmove(reinterpret_cast<void *>(dst), reinterpret_cast<const void *>(src), len);
    g_interp_memcpy_cnt++;
}

/* 处理 SDMA 内联/间接拷贝 */
static void HandleSdma(const uint8_t *sqe)
{
    /* ptrMode 位在字节 15 的 bit0(header 8 + res3 4 + res4 2 + kernel_credit 1) */
    const uint8_t ptr_mode_byte = sqe[15];
    const uint8_t ptr_mode = ptr_mode_byte & 0x1U;

    if (ptr_mode != 0U) {
        /* 间接:base_low@16、base_high:17@20 → rtDavidMemcpyAddrInfo(host 内存) */
        const uint64_t base =
            (uint64_t)ReadU32LE(sqe + PTR_SQE_OFF_LOW) |
            (((uint64_t)(ReadU32LE(sqe + PTR_SQE_OFF_BITS) & PTR_SQE_ADDR_HIGH_MASK))
             << PTR_SQE_ADDR_HIGH_SHIFT);
        if (base == 0ULL) {
            g_interp_ignored_cnt++;
            return;
        }
        const uint64_t src = ReadU64LE(reinterpret_cast<const uint8_t *>(base) + DAVID_INFO_OFF_SRC);
        const uint64_t dst = ReadU64LE(reinterpret_cast<const uint8_t *>(base) + DAVID_INFO_OFF_DST);
        const uint32_t len = ReadU32LE(reinterpret_cast<const uint8_t *>(base) + DAVID_INFO_OFF_LEN);
        DoMemcpy(dst, src, len);
    } else {
        /* 内联:length@28, src@32, dst@40 */
        const uint32_t len = ReadU32LE(sqe + 28);
        const uint64_t src = ReadU64LE(sqe + 32);
        const uint64_t dst = ReadU64LE(sqe + 40);
        DoMemcpy(dst, src, len);
    }
}

/* 处理 PLACE_HOLDER 承载的异步拷贝(task_type=90) */
static void HandlePlaceHolder(const uint8_t *sqe)
{
    const uint16_t task_type = ReadU16LE(sqe + 2);
    if (task_type == PH_TASK_TYPE_MEMCPY_ASYNC_WITHOUT_SDMA) {
        /* RtMemCpyAsyncWithoutSdma:src@16, dest@24, size@32 */
        const uint64_t src = ReadU64LE(sqe + 16);
        const uint64_t dst = ReadU64LE(sqe + 24);
        const uint32_t len = ReadU32LE(sqe + 32);
        DoMemcpy(dst, src, len);
    } else {
        g_interp_ignored_cnt++;
    }
}

/* 处理 WRITE_VALUE 小块写(4/8 字节,事件复位等场景) */
static void HandleWriteValue(const uint8_t *sqe)
{
    const uint32_t addr_low = ReadU32LE(sqe + 16);
    const uint32_t addr_bits = ReadU32LE(sqe + 20);
    const uint64_t addr = addr_low |
        (((uint64_t)(addr_bits & WRITE_VALUE_ADDR_HIGH_MASK)) << WRITE_VALUE_ADDR_HIGH_SHIFT);
    const uint32_t awsize = (addr_bits >> WRITE_VALUE_AWSIZE_SHIFT) & WRITE_VALUE_AWSIZE_MASK;
    const uint32_t len = (awsize >= 3U) ? 8U : 4U;
    if (!MemRangeValid(addr, len)) {
        vdriver_debug_log("HandleWriteValue 丢弃: 地址越界或未注册 %#llx", (unsigned long long)addr);
        g_interp_dropped_cnt++;
        return;
    }
    if (awsize >= 3U) {
        const uint64_t v64 = (uint64_t)ReadU32LE(sqe + 32) |
                             ((uint64_t)ReadU32LE(sqe + 36) << 32U);
        (void)memcpy(reinterpret_cast<void *>(addr), &v64, sizeof(v64));
    } else {
        const uint32_t v32 = ReadU32LE(sqe + 32);
        (void)memcpy(reinterpret_cast<void *>(addr), &v32, sizeof(v32));
    }
    g_interp_memcpy_cnt++;
}

/* 解释执行一条 64B SQE */
void sqe_interp_execute(const uint8_t *sqe)
{
    if (sqe == nullptr) {
        return;
    }
    const uint8_t type = sqe[0] & 0x3FU;
    switch (type) {
        case SQE_TYPE_FFTS:
            g_interp_kernel_cnt++; /* kernel 不模拟执行,仅计完成 */
            break;
        case SQE_TYPE_AICPU:
            g_interp_ignored_cnt++; /* D7: 上层已 fail-fast,防御性忽略 */
            break;
        case SQE_TYPE_SDMA:
            HandleSdma(sqe);
            break;
        case SQE_TYPE_PLACE_HOLDER:
            HandlePlaceHolder(sqe);
            break;
        case SQE_TYPE_WRITE_VALUE:
            HandleWriteValue(sqe);
            break;
        default:
            g_interp_ignored_cnt++;
            break;
    }
}

} /* extern "C" */
