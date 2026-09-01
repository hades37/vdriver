/**
 * sqe_interp.cpp —— STARS SQE 解释器(vdriver 的"设备侧执行"替代)
 *
 * 原则(实施方案.md D6/§4.3):
 *  - 所有 SQE 视为"已完成"(head 推进由 sqcq.c 处理);
 *  - 只有内存效果类 SQE 需要真执行:SDMA 拷贝(内联/间接)、
 *    PLACE_HOLDER 承载的异步拷贝(task_type=90)、WRITE_VALUE 小块写;
 *  - kernel/AICPU/事件类忽略(runtime 侧 D7 已让 AICPU fail-fast);
 *  - 9.1.0 的大块 memset 在 runtime 侧已分解为 host memset + memcpy SQE
 *    (memcpy_starsv2.cc:246 DevMemSetAsync),无需 memset 解释。
 */
#include "sqe_layout.hpp"
#include "vdriver_internal.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace vdriver::sqe;

extern "C" {

/* 统计计数(测试与诊断用) */
static uint64_t g_interp_kernel_cnt = 0;
static uint64_t g_interp_memcpy_cnt = 0;
static uint64_t g_interp_ignored_cnt = 0;

uint64_t sqe_interp_stat_kernel(void) { return g_interp_kernel_cnt; }
uint64_t sqe_interp_stat_memcpy(void) { return g_interp_memcpy_cnt; }
uint64_t sqe_interp_stat_ignored(void) { return g_interp_ignored_cnt; }
void sqe_interp_stat_reset(void)
{
    g_interp_kernel_cnt = 0;
    g_interp_memcpy_cnt = 0;
    g_interp_ignored_cnt = 0;
}

static void DoMemcpy(void *dst, const void *src, uint32_t len)
{
    if (len == 0U || dst == nullptr || src == nullptr) {
        return;
    }
    (void)memmove(dst, src, len);
    g_interp_memcpy_cnt++;
}

/* 处理 SDMA 内联/间接拷贝 */
static void HandleSdma(const uint8_t *sqe)
{
    /* ptrMode 位在字节 15 的 bit0(header 8 + res3 4 + res4 2 + kernel_credit 1) */
    const uint8_t byte13 = sqe[15]; /* ptrMode 在字节15(已修正) | ptrMode:1(bit0) */
    const uint8_t ptr_mode = byte13 & 0x1U;

    vdriver_debug_log("HandleSdma: ptr_mode=%u", ptr_mode);
    if (ptr_mode != 0U) {
        /* 间接:base_low@16、base_high:17@20 → rtDavidMemcpyAddrInfo(host 内存) */
        const uint64_t base =
            (uint64_t)ReadU32LE(sqe + PTR_SQE_OFF_LOW) |
            (((uint64_t)(ReadU32LE(sqe + PTR_SQE_OFF_BITS) & PTR_SQE_ADDR_HIGH_MASK))
             << PTR_SQE_ADDR_HIGH_SHIFT);
        if (base == 0ULL) {
            return;
        }
        const uint64_t src = ReadU64LE(reinterpret_cast<const uint8_t *>(base) + DAVID_INFO_OFF_SRC);
        const uint64_t dst = ReadU64LE(reinterpret_cast<const uint8_t *>(base) + DAVID_INFO_OFF_DST);
        const uint32_t len = ReadU32LE(reinterpret_cast<const uint8_t *>(base) + DAVID_INFO_OFF_LEN);
        DoMemcpy(reinterpret_cast<void *>(dst), reinterpret_cast<const void *>(src), len);
    } else {
        /* 内联:length@28, src@32, dst@40 */
        const uint32_t len = ReadU32LE(sqe + 28);
        const uint64_t src = ReadU64LE(sqe + 32);
        const uint64_t dst = ReadU64LE(sqe + 40);
        DoMemcpy(reinterpret_cast<void *>(dst), reinterpret_cast<const void *>(src), len);
    }
}

/* 处理 PLACE_HOLDER 承载的异步拷贝(task_type=90) */
static void HandlePlaceHolder(const uint8_t *sqe)
{
    const uint16_t task_type = ReadU16LE(sqe + 2);
    vdriver_debug_log("HandlePH: task_type=%u", task_type);
    if (task_type == PH_TASK_TYPE_MEMCPY_ASYNC_WITHOUT_SDMA) {
        /* RtMemCpyAsyncWithoutSdma:src@16, dest@24, size@32 */
        const uint64_t src = ReadU64LE(sqe + 16);
        const uint64_t dest = ReadU64LE(sqe + 24);
        const uint32_t size = ReadU32LE(sqe + 32);
        DoMemcpy(reinterpret_cast<void *>(dest), reinterpret_cast<const void *>(src), size);
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
    if (addr == 0ULL) {
        return;
    }
    const uint32_t awsize = (addr_bits >> WRITE_VALUE_AWSIZE_SHIFT) & WRITE_VALUE_AWSIZE_MASK;
    const uint32_t value = ReadU32LE(sqe + 32);
    if (awsize >= 3U) {
        /* ≥8 字节:写 8 字节(值低位 part0,高位 part1) */
        const uint32_t value_hi = ReadU32LE(sqe + 36);
        uint64_t v64 = value | ((uint64_t)value_hi << 32U);
        (void)memcpy(reinterpret_cast<void *>(addr), &v64, sizeof(v64));
    } else {
        /* 1/2/4 字节:写 4 字节(设备寄存器类语义,mock 下按 4B 落地) */
        uint32_t v32 = value;
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
