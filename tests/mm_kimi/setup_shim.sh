#!/usr/bin/env bash
# setup_shim.sh —— 构建/修复 /tmp/vd_triton_std 标准 triton 环境(/tmp 易失,自愈)
# 组成:
#   1. pip --target 隔离安装标准 triton 3.2.0(零 env 修改);
#   2. vdriver 后端(backends/vdriver):装饰期 dummy driver,backend 名必须为 npu;
#   3. cann 导入占位(language/extra/cann):满足 triton_ascend_kernels import 链;
# 用法: bash tests/mm_kimi/setup_shim.sh
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PY="${PY:-/root/miniconda3/envs/mindspeed/bin/python}"
STD="/tmp/vd_triton_std"

if [ ! -f "${STD}/triton/__init__.py" ]; then
    echo "[setup_shim] 安装标准 triton 3.2.0 → ${STD}"
    "${PY}" -m pip install --target "${STD}" --no-deps --quiet "triton==3.2.0"
fi

# vdriver 后端(装饰期探测用;is_active()=True 使 autotune/jit 装饰可构建)
mkdir -p "${STD}/triton/backends/vdriver"
cp -f "${HERE}/triton_shim_files/backends/vdriver/compiler.py" "${STD}/triton/backends/vdriver/"
cp -f "${HERE}/triton_shim_files/backends/vdriver/driver.py" "${STD}/triton/backends/vdriver/"

# triton_ascend 的 cann extra 导入占位
mkdir -p "${STD}/triton/language/extra/cann"
cp -f "${HERE}/triton_shim_files/extra/cann/__init__.py" "${STD}/triton/language/extra/cann/"
cp -f "${HERE}/triton_shim_files/extra/cann/extension.py" "${STD}/triton/language/extra/cann/"

echo "[setup_shim] 就绪:${STD}"
