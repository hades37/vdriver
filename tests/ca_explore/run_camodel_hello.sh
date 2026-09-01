#!/usr/bin/env bash
# CA(CAMODEL)仿真路线探索:hello_cann 跑官方 libruntime_camodel
# 前置:vdriver build/lib 已构建(用其 libascend_hal.so 兜底 DT_NEEDED)
set -uo pipefail
TK="/root/miniconda3/envs/mindspeed/Ascend/cann-9.1.0"
SIM="${TK}/x86_64-linux/simulator/Ascend910B1"
VD_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CA_LIB=/tmp/ca_lib
mkdir -p "$CA_LIB"
ln -sf "${SIM}/lib/"*.so "$CA_LIB/" 2>/dev/null
ln -sf "${VD_ROOT}/build/lib/libascend_hal.so" "$CA_LIB/libascend_hal.so"
cd "${SIM}"   # startModel 从 cwd 读 config.json
exec env LD_LIBRARY_PATH="${CA_LIB}:${TK}/lib64:${SIM}/lib:${TK}/../lib64" \
    CAMODEL_LOG_PATH=/tmp/camlog \
    "${VD_ROOT}/tests/l1/hello_cann" "$@"
