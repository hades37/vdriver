#!/usr/bin/env bash
# run_tests.sh —— vdriver 测试运行入口
# 关键:glibc 启动时固化 LD_LIBRARY_PATH,且其优先级高于 RUNPATH;
# 继承环境常含 CANN 真包路径,必须把 vdriver 放最前(实施方案.md D2、进展.md M0 发现)。
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/build"

if [ ! -f "${BUILD}/lib/libascend_hal.so" ]; then
    echo "[run_tests] 未找到构建产物,先执行: cmake -B build -S . && cmake --build build -j4" >&2
    exit 1
fi

export LD_LIBRARY_PATH="${BUILD}/lib:${LD_LIBRARY_PATH:-}"

cd "${BUILD}"
echo "==== vdriver 测试(LD_LIBRARY_PATH 前置: ${BUILD}/lib)===="
exec ctest --output-on-failure "$@"
