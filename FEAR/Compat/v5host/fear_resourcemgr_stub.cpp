#include "fear_resourcemgr_stub.h"
#include <stdio.h>
#include <stdlib.h>

template <typename T> struct SvcDefault { static T Get() { return T(); } };
template <> struct SvcDefault<LTRESULT> { static LTRESULT Get() { return LT_UNSUPPORTED; } };
template <> struct SvcDefault<const char*> { static const char* Get() { return ""; } };

CFearResourceMgr g_FearResourceMgr;
CFearResourceAllocator g_FearResourceAllocator;

void CFearResourceAllocator::AddRef() {}
void CFearResourceAllocator::Release() {}
void* CFearResourceAllocator::Allocate(uint32 nNumBytes) { return malloc(nNumBytes); }
void CFearResourceAllocator::Free(void* pBuffer) { free(pBuffer); }
void* CFearResourceAllocator::AllocateContiguous(uint32 nNumBytes) { return malloc(nNumBytes); }
void CFearResourceAllocator::FreeContiguous(void* pBuffer) { free(pBuffer); }
uint32 CFearResourceAllocator::GetAllocatedMemSize() { return 0; }

LTRESULT CFearResourceMgr::InitResourceSystem(bool bBackgroundLoading)
{
    return LT_OK;
}

LTRESULT CFearResourceMgr::CreateRandomAccessAllocator(ILTResourceAllocator*& pAllocator)
{
    pAllocator = &g_FearResourceAllocator;
    return LT_OK;
}

LTRESULT CFearResourceMgr::CreateDualBlockAllocator(uint32 nPoolSize, bool bContiguous, uint32 nStartAddress, ILTResourceAllocator*& pHeadAllocator, ILTResourceAllocator*& pTailAllocator)
{
    printf("STUB ILTResourceMgr::CreateDualBlockAllocator\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearResourceMgr::GetNumStreamingRegions(uint32& nNumRegions)
{
    nNumRegions = 0;
    return LT_OK;
}

HSTREAMINGREGION CFearResourceMgr::GetStreamingRegion(uint32 nIndex)
{
    printf("STUB ILTResourceMgr::GetStreamingRegion\n");
    return SvcDefault<HSTREAMINGREGION>::Get();
}

LTRESULT CFearResourceMgr::GetPointStreamingRegions(const LTVector& vPos, HSTREAMINGREGION& hRegion, HSTREAMINGREGION& hLinked)
{
    hRegion = NULL;
    hLinked = NULL;
    return LT_OK;
}

LTRESULT CFearResourceMgr::GetStreamingRegionPoolSize(uint32& nPoolSize)
{
    nPoolSize = 0;
    return LT_OK;
}

LTRESULT CFearResourceMgr::GetGlobalPoolSize(uint32& nPoolSize)
{
    nPoolSize = 0;
    return LT_OK;
}

HREGIONASSETLIST CFearResourceMgr::GetGlobalAssetList()
{
    return NULL;
}

LTRESULT CFearResourceMgr::GetStreamingRegionResourceLimit(HSTREAMINGREGION hRegion, uint32& nLimit)
{
    nLimit = 0;
    return LT_OK;
}

HREGIONASSETLIST CFearResourceMgr::GetStreamingRegionAssetList(HSTREAMINGREGION hRegion)
{
    return NULL;
}

LTRESULT CFearResourceMgr::GetStreamingRegionName(HSTREAMINGREGION hRegion, char* pszBuffer, uint32 nBufferLen)
{
    if (!pszBuffer || !nBufferLen)
        return LT_INVALIDPARAMS;
    pszBuffer[0] = '\0';
    return LT_OK;
}

LTRESULT CFearResourceMgr::SetActiveRegion(HSTREAMINGREGION hRegion)
{
    printf("STUB ILTResourceMgr::SetActiveRegion\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearResourceMgr::PrefetchRegionAssetList(HREGIONASSETLIST hList)
{
    printf("STUB ILTResourceMgr::PrefetchRegionAssetList\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearResourceMgr::DiscardRegionAssetList(HREGIONASSETLIST hList)
{
    printf("STUB ILTResourceMgr::DiscardRegionAssetList\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearResourceMgr::GetRegionAssetListStats(HREGIONASSETLIST hList, uint32& nNumLoaded, uint32& nTotalAssets, uint32& nMemUsage)
{
    nNumLoaded = 0;
    nTotalAssets = 0;
    nMemUsage = 0;
    return LT_OK;
}

LTRESULT CFearResourceMgr::SetPrefetchAllocator(ILTResourceAllocator* pAllocator)
{
    return LT_OK;
}

LTRESULT CFearResourceMgr::EnablePrefetchOnRequest(bool bEnable)
{
    printf("STUB ILTResourceMgr::EnablePrefetchOnRequest\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearResourceMgr::EnablePrefetchOnLock(bool bEnable)
{
    printf("STUB ILTResourceMgr::EnablePrefetchOnLock\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearResourceMgr::FlushAutoLoadedResources()
{
    printf("STUB ILTResourceMgr::FlushAutoLoadedResources\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearResourceMgr::GetNumLoadingResources(uint32& nNumResources)
{
    nNumResources = 0;
    return LT_OK;
}

HUSERASSETLIST CFearResourceMgr::LoadUserAssetList(const char* pszRelFilename)
{
    printf("STUB ILTResourceMgr::LoadUserAssetList\n");
    return SvcDefault<HUSERASSETLIST>::Get();
}

LTRESULT CFearResourceMgr::PrefetchUserAssetList(HUSERASSETLIST hUserAssetList)
{
    printf("STUB ILTResourceMgr::PrefetchUserAssetList\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearResourceMgr::DiscardUserAssetList(HUSERASSETLIST hUserAssetList)
{
    printf("STUB ILTResourceMgr::DiscardUserAssetList\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearResourceMgr::ReleaseUserAssetList(HUSERASSETLIST hUserAssetList)
{
    printf("STUB ILTResourceMgr::ReleaseUserAssetList\n");
    return SvcDefault<LTRESULT>::Get();
}

HRESOURCEBUNDLE CFearResourceMgr::LoadResourceBundle(const char* pszFilename, ILTResourceAllocator* pAllocator)
{
    printf("STUB ILTResourceMgr::LoadResourceBundle\n");
    return SvcDefault<HRESOURCEBUNDLE>::Get();
}

LTRESULT CFearResourceMgr::IsResourceBundleLoading(HRESOURCEBUNDLE hBundle, bool& bLoading)
{
    printf("STUB ILTResourceMgr::IsResourceBundleLoading\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearResourceMgr::StopResourceBundleLoading(HRESOURCEBUNDLE hBundle)
{
    printf("STUB ILTResourceMgr::StopResourceBundleLoading\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearResourceMgr::UnloadResourceBundle(HRESOURCEBUNDLE hBundle)
{
    printf("STUB ILTResourceMgr::UnloadResourceBundle\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearResourceMgr::WaitForResourceBundleToLoad(HRESOURCEBUNDLE hBundle)
{
    printf("STUB ILTResourceMgr::WaitForResourceBundleToLoad\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearResourceMgr::RegisterResourceListener(ILTResourceListener* pListener)
{
    printf("STUB ILTResourceMgr::RegisterResourceListener\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearResourceMgr::UnregisterResourceListener(ILTResourceListener* pListener)
{
    printf("STUB ILTResourceMgr::UnregisterResourceListener\n");
    return SvcDefault<LTRESULT>::Get();
}
