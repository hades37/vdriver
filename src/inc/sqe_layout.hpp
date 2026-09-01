/**
 * sqe_layout.hpp —— STARS SQE 布局(自包含,供 vdriver 解释器与测试共用)
 *
 * 布局以 runtime 仓定义为准(逐字段对照,禁止凭记忆改动):
 *  - rtStarsSqeHeader_t      task_info_base.hpp:43(8B)
 *  - rtStarsSqe_t union      stars/stars.hpp:459(64B,RT_STARS_SQE_LEN=64)
 *  - RtStarsMemcpyAsyncSqe   stars/stars_dma.hpp:125(SDMA 内联: len@28 src@32 dst@40)
 *  - RtStarsMemcpyAsyncPtrSqe stars/stars_dma.hpp(间接: base@32 → rtDavidMemcpyAddrInfo)
 *  - rtDavidMemcpyAddrInfo   sqe/v200_base/starsv2_base.hpp:46(src@32 dst@40 len@48)
 *  - RtStarsPhSqe            stars/stars.hpp:250(task_type@2, payload@16)
 *  - RtMemCpyAsyncWithoutSdma stars/stars.hpp:223(src@16 dest@24 size@32)
 */
#ifndef VDRIVER_SQE_LAYOUT_HPP
#define VDRIVER_SQE_LAYOUT_HPP

#include <cstdint>
#include <cstddef>

namespace vdriver {
namespace sqe {

#pragma pack(push, 1)

constexpr uint8_t SQE_LEN = 64U;

/* SQE 类型(stars.hpp rtStarsSqeType,占 header 字节 0 低 6 位) */
enum SqeType : uint8_t {
    SQE_TYPE_FFTS         = 0,  /* AI Core kernel */
    SQE_TYPE_AICPU        = 1,
    SQE_TYPE_PLACE_HOLDER = 3,
    SQE_TYPE_EVENT_RECORD = 4,
    SQE_TYPE_EVENT_WAIT   = 5,
    SQE_TYPE_WRITE_VALUE  = 8,
    SQE_TYPE_SDMA         = 0x11, /* M4 真流量修正:byte0=0x11(M2 研究的 11 是
                                   * 循环论证产物;真 SQE hex-dump 为准) */
    SQE_TYPE_INVALID      = 63,
};

struct SqeHeader {
    uint8_t type : 6;
    uint8_t l1_lock : 1;
    uint8_t l1_unlock : 1;
    uint8_t ie : 2;
    uint8_t pre_p : 2;
    uint8_t post_p : 2;
    uint8_t wr_cqe : 1;
    uint8_t reserved : 1;
    uint16_t block_dim;
    uint16_t rt_stream_id;
    uint16_t task_id;
};
static_assert(sizeof(SqeHeader) == 8U, "sqe header must be 8 bytes");

/* PLACE_HOLDER SQE:独立布局(stars.hpp:250,不复用 SqeHeader——task_type@2),
 * 48B payload 从 @16 开始,总长 64 */
struct PhSqe {
    uint8_t type_bits;       /* type:6 | l2_lock:1 | l2_unlock:1 */
    uint8_t ctrl_bits;       /* ie:2 | pre_p:2 | post_p:2 | wr_cqe:1 | res0:1 */
    uint16_t task_type;      /* @2 */
    uint16_t rt_stream_id;   /* @4 */
    uint16_t task_id;        /* @6 */
    uint32_t res1;           /* @8  RUNTIME_BUILD_VERSION */
    uint16_t res2;           /* @12 */
    uint8_t kernel_credit;   /* @14 */
    uint8_t res3;            /* @15 */
    /* payload@16: RtMemCpyAsyncWithoutSdma(stars.hpp:223) */
    uint64_t src;            /* @16 */
    uint64_t dest;           /* @24 */
    uint32_t size;           /* @32 */
    uint32_t pid;            /* @36 */
    uint8_t reserved[24];    /* @40..63 */
};
static_assert(sizeof(PhSqe) == 64U, "ph sqe must be 64 bytes");

/* PH payload 的 task_type(task_base.hpp:176) */
constexpr uint16_t PH_TASK_TYPE_MEMCPY_ASYNC_WITHOUT_SDMA = 90U;

/* SDMA 内联 SQE(stars_dma.hpp:125):len@28, src@32, dst@40 */
/* M4 真流量修正:SDMA 内联布局为 src@32、dst@40、len@48(M2 研究的
 * len@28 有误:真 SQE hex-dump + data_ptr 对照,byte0=0x11、byte1=0x40、
 * @16-31 为保留段)。此前单测以自产 SQE 验证自产布局属循环论证。 */
struct MemcpyAsyncSqe {
    SqeHeader header;
    uint64_t res_prefix;    /* @8 头部扩展 */
    uint64_t res_mid0;      /* @16 保留段(真流量为 0) */
    uint64_t res_mid1;      /* @24 保留段(真流量为 0) */
    uint64_t src_addr;      /* @32 源地址(host VA / 设备地址均可) */
    uint64_t dst_addr;      /* @40 目的地址 */
    uint64_t length;        /* @48 拷贝长度 */
    uint32_t res6;
    uint32_t res7;
};
static_assert(sizeof(MemcpyAsyncSqe) == 64U, "memcpy sqe must be 64 bytes");

/* SDMA 间接描述符(starsv2_base.hpp:46):base 指向的结构,共 64B */
struct DavidMemcpyAddrInfo {
    uint64_t res0[4];        /* @0..31 */
    uint64_t src;            /* @32 */
    uint64_t dst;            /* @40 */
    uint32_t len;            /* @48 */
    uint32_t res1[3];        /* @52..63 */
};
static_assert(sizeof(DavidMemcpyAddrInfo) == 64U, "memcpy addr info must be 64 bytes");
constexpr size_t DAVID_INFO_OFF_SRC = 32U;
constexpr size_t DAVID_INFO_OFF_DST = 40U;
constexpr size_t DAVID_INFO_OFF_LEN = 48U;

/* SDMA Ptr SQE(stars_dma.hpp:172):base_low@16,base_high:17@20 */
struct MemcpyAsyncPtrSqe {
    SqeHeader header;
    uint32_t res3;
    uint16_t res4;
    uint8_t kernel_credit;
    uint8_t ptrMode : 1;
    uint8_t res5 : 7;
    uint32_t sdma_base_addr_low;   /* @16 */
    uint32_t sdma_base_bits;       /* @20: high:17 | res:14 | va:1 */
    uint32_t res_last[10];
};
static_assert(sizeof(MemcpyAsyncPtrSqe) == 64U, "memcpy ptr sqe must be 64 bytes");
constexpr uint32_t PTR_SQE_ADDR_HIGH_SHIFT = 32U;
constexpr uint32_t PTR_SQE_ADDR_HIGH_MASK = 0x1FFFFU; /* 17 bits */
constexpr size_t PTR_SQE_OFF_LOW = 16U;
constexpr size_t PTR_SQE_OFF_BITS = 20U;

/* WRITE_VALUE SQE(stars.hpp:71):addr@16(low@16, high17bit@20), value@32 */
struct WriteValueSqe {
    SqeHeader header;
    uint32_t res3;
    uint32_t res4_bits;
    uint32_t write_addr_low;
    uint32_t write_addr_bits; /* high17 | res6:3 | awsize:3 | snoop | awcache | awprot | va */
    uint32_t res7;
    uint32_t sub_type;
    uint32_t write_value_part0;
    uint32_t write_value_part1;
    uint32_t write_value_part2;
    uint32_t write_value_part3;
    uint32_t write_value_part4;
    uint32_t write_value_part5;
    uint32_t write_value_part6;
    uint32_t write_value_part7;
};
static_assert(sizeof(WriteValueSqe) == 64U, "write value sqe must be 64 bytes");

/* awsize 在 write_addr_bits 的 bit20-22(位域排布:high17 后 res6:3,再 awsize:3) */
constexpr uint32_t WRITE_VALUE_AWSIZE_SHIFT = 20U;
constexpr uint32_t WRITE_VALUE_AWSIZE_MASK = 0x7U;
constexpr uint32_t WRITE_VALUE_ADDR_HIGH_SHIFT = 32U;
constexpr uint32_t WRITE_VALUE_ADDR_HIGH_MASK = 0x1FFFFU; /* 17 bits */

inline uint64_t ReadU64LE(const uint8_t *p)
{
    uint64_t v = 0U;
    for (int i = 7; i >= 0; --i) {
        v = (v << 8U) | p[i];
    }
    return v;
}

inline uint32_t ReadU32LE(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8U) | ((uint32_t)p[2] << 16U) | ((uint32_t)p[3] << 24U);
}

inline uint16_t ReadU16LE(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8U));
}

} /* namespace sqe */
} /* namespace vdriver */

#endif /* VDRIVER_SQE_LAYOUT_HPP */
