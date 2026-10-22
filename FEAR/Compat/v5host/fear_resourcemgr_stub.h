#ifndef __FEAR_RESOURCEMGR_STUB_H__
#define __FEAR_RESOURCEMGR_STUB_H__

#include "ltbasedefs.h"
#include "iltresourcemgr.h"
#include "iltresourceallocator.h"

class CFearResourceMgr : public ILTResourceMgr
{
public:
    declare_interface(CFearResourceMgr)
    CFearResourceMgr() = default;
    ~CFearResourceMgr() {}

    LTRESULT InitResourceSystem(bool bBackgroundLoading) override;
    LTRESULT CreateRandomAccessAllocator(ILTResourceAllocator*& pAllocator) override;
    LTRESULT CreateDualBlockAllocator(uint32 nPoolSize, bool bContiguous, uint32 nStartAddress, ILTResourceAllocator*& pHeadAllocator, ILTResourceAllocator*& pTailAllocator) override;
    LTRESULT GetNumStreamingRegions(uint32& nNumRegions) override;
    HSTREAMINGREGION GetStreamingRegion(uint32 nIndex) override;
    LTRESULT GetPointStreamingRegions(const LTVector& vPos, HSTREAMINGREGION& hRegion, HSTREAMINGREGION& hLinked) override;
    LTRESULT GetStreamingRegionPoolSize(uint32& nPoolSize) override;
    LTRESULT GetGlobalPoolSize(uint32& nPoolSize) override;
    HREGIONASSETLIST GetGlobalAssetList() override;
    LTRESULT GetStreamingRegionResourceLimit(HSTREAMINGREGION hRegion, uint32& nLimit) override;
    HREGIONASSETLIST GetStreamingRegionAssetList(HSTREAMINGREGION hRegion) override;
    LTRESULT GetStreamingRegionName(HSTREAMINGREGION hRegion, char* pszBuffer, uint32 nBufferLen) override;
    LTRESULT SetActiveRegion(HSTREAMINGREGION hRegion) override;
    LTRESULT PrefetchRegionAssetList(HREGIONASSETLIST hList) override;
    LTRESULT DiscardRegionAssetList(HREGIONASSETLIST hList) override;
    LTRESULT GetRegionAssetListStats(HREGIONASSETLIST hList, uint32& nNumLoaded, uint32& nTotalAssets, uint32& nMemUsage) override;
    LTRESULT SetPrefetchAllocator(ILTResourceAllocator* pAllocator) override;
    LTRESULT EnablePrefetchOnRequest(bool bEnable) override;
    LTRESULT EnablePrefetchOnLock(bool bEnable) override;
    LTRESULT FlushAutoLoadedResources() override;
    LTRESULT GetNumLoadingResources(uint32& nNumResources) override;
    HUSERASSETLIST LoadUserAssetList(const char* pszRelFilename) override;
    LTRESULT PrefetchUserAssetList(HUSERASSETLIST hUserAssetList) override;
    LTRESULT DiscardUserAssetList(HUSERASSETLIST hUserAssetList) override;
    LTRESULT ReleaseUserAssetList(HUSERASSETLIST hUserAssetList) override;
    HRESOURCEBUNDLE LoadResourceBundle(const char* pszFilename, ILTResourceAllocator* pAllocator) override;
    LTRESULT IsResourceBundleLoading(HRESOURCEBUNDLE hBundle, bool& bLoading) override;
    LTRESULT StopResourceBundleLoading(HRESOURCEBUNDLE hBundle) override;
    LTRESULT UnloadResourceBundle(HRESOURCEBUNDLE hBundle) override;
    LTRESULT WaitForResourceBundleToLoad(HRESOURCEBUNDLE hBundle) override;
    LTRESULT RegisterResourceListener(ILTResourceListener* pListener) override;
    LTRESULT UnregisterResourceListener(ILTResourceListener* pListener) override;
};

extern CFearResourceMgr g_FearResourceMgr;

class CFearResourceAllocator : public ILTResourceAllocator
{
public:
    void AddRef() override;
    void Release() override;
    void* Allocate(uint32 nNumBytes) override;
    void Free(void* pBuffer) override;
    void* AllocateContiguous(uint32 nNumBytes) override;
    void FreeContiguous(void* pBuffer) override;
    uint32 GetAllocatedMemSize() override;
};

extern CFearResourceAllocator g_FearResourceAllocator;

#endif
