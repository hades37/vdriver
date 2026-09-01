#!/usr/bin/env python3
"""L4 验收:多流 / 事件 / 异步拷贝 / 跨流同步(torch_npu + vdriver)。

覆盖:
  - 双流并发:两个 Stream 各自跑 add(独立 SQ/CQ 资源分配)
  - Event:record / wait / synchronize(事件类 SQE + 跨流依赖)
  - 异步 H2D:pin_memory + non_blocking=True(触发 async 拷贝路径)
  - 结果校验仅记录:内核不模拟执行,数值非目标(见 进展.md 已知行为)
"""
import sys

TRITON_STD_DIR = "/tmp/vd_triton_std"
sys.path.insert(0, TRITON_STD_DIR)

import torch  # noqa: E402
import torch_npu  # noqa: E402

print("torch_npu:", torch_npu.__version__)

torch.npu.set_device(0)
s1 = torch.npu.Stream()
s2 = torch.npu.Stream()
print("streams created: 2")

# 异步 H2D:pin memory + non_blocking(若走 async 拷贝路径将产生 SDMA 类 SQE)
host = torch.arange(8, dtype=torch.float32).pin_memory()
x = host.to("npu", non_blocking=True)
y = torch.ones(8, device="npu")
print("async H2D tensor:", x.shape)

# 双流并发计算
with torch.npu.stream(s1):
    z1 = x + y
with torch.npu.stream(s2):
    z2 = x * 2.0

# 事件:流 1 记录,主流等待
ev = torch.npu.Event()
ev.record(s1)
ev.synchronize()
print("event record/wait ok")

torch.npu.current_stream().synchronize()
s1.synchronize()
s2.synchronize()
print("multi-stream sync ok")

print("z1 =", z1.cpu().tolist(), "(数值非目标)")
print("z2 =", z2.cpu().tolist(), "(数值非目标)")
print("L4 streams/events done")
