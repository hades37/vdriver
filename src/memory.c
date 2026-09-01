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
