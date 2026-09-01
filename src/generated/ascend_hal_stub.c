#include "ascend_hal.h"

// stub for ascend_hal.h
DLLEXPORT void ascendHalCompileStub(void)
{

}
#include "ascend_hal_external.h"

// stub for ascend_hal_external.h
DLLEXPORT drvError_t halCentreNotifySet(int index, int value)
{
    return 0;
}
DLLEXPORT drvError_t halCentreNotifyGet(int index, int *value)
{
    return 0;
}
DLLEXPORT int halBuffFree(void *buff)
{
    return 0;
}
DLLEXPORT int halBuffCreatePool(struct mp_attr *attr, struct mempool_t **mp)
{
    return 0;
}
DLLEXPORT int halBuffDeletePool(struct mempool_t *mp)
{
    return 0;
}
DLLEXPORT int halMbufAlloc(uint64_t size, Mbuf **mbuf)
{
    return 0;
}
DLLEXPORT int halMbufAllocByPool(poolHandle pHandle, Mbuf **mbuf)
{
    return 0;
}
DLLEXPORT int halMbufFree(Mbuf *mbuf)
{
    return 0;
}
DLLEXPORT int halMbufGetDataPtr(Mbuf *mbuf, void **buf, uint64_t *size)
{
    return 0;
}
DLLEXPORT int halMbufGetBuffAddr(Mbuf *mbuf, void **buf)
{
    return 0;
}
DLLEXPORT int halMbufGetBuffSize(Mbuf *mbuf, uint64_t *totalSize)
{
    return 0;
}
DLLEXPORT int halMbufSetDataLen(Mbuf *mbuf, uint64_t len)
{
    return 0;
}
DLLEXPORT int halMbufGetDataLen(Mbuf *mbuf, uint64_t *len)
{
    return 0;
}
DLLEXPORT int halMbufCopyRef(Mbuf *mbuf, Mbuf **newMbuf)
{
    return 0;
}
DLLEXPORT int halMbufChainAppend(Mbuf *mbufChainHead, Mbuf *mbuf)
{
    return 0;
}
DLLEXPORT int halMbufChainGetMbufNum(Mbuf *mbufChainHead, unsigned int *num)
{
    return 0;
}
DLLEXPORT int halMbufChainGetMbuf(Mbuf *mbufChainHead, unsigned int index, Mbuf **mbuf)
{
    return 0;
}
DLLEXPORT int halGrpCreate(const char *name, GroupCfg *cfg)
{
    return 0;
}
DLLEXPORT int halGrpAddProc(const char *name, int pid, GroupShareAttr attr)
{
    return 0;
}
DLLEXPORT int halGrpAttach(const char *name, int timeout)
{
    return 0;
}
DLLEXPORT int halBuffInit(BuffCfg *cfg)
{
    return 0;
}
DLLEXPORT int halBuffCfg(enum BuffConfCmdType cmd, void *data, unsigned int len)
{
    return 0;
}
DLLEXPORT int halBuffGetInfo(enum BuffGetCmdType cmd, void *inBuff, unsigned int inLen,
    void *outBuff, unsigned int *outLen)
{
    return 0;
}
DLLEXPORT int halBuffRecycleByPid(int pid)
{
    return 0;
}
DLLEXPORT drvError_t halEschedAttachDevice(unsigned int devId)
{
    return 0;
}
DLLEXPORT drvError_t halEschedCreateGrp(unsigned int devId, unsigned int grpId, GROUP_TYPE type)
{
    return 0;
}
DLLEXPORT drvError_t halEschedSubscribeEvent(unsigned int devId, unsigned int grpId,
    unsigned int threadId, unsigned long long eventBitmap)
{
    return 0;
}
DLLEXPORT drvError_t halEschedSetPidPriority(unsigned int devId, SCHEDULE_PRIORITY priority)
{
    return 0;
}
DLLEXPORT drvError_t halEschedWaitEvent(unsigned int devId, unsigned int grpId,
    unsigned int threadId, int timeout, struct event_info *event)
{
    return 0;
}
DLLEXPORT drvError_t halQueueInit(unsigned int devId)
{
    return 0;
}
DLLEXPORT drvError_t halQueueDestroy(unsigned int devId, unsigned int qid)
{
    return 0;
}
DLLEXPORT drvError_t halQueueGetStatus(unsigned int devId, unsigned int qid, QUEUE_QUERY_ITEM queryItem,
    unsigned int len,  void *data)
{
    return 0;
}
DLLEXPORT drvError_t halQueueQueryInfo(unsigned int devId, unsigned int qid, QueueInfo *queInfo)
{
    return 0;
}
DLLEXPORT drvError_t halQueueGetQidsbyPid(unsigned int devId, unsigned int pid,
       unsigned int maxQueSize, QidsOfPid *info)
{
    return 0;
}
#include "ascend_hal_base.h"

// stub for ascend_hal_base.h
DLLEXPORT drvError_t halDeviceOpen(uint32_t devid, halDevOpenIn *in, halDevOpenOut *out)
{
    return 0;
}
DLLEXPORT drvError_t halDeviceClose(uint32_t devid, halDevCloseIn *in)
{
    return 0;
}
DLLEXPORT drvError_t halProcessResBackup(halProcResBackupInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halProcessResRestore(halProcResRestoreInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t drvDeviceStateNotifierRegister(drvDeviceStateNotify state_callback)
{
    return 0;
}
DLLEXPORT drvError_t drvDeviceStartupRegister(drvDeviceStartupNotify startup_callback)
{
    return 0;
}
DLLEXPORT drvError_t halGetChipCapability(uint32_t devId, struct halCapabilityInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halGetCapabilityGroupInfo(int device_id, int ts_id, int group_id,
    struct capability_group_info *group_info, int group_count)
{
    return 0;
}
DLLEXPORT drvError_t halGetAPIVersion(int *halAPIVersion)
{
    return 0;
}
DLLEXPORT void halSetRuntimeApiVer(int Version)
{

}
DLLEXPORT drvError_t drvDeviceStatus(uint32_t devId, drvStatus_t *status)
{
    return 0;
}
DLLEXPORT drvError_t drvDeviceOpen(void **devInfo, uint32_t devId)
{
    return 0;
}
DLLEXPORT drvError_t drvDeviceClose(uint32_t devId)
{
    return 0;
}
DLLEXPORT drvError_t drvDeviceGetTransWay(void *src, void *dest, uint8_t *trans_type)
{
    return 0;
}
DLLEXPORT drvError_t drvGetPlatformInfo(uint32_t *info)
{
    return 0;
}
DLLEXPORT drvError_t drvGetDevNum(uint32_t *num_dev)
{
    return 0;
}
DLLEXPORT drvError_t drvGetDevIDByLocalDevID(uint32_t localDevId, uint32_t *devId)
{
    return 0;
}
DLLEXPORT drvError_t halGetDevProbeList(uint32_t *devices, uint32_t len)
{
    return 0;
}
DLLEXPORT drvError_t drvGetDevIDs(uint32_t *devices, uint32_t len)
{
    return 0;
}
DLLEXPORT drvError_t drvGetDeviceLocalIDs(uint32_t *devices, uint32_t len)
{
    return 0;
}
DLLEXPORT drvError_t drvGetLocalDevIDByHostDevID(uint32_t host_dev_id, uint32_t *local_dev_id)
{
    return 0;
}
DLLEXPORT drvError_t halSensorNodeRegister(uint32_t devId, struct halSensorNodeCfg *cfg, uint64_t *handle)
{
    return 0;
}
DLLEXPORT drvError_t halSensorNodeUnregister(uint32_t devId, uint64_t handle)
{
    return 0;
}
DLLEXPORT drvError_t halSensorNodeUpdateState(uint32_t devId, uint64_t handle, int val,
    halGeneralEventType_t assertion)
{
    return 0;
}
drvError_t halGetSocVersion(uint32_t devId, char *socVersion, uint32_t len)
{
    return 0;
}
DLLEXPORT drvError_t halGetDeviceInfo(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value)
{
    return 0;
}
DLLEXPORT drvError_t halGetDeviceInfoByBuff(uint32_t devId, int32_t moduleType,
                                            int32_t infoType, void *buf, int32_t *size)
{
    return 0;
}
DLLEXPORT drvError_t halSetDeviceInfoByBuff(uint32_t devId, int32_t moduleType,
                                            int32_t infoType, void *buf, int32_t size)
{
    return 0;
}
DLLEXPORT drvError_t halRepairFault(uint32_t devid, halRepairFaultInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halGetPhyDeviceInfo(uint32_t phyId, int32_t moduleType, int32_t infoType, int64_t *value)
{
    return 0;
}
DLLEXPORT drvError_t halGetPairDevicesInfo(uint32_t devId, uint32_t otherDevId, int32_t infoType, int64_t *value)
{
    return 0;
}
DLLEXPORT drvError_t halGetPairPhyDevicesInfo(uint32_t devId, uint32_t otherDevId, int32_t infoType, int64_t *value)
{
    return 0;
}
DLLEXPORT drvError_t drvDeviceExceptionHookRegister(drvDeviceExceptionReporFunc exception_callback_func)
{
    return 0;
}
DLLEXPORT void drvFlushCache(uint64_t base, uint32_t len)
{

}
DLLEXPORT drvError_t drvDeviceGetPhyIdByIndex(uint32_t devIndex, uint32_t *phyId)
{
    return 0;
}
DLLEXPORT drvError_t drvDeviceGetIndexByPhyId(uint32_t phyId, uint32_t *devIndex)
{
    return 0;
}
DLLEXPORT drvError_t drvGetProcessSign(struct process_sign *sign)
{
    return 0;
}
DLLEXPORT drvError_t halQueryDevpid(struct halQueryDevpidInfo info, pid_t *dev_pid)
{
    return 0;
}
DLLEXPORT drvError_t drvBindHostPid(struct drvBindHostpidInfo info)
{
    return 0;
}
DLLEXPORT drvError_t drvUnbindHostPid(struct drvBindHostpidInfo info)
{
    return 0;
}
DLLEXPORT drvError_t drvQueryProcessHostPid(
    int pid, unsigned int *chip_id, unsigned int *vfid, unsigned int *host_pid, unsigned int *cp_type)
{
    return 0;
}
DLLEXPORT drvError_t halResAddrMap(
    unsigned int devId, struct res_addr_info *res_info, unsigned long *va, unsigned int *len)
{
    return 0;
}
DLLEXPORT drvError_t halResAddrUnmap(unsigned int devId, struct res_addr_info *res_info)
{
    return 0;
}
DLLEXPORT drvError_t halResAddrMapV2(unsigned int devId, struct res_map_info_in *res_info_in,
    struct res_map_info_out *res_info_out)
{
    return 0;
}
DLLEXPORT drvError_t halResAddrUnmapV2(unsigned int devId, struct res_map_info_in *res_info_in)
{
    return 0;
}
DLLEXPORT drvError_t halBindCgroup(BIND_CGROUP_TYPE bindType)
{
    return 0;
}
DLLEXPORT drvError_t halSetMemSharing(struct drvMemSharingPara *para)
{
    return 0;
}
DLLEXPORT pid_t drvDeviceGetBareTgid(void)
{
    return 0;
}
DLLEXPORT drvError_t drvMemRead(uint32_t devId, MEM_CTRL_TYPE memType, uint32_t offset, uint8_t *value, uint32_t len)
{
    return 0;
}
DLLEXPORT drvError_t drvMemWrite(uint32_t devId, MEM_CTRL_TYPE memType, uint32_t offset, uint8_t *value, uint32_t len)
{
    return 0;
}
DLLEXPORT drvError_t halDeviceEnableP2P(uint32_t dev, uint32_t peer_dev, uint32_t flag)
{
    return 0;
}
DLLEXPORT drvError_t halDeviceDisableP2P(uint32_t dev, uint32_t peer_dev, uint32_t flag)
{
    return 0;
}
DLLEXPORT drvError_t drvGetP2PStatus(uint32_t dev, uint32_t peer_dev, uint32_t *status)
{
    return 0;
}
DLLEXPORT drvError_t halDeviceCanAccessPeer(int *can_access_peer, uint32_t dev, uint32_t peer_dev)
{
    return 0;
}
DLLEXPORT drvError_t drvGetDeviceBootStatus(int phy_id, uint32_t *boot_status)
{
    return 0;
}
DLLEXPORT drvError_t drvCloseIpcNotify(const char *name, struct drvIpcNotifyInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halShrIdCreate(struct drvShrIdInfo *info, char *name, uint32_t name_len)
{
    return 0;
}
DLLEXPORT drvError_t halShrIdDestroy(const char *name)
{
    return 0;
}
DLLEXPORT drvError_t halShrIdOpen(const char *name, struct drvShrIdInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halShrIdClose(const char *name)
{
    return 0;
}
DLLEXPORT drvError_t halShrIdSetAttribute(const char *name, enum shrIdAttrType type, struct shrIdAttr attr)
{
    return 0;
}
DLLEXPORT drvError_t halShrIdGetAttribute(const char *name, enum shrIdAttrType type, struct shrIdAttr *attr)
{
    return 0;
}
DLLEXPORT drvError_t halShrIdInfoGet(const char *name, struct shrIdGetInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halShrIdSetPid(const char *name, pid_t pid[], uint32_t pid_num)
{
    return 0;
}
DLLEXPORT drvError_t halShrIdSetPodPid(const char *name, uint32_t sdid, pid_t pid)
{
    return 0;
}
DLLEXPORT drvError_t halShrIdRecord(const char *name)
{
    return 0;
}
DLLEXPORT void drvDfxShowReport(uint32_t devId)
{

}
DLLEXPORT DV_OFF_ONLINE DVresult drvMemAddressTranslate(DVdeviceptr vptr, UINT64 *pptr)
{
    return 0;
}
DLLEXPORT DV_OFF_ONLINE DVresult drvMemSmmuQuery(DVdevice device, UINT32 *SSID)
{
    return 0;
}
DLLEXPORT DV_OFF_ONLINE DVresult drvMemsetD8(DVdeviceptr dst, size_t destMax, UINT8 value, size_t num)
{
    return 0;
}
DLLEXPORT DV_OFF_ONLINE DVresult drvMemcpy(DVdeviceptr dst, size_t dest_max, DVdeviceptr src, size_t byte_count)
{
    return 0;
}
DLLEXPORT drvError_t halMemcpy(void *dst, size_t dst_size, void *src, size_t count, struct memcpy_info *info)
{
    return 0;
}
DLLEXPORT DV_OFF_ONLINE DVresult halMemCpyAsync(
    DVdeviceptr dst, size_t dest_max, DVdeviceptr src, size_t byte_count, uint64_t *copy_fd)
{
    return 0;
}
DLLEXPORT DV_OFF_ONLINE DVresult halMemCpyAsyncWaitFinish(uint64_t copy_fd)
{
    return 0;
}
DLLEXPORT DV_ONLINE drvError_t halMemcpy2D(struct MEMCPY2D *p_copy)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halMemcpyBatch(uint64_t dst[], uint64_t src[], size_t size[], size_t count)
{
    return 0;
}
DLLEXPORT DV_OFFLINE drvError_t halSdmaCopy(
    DVdeviceptr dst, size_t dst_size, DVdeviceptr src, size_t len)
{
    return 0;
}
DLLEXPORT DV_OFFLINE drvError_t halSdmaBatchCopy(
    void *dst[], void *src[], size_t size[], int count)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult drvMemConvertAddr(
    DVdeviceptr p_src, DVdeviceptr p_dst, UINT32 len, struct DMA_ADDR *dma_addr)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult drvMemDestroyAddr(struct DMA_ADDR *ptr)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halMemDestroyAddrBatch(struct DMA_ADDR *ptr[], uint32_t num)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halMemcpySumbit(struct DMA_ADDR *dma_addr, int flag)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halMemcpyWait(struct DMA_ADDR *dma_addr)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult drvMemPrefetchToDevice(DVdeviceptr dev_ptr, size_t len, DVdevice device)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halShmemCreateHandle(DVdeviceptr vptr, size_t byte_count, char *name, uint32_t name_len)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halShmemDestroyHandle(const char *name)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halShmemSetPidHandle(const char *name, int pid[], int num)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halShmemSetPodPid(const char *name, uint32_t sdid, int pid[], int num)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halShmemOpenHandle(const char *name, DVdeviceptr *vptr)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halShmemOpenHandleByDevId(DVdevice dev_id, const char *name, DVdeviceptr *vptr)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halShmemCloseHandle(DVdeviceptr vptr)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halShmemSetAttribute(const char *name, uint32_t type, uint64_t attr)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halShmemGetAttribute(const char *name, enum ShmemAttrType type, uint64_t *attr)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halShmemInfoGet(const char *name, struct ShmemGetInfo *info)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult drvMemGetAttribute(DVdeviceptr vptr, struct DVattribute *attr)
{
    return 0;
}
DLLEXPORT DV_ONLINE drvError_t halMemAgentOpen(uint32_t devid, uint32_t flag)
{
    return 0;
}
DLLEXPORT DV_ONLINE drvError_t halMemAgentClose(uint32_t devid, uint32_t flag)
{
    return 0;
}
DLLEXPORT DV_ONLINE int drvMemDeviceOpen(uint32_t devid, int devfd)
{
    return 0;
}
DLLEXPORT DV_ONLINE int drvMemDeviceClose(uint32_t devid)
{
    return 0;
}
DLLEXPORT drvError_t halHostRegister(void *src_ptr, UINT64 size, UINT32 flag, UINT32 devid, void **dst_ptr)
{
    return 0;
}
DLLEXPORT drvError_t halHostUnregister(void *src_ptr, UINT32 devid)
{
    return 0;
}
DLLEXPORT drvError_t halHostUnregisterEx(void *src_ptr, UINT32 devid, UINT32 flag)
{
    return 0;
}
DLLEXPORT drvError_t halHostRegisterCapabilities(uint32_t devid, uint32_t acc_module_type, uint32_t *mem_map_cap)
{
    return 0;
}
DLLEXPORT drvError_t halMemAlloc(void **pp, unsigned long long size, unsigned long long flag)
{
    return 0;
}
DLLEXPORT drvError_t halMemFree(void *pp)
{
    return 0;
}
DLLEXPORT drvError_t halMemAdvise(DVdeviceptr ptr, size_t count, unsigned int type, DVdevice device)
{
    return 0;
}
DLLEXPORT DV_ONLINE drvError_t halCheckProcessStatus(
    DVdevice device, processType_t process_type, processStatus_t status, bool *is_matched)
{
    return 0;
}
DLLEXPORT DV_ONLINE drvError_t halCheckProcessStatusEx(
    DVdevice device, processType_t process_type, processStatus_t status, struct drv_process_status_output *out)
{
    return 0;
}
DLLEXPORT DV_ONLINE drvError_t halMemGetAddressReserveRange(
    void **ptr, size_t *size, drv_mem_addr_reserve_type type, uint64_t flag)
{
    return 0;
}
DLLEXPORT drvError_t halMemAddressReserve(void **ptr, size_t size, size_t alignment, void *addr, uint64_t flag)
{
    return 0;
}
DLLEXPORT drvError_t halMemAddressFree(void *ptr)
{
    return 0;
}
DLLEXPORT drvError_t halMemCreate(
    drv_mem_handle_t **handle, size_t size, const struct drv_mem_prop *prop, uint64_t flag)
{
    return 0;
}
DLLEXPORT drvError_t halMemRelease(drv_mem_handle_t *handle)
{
    return 0;
}
DLLEXPORT drvError_t halMemRetainAllocationHandle(drv_mem_handle_t **handle, void *ptr)
{
    return 0;
}
DLLEXPORT drvError_t halMemMap(void *ptr, size_t size, size_t offset, drv_mem_handle_t *handle, uint64_t flag)
{
    return 0;
}
DLLEXPORT drvError_t halMemUnmap(void *ptr)
{
    return 0;
}
DLLEXPORT drvError_t halMemSetAccess(void *ptr, size_t size, struct drv_mem_access_desc *desc, size_t count)
{
    return 0;
}
DLLEXPORT drvError_t halMemGetAccess(void *ptr, struct drv_mem_location *location, uint64_t *flags)
{
    return 0;
}
DLLEXPORT drvError_t halMemExportToShareableHandle(
    drv_mem_handle_t *handle, drv_mem_handle_type handle_type, uint64_t flags, uint64_t *shareable_handle)
{
    return 0;
}
DLLEXPORT drvError_t halMemExportToShareableHandleV2(
    drv_mem_handle_t *handle, drv_mem_handle_type handle_type, uint64_t flags, struct MemShareHandle *share_handle)
{
    return 0;
}
DLLEXPORT drvError_t halMemImportFromShareableHandle(
    uint64_t shareable_handle, uint32_t devid, drv_mem_handle_t **handle)
{
    return 0;
}
DLLEXPORT drvError_t halMemImportFromShareableHandleV2(
    drv_mem_handle_type handle_type, struct MemShareHandle *share_handle, uint32_t devid, drv_mem_handle_t **handle)
{
    return 0;
}
DLLEXPORT drvError_t halMemTransShareableHandle(drv_mem_handle_type handle_type, struct MemShareHandle *share_handle,
    uint32_t *server_id, uint64_t *shareable_handle)
{
    return 0;
}
DLLEXPORT drvError_t halMemSetPidToShareableHandle(uint64_t shareable_handle, int pid[], uint32_t pid_num)
{
    return 0;
}
DLLEXPORT drvError_t halMemShareHandleSetAttribute(
    uint64_t shareable_handle, enum ShareHandleAttrType type, struct ShareHandleAttr attr)
{
    return 0;
}
DLLEXPORT drvError_t halMemShareHandleGetAttribute(
    uint64_t shareable_handle, enum ShareHandleAttrType type, struct ShareHandleAttr *attr)
{
    return 0;
}
DLLEXPORT drvError_t halMemShareHandleInfoGet(uint64_t shareable_handle, struct ShareHandleGetInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halMemGetAllocationGranularity(
    const struct drv_mem_prop *prop, drv_mem_granularity_options option, size_t *granularity)
{
    return 0;
}
DLLEXPORT drvError_t halMemGetAddressRange(DVdeviceptr ptr, DVdeviceptr *pbase, size_t *psize)
{
    return 0;
}
DLLEXPORT drvError_t halMemRegUbSegment(uint32_t devid, uint64_t va, uint64_t size)
{
    return 0;
}
DLLEXPORT drvError_t halMemUnRegUbSegment(uint32_t devid, uint64_t va, uint64_t size)
{
    return 0;
}
DLLEXPORT drvError_t halMemHandleSetAttribute(drv_mem_handle_t *handle, HandleAttrType type, HandleAttr attr)
{
    return 0;
}
DLLEXPORT drvError_t halMemHandleGetAttribute(drv_mem_handle_t *handle, HandleAttrType type, HandleAttr *attr)
{
    return 0;
}
DLLEXPORT DVresult halMemGetInfoEx(DVdevice device, unsigned int type, struct MemInfo *info)
{
    return 0;
}
DLLEXPORT DVresult halMemGetInfo(DVdevice device, unsigned int type, struct MemInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halMemCtl(
    int type, void *param_value, size_t param_value_size, void *out_value, size_t *out_size_ret)
{
    return 0;
}
DLLEXPORT drvError_t halGetMemUsageInfo(uint32_t dev_id, struct mem_module_usage *mem_usage, size_t in_num, size_t *out_num)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcGetCapacity(struct drvHdcCapacity *capacity)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcClientCreate(HDC_CLIENT *client, int maxSessionNum, int serviceType, int flag)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcClientDestroy(HDC_CLIENT client)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcSessionConnect(int peer_node, int peer_devid, HDC_CLIENT client, HDC_SESSION *session)
{
    return 0;
}
DLLEXPORT hdcError_t halHdcSessionConnectEx(int peer_node, int peer_devid, int peer_pid, HDC_CLIENT client,
    HDC_SESSION *pSession)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcServerCreate(int devid, int serviceType, HDC_SERVER *pServer)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcServerDestroy(HDC_SERVER server)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcSessionAccept(HDC_SERVER server, HDC_SESSION *session)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcSessionClose(HDC_SESSION session)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcAllocMsg(HDC_SESSION session, struct drvHdcMsg **ppMsg, int count)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcFreeMsg(struct drvHdcMsg *msg)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcReuseMsg(struct drvHdcMsg *msg)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcAddMsgBuffer(struct drvHdcMsg *msg, char *pBuf, int len)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcGetMsgBuffer(struct drvHdcMsg *msg, int index, char **pBuf, int *pLen)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcRecvPeek(HDC_SESSION session, int *msgLen, int flag)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcRecvBuf(HDC_SESSION session, char *pBuf, int bufLen, int *msgLen)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcSetSessionReference(HDC_SESSION session)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcGetTrustedBasePath(int peer_node, int peer_devid, char *base_path, unsigned int path_len)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcSendFile(int peer_node, int peer_devid, const char *file, const char *dst_path,
    void (*progress_notifier)(struct drvHdcProgInfo *))
{
    return 0;
}
DLLEXPORT drvError_t drvHdcGetTrustedBasePathV2(int peer_node, int peer_devid, char *base_path, unsigned int path_len)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcSendFileV2(int peer_node, int peer_devid, const char *file, const char *dst_path,
    void (*progress_notifier)(struct drvHdcProgInfo *))
{
    return 0;
}
DLLEXPORT void *drvHdcMallocEx(enum drvHdcMemType mem_type, void *addr, unsigned int align, unsigned int len, int devid,
    unsigned int flag)
{
    return NULL;
}
DLLEXPORT drvError_t drvHdcFreeEx(enum drvHdcMemType mem_type, void *buf)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcDmaMap(enum drvHdcMemType mem_type, void *buf, int devid)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcDmaUnMap(enum drvHdcMemType mem_type, void *buf)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcDmaReMap(enum drvHdcMemType mem_type, void *buf, int devid)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcEpollCreate(int size, HDC_EPOLL *epoll)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcEpollClose(HDC_EPOLL epoll)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcEpollCtl(HDC_EPOLL epoll, int op, void *target, struct drvHdcEvent *event)
{
    return 0;
}
DLLEXPORT drvError_t drvHdcEpollWait(HDC_EPOLL epoll, struct drvHdcEvent *events, int maxevents, int timeout,
                                     int *eventnum)
{
    return 0;
}
DLLEXPORT drvError_t halHdcGetSessionInfo(HDC_SESSION session, struct drvHdcSessionInfo *info)
{
    return 0;
}
DLLEXPORT hdcError_t halHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, UINT64 flag, UINT32 timeout)
{
    return 0;
}
DLLEXPORT hdcError_t halHdcFastSend(HDC_SESSION session, struct drvHdcFastSendMsg msg, UINT64 flag, UINT32 timeout)
{
    return 0;
}
DLLEXPORT hdcError_t halHdcRecv(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
    UINT64 flag, int *recvBufCount, UINT32 timeout)
{
    return 0;
}
DLLEXPORT hdcError_t halHdcRecvEx(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
    int *recvBufCount, struct drvHdcRecvConfig *userConfig)
{
    return 0;
}
DLLEXPORT hdcError_t halHdcFastRecv(HDC_SESSION session, struct drvHdcFastRecvMsg *msg, UINT64 flag, UINT32 timeout)
{
    return 0;
}
DLLEXPORT drvError_t halHdcGetSessionAttr(HDC_SESSION session, int attr, int *value)
{
    return 0;
}
DLLEXPORT hdcError_t halHdcGetServerAttr(HDC_SERVER server, int attr, int *value)
{
    return 0;
}
DLLEXPORT hdcError_t halHdcNotifyRegister(int service_type, struct HdcSessionNotify *notify)
{
    return 0;
}
DLLEXPORT void halHdcNotifyUnregister(int service_type)
{

}
DLLEXPORT hdcError_t halHdcSessionCloseEx(HDC_SESSION session, int type)
{
    return 0;
}
int log_read_by_type(int device_id, char *buf, unsigned int *size, int timeout, enum log_channel_type channel_type)
{
    return 0;
}
int log_get_channel_type(int device_id, int *channel_type_set, int *channel_type_num, int set_size)
{
    return 0;
}
int log_set_dfx_param(uint32_t devid, uint32_t chan_type, uint32_t cmd_type, void *data, uint32_t data_len)
{
    return 0;
}
int log_get_dfx_param(uint32_t devid, uint32_t chan_type, uint32_t cmd_type, void *data, uint32_t data_len)
{
    return 0;
}
void* log_type_alloc_mem(uint32_t device_id, uint32_t type, uint32_t *size)
{
    return NULL;
}
DLLEXPORT int prof_drv_get_channels(unsigned int device_id, channel_list_t *channels)
{
    return 0;
}
DLLEXPORT int prof_drv_start(unsigned int device_id, unsigned int channel_id, struct prof_start_para *start_para)
{
    return 0;
}
DLLEXPORT int prof_stop(unsigned int device_id, unsigned int channel_id)
{
    return 0;
}
DLLEXPORT int prof_channel_read(unsigned int device_id, unsigned int channel_id, char *out_buf, unsigned int buf_size)
{
    return 0;
}
DLLEXPORT int prof_channel_poll(struct prof_poll_info *out_buf, int num, int timeout)
{
    return 0;
}
DLLEXPORT int halProfDataFlush(unsigned int device_id, unsigned int channel_id, unsigned int *data_len)
{
    return 0;
}
DLLEXPORT int halBuffAlloc(uint64_t size, void **buff)
{
    return 0;
}
DLLEXPORT int halBuffAllocByPool(poolHandle pHandle, void **buff)
{
    return 0;
}
DLLEXPORT int halBuffAllocEx(uint64_t size, unsigned long flag, int grp_id, void **buff)
{
    return 0;
}
DLLEXPORT int halBuffAllocAlignEx(uint64_t size, unsigned int align, unsigned long flag, int grp_id, void **buff)
{
    return 0;
}
DLLEXPORT drvError_t halBuffGet(Mbuf *mbuf, void *buf, unsigned long size)
{
    return 0;
}
DLLEXPORT void halBuffPut(Mbuf *mbuf, void *buf)
{

}
DLLEXPORT int halBuffPoolGet(void* poolStart)
{
    return 0;
}
DLLEXPORT int halBuffPoolPut(void* poolStart)
{
    return 0;
}
DLLEXPORT int halMbufAllocEx(uint64_t size, unsigned int align, unsigned long flag, int grp_id, Mbuf **mbuf)
{
    return 0;
}
DLLEXPORT int halMbufVerify(Mbuf *mbuf, unsigned int type)
{
    return 0;
}
DLLEXPORT int halMbufBuild(void *buff, uint64_t len, Mbuf **mbuf)
{
    return 0;
}
DLLEXPORT int halMbufUnBuild(Mbuf *mbuf, void **buff, uint64_t *len)
{
    return 0;
}
DLLEXPORT drvError_t halBufEventSubscribe(
    const char *grpName, unsigned int threadGrpId, unsigned int event_id, unsigned int devid)
{
    return 0;
}
DLLEXPORT drvError_t halBufEventReport(const char *grpName)
{
    return 0;
}
DLLEXPORT drvError_t halGrpCacheAlloc(const char *name, unsigned int devId, GrpCacheAllocPara *para)
{
    return 0;
}
DLLEXPORT drvError_t halGrpCacheFree(const char *name, unsigned int devId)
{
    return 0;
}
DLLEXPORT int halGrpQuery(GroupQueryCmdType cmd,
    void *inBuff, unsigned int inLen, void *outBuff, unsigned int *outLen)
{
    return 0;
}
DLLEXPORT drvError_t halBuffProcCacheFree(unsigned long flag)
{
    return 0;
}
DLLEXPORT drvError_t halBuffGetDQSPoolInfo(struct mempool_t *mp, DqsPoolInfo *poolInfo)
{
    return 0;
}
DLLEXPORT drvError_t halBuffGetDQSPoolInfoById(unsigned int poolId, DqsPoolInfo *poolInfo)
{
    return 0;
}
DLLEXPORT drvError_t halMbufGetDQSPoolInfo(Mbuf *mbuf, DqsPoolInfo *poolInfo)
{
    return 0;
}
DLLEXPORT drvError_t halBuffCreateInterGrp(unsigned int grpId, const char *name, unsigned int len)
{
    return 0;
}
DLLEXPORT drvError_t halBuffDestoryInterGrp(unsigned int grpId)
{
    return 0;
}
DLLEXPORT drvError_t halMbufGetDqsHandle(Mbuf *mbuf,  uint64_t *handle)
{
    return 0;
}
DLLEXPORT drvError_t halResourceIdAlloc(
    uint32_t devId, struct halResourceIdInputInfo *in, struct halResourceIdOutputInfo *out)
{
    return 0;
}
DLLEXPORT drvError_t halResourceIdFree(uint32_t devId, struct halResourceIdInputInfo *in)
{
    return 0;
}
DLLEXPORT drvError_t halResourceEnable(uint32_t devId, struct halResourceIdInputInfo *in)
{
    return 0;
}
DLLEXPORT drvError_t halResourceDisable(uint32_t devId, struct halResourceIdInputInfo *in)
{
    return 0;
}
DLLEXPORT drvError_t halResourceConfig(
    uint32_t devId, struct halResourceIdInputInfo *in, struct halResourceConfigInfo *para)
{
    return 0;
}
DLLEXPORT drvError_t halResourceDetailQuery(
    uint32_t devId, struct halResourceIdInputInfo *in, struct halResourceDetailInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halResourceInfoQuery(
    uint32_t devId, uint32_t tsId, drvResourceType_t type, struct halResourceInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halResourceIdCheck(struct drvResIdKey *info)
{
    return 0;
}
DLLEXPORT drvError_t halResourceIdInfoGet(struct drvResIdKey *key, drvResIdProcType type, uint64_t *value)
{
    return 0;
}
DLLEXPORT drvError_t halResourceIdRestore(struct drvResIdKey *info)
{
    return 0;
}
DLLEXPORT drvError_t halTsdrvCtl(uint32_t devId, int cmd, void *param, size_t paramSize, void *out, size_t *outSize)
{
    return 0;
}
DLLEXPORT drvError_t halSqCqAllocate(uint32_t devId, struct halSqCqInputInfo *in, struct halSqCqOutputInfo *out)
{
    return 0;
}
DLLEXPORT drvError_t halSqCqFree(uint32_t devId, struct halSqCqFreeInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halSqCqQuery(uint32_t devId, struct halSqCqQueryInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halSqCqConfig(uint32_t devId, struct halSqCqConfigInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halSqMemGet(uint32_t devId, struct halSqMemGetInput *in, struct halSqMemGetOutput *out)
{
    return 0;
}
DLLEXPORT drvError_t halSqMsgSend(uint32_t devId, struct halSqMsgInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halCqReportIrqWait(uint32_t devId, struct halReportInfoInput *in, struct halReportInfoOutput *out)
{
    return 0;
}
DLLEXPORT drvError_t halCqReportGet(uint32_t devId, struct halReportGetInput *in, struct halReportGetOutput *out)
{
    return 0;
}
DLLEXPORT drvError_t halReportRelease(uint32_t devId, struct halReportReleaseInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halSqTaskSend(uint32_t devId, struct halTaskSendInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halCqReportRecv(uint32_t devId, struct halReportRecvInfo *info)
{
    return 0;
}
drvError_t halStreamTaskFill(uint32_t dev_id, uint32_t stream_id, void *stream_mem, void *task_info, uint32_t task_cnt)
{
    return 0;
}
drvError_t halSqSwitchStreamBatch(uint32_t dev_id, struct sq_switch_stream_info *info, uint32_t num)
{
    return 0;
}
DLLEXPORT drvError_t halSqTaskArgsAsyncCopy(uint32_t devId, struct halSqTaskArgsInfo *info)
{
    return 0;
}
DLLEXPORT drvError_t halAsyncDmaWqeCreate(
    uint32_t devId, struct halAsyncDmaInputPara *in, struct halAsyncDmaOutputPara *out)
{
    return 0;
}
DLLEXPORT drvError_t halAsyncDmaWqeDestory(uint32_t devId, struct halAsyncDmaDestoryPara *para)
{
    return 0;
}
DLLEXPORT drvError_t halAsyncDmaCreate(
    uint32_t devId, struct halAsyncDmaInputPara *in, struct halAsyncDmaOutputPara *out)
{
    return 0;
}
DLLEXPORT drvError_t halAsyncDmaDestory(uint32_t devId, struct halAsyncDmaDestoryPara *para)
{
    return 0;
}
drvError_t halAsyncDmaCreate2D(uint32_t devId, struct halAsyncDmaInput2DPara *in, struct halAsyncDmaOutputPara *out)
{
    return 0;
}
drvError_t halAsyncDmaDestroy2D(uint32_t devId, struct halAsyncDmaDestroy2DPara *para)
{
    return 0;
}
drvError_t halAsyncDmaCreateBatch(uint32_t devId, struct halAsyncDmaInputBatchPara *in,
    struct halAsyncDmaOutputPara *out)
{
    return 0;
}
drvError_t halAsyncDmaDestroyBatch(uint32_t devId, struct halAsyncDmaDestroyBatchPara *para)
{
    return 0;
}
drvError_t halAsyncDmaJettyCreate(uint32_t devId, struct halAsyncDmaJettyCreateIn *in,
    struct halAsyncDmaJettyCreateOut *out)
{
    return 0;
}
drvError_t halAsyncDmaJettyDestroy(uint32_t devId, struct halAsyncJettyDestroyPara *para)
{
    return 0;
}
drvError_t halAsyncDmaJettyQuery(uint32_t devId, struct halAsyncDmaJettyQueryIn *in,
    struct halAsyncDmaJettyQueryOut *out)
{
    return 0;
}
drvError_t halAsyncDmaWqeConvert(uint32_t devId, struct halAsyncDmaWqeInputPara *in,
    struct halAsyncDmaWqeOutputPara *out)
{
    return 0;
}
drvError_t halAsyncDmaJettyWqeFill(uint32_t devId, struct halAsyncDmaJettyFillInfo *para)
{
    return 0;
}
DLLEXPORT drvError_t halCtl(int cmd, void *param_value, size_t param_value_size, void *out_value, size_t *out_size_ret)
{
    return 0;
}
DLLEXPORT drvError_t halGetTsegInfoByVa(uint32_t devid, uint64_t va, uint64_t size, uint32_t flag,
    struct halTsegInfo *tsegInfo)
{
    return 0;
}
DLLEXPORT drvError_t halPutTsegInfo(uint32_t devid, struct halTsegInfo *tsegInfo)
{
    return 0;
}
DLLEXPORT drvError_t halEschedDettachDevice(unsigned int devId)
{
    return 0;
}
DLLEXPORT drvError_t halEschedCreateGrpEx(uint32_t devId, struct esched_grp_para *grpPara, unsigned int *grpId)
{
    return 0;
}
DLLEXPORT drvError_t halEschedQueryInfo(unsigned int devId, ESCHED_QUERY_TYPE type,
    struct esched_input_info *inPut, struct esched_output_info *outPut)
{
    return 0;
}
DLLEXPORT drvError_t halEschedSetGrpEventQos(unsigned int devId, unsigned int grpId,
    EVENT_ID eventId, struct event_sched_grp_qos *qos)
{
    return 0;
}
DLLEXPORT drvError_t halEschedSetEventPriority(unsigned int devId, EVENT_ID eventId, SCHEDULE_PRIORITY priority)
{
    return 0;
}
DLLEXPORT drvError_t halEschedRegisterFinishFunc(unsigned int grpId, unsigned int event_id,
    void (*finishFunc)(unsigned int devId, unsigned int grpId, unsigned int event_id, unsigned int subevent_id))
{
    return 0;
}
DLLEXPORT drvError_t halEschedDumpEventTrace(unsigned int devId, char *buff,
    unsigned int buffLen, unsigned int *dataLen)
{
    return 0;
}
DLLEXPORT drvError_t halEschedTraceRecord(unsigned int devId, const char *recordReason, const char *key)
{
    return 0;
}
DLLEXPORT drvError_t halEschedSubmitEvent(unsigned int devId, struct event_summary *event)
{
    return 0;
}
DLLEXPORT drvError_t halEschedSubmitEventToThread(uint32_t devId, struct event_summary *event)
{
    return 0;
}
DLLEXPORT drvError_t halEschedSubmitEventBatch(unsigned int devId, SUBMIT_FLAG flag,
    struct event_summary *events, unsigned int event_num, unsigned int *succ_event_num)
{
    return 0;
}
DLLEXPORT drvError_t halEschedSubmitEventSync(unsigned int devId,
    struct event_summary *event, int timeout, struct event_reply *reply)
{
    return 0;
}
DLLEXPORT drvError_t halEschedThreadSwapout(unsigned int devId, unsigned int grpId, unsigned int threadId)
{
    return 0;
}
DLLEXPORT drvError_t halEschedThreadGiveup(unsigned int devId, unsigned int grpId, unsigned int threadId)
{
    return 0;
}
DLLEXPORT drvError_t halEschedGetEvent(unsigned int devId, unsigned int grpId, unsigned int threadId,
    EVENT_ID eventId, struct event_info *event)
{
    return 0;
}
DLLEXPORT drvError_t halEschedAckEvent(unsigned int devId, EVENT_ID eventId, unsigned int subeventId,
    char *msg, unsigned int msgLen)
{
    return 0;
}
DLLEXPORT drvError_t halEschedRegisterAckFunc(unsigned int grpId, EVENT_ID eventId,
    void (*ackFunc)(unsigned int devId, unsigned int subevent_id, char *msg, unsigned int msgLen))
{
    return 0;
}
DLLEXPORT drvError_t halEschedTableAddEntry(unsigned int devId, unsigned int tableId,
    struct esched_table_key *key, struct esched_table_entry *entry)
{
    return 0;
}
DLLEXPORT drvError_t halEschedTableDelEntry(unsigned int devId, unsigned int tableId, struct esched_table_key *key)
{
    return 0;
}
DLLEXPORT drvError_t halEschedTableQueryEntryStat(unsigned int devId, unsigned int tableId,
    struct esched_table_key *key, struct esched_table_key_entry_stat *stat)
{
    return 0;
}
drvError_t halQueueSubEvent(struct QueueSubPara *subPara)
{
    return 0;
}
drvError_t halQueueUnsubEvent(struct QueueUnsubPara *unsubPara)
{
    return 0;
}
DLLEXPORT drvError_t halQueueCreate(unsigned int devId, const QueueAttr *queAttr, unsigned int *qid)
{
    return 0;
}
DLLEXPORT drvError_t halQueueGrant(unsigned int devId, int qid, int pid, QueueShareAttr attr)
{
    return 0;
}
DLLEXPORT drvError_t halQueueAttach(unsigned int devId, unsigned int qid, int timeOut)
{
    return 0;
}
DLLEXPORT drvError_t halQueueEnQueue(unsigned int devId, unsigned int qid, void *mbuf)
{
    return 0;
}
DLLEXPORT drvError_t halQueueDeQueue(unsigned int devId, unsigned int qid, void **mbuf)
{
    return 0;
}
DLLEXPORT drvError_t halQueueSubscribe(
    unsigned int devId, unsigned int qid, unsigned int groupId, int type)
{
    return 0;
}
DLLEXPORT drvError_t halQueueUnsubscribe(unsigned int devId, unsigned int qid)
{
    return 0;
}
DLLEXPORT drvError_t halQueueSubF2NFEvent(unsigned int devId, unsigned int qid, unsigned int groupId)
{
    return 0;
}
DLLEXPORT drvError_t halQueueUnsubF2NFEvent(unsigned int devId, unsigned int qid)
{
    return 0;
}
DLLEXPORT drvError_t halQueueGetQidbyName(unsigned int devId, const char *name, unsigned int *qid)
{
    return 0;
}
DLLEXPORT drvError_t halQueueCtrlEvent(struct QueueSubscriber *subscriber, QUE_EVENT_CMD cmdType)
{
    return 0;
}
DLLEXPORT drvError_t halQueuePeek(
    unsigned int devId, unsigned int qid, uint64_t *buf_len, int timeout)
{
    return 0;
}
DLLEXPORT drvError_t halQueuePeekData(unsigned int devId, unsigned int qid, unsigned int flag,
    QueuePeekDataType type, void **mbuf)
{
    return 0;
}
DLLEXPORT drvError_t halQueueEnQueueBuff(unsigned int devId, unsigned int qid,
    struct buff_iovec *vector, int timeout)
{
    return 0;
}
DLLEXPORT drvError_t halQueueDeQueueBuff(unsigned int devId, unsigned int qid,
    struct buff_iovec *vector, int timeout)
{
    return 0;
}
DLLEXPORT drvError_t halEventProc(unsigned int devId, struct event_info *event)
{
    return 0;
}
DLLEXPORT drvError_t halDrvEventThreadInit(unsigned int devId)
{
    return 0;
}
DLLEXPORT drvError_t halDrvEventThreadUninit(unsigned int devId)
{
    return 0;
}
DLLEXPORT drvError_t halQueueQuery(unsigned int devId, QueueQueryCmdType cmd,
    QueueQueryInputPara *inPut, QueueQueryOutputPara *outPut)
{
    return 0;
}
DLLEXPORT drvError_t halQueueSet(unsigned int devId, QueueSetCmdType cmd, QueueSetInputPara *input)
{
    return 0;
}
DLLEXPORT drvError_t halBufferModeNotify(PSM_STATUS status, void *rsv)
{
    return 0;
}
DLLEXPORT drvError_t halQueueModeNotify(PSM_STATUS status, void *rsv)
{
    return 0;
}
DLLEXPORT drvError_t halQueueExport(unsigned int devId, unsigned int qid, struct shareQueInfo *queInfo)
{
    return 0;
}
DLLEXPORT drvError_t halQueueUnexport(unsigned int devId, unsigned int qid, struct shareQueInfo *queInfo)
{
    return 0;
}
DLLEXPORT drvError_t halQueueImport(unsigned int devId, struct shareQueInfo *queInfo, unsigned int* qid)
{
    return 0;
}
DLLEXPORT drvError_t halQueueUnimport(unsigned int devId, unsigned int qid, struct shareQueInfo *queInfo)
{
    return 0;
}
DLLEXPORT drvError_t halQueueGetDqsQueInfo(unsigned int devId, unsigned int qid, DqsQueueInfo *info)
{
    return 0;
}
drvError_t halNotifyGetInfo(uint32_t devId, uint32_t tsId, uint32_t type, uint32_t *val)
{
    return 0;
}
DLLEXPORT drvError_t halGetChipCount(int *chip_count)
{
    return 0;
}
DLLEXPORT drvError_t  halGetChipList(int chip_list[], int count)
{
    return 0;
}
DLLEXPORT drvError_t  halGetDeviceCountFromChip(int chip_id, int *device_count)
{
    return 0;
}
DLLEXPORT drvError_t  halGetDeviceFromChip(int chip_id, int device_list[], int count)
{
    return 0;
}
DLLEXPORT drvError_t  halGetChipFromDevice(int device_id, int *chip_id)
{
    return 0;
}
DLLEXPORT drvError_t halGetChipInfo(unsigned int devId, halChipInfo *chipInfo)
{
    return 0;
}
DLLEXPORT int32_t halMapErrorCode(drvError_t code)
{
    return 0;
}
DLLEXPORT drvError_t halGetVdevNum(uint32_t *num_dev)
{
    return 0;
}
DLLEXPORT drvError_t halGetDevNumEx(uint32_t hw_type, uint32_t *devNum)
{
    return 0;
}
DLLEXPORT drvError_t halGetVdevIDs(uint32_t *devices, uint32_t len)
{
    return 0;
}
DLLEXPORT drvError_t halGetDevIDsEx(uint32_t hw_type, uint32_t *devices, uint32_t len)
{
    return 0;
}
DLLEXPORT drvError_t halGetFaultEvent(uint32_t devId, struct halEventFilter* filter,
    struct halFaultEventInfo* eventInfo, uint32_t len, uint32_t *eventCount)
{
    return 0;
}
DLLEXPORT drvError_t halGetNotifyEvent(uint32_t devId, struct halEventFilter* filter,
    struct halFaultEventInfo* eventInfo, uint32_t len, uint32_t *eventCount)
{
    return 0;
}
DLLEXPORT drvError_t halReadFaultEvent(int32_t devId, int timeout,
    struct halEventFilter* filter, struct halFaultEventInfo* eventInfo)
{
    return 0;
}
DLLEXPORT drvError_t halSubscribeFaultEvent(int device_id, struct halEventFilter filter, halfaulteventcb handler)
{
    return 0;
}
DLLEXPORT drvError_t halParseSDID(uint32_t sdid, struct halSDIDParseInfo *sdid_parse)
{
    return 0;
}
DLLEXPORT bool halSupportFeature(uint32_t devId, drvFeature_t type)
{
    return 0;
}
DLLEXPORT drvError_t halGetHostID(uint32_t *host_id)
{
    return 0;
}
DLLEXPORT int halGetUserConfig(unsigned int devId, const char *name, unsigned char *buf, unsigned int *bufSize)
{
    return 0;
}
DLLEXPORT int halSetUserConfig(unsigned int devId, const char *name, unsigned char *buf, unsigned int bufSize)
{
    return 0;
}
DLLEXPORT int halClearUserConfig(unsigned int devId, const char *name)
{
    return 0;
}
DLLEXPORT int halGetDeviceVfMax(unsigned int devId, unsigned int *vf_max_num)
{
    return 0;
}
DLLEXPORT int halGetDeviceVfList(unsigned int devId, unsigned int *vf_list,
    unsigned int list_len, unsigned int *vf_num)
{
    return 0;
}
DLLEXPORT int halTsDevRecord(unsigned int devId, unsigned int tsId, unsigned int record_type, unsigned int record_Id)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halMemInitSvmDevice(int hostpid, unsigned int vfid, unsigned int dev_id)
{
    return 0;
}
DLLEXPORT DV_ONLINE DVresult halMemBindSibling(
    int hostPid, int aicpuPid, unsigned int vfid, unsigned int dev_id, unsigned int flag)
{
    return 0;
}
DLLEXPORT DV_ONLINE drvError_t halResMap(
    unsigned int devId, struct res_map_info *res_info, unsigned long *va, unsigned int *len)
{
    return 0;
}
DLLEXPORT DV_ONLINE drvError_t halResUnmap(unsigned int devId, struct res_map_info *res_info)
{
    return 0;
}
DLLEXPORT DV_ONLINE unsigned int halGetMaxResMapType(void)
{
    return 0;
}
DLLEXPORT DV_ONLINE drvError_t halResRead(
    unsigned int dev_id, struct res_map_info *res_info, void *data, unsigned int len)
{
    return 0;
}
DLLEXPORT DV_ONLINE drvError_t halResWrite(
    unsigned int dev_id, struct res_map_info *res_info, void *data, unsigned int len)
{
    return 0;
}
DLLEXPORT drvError_t halStreamBackup(uint32_t dev_id, struct stream_backup_info *in)
{
    return 0;
}
DLLEXPORT drvError_t halStreamRestore(uint32_t dev_id, struct stream_backup_info *in)
{
    return 0;
}
DLLEXPORT drvError_t halGetDeviceSplitMode(unsigned int dev_id, unsigned int *mode)
{
    return 0;
}
DLLEXPORT drvError_t halMemPoolCreate(soma_mem_pool_t pool, soma_mem_pool_prop prop)
{
    return 0;
}
DLLEXPORT drvError_t halMemPoolDestroy(soma_mem_pool_t pool)
{
    return 0;
}
DLLEXPORT DV_ONLINE drvError_t halMemPoolMalloc(soma_mem_pool_t pool, uint64_t va, uint64_t size, int32_t policy)
{
    return 0;
}
DLLEXPORT DV_ONLINE drvError_t halMemPoolFree(soma_mem_pool_t pool, uint64_t va, int32_t policy)
{
    return 0;
}
DLLEXPORT DV_ONLINE drvError_t halMemPoolTrim(soma_mem_pool_t pool, uint64_t *size, uint64_t poolUsedSize, uint64_t poolFreeSize)
{
    return 0;
}
