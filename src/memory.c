/**
 * memory.c —— vdriver 内存模块(强语义,覆盖桩)
 *
 * 模型(M1 评审修复 + M2 注册表):
 *  - 所有"设备内存"由 host 后备块承载;分配 512B 对齐、内容清零(模拟 HBM 上电态);
 *  - 注册表(带锁哈希表)记录 {用户指针 → 基址/大小}:halMemFree 精确匹配,
 *    杜绝魔数探测的 UB 与 double-free(M1 评审严重①);入口溢出防护(严重②);
 *  - halMemcpy 按 drvMemcpyKind_t 全部退化为 memmove(dst_size 为目的容量约束,
 *    base.h:2033-2049 注释 + npu_driver_mem.cc:2207 实传 destMax,M1 评审核对通过);
 *  - halHostRegister 身份映射(D4 下基本不触达)。
 *  - 用户指针即 aligned_alloc 原始指针(注册表取代头部偏移,对齐天然保证)。
 */
#include "vdriver_internal.h"

#include "ascend_hal.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MEM_ALIGN       512U
#define MEM_TABLE_SIZE  4096U /* 开地址哈希容量 */

typedef struct {
    const void *user; /* 用户指针(halMemAlloc 的 *pp,即 aligned_alloc 原始指针) */
    void *base;       /* 与 user 相同,显式保留以便扩展头部信息 */
    uint64_t size;
    int used;
} mem_entry_t;

static mem_entry_t g_mem_table[MEM_TABLE_SIZE];
static pthread_mutex_t g_mem_lock = PTHREAD_MUTEX_INITIALIZER;

static inline size_t MemHash(const void *p)
{
    uintptr_t v = (uintptr_t)p;
    v ^= v >> 16U;
    v *= 0x9E3779B97F4A7C15ULL;
    return (size_t)(v & (MEM_TABLE_SIZE - 1U));
}

/* 注册表写入:调用方持锁;成功 0,表满 -1 */
static int MemTableInsert(const void *user, void *base, uint64_t size)
{
    size_t idx = MemHash(user);
    for (size_t probe = 0; probe < MEM_TABLE_SIZE; probe++) {
        mem_entry_t *e = &g_mem_table[(idx + probe) % MEM_TABLE_SIZE];
        if (!e->used) {
            e->used = 1;
            e->user = user;
            e->base = base;
            e->size = size;
            return 0;
        }
    }
    return -1;
}

/* 精确查找:调用方持锁;开地址约定——首个空槽之后必无同键 */
static mem_entry_t *MemTableFindLocked(const void *user)
{
    size_t idx = MemHash(user);
    for (size_t probe = 0; probe < MEM_TABLE_SIZE; probe++) {
        mem_entry_t *e = &g_mem_table[(idx + probe) % MEM_TABLE_SIZE];
        if (!e->used) {
            return NULL;
        }
        if (e->user == user) {
            return e;
        }
    }
    return NULL;
}

int vdriver_mem_query(const void *user_ptr, uint64_t *size)
{
    if (user_ptr == NULL || size == NULL) {
        return -1;
    }
    const uint64_t addr = (uint64_t)(uintptr_t)user_ptr;
    pthread_mutex_lock(&g_mem_lock);
    mem_entry_t *e = MemTableFindLocked(user_ptr);
    if (e != NULL) {
        *size = e->size; /* 块首精确命中:返回块大小 */
    } else {
        /* 块内地址:返回剩余可写字节数(SQE 引用块中段场景) */
        for (uint32_t i = 0; i < MEM_TABLE_SIZE; i++) {
            const mem_entry_t *it = &g_mem_table[i];
            if (!it->used) {
                continue;
            }
            const uint64_t base = (uint64_t)(uintptr_t)it->user;
            if (addr > base && (addr - base) < it->size) {
                *size = it->size - (addr - base);
                pthread_mutex_unlock(&g_mem_lock);
                return 0;
            }
        }
    }
    pthread_mutex_unlock(&g_mem_lock);
    return (e != NULL) ? 0 : -1;
}

DLLEXPORT drvError_t halMemAlloc(void **pp, unsigned long long size, unsigned long long flag)
{
    (void)flag; /* 对齐语义统一 512B,覆盖 flag 高位对齐的最大需求 */
    if (pp == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    *pp = NULL;
    /* M1 评审严重②:溢出防护(size + 对齐余量不得回绕) */
    if (size == 0ULL || size > (unsigned long long)(SIZE_MAX - 2ULL * MEM_ALIGN)) {
        return DRV_ERROR_INVALID_VALUE;
    }

    const size_t total = (size_t)(((size + MEM_ALIGN) / MEM_ALIGN) * MEM_ALIGN);
    void *base = aligned_alloc(MEM_ALIGN, total);
    if (base == NULL) {
        vdriver_debug_log("halMemAlloc: alloc %llu bytes failed", size);
        return DRV_ERROR_INVALID_VALUE; /* 失败码与真 HAL 满时口径未对照,标注待验证 */
    }
    /* 新分配块清零:模拟 HBM 上电态,避免上层读到进程堆残留 */
    (void)memset(base, 0, total);

    pthread_mutex_lock(&g_mem_lock);
    const int rc = MemTableInsert(base, base, size);
    pthread_mutex_unlock(&g_mem_lock);
    if (rc != 0) {
        free(base);
        vdriver_debug_log("halMemAlloc: registry full");
        return DRV_ERROR_INVALID_VALUE;
    }

    *pp = base;
    vdriver_debug_log("halMemAlloc: size=%llu -> %p", size, *pp);
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halMemFree(void *pp)
{
    if (pp == NULL) {
        return DRV_ERROR_NONE; /* 真机对空指针幂等 */
    }
    pthread_mutex_lock(&g_mem_lock);
    mem_entry_t *e = MemTableFindLocked(pp);
    if (e == NULL) {
        pthread_mutex_unlock(&g_mem_lock);
        vdriver_debug_log("halMemFree: 非 vdriver 分配指针 %p(或已释放)", pp);
        return DRV_ERROR_INVALID_VALUE; /* M1 评审严重①:精确匹配,无探测、防 double-free */
    }
    void *base = e->base;
    e->used = 0;
    e->user = NULL;
    e->base = NULL;
    e->size = 0;
    pthread_mutex_unlock(&g_mem_lock);
    free(base);
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
    if (info->dir > DRV_MEMCPY_DEVICE_TO_DEVICE) {
        return DRV_ERROR_INVALID_VALUE;
    }
    (void)memmove(dst, src, count); /* host 后备模型下四向统一 memmove */
    vdriver_debug_log("halMemcpy: dst=%p src=%p count=%zu head=%02x %02x %02x %02x",
                      dst, src, count,
                      count > 0 ? ((uint8_t *)src)[0] : 0, count > 1 ? ((uint8_t *)src)[1] : 0,
                      count > 2 ? ((uint8_t *)src)[2] : 0, count > 3 ? ((uint8_t *)src)[3] : 0);
    return DRV_ERROR_NONE;
}

DLLEXPORT drvError_t halHostRegister(void *src_ptr, UINT64 size, UINT32 flag, UINT32 devid,
                                     void **dst_ptr)
{
    (void)flag;
    (void)size;
    (void)devid;
    if (src_ptr == NULL || dst_ptr == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    *dst_ptr = src_ptr; /* D4(VIRTUAL+UNIFIED)下基本不触达;身份映射保证语义自洽 */
    return DRV_ERROR_NONE;
}

/* ---------------------------------------------------------------------------
 * drvMemGetAttribute(M4 联调补强):runtime 的 PtrGetRealLocation 依赖本接口
 * 判定指针位置(未填 memType 会落到"未知类型"分支并拒绝 H2D_EX 拷贝,
 * DSARandomNormal 等算子的 param 分发即在此失败)。
 * vdriver 语义:注册表内=锁定设备内存;其余任意 host VA=锁定 host 内存
 * (flat 地址模型下所有 host 内存可被设备访问)。
 */
drvError_t drvMemGetAttribute(DVdeviceptr vptr, struct DVattribute *attr)
{
    if (attr == NULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    (void)memset(attr, 0, sizeof(*attr));
    attr->memType = DV_MEM_LOCK_HOST;
    attr->pageSize = 4096U;
    void *user_ptr = (void *)(uintptr_t)vptr;
    uint64_t size = 0;
    if (vdriver_mem_query(user_ptr, &size) == 0) {
        attr->memType = DV_MEM_LOCK_DEV; /* 注册表命中:设备分配 */
    }
    return DRV_ERROR_NONE;
}
/* ---------------------------------------------------------------------------
 * halMemCpyAsync / halMemCpyAsyncWaitFinish(M4 联调补强):
 * runtime 按弱符号存在性选择拷贝路径——桩原样"定义"了本符号导致 runtime
 * 走异步分支而桩不搬数据,H2D/D2H 内容为垃圾(hello_cann 用同步 aclrtMemcpy
 * 才幸免)。这里实现真语义:host 后备模型下立即 memmove,完成态置
 * ASYNC_COPY_STATU_SUCC(=1,runtime pool.hpp:44)。
 */
drvError_t halMemCpyAsync(DVdeviceptr dst, size_t dest_max, DVdeviceptr src,
                          size_t byte_count, uint64_t *copy_fd)
{
    if (dst == 0ULL || src == 0ULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    if (byte_count > dest_max) {
        vdriver_debug_log("halMemCpyAsync: count(%zu) > dest_max(%zu)", byte_count, dest_max);
        return DRV_ERROR_INVALID_VALUE;
    }
    (void)memmove((void *)(uintptr_t)dst, (const void *)(uintptr_t)src, byte_count);
    vdriver_debug_log("halMemCpyAsync: dst=%p src=%p count=%zu head=%02x %02x %02x %02x",
                      (void *)(uintptr_t)dst, (void *)(uintptr_t)src, byte_count,
                      byte_count > 0 ? ((uint8_t *)(uintptr_t)src)[0] : 0,
                      byte_count > 1 ? ((uint8_t *)(uintptr_t)src)[1] : 0,
                      byte_count > 2 ? ((uint8_t *)(uintptr_t)src)[2] : 0,
                      byte_count > 3 ? ((uint8_t *)(uintptr_t)src)[3] : 0);
    if (copy_fd != NULL) {
        *copy_fd = 1ULL; /* ASYNC_COPY_STATU_SUCC:立即完成 */
    }
    return DRV_ERROR_NONE;
}

drvError_t halMemCpyAsyncWaitFinish(uint64_t copy_fd)
{
    (void)copy_fd; /* 拷贝同步完成,无需等待 */
    return DRV_ERROR_NONE;
}
/* ---------------------------------------------------------------------------
 * halSdmaCopy(M4 联调补强):ini 配 SDMA_COPY_BY_HAL 时 runtime 的同步拷贝
 * 走本接口(桩原样返回 0 但不搬数据 → torch H2D/D2H 内容为垃圾)。
 * host 后备模型下等价于一次 memmove;失败按契约回退 memcpy_s 语义。
 */
drvError_t halSdmaCopy(DVdeviceptr dst, size_t dst_size, DVdeviceptr src, size_t len)
{
    if (dst == 0ULL || src == 0ULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    if (len > dst_size) {
        vdriver_debug_log("halSdmaCopy: len(%zu) > dst_size(%zu)", len, dst_size);
        return DRV_ERROR_INVALID_VALUE;
    }
    (void)memmove((void *)(uintptr_t)dst, (const void *)(uintptr_t)src, len);
    return DRV_ERROR_NONE;
}

/* ---------------------------------------------------------------------------
 * drvMemcpy(M4 联调补强):runtime MemCopySyncAdapter 走 drvMemcpy;
 * 桩原样返回 0 不搬数据。host 后备模型下直接 memmove。
 */
drvError_t drvMemcpy(DVdeviceptr dst, size_t dest_max, DVdeviceptr src, size_t byte_count)
{
    if (dst == 0ULL || src == 0ULL) {
        return DRV_ERROR_INVALID_VALUE;
    }
    if (byte_count > dest_max) {
        return DRV_ERROR_INVALID_VALUE;
    }
    (void)memmove((void *)(uintptr_t)dst, (const void *)(uintptr_t)src, byte_count);
    return DRV_ERROR_NONE;
}
