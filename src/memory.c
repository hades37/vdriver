/**
 * memory.c —— vdriver 内存模块(强语义,覆盖桩)
 *
 * 模型:所有"设备内存"由 host 后备块承载(实施方案.md §4.2):
 *   halMemAlloc 以 512B 对齐分配 size+ALIGN 头部块,头部记录 magic+size,
 *   返回 base+ALIGN 作为设备指针 —— 512B 对齐覆盖 32B(HostAlloc)/512B 需求。
 *   halMemcpy 依 drvMemcpyKind_t 全部退化为 memmove(D2D/H2D/D2H 同为 host 块间搬运)。
 * 复用 cmodel_driver 的账本思路(driver_mem.c:114-217),但补齐其 ESL 版缺失的
 * 真实后备内存与真实拷贝(调研报告2 §3)。
 */
#include "vdriver_internal.h"

#include "ascend_hal.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* 头部布局:base 处存放 magic+size,用户指针 = base + MEM_HDR_ALIGN */
#define MEM_HDR_ALIGN 512U
#define MEM_MAGIC     0x56445249ULL /* "VDRI" */

typedef struct {
    uint64_t magic;
    uint64_t size;
} mem_hdr_t;

static inline void *hdr_base(void *user_ptr)
{
    return (void *)((uint8_t *)user_ptr - MEM_HDR_ALIGN);
}

DLLEXPORT drvError_t halMemAlloc(void **pp, unsigned long long size, unsigned long long flag)
{
    (void)flag; /* 对齐语义统一按 512B 兜底,覆盖 flag 高位对齐的最大需求(512B) */
    if (pp == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    *pp = NULL;
    if (size == 0ULL) {
        return DRV_ERROR_INVALID_VALUE;
    }

    void *base = aligned_alloc(MEM_HDR_ALIGN, ((size + MEM_HDR_ALIGN + MEM_HDR_ALIGN - 1U) /
                                               MEM_HDR_ALIGN) * MEM_HDR_ALIGN);
    if (base == NULL) {
        vdriver_debug_log("halMemAlloc: alloc %llu bytes failed", size);
        return DRV_ERROR_INVALID_VALUE; /* 与真 HAL 满时语义一致:非 0 错误码 */
    }
    mem_hdr_t *hdr = (mem_hdr_t *)base;
    hdr->magic = MEM_MAGIC;
    hdr->size = size;
    /* 新分配块清零:模拟 HBM 上电态,避免上层读到进程堆残留导致难查的假数据 */
    (void)memset((uint8_t *)base + MEM_HDR_ALIGN, 0, size);
    *pp = (uint8_t *)base + MEM_HDR_ALIGN;
    vdriver_debug_log("halMemAlloc: size=%llu -> %p", size, *pp);
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halMemFree(void *pp)
{
    if (pp == NULL) {
        return DRV_ERROR_NONE; /* 真机对空指针幂等 */
    }
    mem_hdr_t *hdr = (mem_hdr_t *)hdr_base(pp);
    if (hdr->magic != MEM_MAGIC) {
        vdriver_debug_log("halMemFree: bad magic on %p (非 vdriver 分配)", pp);
        return DRV_ERROR_INVALID_VALUE;
    }
    free(hdr_base(pp));
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halMemcpy(void *dst, size_t dst_size, void *src, size_t count,
                               struct memcpy_info *info)
{
    if (dst == NULL || src == NULL || info == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    if (count > dst_size) {
        vdriver_debug_log("halMemcpy: count(%zu) > dst_size(%zu)", count, dst_size);
        return DRV_ERROR_INVALID_VALUE;
    }
    /* 方向仅用于日志/校验;host 后备模型下 H2H/H2D/D2H/D2D 统一 memmove */
    if (info->dir > DRV_MEMCPY_DEVICE_TO_DEVICE) {
        return DRV_ERROR_INVALID_VALUE;
    }
    (void)memmove(dst, src, count);
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halHostRegister(void *src_ptr, UINT64 size, UINT32 flag, UINT32 devid,
                                     void **dst_ptr)
{
    (void)flag;
    (void)devid;
    if (src_ptr == NULL || dst_ptr == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    (void)size;
    /* D4(VIRTUAL+UNIFIED)下该接口基本不触达;身份映射保证语义自洽 */
    *dst_ptr = src_ptr;
    return DRV_ERROR_NONE;
}
