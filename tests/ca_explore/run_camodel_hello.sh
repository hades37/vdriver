#!/usr/bin/env bash
# CA/PV 仿真路线(已验证 ✅):hello_cann 跑官方 libruntime_cmodel + libnpu_drv_pvmodel
# 内核在 x86 上真仿真执行,结果数值正确("Sample run successfully!" + 8/8 数值匹配)
#
# 原理:
#   libruntime.so        → libruntime_cmodel.so   (官方仿真 runtime,PV 模型)
#   libascend_hal.so     → libnpu_drv_pvmodel.so  (官方仿真驱动,满足 hal DT_NEEDED)
#   + simulator/Ascend910B1/lib 的仿真模型库(libmodel_top/libstars_pv/libffts_model/...)
#   ⚠ 必须保证 hal*/drv* 符号全部绑定到仿真驱动:不得让任何真包/自研
#     libascend_hal.so 混入搜索路径(符号 interposition 会让调用错路由)。
set -uo pipefail
TK="/root/miniconda3/envs/mindspeed/Ascend/cann-9.1.0"
SIM="${TK}/x86_64-linux/simulator/Ascend910B1"
CA_LIB=/tmp/ca_lib
mkdir -p "$CA_LIB"
ln -sf "${SIM}/lib/libruntime_cmodel.so"   "$CA_LIB/libruntime.so"      # PV runtime 顶替
ln -sf "${SIM}/lib/libnpu_drv_pvmodel.so"  "$CA_LIB/libascend_hal.so"   # PV 驱动满足 hal DT_NEEDED
ln -sf "${SIM}/lib/"*.so "$CA_LIB/" 2>/dev/null
BIN="${BIN:-$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)/tests/l1/hello_cann}"
cd "${SIM}"   # 仿真模型从 cwd 读 config*.json
exec env LD_LIBRARY_PATH="${CA_LIB}:${TK}/lib64:${SIM}/lib:${TK}/../lib64" \
    CAMODEL_LOG_PATH=/tmp/camlog \
    "${BIN}" "$@"
