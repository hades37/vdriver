"""vdriver shim backend: 按路径复用 CUDA 编译器实现,仅让装饰可构建。"""
import importlib.util
import os

_here = os.path.dirname(os.path.abspath(__file__))
_cuda_compiler = os.path.join(os.path.dirname(_here), "nvidia", "compiler.py")
_spec = importlib.util.spec_from_file_location("vdriver_cuda_compiler", _cuda_compiler)
_mod = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_mod)
_CUDABackend = _mod.CUDABackend
del _mod


class VdriverBackend(_CUDABackend):
    pass


del _CUDABackend  # 避免被发现逻辑计为具体子类
