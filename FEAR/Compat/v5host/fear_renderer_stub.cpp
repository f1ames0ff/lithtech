#include <windows.h>
#include "fear_renderer_stub.h"
#include "fear_render_api.h"
#include "fear_host_api.h"
#include <stdio.h>
#include <string.h>
#include "rendererframestats.h"

template <typename T> struct SvcDefault { static T Get() { return T(); } };
template <> struct SvcDefault<LTRESULT> { static LTRESULT Get() { return LT_UNSUPPORTED; } };
template <> struct SvcDefault<const char*> { static const char* Get() { return ""; } };

#define FEAR_FWD_LOG(name) \
    do { static int s_nFearLogged = 0; if (!s_nFearLogged) { s_nFearLogged = 1; printf("FWD %s\n", (name)); } } while (0)
#define FEAR_UNIMPL_LOG(name) \
    do { static int s_nFearLogged = 0; if (!s_nFearLogged) { s_nFearLogged = 1; printf("UNIMPL %s\n", (name)); } } while (0)
#define FEAR_LOCAL_LOG(name) \
    do { static int s_nFearLogged = 0; if (!s_nFearLogged) { s_nFearLogged = 1; printf("LOCAL %s\n", (name)); } } while (0)

static const FearRenderApi *FearRender(void)
{
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    return pApi ? pApi->m_pRenderApi : NULL;
}

static int g_n3DDepth = 0;
static HRENDERTARGET g_hCurrentTarget = NULL;
static CRendererFrameStats g_FrameStats;

class CFearMaterialInstance
{
public:
    CFearMaterialInstance() : m_nRefCount(1) {}

    LONG AddRef()
    {
        return InterlockedIncrement(&m_nRefCount);
    }

    LONG Release()
    {
        LONG nRefCount = InterlockedDecrement(&m_nRefCount);
        if (nRefCount < 1)
        {
            InterlockedExchange(&m_nRefCount, 1);
            nRefCount = 1;
        }
        return nRefCount;
    }

    virtual void *FearMaterialInstanceAnchor() { return NULL; }

private:
    volatile LONG m_nRefCount;
};

static CFearMaterialInstance g_FearMaterialInstance;

CFearRenderer g_FearRenderer;

RMode* CFearRenderer::GetRenderModes()
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetRenderModes");
    return SvcDefault<RMode*>::Get();
}

void CFearRenderer::RelinquishRenderModes(RMode *pModes)
{
    FEAR_UNIMPL_LOG("ILTRenderer::RelinquishRenderModes");
}

LTRESULT CFearRenderer::GetCurrentRenderTargetDims(uint32& nWidth, uint32& nHeight)
{
    FEAR_FWD_LOG("ILTRenderer::GetCurrentRenderTargetDims");
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    int nW = 0;
    int nH = 0;
    if (!pRender->GetTextureDims(NULL, &nW, &nH))
        return LT_ERROR;

    nWidth = (uint32)nW;
    nHeight = (uint32)nH;
    return LT_OK;
}

LTRESULT CFearRenderer::ClearRenderTarget(uint32 nClearFlags, uint32 ClearColor)
{
    FEAR_FWD_LOG("ILTRenderer::ClearRenderTarget");
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    float fDepth = (nClearFlags & CLEARRTARGET_ZBUFFER) ? 1.0f : 0.0f;
    unsigned int nStencil = (nClearFlags & CLEARRTARGET_STENCIL) ? 1u : 0u;
    return pRender->Clear(ClearColor, fDepth, nStencil) ? LT_OK : LT_ERROR;
}

bool CFearRenderer::IsIn3D()
{
    FEAR_LOCAL_LOG("ILTRenderer::IsIn3D");
    return g_n3DDepth > 0;
}

LTRESULT CFearRenderer::Start3D()
{
    FEAR_FWD_LOG("ILTRenderer::Start3D -> Start2D");
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    if (!pRender->Start2D())
        return LT_ERROR;

    g_n3DDepth++;
    return LT_OK;
}

LTRESULT CFearRenderer::End3D()
{
    FEAR_FWD_LOG("ILTRenderer::End3D -> End2D");
    if (g_n3DDepth <= 0)
        return LT_OK;

    g_n3DDepth--;

    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    return pRender->End2D() ? LT_OK : LT_ERROR;
}

LTRESULT CFearRenderer::FlipScreen()
{
    FEAR_FWD_LOG("ILTRenderer::FlipScreen -> Present");
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    return pRender->Present() ? LT_OK : LT_ERROR;
}

LTRESULT CFearRenderer::SetFrameStats(CRendererFrameStats* pFrameStats)
{
    FEAR_FWD_LOG("ILTRenderer::SetFrameStats");
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    if (!pFrameStats)
        return LT_OK;

    int nCalls = pFrameStats->GetFrameStat(eFS_Render_RenderCalls);
    int nTris = pFrameStats->GetFrameStat(eFS_Triangles_Model)
        + pFrameStats->GetFrameStat(eFS_Triangles_ModelShadow)
        + pFrameStats->GetFrameStat(eFS_Triangles_World)
        + pFrameStats->GetFrameStat(eFS_Triangles_WorldShadow)
        + pFrameStats->GetFrameStat(eFS_Triangles_WorldModel)
        + pFrameStats->GetFrameStat(eFS_Triangles_WorldModelShadow)
        + pFrameStats->GetFrameStat(eFS_Triangles_CustomRender);

    pRender->SetFrameStats(nCalls, nTris);
    return LT_OK;
}

LTRESULT CFearRenderer::GetFrameStats(CRendererFrameStats*& pFrameStats)
{
    FEAR_FWD_LOG("ILTRenderer::GetFrameStats");
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    int nCalls = 0;
    int nTris = 0;
    if (!pRender->GetFrameStats(&nCalls, &nTris))
        return LT_ERROR;

    g_FrameStats.ClearFrameStats();
    g_FrameStats.SetFrameStat(eFS_Render_RenderCalls, nCalls);
    g_FrameStats.SetFrameStat(eFS_Triangles_Model, nTris);
    pFrameStats = &g_FrameStats;
    return LT_OK;
}

LTRESULT CFearRenderer::SetDepthBiasTableIndex(uint32 nIndex, float fSlopeScale, float fDepthBias)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetDepthBiasTableIndex");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetObjectDepthBiasTableIndex(HOBJECT hObject, uint32 nTableIndex)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetObjectDepthBiasTableIndex");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::WorldPosToScreenPos(HOBJECT hCamera, const LTVector& vWorldPos, LTVector& vScreenPos)
{
    FEAR_UNIMPL_LOG("ILTRenderer::WorldPosToScreenPos");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::ScreenPosToWorldPos(HOBJECT hCamera, const LTVector2& vScreenPos, float fDepth, LTVector& vWorldPos)
{
    FEAR_UNIMPL_LOG("ILTRenderer::ScreenPosToWorldPos");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::RenderCamera(HLOCALOBJ hCamera, const char *pTechniqueOverride)
{
    FEAR_FWD_LOG("ILTRenderer::RenderCamera");
    const FearRenderApi *pRender = FearRender();
    if (!pRender || !pRender->RenderWorld)
        return LT_UNSUPPORTED;
    return pRender->RenderWorld() ? LT_OK : LT_ERROR;
}

LTRESULT CFearRenderer::RenderCamera(const LTRigidTransform& tCamera, const LTVector& vInitialSectorPos, const LTVector2& vFOV, bool bOrthogonal, const LTRect2f& rViewport, const LTPlane* pClipPlane, const char *pTechniqueOverride)
{
    FEAR_UNIMPL_LOG("ILTRenderer::RenderCamera");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::RenderObjects(HLOCALOBJ hCamera, HLOCALOBJ *pObjects, int nObjects, const char *pTechniqueOverride)
{
    FEAR_UNIMPL_LOG("ILTRenderer::RenderObjects");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::MakeCubicEnvMap(const LTVector& vCenter, uint32 nSize, const char* pszFile)
{
    FEAR_UNIMPL_LOG("ILTRenderer::MakeCubicEnvMap");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::MakeScreenShot(const char* pszFile)
{
    FEAR_UNIMPL_LOG("ILTRenderer::MakeScreenShot");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::CreateRenderTarget(uint32 nWidth, uint32 nHeight, uint32 nOptions, HRENDERTARGET& hTarget)
{
    FEAR_FWD_LOG("ILTRenderer::CreateRenderTarget");
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    hTarget = (HRENDERTARGET)pRender->CreateRenderTarget((int)nWidth, (int)nHeight, (int)nOptions);
    return hTarget ? LT_OK : LT_ERROR;
}

LTRESULT CFearRenderer::AddRenderTargetRef(HRENDERTARGET hTarget)
{
    FEAR_UNIMPL_LOG("ILTRenderer::AddRenderTargetRef");
    return LT_OK;
}

LTRESULT CFearRenderer::ReleaseRenderTarget(HRENDERTARGET hTarget)
{
    FEAR_FWD_LOG("ILTRenderer::ReleaseRenderTarget");
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    pRender->ReleaseRenderTarget((void *)hTarget);
    if (g_hCurrentTarget == hTarget)
        g_hCurrentTarget = NULL;
    return LT_OK;
}

LTRESULT CFearRenderer::SetRenderTarget(HRENDERTARGET hTarget, ECubeTextureFaces eSurface)
{
    FEAR_FWD_LOG("ILTRenderer::SetRenderTarget");
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    if (!pRender->SetRenderTarget((void *)hTarget))
        return LT_ERROR;

    g_hCurrentTarget = hTarget;
    return LT_OK;
}

HRENDERTARGET CFearRenderer::GetCurrentRenderTarget()
{
    FEAR_LOCAL_LOG("ILTRenderer::GetCurrentRenderTarget");
    return g_hCurrentTarget;
}

LTRESULT CFearRenderer::SetRenderTargetScreen()
{
    FEAR_FWD_LOG("ILTRenderer::SetRenderTargetScreen");
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    g_hCurrentTarget = NULL;
    return pRender->SetRenderTarget(NULL) ? LT_OK : LT_ERROR;
}

LTRESULT CFearRenderer::StretchRect(HRENDERTARGET hSourceTarget, const LTRect2f& rSourceRect, HRENDERTARGET hDestTarget, const LTRect2f& rDestRect)
{
    FEAR_UNIMPL_LOG("ILTRenderer::StretchRect");
    return SvcDefault<LTRESULT>::Get();
}

HRENDERTARGETVIS CFearRenderer::CreateRenderTargetVisQuery(HRENDERTARGET hTarget)
{
    FEAR_UNIMPL_LOG("ILTRenderer::CreateRenderTargetVisQuery");
    return SvcDefault<HRENDERTARGETVIS>::Get();
}

LTRESULT CFearRenderer::AddRenderTargetVisQueryRef(HRENDERTARGETVIS hTargetVis)
{
    FEAR_UNIMPL_LOG("ILTRenderer::AddRenderTargetVisQueryRef");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::ReleaseRenderTargetVisQuery(HRENDERTARGETVIS hTargetVis)
{
    FEAR_UNIMPL_LOG("ILTRenderer::ReleaseRenderTargetVisQuery");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::WasRenderTargetVisVisible(HRENDERTARGETVIS hTargetVis, bool& bRendered)
{
    FEAR_UNIMPL_LOG("ILTRenderer::WasRenderTargetVisVisible");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::ClearRenderTargetVisVisible(HRENDERTARGETVIS hTargetVis)
{
    FEAR_UNIMPL_LOG("ILTRenderer::ClearRenderTargetVisVisible");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetRenderTargetVisTarget(HRENDERTARGETVIS hTargetVis, HRENDERTARGET hRenderTarget)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetRenderTargetVisTarget");
    return SvcDefault<LTRESULT>::Get();
}

HMATERIAL CFearRenderer::CreateMaterialInstance(const char* pszMaterialName)
{
    FEAR_UNIMPL_LOG("ILTRenderer::CreateMaterialInstance");
    g_FearMaterialInstance.AddRef();
    return reinterpret_cast<HMATERIAL>(&g_FearMaterialInstance);
}

LTRESULT CFearRenderer::ReleaseMaterialInstance(HMATERIAL hMaterial)
{
    FEAR_UNIMPL_LOG("ILTRenderer::ReleaseMaterialInstance");
    if (hMaterial == reinterpret_cast<HMATERIAL>(&g_FearMaterialInstance))
        g_FearMaterialInstance.Release();
    return LT_OK;
}

LTRESULT CFearRenderer::AddRefMaterialInstance(HMATERIAL hMaterial)
{
    FEAR_UNIMPL_LOG("ILTRenderer::AddRefMaterialInstance");
    if (hMaterial == reinterpret_cast<HMATERIAL>(&g_FearMaterialInstance))
        g_FearMaterialInstance.AddRef();
    return LT_OK;
}

HMATERIAL CFearRenderer::CloneMaterialInstance(HMATERIAL hMaterial)
{
    FEAR_UNIMPL_LOG("ILTRenderer::CloneMaterialInstance");
    g_FearMaterialInstance.AddRef();
    return reinterpret_cast<HMATERIAL>(&g_FearMaterialInstance);
}

LTRESULT CFearRenderer::GetMaterialFileName(HMATERIAL hMaterial, char* pszBuffer, uint32 nBufferLen)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetMaterialFileName");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::OptimizeVideoMemory()
{
    FEAR_UNIMPL_LOG("ILTRenderer::OptimizeVideoMemory");
    return LT_OK;
}

LTRESULT CFearRenderer::SetInstanceParamFloat(HMATERIAL hMaterial, const char* pszParam, float fVal)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetInstanceParamFloat");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetInstanceParamBool(HMATERIAL hMaterial, const char* pszParam, bool bVal)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetInstanceParamBool");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetInstanceParamVector(HMATERIAL hMaterial, const char* pszParam, const LTVector& vVal)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetInstanceParamVector");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetInstanceParamVector4(HMATERIAL hMaterial, const char* pszParam, const LTVector4& vVal)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetInstanceParamVector4");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetInstanceParamMatrix(HMATERIAL hMaterial, const char* pszParam, const LTMatrix& mVal)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetInstanceParamMatrix");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetInstanceParamTexture(HMATERIAL hMaterial, const char* pszParam, HTEXTURE Texture)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetInstanceParamTexture");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetInstanceParamTextureAnim(HMATERIAL hMaterial, const char* pszParam, const char* pszTextureAnim)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetInstanceParamTextureAnim");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetInstanceParamRenderTarget(HMATERIAL hMaterial, const char* pszParam, HRENDERTARGET hRenderTarget)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetInstanceParamRenderTarget");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetInstanceParamRenderTargetVis(HMATERIAL hMaterial, const char* pszParam, HRENDERTARGETVIS hRenderTargetVis)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetInstanceParamRenderTargetVis");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetInstanceParamVideoTexture(HMATERIAL hMaterial, const char* pszParam, HVIDEOTEXTURE hVideoTexture)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetInstanceParamVideoTexture");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetInstanceParamInt(HMATERIAL hMaterial, const char* pszParam, int32 nVal)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetInstanceParamInt");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetInstanceParamFloat(HMATERIAL hMaterial, const char* pszParam, float& fVal)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetInstanceParamFloat");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetInstanceParamBool(HMATERIAL hMaterial, const char* pszParam, bool& bVal)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetInstanceParamBool");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetInstanceParamVector(HMATERIAL hMaterial, const char* pszParam, LTVector& vVal)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetInstanceParamVector");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetInstanceParamVector4(HMATERIAL hMaterial, const char* pszParam, LTVector4& vVal)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetInstanceParamVector4");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetInstanceParamMatrix(HMATERIAL hMaterial, const char* pszParam, LTMatrix& mVal)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetInstanceParamMatrix");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetInstanceParamTexture(HMATERIAL hMaterial, const char* pszParam, HTEXTURE& Texture)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetInstanceParamTexture");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetInstanceParamTextureAnim(HMATERIAL hMaterial, const char* pszParam, char* pszBuffer, uint32 nBufferLen)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetInstanceParamTextureAnim");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetInstanceParamRenderTarget(HMATERIAL hMaterial, const char* pszParam, HRENDERTARGET& hRenderTarget)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetInstanceParamRenderTarget");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetInstanceParamRenderTargetVis(HMATERIAL hMaterial, const char* pszParam, HRENDERTARGETVIS& hRenderTargetVis)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetInstanceParamRenderTargetVis");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetInstanceParamVideoTexture(HMATERIAL hMaterial, const char* pszParam, HVIDEOTEXTURE& hVideoTexture)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetInstanceParamVideoTexture");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetInstanceParamInt(HMATERIAL hMaterial, const char* pszParam, int32& nVal)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetInstanceParamInt");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::RestoreInstanceParamDefault(HMATERIAL hMaterial, const char* pszParam)
{
    FEAR_UNIMPL_LOG("ILTRenderer::RestoreInstanceParamDefault");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::CreateModelDecal(HOBJECT hObject, const LTMatrix3x4 &mProjection, HMATERIAL hMaterial, bool bColorOverride, uint32 nColor, HMODELDECAL &hDecal)
{
    FEAR_UNIMPL_LOG("ILTRenderer::CreateModelDecal");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::DestroyModelDecal(HMODELDECAL hDecal)
{
    FEAR_UNIMPL_LOG("ILTRenderer::DestroyModelDecal");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetModelDecals(HOBJECT hObject, HMODELDECAL *pDecalArray, uint32 &nDecalArraySize)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetModelDecals");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetModelDecalObject(HMODELDECAL hDecal, HOBJECT &hObject)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetModelDecalObject");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetModelDecalObject(HMODELDECAL hDecal, HOBJECT hObject)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetModelDecalObject");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetModelDecalProjection(HMODELDECAL hDecal, LTMatrix3x4 &mProjection)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetModelDecalProjection");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetModelDecalProjection(HMODELDECAL hDecal, const LTMatrix3x4 &mProjection)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetModelDecalProjection");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetModelDecalMaterial(HMODELDECAL hDecal, HMATERIAL &hMaterial)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetModelDecalMaterial");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetModelDecalMaterial(HMODELDECAL hDecal, HMATERIAL hMaterial)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetModelDecalMaterial");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetModelDecalColorOverride(HMODELDECAL hDecal, bool &bColorOverride)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetModelDecalColorOverride");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetModelDecalColorOverride(HMODELDECAL hDecal, bool bColorOverride)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetModelDecalColorOverride");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::GetModelDecalColor(HMODELDECAL hDecal, float &fColorR, float &fColorG, float &fColorB, float &fColorA)
{
    FEAR_UNIMPL_LOG("ILTRenderer::GetModelDecalColor");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetModelDecalColor(HMODELDECAL hDecal, float fColorR, float fColorG, float fColorB, float fColorA)
{
    FEAR_UNIMPL_LOG("ILTRenderer::SetModelDecalColor");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearRenderer::SetModelDecalDeleteFn(ModelDecalDeleteFn pCallback, void* pUser)
{
    FEAR_FWD_LOG("ILTRenderer::SetModelDecalDeleteFn");
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    pRender->SetModelDecalDeleteFn((void *)pCallback, pUser);
    return LT_OK;
}

LTRESULT CFearRenderer::GetModelDecalDeleteFn(ModelDecalDeleteFn &pCallback, void* &pUser)
{
    FEAR_FWD_LOG("ILTRenderer::GetModelDecalDeleteFn");
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    void *pFn = NULL;
    void *pFnUser = NULL;
    pRender->GetModelDecalDeleteFn(&pFn, &pFnUser);
    pCallback = (ModelDecalDeleteFn)pFn;
    pUser = pFnUser;
    return LT_OK;
}
