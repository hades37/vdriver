#!/usr/bin/env bash
# vdriver 单卡用例:Kimi-K3 减层训练(流程验证,内核不模拟执行,数值非目标)
# 用法: bash tests/mm_kimi/run_1p.sh [MM_ROOT]
set -uo pipefail

MM_ROOT="${1:-/mnt/e/ubuntu-workspace/MindSpeed-MM}"
VD_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TK="/root/miniconda3/envs/mindspeed/Ascend/cann-9.1.0/x86_64-linux"
PY="${PY:-/root/miniconda3/envs/mindspeed/bin/python}"

cd "$MM_ROOT"

# 1) vdriver 抢位:libascend_hal.so + 哑 libtsdclient.so 置于搜索序最前
export LD_LIBRARY_PATH="${VD_ROOT}/build/lib:${TK}/lib64:${LD_LIBRARY_PATH:-}"
# 2) 标准 triton 3.2.0 前置(mindspeed env 的 triton_ascend 是命名空间碎片)
#    + vdriver_shim(sitecustomize:伪 mindspeed_ops eager conv1d 等)
export PYTHONPATH="${VD_ROOT}/tests/mm_kimi/vdriver_shim:/tmp/vd_triton_std:${MM_ROOT}:${PYTHONPATH:-}"
# 3) 训练脚本同款环境变量
export NON_MEGATRON=true
export TASK_QUEUE_ENABLE=1
export CPU_AFFINITY_CONF=0
export HCCL_CONNECT_TIMEOUT=1200
export VDRIVER_LOG="${VDRIVER_LOG:-1}"

# 0) 自愈 /tmp 垫片(标准 triton + vdriver 后端 + cann 占位)
bash "${VD_ROOT}/tests/mm_kimi/setup_shim.sh" > /dev/null

mkdir -p "${VD_ROOT}/tests/mm_kimi/logs"
LOG="${VD_ROOT}/tests/mm_kimi/logs/train_1p_$(date +%H%M%S).log"

torchrun --nproc_per_node 1 --nnodes 1 --node_rank 0 \
    --master_addr localhost --master_port 6099 \
    mindspeed_mm/fsdp/train/trainer.py \
    "${VD_ROOT}/tests/mm_kimi/kimik3_1p_vdriver.yaml" \
    2>&1 | tee "$LOG"
RC=${PIPESTATUS[0]}
echo "== trainer exit=$RC, log=$LOG =="
exit "$RC"
