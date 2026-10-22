#ifndef __FEAR_RENDERER_STUB_H__
#define __FEAR_RENDERER_STUB_H__

#include "ltbasedefs.h"
#include "iltrenderer.h"

class CFearRenderer : public ILTRenderer
{
public:
    declare_interface(CFearRenderer)
    CFearRenderer() = default;
    ~CFearRenderer() {}

    RMode* GetRenderModes() override;
    void RelinquishRenderModes(RMode *pModes) override;
    LTRESULT GetCurrentRenderTargetDims(uint32& nWidth, uint32& nHeight) override;
    LTRESULT ClearRenderTarget(uint32 nClearFlags, uint32 ClearColor) override;
    bool IsIn3D() override;
    LTRESULT Start3D() override;
    LTRESULT End3D() override;
    LTRESULT FlipScreen() override;
    LTRESULT SetFrameStats(CRendererFrameStats* pFrameStats) override;
    LTRESULT GetFrameStats(CRendererFrameStats*& pFrameStats) override;
    LTRESULT SetDepthBiasTableIndex(uint32 nIndex, float fSlopeScale, float fDepthBias) override;
    LTRESULT SetObjectDepthBiasTableIndex(HOBJECT hObject, uint32 nTableIndex) override;
    LTRESULT WorldPosToScreenPos(HOBJECT hCamera, const LTVector& vWorldPos, LTVector& vScreenPos) override;
    LTRESULT ScreenPosToWorldPos(HOBJECT hCamera, const LTVector2& vScreenPos, float fDepth, LTVector& vWorldPos) override;
    LTRESULT RenderCamera(HLOCALOBJ hCamera, const char *pTechniqueOverride) override;
    LTRESULT RenderCamera(const LTRigidTransform& tCamera, const LTVector& vInitialSectorPos, const LTVector2& vFOV, bool bOrthogonal, const LTRect2f& rViewport, const LTPlane* pClipPlane, const char *pTechniqueOverride) override;
    LTRESULT RenderObjects(HLOCALOBJ hCamera, HLOCALOBJ *pObjects, int nObjects, const char *pTechniqueOverride) override;
    LTRESULT MakeCubicEnvMap(const LTVector& vCenter, uint32 nSize, const char* pszFile) override;
    LTRESULT MakeScreenShot(const char* pszFile) override;
    LTRESULT CreateRenderTarget(uint32 nWidth, uint32 nHeight, uint32 nOptions, HRENDERTARGET& hTarget) override;
    LTRESULT AddRenderTargetRef(HRENDERTARGET hTarget) override;
    LTRESULT ReleaseRenderTarget(HRENDERTARGET hTarget) override;
    LTRESULT SetRenderTarget(HRENDERTARGET hTarget, ECubeTextureFaces eSurface) override;
    HRENDERTARGET GetCurrentRenderTarget() override;
    LTRESULT SetRenderTargetScreen() override;
    LTRESULT StretchRect(HRENDERTARGET hSourceTarget, const LTRect2f& rSourceRect, HRENDERTARGET hDestTarget, const LTRect2f& rDestRect) override;
    HRENDERTARGETVIS CreateRenderTargetVisQuery(HRENDERTARGET hTarget) override;
    LTRESULT AddRenderTargetVisQueryRef(HRENDERTARGETVIS hTargetVis) override;
    LTRESULT ReleaseRenderTargetVisQuery(HRENDERTARGETVIS hTargetVis) override;
    LTRESULT WasRenderTargetVisVisible(HRENDERTARGETVIS hTargetVis, bool& bRendered) override;
    LTRESULT ClearRenderTargetVisVisible(HRENDERTARGETVIS hTargetVis) override;
    LTRESULT SetRenderTargetVisTarget(HRENDERTARGETVIS hTargetVis, HRENDERTARGET hRenderTarget) override;
    HMATERIAL CreateMaterialInstance(const char* pszMaterialName) override;
    LTRESULT ReleaseMaterialInstance(HMATERIAL hMaterial) override;
    LTRESULT AddRefMaterialInstance(HMATERIAL hMaterial) override;
    HMATERIAL CloneMaterialInstance(HMATERIAL hMaterial) override;
    LTRESULT GetMaterialFileName(HMATERIAL hMaterial, char* pszBuffer, uint32 nBufferLen) override;
    LTRESULT OptimizeVideoMemory() override;
    LTRESULT SetInstanceParamFloat(HMATERIAL hMaterial, const char* pszParam, float fVal) override;
    LTRESULT SetInstanceParamBool(HMATERIAL hMaterial, const char* pszParam, bool bVal) override;
    LTRESULT SetInstanceParamVector(HMATERIAL hMaterial, const char* pszParam, const LTVector& vVal) override;
    LTRESULT SetInstanceParamVector4(HMATERIAL hMaterial, const char* pszParam, const LTVector4& vVal) override;
    LTRESULT SetInstanceParamMatrix(HMATERIAL hMaterial, const char* pszParam, const LTMatrix& mVal) override;
    LTRESULT SetInstanceParamTexture(HMATERIAL hMaterial, const char* pszParam, HTEXTURE Texture) override;
    LTRESULT SetInstanceParamTextureAnim(HMATERIAL hMaterial, const char* pszParam, const char* pszTextureAnim) override;
    LTRESULT SetInstanceParamRenderTarget(HMATERIAL hMaterial, const char* pszParam, HRENDERTARGET hRenderTarget) override;
    LTRESULT SetInstanceParamRenderTargetVis(HMATERIAL hMaterial, const char* pszParam, HRENDERTARGETVIS hRenderTargetVis) override;
    LTRESULT SetInstanceParamVideoTexture(HMATERIAL hMaterial, const char* pszParam, HVIDEOTEXTURE hVideoTexture) override;
    LTRESULT SetInstanceParamInt(HMATERIAL hMaterial, const char* pszParam, int32 nVal) override;
    LTRESULT GetInstanceParamFloat(HMATERIAL hMaterial, const char* pszParam, float& fVal) override;
    LTRESULT GetInstanceParamBool(HMATERIAL hMaterial, const char* pszParam, bool& bVal) override;
    LTRESULT GetInstanceParamVector(HMATERIAL hMaterial, const char* pszParam, LTVector& vVal) override;
    LTRESULT GetInstanceParamVector4(HMATERIAL hMaterial, const char* pszParam, LTVector4& vVal) override;
    LTRESULT GetInstanceParamMatrix(HMATERIAL hMaterial, const char* pszParam, LTMatrix& mVal) override;
    LTRESULT GetInstanceParamTexture(HMATERIAL hMaterial, const char* pszParam, HTEXTURE& Texture) override;
    LTRESULT GetInstanceParamTextureAnim(HMATERIAL hMaterial, const char* pszParam, char* pszBuffer, uint32 nBufferLen) override;
    LTRESULT GetInstanceParamRenderTarget(HMATERIAL hMaterial, const char* pszParam, HRENDERTARGET& hRenderTarget) override;
    LTRESULT GetInstanceParamRenderTargetVis(HMATERIAL hMaterial, const char* pszParam, HRENDERTARGETVIS& hRenderTargetVis) override;
    LTRESULT GetInstanceParamVideoTexture(HMATERIAL hMaterial, const char* pszParam, HVIDEOTEXTURE& hVideoTexture) override;
    LTRESULT GetInstanceParamInt(HMATERIAL hMaterial, const char* pszParam, int32& nVal) override;
    LTRESULT RestoreInstanceParamDefault(HMATERIAL hMaterial, const char* pszParam) override;
    LTRESULT CreateModelDecal(HOBJECT hObject, const LTMatrix3x4 &mProjection, HMATERIAL hMaterial, bool bColorOverride, uint32 nColor, HMODELDECAL &hDecal) override;
    LTRESULT DestroyModelDecal(HMODELDECAL hDecal) override;
    LTRESULT GetModelDecals(HOBJECT hObject, HMODELDECAL *pDecalArray, uint32 &nDecalArraySize) override;
    LTRESULT GetModelDecalObject(HMODELDECAL hDecal, HOBJECT &hObject) override;
    LTRESULT SetModelDecalObject(HMODELDECAL hDecal, HOBJECT hObject) override;
    LTRESULT GetModelDecalProjection(HMODELDECAL hDecal, LTMatrix3x4 &mProjection) override;
    LTRESULT SetModelDecalProjection(HMODELDECAL hDecal, const LTMatrix3x4 &mProjection) override;
    LTRESULT GetModelDecalMaterial(HMODELDECAL hDecal, HMATERIAL &hMaterial) override;
    LTRESULT SetModelDecalMaterial(HMODELDECAL hDecal, HMATERIAL hMaterial) override;
    LTRESULT GetModelDecalColorOverride(HMODELDECAL hDecal, bool &bColorOverride) override;
    LTRESULT SetModelDecalColorOverride(HMODELDECAL hDecal, bool bColorOverride) override;
    LTRESULT GetModelDecalColor(HMODELDECAL hDecal, float &fColorR, float &fColorG, float &fColorB, float &fColorA) override;
    LTRESULT SetModelDecalColor(HMODELDECAL hDecal, float fColorR, float fColorG, float fColorB, float fColorA) override;
    LTRESULT SetModelDecalDeleteFn(ModelDecalDeleteFn pCallback, void* pUser) override;
    LTRESULT GetModelDecalDeleteFn(ModelDecalDeleteFn &pCallback, void* &pUser) override;
};

extern CFearRenderer g_FearRenderer;

#endif
