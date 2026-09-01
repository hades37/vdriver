/**
 * tsdclient_dummy.c —— vdriver 附带的哑 libtsdclient.so
 *
 * 目的:runtime 的 TsdClientInit(runtime.cc:299-335)mmDlopen("libtsdclient.so") 后
 * dlsym("TsdOpen"/"TsdOpenEx")——LD_LIBRARY_PATH 前置本哑库时,符号缺失 → 提前返回,
 * tsdOpen_ 保持 nullptr → runtime.cc:1851 "no TsdOpen" 分支干净跳过 TSD 全路径
 * (TsdOpen/AicpuSd/FlowGw),SetDevice 不再因 hdc 会话失败而 507033。
 * 与决策 D7/D8 一致:mock 场景无设备守护进程。
 */

/* 显式占位符号:空翻译单元违反 ISO C 约束(终审风格②) */
int vdriver_tsdclient_dummy;
