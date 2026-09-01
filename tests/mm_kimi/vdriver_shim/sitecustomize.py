"""vdriver 测试垫片(sitecustomize,随 PYTHONPATH 在每个解释器启动时执行)。

作用(全部为测试适配,不修改 MindSpeed-MM / 环境包):
  1. 前置标准 triton(/tmp/vd_triton_std,遮蔽 triton_ascend 命名空间碎片);
     /tmp/vd_triton_std/triton/backends/vdriver/ 提供装饰期 dummy driver;
     /tmp/vd_triton_std/triton/language/extra/cann/ 提供 triton_ascend 导入占位。
  2. 预注册伪 mindspeed_ops.api.triton.convolution,casual_conv1d 用 eager torch
     实现(MM 仓 conv1d 仅 triton/ascendc 两实现,mindspeed_ops 未安装)。
"""

import os
import sys
import types

_STD_TRITON = "/tmp/vd_triton_std"
if os.path.isdir(os.path.join(_STD_TRITON, "triton")) and _STD_TRITON not in sys.path:
    sys.path.insert(0, _STD_TRITON)


def _register_module(name: str) -> types.ModuleType:
    mod = types.ModuleType(name)
    sys.modules[name] = mod
    return mod


def _eager_causal_conv1d(
    x,
    weight,
    bias=None,
    residual=None,
    initial_state=None,
    output_final_state=False,
    activation=None,
    cu_seqlens=None,
    **_kwargs,
):
    """因果深度卷积的 torch 等价实现(流程验证用,数值非目标)。

    x: [B,T,D];weight: [W,D](由 [D,1,W] rearrange);返回 (y [B,T,D], final_state)。
    """
    import torch
    import torch.nn.functional as F

    if cu_seqlens is not None:
        raise NotImplementedError("vdriver: varlen causal_conv1d 未实现(训练用例不触发)")

    _, _, dim = x.shape
    kernel = weight.shape[0]
    xt = x.transpose(1, 2).contiguous()  # [B,D,T]
    if initial_state is not None:
        keep = min(kernel - 1, initial_state.shape[-1])
        xt = torch.cat([initial_state[..., -keep:], xt], dim=-1)
    else:
        xt = F.pad(xt, (kernel - 1, 0))
    y = F.conv1d(xt, weight.transpose(0, 1).unsqueeze(1), groups=dim)  # [B,D,T]
    y = y.transpose(1, 2)  # [B,T,D]
    if bias is not None:
        y = y + bias
    act = str(activation).lower() if activation is not None else ""
    if act in ("silu", "swish"):
        y = F.silu(y)
    elif act == "gelu":
        y = F.gelu(y)
    elif act not in ("", "none"):
        raise NotImplementedError(f"vdriver eager causal_conv1d: activation={activation}")
    if residual is not None:
        y = y + residual
    final_state = xt[..., -kernel:] if output_final_state else None
    return y, final_state


_mindspeed_ops = _register_module("mindspeed_ops")
_api = _register_module("mindspeed_ops.api")
_triton = _register_module("mindspeed_ops.api.triton")
_convolution = _register_module("mindspeed_ops.api.triton.convolution")
_convolution.causal_conv1d = _eager_causal_conv1d
_mindspeed_ops.api = _api
_api.triton = _triton
_triton.convolution = _convolution

# ---------------------------------------------------------------------------
# 3. 单卡(world_size=1)去 HCCL:HCCL 初始化需要真设备通信资源(vdriver 不建模)。
#    强制 gloo 建组,并把单 rank 集合通信短路为恒等(broadcast/all_reduce 等在
#    world_size=1 时数学上即恒等;张量留在 NPU 上,不做跨进程搬运)。
# ---------------------------------------------------------------------------
if os.environ.get("VDRIVER_SINGLE_RANK_DIST", "1") == "1":
    import torch  # noqa: E402

    def _world_size() -> int:
        try:
            return int(os.environ.get("WORLD_SIZE", "1"))
        except ValueError:
            return 1

    if _world_size() == 1:
        _orig_ipg = torch.distributed.init_process_group

        def _patched_ipg(*args, **kwargs):
            backend = kwargs.get("backend") or (args[0] if args else None)
            if backend and "hccl" in str(backend).lower():
                kwargs["backend"] = "gloo"
                args = ()
                kwargs.pop("device_id", None)  # gloo 不支持 npu device_id 急切初始化
                sys.stderr.write("[vdriver-shim] 单卡:HCCL→gloo(无真设备通信,单 rank 恒等)\n")
            return _orig_ipg(*args, **kwargs)

        torch.distributed.init_process_group = _patched_ipg

        import torch.distributed as _dist
        from torch.distributed import distributed_c10d as _c10d

        def _shortcircuit(fn_name, tensor_names):
            _orig = getattr(_c10d, fn_name)

            def _patched(*args, **kwargs):
                group = kwargs.get("group", args[-1] if args and fn_name == "barrier" else None)
                if _dist.is_initialized() and _dist.get_world_size() == 1:
                    if fn_name == "barrier":
                        return None
                    for name in tensor_names:
                        if name in kwargs:
                            t = kwargs[name]
                            if fn_name in ("broadcast", "all_reduce", "reduce"):
                                return t
                            if fn_name == "all_gather_into_tensor":
                                return t.reshape(-1)
                    # 位置参数兜底:broadcast(tensor,...)/all_reduce(tensor,...)
                    if args and fn_name in ("broadcast", "all_reduce", "reduce") and hasattr(args[0], "shape"):
                        return args[0]
                return _orig(*args, **kwargs)

            setattr(_c10d, fn_name, _patched)
            setattr(_dist, fn_name, _patched)

        for _fn, _names in (
            ("broadcast", ("tensor",)),
            ("all_reduce", ("tensor",)),
            ("reduce", ("tensor",)),
            ("all_gather_into_tensor", ("input_tensor",)),
            ("barrier", ()),
        ):
            _shortcircuit(_fn, _names)

# ---------------------------------------------------------------------------
# 4. DTensor 随机初始化(meta init)的 RNG 状态跨设备分发在 torch_npu 上不可用
#    (default_generator.set_state 拒绝 CPU 格式状态:"offset must be a multiple
#    of 4")。流程用例不依赖 RNG 保真:_set_device_state 置为 no-op,
#    让 meta 权重初始化(normal_ 等 AICPU 算子)继续执行。
# ---------------------------------------------------------------------------
import torch  # noqa: E402
from torch.distributed.tensor import _random as _dt_random  # noqa: E402


def _noop_set_device_state(self, rng_state):
    sys.stderr.write("[vdriver-shim] DTensor RNG set_device_state 跳过(npu set_state 不兼容)\n")
    return None


_dt_random.OffsetBasedRNGTracker._set_device_state = _noop_set_device_state

# torch.npu.set_rng_state(torch_npu.npu 模块属性)→ default_generator.set_state
# 拒绝 CPU 格式状态;DTensor/fork_rng 走此路径。容错跳过(RNG 保真非目标)。
_orig_npu_set_rng_state = torch.npu.set_rng_state


def _safe_npu_set_rng_state(new_state, device=None):
    try:
        return _orig_npu_set_rng_state(new_state, device)
    except Exception as _e:  # noqa: BLE001
        sys.stderr.write(f"[vdriver-shim] torch.npu.set_rng_state 跳过:{_e}\n")
        return None


torch.npu.set_rng_state = _safe_npu_set_rng_state

# ---------------------------------------------------------------------------
# 5. 临时取证:抓 161002 matmul 失败的具体形状(定位后移除)
# ---------------------------------------------------------------------------
_orig_Flinear = torch.nn.functional.linear


def _dbg_linear(input, weight, bias=None):
    try:
        return _orig_Flinear(input, weight, bias)
    except Exception as _e:  # noqa: BLE001
        if "161002" in str(_e):
            import traceback
            frames = [f.name + ":" + f.filename.split("/")[-1] + ":" + str(f.lineno)
                      for f in traceback.extract_stack()[-8:-1]]
            sys.stderr.write(
                f"[vdriver-shim] linear FAIL: input={tuple(input.shape)} {input.dtype} "
                f"w={tuple(weight.shape)}\n  stack: {' <- '.join(frames)}\n")
        raise


torch.nn.functional.linear = _dbg_linear
