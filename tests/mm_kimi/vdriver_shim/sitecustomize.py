"""vdriver 测试垫片(sitecustomize,随 PYTHONPATH 在每个解释器启动时执行)。

作用(全部为测试适配,不修改 MindSpeed-MM / 环境包):
  1. 前置标准 triton(/tmp/vd_triton_std,遮蔽 triton_ascend 命名空间碎片);
     /tmp/vd_triton_std/triton/backends/vdriver/ 提供装饰期 dummy driver;
     /tmp/vd_triton_std/triton/language/extra/cann/ 提供 triton_ascend 导入占位。
  2. 预注册伪 mindspeed_ops.api.triton.convolution,casual_conv1d 用 eager torch
     实现(MM 仓 conv1d 仅 triton/ascendc 两实现,mindspeed_ops 未安装)。
"""

import faulthandler
import os
import sys
faulthandler.enable()
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

    _, _, dim = x.shape
    kernel = weight.shape[0]

    if cu_seqlens is not None:
        # 变长打包:x [1,T,D],cu_seqlens [B+1];逐段因果卷积后拼接
        import torch.nn.functional as F
        cu = cu_seqlens.cpu().tolist() if torch.is_tensor(cu_seqlens) else list(cu_seqlens)
        sys.stderr.write(f"[vdriver-shim] conv1d varlen: x={tuple(x.shape)} cu={cu}\n")
        w = weight.transpose(0, 1).unsqueeze(1)  # [D,1,W]
        outs = []
        for i in range(len(cu) - 1):
            seg = x[0, cu[i]:cu[i + 1], :]            # [Ti,D]
            if seg.shape[0] == 0:
                continue  # 空段(变长打包可含 0 长序列)
            segt = seg.transpose(0, 1).unsqueeze(0)   # [1,D,Ti]
            segt = F.pad(segt, (kernel - 1, 0))
            y = F.conv1d(segt, w, groups=dim).transpose(1, 2)  # [Ti,D]
            if bias is not None:
                y = y + bias
            if activation is not None:
                act = str(activation).lower()
                if act in ("silu", "swish"):
                    y = F.silu(y)
                elif act == "gelu":
                    y = F.gelu(y)
            outs.append(y)
        if not outs:
            return x, None
        y = torch.cat(outs, dim=0).unsqueeze(0)  # [1,T,D]
        if residual is not None:
            y = y + residual
        return (y, xt[..., -kernel:] if False else None)

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

# ---------------------------------------------------------------------------
# 6. 临时取证:vision 特征提取的输入/输出形状(定位 0-dim image_features)
# ---------------------------------------------------------------------------
try:
    from mindspeed_mm.fsdp.utils import register as _vd_reg

    _orig_import_plugin = _vd_reg.import_plugin

    def _vd_desc(x):
        import torch as _t
        if isinstance(x, _t.Tensor):
            return f"T{tuple(x.shape)}/{str(x.dtype).split('.')[-1]}"
        if isinstance(x, (list, tuple)):
            return "[" + ",".join(_vd_desc(i) for i in x) + "]"
        return type(x).__name__

    def _wrapped_import_plugin(paths):
        _orig_import_plugin(paths)
        try:
            import mindspeed_mm.fsdp.models.kimi_k3 as _kk
            _cls = _kk.KimiK3ForConditionalGeneration
            _orig_ext = _cls._extract_image_features

            def _ext(self, pixel_values, grid_thws):
                out = _orig_ext(self, pixel_values, grid_thws)
                sys.stderr.write(
                    f"[vdriver-shim] extract: pv={_vd_desc(pixel_values)} "
                    f"grid={_vd_desc(grid_thws)} -> out={_vd_desc(out)}\n")
                return out

            _cls._extract_image_features = _ext

            import mindspeed_mm.fsdp.models.kimi_k3.modeling_kimi_k3 as _kmod
            _orig_pj = _kmod.PatchMergerMLPV2.forward

            def _pj(self, x, *a, **k):
                out = _orig_pj(self, x, *a, **k)
                sys.stderr.write(
                    f"[vdriver-shim] projector: in={_vd_desc(x)} -> out={_vd_desc(out)}\n")
                return out

            _kmod.PatchMergerMLPV2.forward = _pj

            _orig_merge = _cls._merge_input_ids_with_image_features

            def _merge(self, image_features, inputs_embeds, input_ids,
                       attention_mask, labels=None):
                sys.stderr.write(
                    f"[vdriver-shim] merge: feats={_vd_desc(image_features)} "
                    f"embeds={_vd_desc(inputs_embeds)} ids={_vd_desc(input_ids)} "
                    f"mask={_vd_desc(attention_mask)}\n")
                return _orig_merge(self, image_features, inputs_embeds,
                                   input_ids, attention_mask, labels)

            _cls._merge_input_ids_with_image_features = _merge
        except Exception as _e:  # noqa: BLE001
            sys.stderr.write(f"[vdriver-shim] probe skip: {_e}\n")

    _vd_reg.import_plugin = _wrapped_import_plugin
except Exception as _e:  # noqa: BLE001
    sys.stderr.write(f"[vdriver-shim] register probe unavailable: {_e}\n")

# ---------------------------------------------------------------------------
# 7. 控制流关键算子 host 回退:比较/逻辑类算子的结果常驱动形状与索引
#    (nonzero、布尔掩码赋值等)。vdriver 不模拟内核 → 设备上的比较结果恒为
#    垃圾/0 → 形状类操作崩溃。此类算子的输入多为 host 源数据(input_ids/
#    mask 等),host 计算结果精确;重算力算子(matmul/attention)不回退,
#    数值仍为非目标。
# ---------------------------------------------------------------------------
_orig_nonzero = torch.Tensor.nonzero


def _shim_nonzero(self, *args, **kwargs):
    if self.device.type == "npu":
        return _orig_nonzero(self.cpu(), *args, **kwargs).to(self.device)
    return _orig_nonzero(self, *args, **kwargs)


torch.Tensor.nonzero = _shim_nonzero
torch.nonzero = lambda *a, **k: _shim_nonzero(*a, **k)

_orig_setitem = torch.Tensor.__setitem__


def _shim_setitem(self, key, value):
    try:
        return _orig_setitem(self, key, value)
    except (IndexError, RuntimeError) as _e:
        if (self.device.type == "npu" and isinstance(key, torch.Tensor)
                and key.dtype == torch.bool):
            cpu_idx = _orig_nonzero(key.cpu(), as_tuple=True)
            _orig_setitem(self, cpu_idx, value)
            return None
        raise


torch.Tensor.__setitem__ = _shim_setitem


def _host_binary_wrapper(name):
    _orig = getattr(torch.Tensor, name)

    def _wrapped(self, other):
        if self.device.type == "npu":
            other_dev = getattr(other, "device", None)
            if other_dev is None or other_dev.type == "npu":
                other_c = other.cpu() if torch.is_tensor(other) else other
                return getattr(_orig(self.cpu(), other_c) if not torch.is_tensor(other)
                               else _orig(self.cpu(), other_c), "to")(self.device) \
                    if False else _to_npu(_orig(self.cpu(), other_c), self.device)
        return _orig(self, other)

    return _wrapped


def _to_npu(t, device):
    return t if not torch.is_tensor(t) else t.to(device)


for _op in ("__eq__", "__ne__", "__lt__", "__le__", "__gt__", "__ge__",
            "__and__", "__or__", "__xor__"):
    setattr(torch.Tensor, _op, _host_binary_wrapper(_op))


def _to_npu_deep(out, device):
    if torch.is_tensor(out):
        return out.to(device)
    if isinstance(out, (list, tuple)):
        return type(out)(_to_npu_deep(o, device) for o in out) \
            if isinstance(out, list) else tuple(_to_npu_deep(o, device) for o in out)
    return out


def _host_unary_wrapper(name):
    _orig = getattr(torch.Tensor, name)

    def _wrapped(self, *args, **kwargs):
        if self.device.type == "npu" and self.numel() <= 1 << 22:  # ≤4M 元素
            return _to_npu_deep(_orig(self.cpu(), *args, **kwargs), self.device)
        return _orig(self, *args, **kwargs)

    return _wrapped


for _op in ("sum", "any", "all", "count_nonzero", "cumsum", "cumprod",
            "argmax", "argmin", "amax", "amin", "median", "norm",
            "topk", "sort", "argsort", "unique", "gather", "index_select",
            "take_along_dim", "scatter_add", "scatter"):
    if hasattr(torch.Tensor, _op):
        setattr(torch.Tensor, _op, _host_unary_wrapper(_op))
    if hasattr(torch, _op):
        setattr(torch, _op, _host_unary_wrapper(_op))

# 函数版补集(F.*):pad/where 等出现在控制流路径(cu_seqlens 构造等)
import torch.nn.functional as _F

for _fop in ("pad", "where", "clamp", "one_hot", "scatter",
             "log_softmax", "softmax"):
    if hasattr(_F, _fop):
        setattr(_F, _fop, _host_unary_wrapper(_fop))
