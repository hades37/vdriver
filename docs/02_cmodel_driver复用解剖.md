# 调研报告 2:cmodel_driver 复用解剖(vdriver 参考)

> 调研代理 2 产出。对象:runtime/src/cmodel_driver 4 个 .c + 4 个 .h。

## 1. 初始化流程
`drvDriverStubInit`(driver_impl.c:135-167):①一次性守卫 `drvInitCheck`(123-133,原子计数);②纯软件步骤:stream/event/taskpool/sqcq 四张 ID 表 memset(138-141)、`drvQueueInit`(driver_queue.c:194-211,memset 环+sem_init+mutex)、`drvMemMgmtInit`(driver_mem.c:58-86,每设备 malloc 管理链表头尾);③`#ifndef __DRV_CFG_DEV_PLATFORM_ESL__` 内为仿真器步骤(144-164):`tsRegDrvReportIrqTriger(drvReportIrqTrigger)` 向 tsch 注册完成中断回调、读 `CAMODEL_LOG_PATH`、`startModel()`、`start_task_scheduler()`。

**startModel/start_task_scheduler 是真链接的库**:`startModel` 来自 model_api.h,由 pvmodel 变体链 `pvmodel_${product}`、camodel 变体链 `camodel_${product}`(CMakeLists.txt:131-139、193-202;module.mk:101、271)提供;`start_task_scheduler` 来自 libtsch,是纯主机线程。910B1/310B 等新平台不链仿真库(CMakeLists.txt:122-128、177-183),符号由 tests/cmodel_test/local_cmodel_build/driver_backend_shim.c 补:`startModel/stopModel/start_task_scheduler/stop_task_scheduler` 全空实现(shim:81-93),`busDirectRead/Write` 仅校验地址不拷贝(shim:40-50)。

**vdriver 结论**:保留①②,删除③(ESL 宏即现成"无仿真器"开关,impl:144-164);完成回报需自产(见 §4)。

## 2. 设备
`drvDeviceOpen`(driver_api.c:85-91):`devInfo` 被 UNUSED,**不写任何字段**,仅触发 init 后返回成功;接受伪设备号 64(impl:32、55);`drvGetDevIDs` 不填设备表(api:78-83),`drvGetDevNum` 恒答 1(api:71-76)。

`halGetDeviceInfo`(api:362-396)只答两类:(MODULE_TYPE_SYSTEM, INFO_TYPE_VERSION)→按平台宏返回配置字 mini_v1=0x0/mini_v2=0x10400/cloud_v1=0x100/lhisies=0x10301/lite=0xB0C00/mc62=0xE1000(api:19-24、369-381),即"芯片型号";(SYSTEM, INFO_TYPE_CORE_NUM)→1(382-383);**其余 infoType 一律 *value=0**(392),故 INFO_TYPE_ENV(=0)/INFO_TYPE_ADDR_MODE(=30)/INFO_TYPE_RUN_MACH(=31,ascend_hal_base.h:367-398)全返回 0;CLOUD_V1 下 CORE_NUM_LEVEL=1(385-390)。另有 `halGetChipCapability`:sdma_reduce_support=1、ts_group_number=5(api:615-622),`halGetCapabilityGroupInfo` 返回造出的假组信息(624-658)。

## 3. 内存
HBM_BASE=0x10000000,HBM_MAX=0x40000000(V200/V210 为 0x30000000),MAX_ALLOC=二者差(driver_mem.h:30-45)。分配(`drvMemAlloc`→`drvMemAllocDeviceHBM`,mem:256-289、114-179):size 向上 512B 对齐(265-270),在双链表上首次适配,节点切分/合并(131-176、181-217),返回地址=偏移+HBM_BASE(138-165)。

**是纯账本式假地址,无任何真实后备内存**:host malloc 只用于管理节点(67-68、143);HBM 实体由仿真器进程提供,pvmodel 下经 busDirect* 访问。`drvMemAddressTranslate` 恒等映射(api:102-108);`halHostRegister/Unregister` 空桩(api:136-151);`halMemAlloc` 对 MEM_HOST 真 malloc(api:111-122)。无 halMemcpy,对应 `drvMemcpy`(api:660-676)按地址落在 HBM 区间与否选 kind,进 `drvModelMemcpy`(mem:305-344):H2H=memcpy_s;H2D/D2H=busDirectWrite/Read(仿真器总线;shim 下只校验不拷贝——vdriver 须改为真 memcpy);D2D 不支持(默认分支报 LEN)。ESL 宏把 H2D/D2H 分支整体编译掉(315-328)。

## 4. 队列/完成(核心参考)
- **SQ**:`halSqMemGet` 从 `g_drvQosQueue[dev][qos]` 发 64B SQE 槽(api:223-266;queue.h:26、39-41,环深 512);`halSqMsgSend` 置 IsSubmit 后调 `drvSetTaskCommand`(api:268-310;queue:157-192),`drvSubmitCommand` 把 SQE 拷入 tsch 的 `ts_task_cmd_queue` 再 `ts_trigger_interrupt`(queue:134-155)。**SQE 消费者=libtsch 调度线程(即仿真器侧)**,没有直连仿真器的 driver 侧钩子;shim 下 ts_* 全为空转(shim:52-80)。
- **CQ**:完成报文由 tsch 产生并触发 `DRV_INTERRUPT_REPORT_READY`,驱动注册的 `drvReportIrqTrigger`(queue:213-236)把 ts 报告环搬入 `g_drvReportQueue`(深 64,queue.h:31;`drvMoveTsReport` queue:42-87)并 `drvSemPost`;`halCqReportGet` NORMAL 型 sem_wait 阻塞后逐条交出(api:575-584);`halCqReportIrqWait` 空桩(api:530-553);CALLBACK 型只回全局单槽 `g_ModelCqReport`(api:61、570-571),基本是占位。
- **vdriver 最简完成回报**:保留 `g_drvReportQueue+g_drvSem+halCqReportGet` 骨架不动,在 `halSqMsgSend` 提交点(或单主机线程消费 SQE 后)直接组装 12B report(streamId/taskId/sqId/sequenceId 等,api:31-45)写环并 `drvSemPost`,同步即时完成,无需定时器线程。**注意**:ESL 下 `drvMoveTsReport` 直接返回成功(queue:84-86)而 `halCqReportGet` 仍会死等信号量——不自产 report 必挂死,这是现成代码的坑。

## 5. 符号与构建
源仅 4 个 .c(CMakeLists.txt:12-17);头依赖 driver/ascend_hal.h、securec.h、mmpa、tsch/*、model_api.h(cmodel_driver.h:13-14、impl:25、mem:13-19、queue:13-14)。**libnpu_drv(ESL)只链 c_sec**(CMakeLists:80-84;module.mk:63),逻辑上"头+securec/libc"即可;但 tsch/model 头在主仓无实体(仅 tests shim 与 src/runtime/core/inc/tsch_defines.h),独立编译需自带这三个精简头(local_cmodel_build/CMakeLists.txt:16-28 先例;主构建 inc/ 为生成物,方式未确认)。**可独立编出 .so:可以**,把 ESL 宏改定义或用 shim 方案即可。

三变体:npu_drv=ESL 纯桩(CMakeLists:67-71;module.mk:65);pvmodel=+libtsch+lib_pvmodel(module.mk:101);camodel=+libtsch_camodel+libcamodel(module.mk:271);新平台三变体皆头接口+shim 补符号(CMakeLists:122-128、177-183)。导出非 static 函数 104 个:hal* 42、drv*/__drv* 57、AICPUModel* 3、cmodelDrv* 2;NpuDriver 绑定表在仓外 libascend_hal(driver 仓,未确认),runtime 源码内 0 处直接调用,交集需按 include/driver 中 DLLEXPORT 原型核对。

## 6. 复用判定

| 文件/函数 | 判定 | 说明 |
|---|---|---|
| driver_mem.c/h 全部(HBM 账本、合并、512 对齐) | 原样复用 | HBM_BASE/MAX、对齐直接沿用 |
| drvModelMemcpy H2H 分支 | 原样复用 | 即 memcpy_s |
| drvModelMemcpy H2D/D2H(busDirect) | 改造 | 换成对 host 后备内存的 memcpy_s(参 shim:40-50 但须真拷) |
| driver_queue.c 环+sem+mutex、queue.h | 原样复用 | CQ 骨架 |
| drvSubmitCommand/drvSetTaskCommand 的 ts_* 调用 | 替换 | SQE 改本机队列,提交点自产 report |
| drvMoveTsReport、tsRegDrvReportIrqTriger、drvReportIrqTrigger | 替换 | 完成源从 tsch 中断改本地 |
| driver_impl.c ID 管理与 init/exit | 改造 | 删 startModel/start_task_scheduler/ESL 块(144-164) |
| driver_api.c 其余桩(device/info/resource/shm/P2P 空实现) | 原样复用 | 已是合法假驱动 |
| halSqMemGet/halSqMsgSend/halCqReportGet | 改造 | 补真实 SQE 收容与即时完成 |
| ESL 条件块、module.mk、CMakeLists | 丢弃 | 参 local_cmodel_build 写独立 CMake |
| driver_backend_shim.c | 参考模板 | vdriver"无仿真器后端"的雏形 |
| startModel/stopModel/task_scheduler 依赖 | 丢弃 | 纯仿真器范畴 |

**工作量估算**:骨架移植+构建独立化 1-2 人日;完成回报改造(SQE 消费线程/report 生成)2-3 人日;内存真后备+memcpy 1-2 人日;与 torch_npu→libopapi→runtime 联调 3-5 人日;**合计约 7-12 人日**(若需 event/callback stream、多设备再加 3-5 人日)。
