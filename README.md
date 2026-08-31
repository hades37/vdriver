# vdriver —— Ascend CANN 虚拟驱动

假 `libascend_hal.so`:让 **torch_npu → libopapi(libnnopbase)→ libruntime** 全部上层真实流程离设备走通,仅最底层 mock。

**主文档:[实施方案.md](./实施方案.md)** —— 关键决策(D1-D8)、符号契约(强语义 17 符号/桩兜底/禁桩清单)、模块设计、目录与构建、分级验收(L1-L4)、里程碑(11-18 人日)、风险清单。

**调研归档:[docs/](./docs/)**
- [01_逐符号实现详单.md](./docs/01_逐符号实现详单.md) —— 每个 hal* 符号的调用点/出参语义/mock 要点;两个必炸点(`halGetAPIVersion` 无判空、`halSqCqQuery` 不同步推进则自旋)
- [02_cmodel_driver复用解剖.md](./docs/02_cmodel_driver复用解剖.md) —— 官方主机版驱动后端的复用判定表(内存账本/队列骨架可直接移植)
- [03_集成安装设计.md](./docs/03_集成安装设计.md) —— 链接事实(weak 直链+soname 查找)、同名替换方案、dlsym 兼容、SoC 命名传导链
- [04_上层兼容性核对.md](./docs/04_上层兼容性核对.md) —— hello_cann 逐 API→hal 符号映射、AICPU fail-fast 设计、torch_npu 隐藏触点

## 一图速览

```
torch_npu ──► libopapi/libnnopbase ──► libruntime ──► vdriver(假 libascend_hal.so)
  (原包)          (原包)                  (原包)          ├─ device.c  设备/SoC=Ascend910B1/VIRTUAL+flat
                                                          ├─ memory.c  host 后备 + 512B 对齐账本
                                                          ├─ sqcq.c    SQE 记账 + head 同步推进
                                                          ├─ sqe_interp.c  COPY/MEMSET 类 SQE 代执行
                                                          └─ stub_base.c   h2c 全量桩兜底(~361 符号)
```

## 快速使用(规划)
```bash
cmake -B build -S . -DRUNTIME_INC=/path/to/runtime/include/driver && cmake --build build
export LD_LIBRARY_PATH=/path/to/vdriver/lib:$LD_LIBRARY_PATH   # 前置同名 so,DT_NEEDED 即命中
```

选型依据见上层目录《meta_device_mock选型调研报告.md》(结论:HAL 边界为最佳 mock 坐席)。
