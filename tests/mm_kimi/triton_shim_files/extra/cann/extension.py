"""vdriver shim: triton_ascend cann.extension 的导入占位。

extract_slice/insert_slice 是 triton-ascend 的 kernel 内张量切片元操作;
vdriver 用例全部走 eager 路径,这些仅保证模块可导入。
"""

def extract_slice(*_args, **_kwargs):
    raise NotImplementedError("vdriver shim: triton-ascend cann.extension.extract_slice 不应在 eager 用例中被调用")


def insert_slice(*_args, **_kwargs):
    raise NotImplementedError("vdriver shim: triton-ascend cann.extension.insert_slice 不应在 eager 用例中被调用")
