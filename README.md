# vdriver —— Ascend CANN 虚拟驱动

假 `libascend_hal.so`:让 **torch_npu → libopapi(libnnopbase)→ libruntime** 全部上层真实流程离设备走通,仅最底层 mock。已在 **CANN 9.1.0 真包**上完成全量验收。

**主文档:[实施方案.md](./实施方案.md)** · **阶段进展:[进展.md](./进展.md)** · **调研归档:[docs/](./docs/)**

## 验收结果(全部实测通过)

| 级别 | 内容 | 结果 |
|---|---|---|
| 单元 | M0 加载/SONAME、M1 设备+内存契约(33 项)、M2 SQ/CQ+SQE 解释(22 项) | ctest 3/3 |
| L1/L2 | hello_cann(真包 aclnnAdd 两段式全链) | exit=0 ✅ |
| L3 | torch_npu 2.10:`is_available`/`set_device`/张量/`x+y`/D2H | exit=0 ✅ |
| L4 | 双流并发+Event+异步 H2D;内核 .o 二进制加载(GetFunction) | exit=0 ✅ / 8/8 ✅ |
| 真流量 | SQE hex-dump 取证:PH×6/AICPU×2/kernel×1,丢弃 0 | 布局实证 |

结果数值为未初始化值(内核不模拟执行,属非目标,见进展.md 已知行为)。

## 一图速览

```
torch_npu ──► libopapi/libnnopbase ──► libruntime ──► vdriver(假 libascend_hal.so)
  (原包)          (原包)                  (原包)        ├─ device.c  SoC=Ascend910B1/VIRTUAL+flat
                                                        ├─ memory.c  注册表 + host 后备 512B 对齐
                                                        ├─ sqcq.c    SQ 环 + head=tail 同步发布
                                                        ├─ sqe_interp.cpp  拷贝类 SQE 代执行(注册表地址防护)
                                                        ├─ res.c     资源 ID 位图分配器
                                                        ├─ stub_base.c  h2c 全量桩(~361 符号)
                                                        └─ 哑 libtsdclient.so  跳过 TSD 全路径
```

## 快速使用

```bash
# 构建
cmake -B build -S . [-DRUNTIME_INC=/path/to/runtime/include] && cmake --build build -j4

# 单元测试
./tests/run_tests.sh

# 真包 E2E(hello_cann)
TK=/root/miniconda3/envs/mindspeed/Ascend/cann-9.1.0/x86_64-linux
g++ -o /tmp/hello_cann ../runtime/example/0_quickstart/0_hello_cann/main.cpp \
    -I$TK/include -I$TK/pkg_inc -L$TK/lib64 -lascendcl -lopapi -lnnopbase -lruntime -lpthread
LD_LIBRARY_PATH=$PWD/build/lib:$TK/lib64:$LD_LIBRARY_PATH /tmp/hello_cann

# torch_npu E2E(详见 tests/l3_torch_add.py 头注释的 triton shim 说明)
LD_LIBRARY_PATH=$PWD/build/lib:$TK/lib64:$LD_LIBRARY_PATH \
  python tests/l3_torch_add.py
```

调研归档:
- [01_逐符号实现详单.md](./docs/01_逐符号实现详单.md) —— 每个 hal* 符号的调用点/出参语义/mock 要点
- [02_cmodel_driver复用解剖.md](./docs/02_cmodel_driver复用解剖.md) —— 官方主机版驱动后端复用判定表
- [03_集成安装设计.md](./docs/03_集成安装设计.md) —— 链接事实/同名替换方案/dlsym 兼容/SoC 传导链
- [04_上层兼容性核对.md](./docs/04_上层兼容性核对.md) —— 样例→符号映射/AICPU fail-fast/torch_npu 触点

选型依据见上层目录《meta_device_mock选型调研报告.md》。
