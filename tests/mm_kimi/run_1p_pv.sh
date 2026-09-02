#!/usr/bin/env bash
# Kimi-K3 单卡用例 —— PV 仿真数值语义层(内核 x86 真执行,loss/中间值有意义)
# 与 run_1p.sh(vdriver 流程层)同一 yaml/数据/shim,仅运行时换为官方仿真栈:
#   libruntime.so → libruntime_cmodel.so, libascend_hal.so → libnpu_drv_pvmodel.so
# 用法: bash tests/mm_kimi/run_1p_pv.sh [MM_ROOT]
set -uo pipefail

MM_ROOT="${1:-/mnt/e/ubuntu-workspace/MindSpeed-MM}"
VD_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TK="/root/miniconda3/envs/mindspeed/Ascend/cann-9.1.0"
SIM="${TK}/x86_64-linux/simulator/Ascend910B1"
PY="${PY:-/root/miniconda3/envs/mindspeed/bin/python}"

# 自愈 /tmp 垫片 + PV 软链
bash "${VD_ROOT}/tests/mm_kimi/setup_shim.sh" > /dev/null
CA_LIB=/tmp/ca_lib
mkdir -p "$CA_LIB"
ln -sf "${SIM}/lib/libruntime_cmodel.so"  "$CA_LIB/libruntime.so"
ln -sf "${SIM}/lib/libnpu_drv_pvmodel.so" "$CA_LIB/libascend_hal.so"
ln -sf "${SIM}/lib/"*.so "$CA_LIB/" 2>/dev/null

cd "${MM_ROOT}"   # 预编译驱动自带仿真配置,不依赖 cwd(已实测)

export LD_LIBRARY_PATH="${CA_LIB}:${TK}/lib64:${SIM}/lib:${TK}/../lib64:${LD_LIBRARY_PATH:-}"
export PYTHONPATH="${VD_ROOT}/tests/mm_kimi/vdriver_shim:/tmp/vd_triton_std:${MM_ROOT}:${PYTHONPATH:-}"
export VDRIVER_PV_MODE=1            # shim:关闭 host 回退(真内核执行)
export VDRIVER_SINGLE_RANK_DIST=1   # 单卡去 HCCL:PV 仿真 hccl 同样 err 19(实测)
export NON_MEGATRON=true
export TASK_QUEUE_ENABLE=1
export VDRIVER_LOG="${VDRIVER_LOG:-0}"

mkdir -p "${VD_ROOT}/tests/mm_kimi/logs"
LOG="${VD_ROOT}/tests/mm_kimi/logs/train_pv_$(date +%H%M%S).log"

torchrun --nproc_per_node 1 --nnodes 1 --node_rank 0 \
    --master_addr localhost --master_port 6098 \
    "${MM_ROOT}/mindspeed_mm/fsdp/train/trainer.py" \
    "${VD_ROOT}/tests/mm_kimi/kimik3_1p_vdriver.yaml" \
    2>&1 | tee "$LOG"
RC=${PIPESTATUS[0]}
echo "== trainer(PV) exit=$RC, log=$LOG =="
exit "$RC"
