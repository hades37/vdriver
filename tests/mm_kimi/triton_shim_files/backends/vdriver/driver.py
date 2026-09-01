"""vdriver shim driver: 单一 active driver,满足装饰期探测;不做任何真实编译/启动。"""
from triton.backends.compiler import GPUTarget
from triton.backends.driver import DriverBase


class VdriverDriver(DriverBase):
    def __init__(self):
        pass

    @classmethod
    def is_active(cls):
        return True

    def get_current_target(self):
        return GPUTarget("npu", 0, 32)

    def get_benchmarker(self):
        def _noop_bench(kernel_call, **kwargs):
            return None
        return _noop_bench

    def get_device_interface(self):
        import torch
        return torch.npu
