"""vdriver shim: triton_ascend 的 cann extra 命名空间占位(标准 triton 无此模块)。

仅满足 import 链;其中算子级函数不应被真实 kernel 编译调用。
"""
from . import extension  # noqa: F401
