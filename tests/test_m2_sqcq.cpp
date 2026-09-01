/**
 * test_m2_sqcq —— M2 验收:SQ/CQ 记账 + SQE 解释执行
 *
 * 覆盖(实施方案.md M2 / 详单表):
 *  - halSqCqAllocate/Free:开设备约束、id 分配、重复分配
 *  - halSqTaskSend:pos 出参、head=tail 推进、逐条解释执行
 *  - SQE 解释:SDMA 内联(H2D/D2D)、SDMA 间接(rtDavidMemcpyAddrInfo)、
 *    PLACE_HOLDER(task_type=90)、WRITE_VALUE(8B)、FFTS 仅计数
 *  - halSqCqQuery:SQ_HEAD/SQ_TAIL/SQ_STATUS/SQ_CQE_STATUS
 *  - halSqCqConfig:SQ_HEAD 回拉
 *  - 报告三件套:IrqWait bitmap=0 / ReportGet count=0 / Release 0
 */
#include "ascend_hal.h"
#include "vdriver_internal.h"
#include "sqe_layout.hpp"

#include <cstdio>
#include <cstring>
#include <cstdint>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("[PASS] %s\n", msg); } \
    else { printf("[FAIL] %s\n", msg); g_fail++; } \
} while (0)

using vdriver::sqe::SqeType;

/* 构造一条 SDMA 内联拷贝 SQE */
static void BuildSdmaInline(uint8_t *sqe, uint64_t src, uint64_t dst, uint32_t len,
                            uint16_t stream_id, uint16_t task_id)
{
    (void)memset(sqe, 0, vdriver::sqe::SQE_LEN);
    auto *s = reinterpret_cast<vdriver::sqe::MemcpyAsyncSqe *>(sqe);
    s->header.type = vdriver::sqe::SQE_TYPE_SDMA;
    s->header.wr_cqe = 1U;
    s->header.rt_stream_id = stream_id;
    s->header.task_id = task_id;
    s->length = len;
    s->src_addr_low = (uint32_t)(src & 0xFFFFFFFFULL);
    s->src_addr_high = (uint32_t)(src >> 32U);
    s->dst_addr_low = (uint32_t)(dst & 0xFFFFFFFFULL);
    s->dst_addr_high = (uint32_t)(dst >> 32U);
}

int main()
{
    /* ---- 未开设备:Allocate/TaskSend 拒绝(评审建议⑥启用校验) ---- */
    struct halSqCqInputInfo in = {};
    in.type = DRV_NORMAL_TYPE;
    in.tsId = 0;
    in.sqeSize = vdriver::sqe::SQE_LEN;
    in.sqeDepth = 1024U;
    in.cqeDepth = 256U;
    struct halSqCqOutputInfo out = {};
    CHECK(halSqCqAllocate(0, &in, &out) == DRV_ERROR_INVALID_VALUE, "未开设备时 Allocate 拒绝");

    /* ---- 开设备 + 分配 ---- */
    halDevOpenIn open_in = {};
    halDevOpenOut open_out = {};
    CHECK(halDeviceOpen(0, &open_in, &open_out) == DRV_ERROR_NONE, "halDeviceOpen");
    CHECK(halSqCqAllocate(0, &in, &out) == DRV_ERROR_NONE && out.sqId < 64U, "halSqCqAllocate");
    const uint32_t sq_id = out.sqId;

    /* ---- 设备内存与注册 host 源(异步 H2D 的 src 必须是注册内存) ---- */
    void *dev_a = nullptr;
    void *dev_b = nullptr;
    void *host_src = nullptr;
    CHECK(halMemAlloc(&dev_a, 1024, 0) == DRV_ERROR_NONE, "halMemAlloc dev_a");
    CHECK(halMemAlloc(&dev_b, 1024, 0) == DRV_ERROR_NONE, "halMemAlloc dev_b");
    CHECK(halMemAlloc(&host_src, 256, 0) == DRV_ERROR_NONE, "halMemAlloc host_src");
    const char pattern[64] = "vdriver-sdma-inline-pattern";
    (void)memcpy(host_src, pattern, sizeof(pattern)); /* host 块可直接写 */
    uint8_t back[64] = {};

    /* ---- SQE 组包:5 条 ---- */
    static uint8_t sqes[5 * vdriver::sqe::SQE_LEN];
    (void)memset(sqes, 0, sizeof(sqes));

    /* #0 SDMA 内联:H2D(stack→dev_a) */
    BuildSdmaInline(sqes + 0 * 64, (uint64_t)(uintptr_t)host_src,
                    (uint64_t)(uintptr_t)dev_a, sizeof(pattern), 1, 100);
    /* #1 SDMA 内联:D2D(dev_a→dev_b) */
    BuildSdmaInline(sqes + 1 * 64, (uint64_t)(uintptr_t)dev_a,
                    (uint64_t)(uintptr_t)dev_b, sizeof(pattern), 1, 101);
    /* #2 SDMA 间接:dev_b→dev_a(dev 描述符在 host 栈) */
    vdriver::sqe::DavidMemcpyAddrInfo addr_info = {};
    addr_info.src = (uint64_t)(uintptr_t)dev_b;
    addr_info.dst = (uint64_t)(uintptr_t)dev_a;
    addr_info.len = sizeof(pattern);
    auto *ptr_sqe = reinterpret_cast<vdriver::sqe::MemcpyAsyncPtrSqe *>(sqes + 2 * 64);
    ptr_sqe->header.type = vdriver::sqe::SQE_TYPE_SDMA;
    ptr_sqe->header.wr_cqe = 1U;
    ptr_sqe->header.rt_stream_id = 1;
    ptr_sqe->header.task_id = 102;
    ptr_sqe->ptrMode = 1U;
    const uint64_t info_addr = (uint64_t)(uintptr_t)&addr_info;
    ptr_sqe->sdma_base_addr_low = (uint32_t)(info_addr & 0xFFFFFFFFULL);
    ptr_sqe->sdma_base_bits = (uint32_t)((info_addr >> 32U) & vdriver::sqe::PTR_SQE_ADDR_HIGH_MASK);
    /* #3 PLACE_HOLDER task_type=90:dev_a→dev_b */
    auto *ph = reinterpret_cast<vdriver::sqe::PhSqe *>(sqes + 3 * 64);
    ph->type_bits = (uint8_t)vdriver::sqe::SQE_TYPE_PLACE_HOLDER;
    ph->task_type = vdriver::sqe::PH_TASK_TYPE_MEMCPY_ASYNC_WITHOUT_SDMA;
    ph->rt_stream_id = 1;
    ph->task_id = 103;
    ph->src = (uint64_t)(uintptr_t)dev_a;
    ph->dest = (uint64_t)(uintptr_t)dev_b;
    ph->size = sizeof(pattern);
    /* #4 WRITE_VALUE:向 dev_a+256 写 8 字节标记 */
    auto *wv = reinterpret_cast<vdriver::sqe::WriteValueSqe *>(sqes + 4 * 64);
    wv->header.type = vdriver::sqe::SQE_TYPE_WRITE_VALUE;
    wv->header.rt_stream_id = 1;
    wv->header.task_id = 104;
    const uint64_t waddr = (uint64_t)(uintptr_t)dev_a + 256U;
    wv->write_addr_low = (uint32_t)(waddr & 0xFFFFFFFFULL);
    wv->write_addr_bits = (uint32_t)((waddr >> 32U) & vdriver::sqe::WRITE_VALUE_ADDR_HIGH_MASK) |
                          (3U << vdriver::sqe::WRITE_VALUE_AWSIZE_SHIFT); /* awsize=3 → 8B */
    wv->write_value_part0 = 0xDEADBEEFU;
    wv->write_value_part1 = 0x12345678U;

    /* ---- 下发 ---- */
    sqe_interp_stat_reset();
    struct halTaskSendInfo send = {};
    send.type = DRV_NORMAL_TYPE;
    send.tsId = 0;
    send.sqId = sq_id;
    send.sqe_addr = sqes;
    send.sqe_num = 5U;
    CHECK(halSqTaskSend(0, &send) == DRV_ERROR_NONE, "halSqTaskSend 5 条");
    CHECK(send.pos == 0U, "pos 出参=首 SQE 位置 0");
    printf("[INFO] 解释统计 k=%llu m=%llu i=%llu(期望 m=5 k=0)\n",
           (unsigned long long)sqe_interp_stat_kernel(),
           (unsigned long long)sqe_interp_stat_memcpy(),
           (unsigned long long)sqe_interp_stat_ignored());
    CHECK(sqe_interp_stat_kernel() == 0 && sqe_interp_stat_memcpy() == 5,
          "解释统计:memcpy=5(kernel=0)");

    /* ---- 执行效果验证 ---- */
    struct memcpy_info info = {};
    info.dir = DRV_MEMCPY_DEVICE_TO_HOST;
    CHECK(halMemcpy(back, sizeof(back), dev_b, sizeof(pattern), &info) == DRV_ERROR_NONE &&
          memcmp(back, pattern, sizeof(pattern)) == 0, "SDMA 内联+PH 链式拷贝内容一致");
    const uint64_t wv_got = *reinterpret_cast<volatile uint64_t *>((uint8_t *)dev_a + 256U);
    CHECK(wv_got == 0x12345678DEADBEEFULL, "WRITE_VALUE 8 字节落地");

    /* ---- 记账语义 ---- */
    struct halSqCqQueryInfo q = {};
    q.type = DRV_NORMAL_TYPE;
    q.tsId = 0;
    q.sqId = sq_id;
    q.prop = DRV_SQCQ_PROP_SQ_HEAD;
    CHECK(halSqCqQuery(0, &q) == DRV_ERROR_NONE && q.value[0] == 5U, "SQ_HEAD=5");
    q.prop = DRV_SQCQ_PROP_SQ_TAIL;
    CHECK(halSqCqQuery(0, &q) == DRV_ERROR_NONE && q.value[0] == 5U, "SQ_TAIL=5");
    q.prop = DRV_SQCQ_PROP_SQ_STATUS;
    CHECK(halSqCqQuery(0, &q) == DRV_ERROR_NONE && q.value[0] != 0U, "SQ_STATUS 非 0");
    q.prop = DRV_SQCQ_PROP_SQ_CQE_STATUS;
    CHECK(halSqCqQuery(0, &q) == DRV_ERROR_NONE && q.value[0] == 0U, "SQ_CQE_STATUS=0");

    /* ---- Config:head 回拉 ---- */
    struct halSqCqConfigInfo cfg = {};
    cfg.type = DRV_NORMAL_TYPE;
    cfg.tsId = 0;
    cfg.sqId = sq_id;
    cfg.prop = DRV_SQCQ_PROP_SQ_HEAD;
    cfg.value[0] = 0U;
    CHECK(halSqCqConfig(0, &cfg) == DRV_ERROR_NONE, "halSqCqConfig(SQ_HEAD=0)");
    q.prop = DRV_SQCQ_PROP_SQ_HEAD;
    CHECK(halSqCqQuery(0, &q) == DRV_ERROR_NONE && q.value[0] == 0U, "SQ_HEAD 被回拉为 0");

    /* ---- 报告三件套 ---- */
    struct halReportInfoInput irq_in = {};
    irq_in.type = DRV_NORMAL_TYPE;
    irq_in.timeout = -1;
    uint64_t bitmap[2] = {0xFFFFFFFFULL, 0xFFFFFFFFULL};
    struct halReportInfoOutput irq_out = {};
    irq_out.cqIdBitmap = bitmap;
    irq_out.cqIdBitmapSize = 2U;
    CHECK(halCqReportIrqWait(0, &irq_in, &irq_out) == DRV_ERROR_NONE &&
          bitmap[0] == 0ULL && bitmap[1] == 0ULL, "IrqWait bitmap 清零");
    struct halReportGetInput get_in = {};
    get_in.type = DRV_NORMAL_TYPE;
    struct halReportGetOutput get_out = {};
    get_out.count = 99U;
    CHECK(halCqReportGet(0, &get_in, &get_out) == DRV_ERROR_NONE && get_out.count == 0U &&
          get_out.reportPtr == nullptr, "ReportGet count=0");
    struct halReportReleaseInfo rel = {};
    CHECK(halReportRelease(0, &rel) == DRV_ERROR_NONE, "ReportRelease=0");

    /* ---- 异常路径 ---- */
    struct halTaskSendInfo bad = {};
    bad.type = DRV_NORMAL_TYPE;
    bad.sqId = 63U; /* 未分配 */
    bad.sqe_addr = sqes;
    bad.sqe_num = 1U;
    CHECK(halSqTaskSend(0, &bad) == DRV_ERROR_INVALID_VALUE, "未知 sqId 拒绝");

    /* ---- 释放 + 重分配 ---- */
    struct halSqCqFreeInfo fin = {};
    fin.type = DRV_NORMAL_TYPE;
    fin.tsId = 0;
    fin.sqId = sq_id;
    fin.cqId = out.cqId;
    CHECK(halSqCqFree(0, &fin) == DRV_ERROR_NONE, "halSqCqFree");
    struct halSqCqOutputInfo out2 = {};
    CHECK(halSqCqAllocate(0, &in, &out2) == DRV_ERROR_NONE, "释放后可重分配");

    halMemFree(dev_a);
    halMemFree(dev_b);
    halDevCloseIn close_in = {};
    (void)halDeviceClose(0, &close_in);

    printf("---- M2 %s ----\n", g_fail == 0 ? "全部通过" : "存在失败项");
    return g_fail == 0 ? 0 : 1;
}
