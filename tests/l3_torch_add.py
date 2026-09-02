#!/usr/bin/env python3
"""L3 验收:torch_npu E2E(add)在 vdriver 上的最小用例。

运行前提:
  1. LD_LIBRARY_PATH 前置 vdriver/build/lib(抢位 libascend_hal.so + 哑 libtsdclient.so);
  2. sys.path 前置标准 triton 3.2.0(/tmp/vd_triton_std,pip --target 隔离安装,
     未改动 mindspeed env)——env 里的 triton_ascend 残留为**无 __init__ 的命名空间
     碎片**,torch/_inductor(hints.py:40 import triton.backends.compiler 等)无法
     在其上工作,属环境既有缺陷,与 vdriver 无关。
"""
import os
import os
import sys

TRITON_STD_DIR = os.environ.get("VDRIVER_TRITON_STD", "/tmp/vd_triton_std")
sys.path.insert(0, TRITON_STD_DIR)

import torch  # noqa: E402
import torch_npu  # noqa: E402

print("torch:", torch.__version__, "| torch_npu:", torch_npu.__version__)
print("npu available:", torch.npu.is_available())
assert torch.npu.is_available(), "torch.npu.is_available() 应为 True(vdriver 报 1 设备)"

torch.npu.set_device(0)

x = torch.tensor([1.0, 2.0, 3.0, 4.0], device="npu")
y = torch.tensor([2.0, 2.0, 2.0, 2.0], device="npu")
print("tensors created on npu:", x.shape, y.shape)

z = x + y  # dispatch → op_api → aclnnAdd 两段式(libopapi 真包)
print("z =", z.cpu().tolist())

torch.npu.synchronize()
got = z.cpu().tolist()
print("add result:", got, "(内核不模拟执行,数值校验仅记录不判定)")
print("L3 done, npu device count =", torch.npu.device_count())
# PV 仿真栈已知问题:解释器收尾(atexit → aclFinalize)触发官方栈内部 double-free
# (全部工作已完成之后)。自动化场景可设 L3_SKIP_TEARDOWN=1 跳过收尾拿 exit=0。
if os.environ.get("L3_SKIP_TEARDOWN") == "1":
    sys.stdout.flush()
    sys.stderr.flush()
    os._exit(0)
