# vdriver —— Ascend CANN 虚拟驱动(假 libascend_hal.so)
# 详见 实施方案.md;构建:cmake -B build -S . [-DRUNTIME_INC=/path/to/runtime/include]
#
# 符号导出口径(nm -D):强符号 T + 弱符号 W;
# h2c 桩基座按头文件生成(317 T + 62 W ≈ 378 函数,见 src/generated/ 再生成命令),
# 强语义模块(src/*.c)与桩同名符号通过 objcopy --weaken-symbol 去重,链接期以强定义为准。
cmake_minimum_required(VERSION 3.16)
project(vdriver C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# runtime 仓 include 路径(含 driver/ 下 ascend_hal*.h;与 9.1.0 安装包头逐字节一致,M0 已核验)
set(RUNTIME_INC "${CMAKE_CURRENT_SOURCE_DIR}/../runtime/include"
    CACHE PATH "Path to CANN runtime repo include dir")

if(NOT EXISTS "${RUNTIME_INC}/driver/ascend_hal_base.h")
    message(FATAL_ERROR "RUNTIME_INC(${RUNTIME_INC}) 下未找到 driver/ascend_hal_base.h,请传 -DRUNTIME_INC=<runtime仓>/include")
endif()

# ---------- 强语义符号清单(在桩中被宏重命名,由 src/ 模块给出强定义) ----------
# M1: 设备+内存;M2: SQ/CQ + 任务下发。
# 有意留在桩的符号(h2c return-0 恰为所需语义,勿误判遗漏):
#   halGetAPIVersion(返回 0 即可)、halHostUnregister 系列(VIRTUAL+flat 下被
#   RUNTIME_WHEN_NO_VIRTUAL_MODEL_RETURN 短路)、halSupportFeature(空降级走 legacy)、
#   halMemcpySumbit/Wait(禁止实现——空桩丢数据,缺席使 H2D 退化 SYNC,见方案 §3.3)。
set(VDRIVER_STRONG_SYMBOLS
    halGetDeviceInfo
    halDeviceOpen
    halDeviceClose
    halGetChipCapability
    halGetSocVersion
    drvGetDevNum
    drvGetDevIDs
    drvGetPlatformInfo
    drvMemSmmuQuery
    halMemAlloc
    halMemFree
    halMemcpy
    halHostRegister
    halSqCqAllocate
    halSqCqFree
    halSqCqQuery
    halSqCqConfig
    halSqTaskSend
    halCqReportIrqWait
    halCqReportGet
    halReportRelease
    halResourceIdAlloc
    halResourceIdFree
)

# ---------- h2c 桩基座:OBJECT 库,强语义符号经编译期宏重命名去重 ----------
# 方案:对强语义符号 -D<sym>=vdrv_stub_<sym>,桩内定义/声明同步改名,不与强模块冲突;
# 链接后真实符号由 src/ 模块导出,vdrv_stub_* 仅作桩语义备份。(评审建议项:同符号去重)
add_library(hal_stub_obj OBJECT src/generated/ascend_hal_stub.c)
target_include_directories(hal_stub_obj PRIVATE
    ${RUNTIME_INC}
    ${RUNTIME_INC}/driver)
foreach(sym IN LISTS VDRIVER_STRONG_SYMBOLS)
    target_compile_definitions(hal_stub_obj PRIVATE ${sym}=vdrv_stub_${sym})
endforeach()
target_compile_options(hal_stub_obj PRIVATE -fstack-protector-strong)

# ---------- 主库 ----------
add_library(vdriver SHARED
    $<TARGET_OBJECTS:hal_stub_obj>
    src/vdriver.c
    src/device.c
    src/memory.c
    src/sqcq.c
    src/sqe_interp.cpp
    src/res.c)

target_include_directories(vdriver PRIVATE
    ${RUNTIME_INC}
    ${RUNTIME_INC}/driver
    src/inc)

target_compile_options(vdriver PRIVATE -fstack-protector-strong)
target_link_libraries(vdriver PRIVATE ${CMAKE_DL_LIBS} pthread)
# 导出白名单:契约符号 hal*/drv* + 内部 vdriver*/sqe_interp*;vdrv_stub_* 收敛 local
target_link_options(vdriver PRIVATE
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/vdriver.map")

# 关键:无 VERSION 属性 → CMake 自动 SONAME=libascend_hal.so,DT_NEEDED 按名命中(D2)
set_target_properties(vdriver PROPERTIES
    OUTPUT_NAME "ascend_hal"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# 哑 libtsdclient:runtime TsdClientInit 的 dlsym(TsdOpen/TsdOpenEx) 缺符号 →
# tsdOpen_=NULL → "no TsdOpen"(runtime.cc:1851)整体跳过 TSD 路径,SetDevice 不再
# 因 hdc 会话失败而报错(M3 L1 实测抓到)。与 D7/D8(无设备守护进程)一致。
add_library(tsdclient_dummy SHARED src/dummy/tsdclient_dummy.c)
set_target_properties(tsdclient_dummy PROPERTIES
    OUTPUT_NAME "tsdclient"
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ---------- 测试 ----------
enable_testing()

# M0:同名抢占/SONAME/符号契约抽查(需真实头文件,评审严重项修复:不再手抄原型)
add_executable(test_m0_load tests/test_m0_load.cpp)
target_include_directories(test_m0_load PRIVATE ${RUNTIME_INC}/driver)
target_link_libraries(test_m0_load PRIVATE ${CMAKE_DL_LIBS})
set_target_properties(test_m0_load PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

# M1:设备+内存符号契约测试(直接链接 vdriver,不依赖 dlopen 搜索序)
add_executable(test_m1_device_memory tests/test_m1_device_memory.cpp)
target_include_directories(test_m1_device_memory PRIVATE
    ${RUNTIME_INC}/driver
    src/inc)
target_link_libraries(test_m1_device_memory PRIVATE vdriver pthread)
add_dependencies(test_m1_device_memory vdriver)
set_target_properties(test_m1_device_memory PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

# M2:SQ/CQ 记账 + SQE 解释执行测试
add_executable(test_m2_sqcq tests/test_m2_sqcq.cpp)
target_include_directories(test_m2_sqcq PRIVATE
    ${RUNTIME_INC}/driver
    src/inc)
target_link_libraries(test_m2_sqcq PRIVATE vdriver pthread)
add_dependencies(test_m2_sqcq vdriver)
set_target_properties(test_m2_sqcq PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

# glibc 启动时固化搜索路径,必须在启动环境前置 vdriver 目录;
# configure 期判空拼接,避免空 $ENV{} 产生尾随冒号(glibc 解释为当前工作目录,评审严重项)
if(DEFINED ENV{LD_LIBRARY_PATH} AND NOT "$ENV{LD_LIBRARY_PATH}" STREQUAL "")
    set(M0_TEST_LDLP "${CMAKE_BINARY_DIR}/lib:$ENV{LD_LIBRARY_PATH}")
else()
    set(M0_TEST_LDLP "${CMAKE_BINARY_DIR}/lib")
endif()

add_test(NAME m0_load
    COMMAND test_m0_load $<TARGET_FILE_DIR:vdriver>)
set_tests_properties(m0_load PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=${M0_TEST_LDLP}")

add_test(NAME m1_device_memory
    COMMAND test_m1_device_memory)
set_tests_properties(m1_device_memory PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=${M0_TEST_LDLP}")

add_test(NAME m2_sqcq
    COMMAND test_m2_sqcq)
set_tests_properties(m2_sqcq PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=${M0_TEST_LDLP}")
