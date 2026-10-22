#ifndef __FEAR_DRAWPRIM_STUB_H__
#define __FEAR_DRAWPRIM_STUB_H__

#include "ltbasedefs.h"
#include "iltdrawprim.h"

class CFearDrawPrim : public ILTDrawPrim
{
public:
    declare_interface(CFearDrawPrim)
    CFearDrawPrim() = default;
    ~CFearDrawPrim() {}

    LTRESULT BeginDrawPrimBlock() override;
    LTRESULT EndDrawPrimBlock() override;
    LTRESULT SetCamera(const HOBJECT hCamera) override;
    HOBJECT GetCamera() override;
    LTRESULT SetZMode(const ELTDrawPrimZMode eZMode) override;
    ELTDrawPrimZMode GetZMode() override;
    LTRESULT SetTexture(HTEXTURE hTexture) override;
    LTRESULT SetDynamicTexture(HDYNAMICTEXTURE hTexture) override;
    LTRESULT SetVideoTexture(HVIDEOTEXTURE hVideoTexture) override;
    LTRESULT SetRenderTarget(HRENDERTARGET hRenderTarget) override;
    LTRESULT SetRenderMode(const ELTDrawPrimRenderMode eRenderMode) override;
    ELTDrawPrimRenderMode GetRenderMode() override;
    LTRESULT SetTransformMode(const ELTDrawPrimTransformMode eTransformMode) override;
    ELTDrawPrimTransformMode GetTransformMode() override;
    LTRESULT DrawPrim(LT_POLYGT3 *pPrim, const uint32 nCount) override;
    LTRESULT DrawPrim(LT_POLYG3 *pPrim, const uint32 nCount) override;
    LTRESULT DrawPrim(LT_POLYGT4 *pPrim, const uint32 nCount) override;
    LTRESULT DrawPrim(LT_POLYG4 *pPrim, const uint32 nCount) override;
    LTRESULT DrawPrim(LT_LINEG *pPrim, const uint32 nCount) override;
    LTRESULT BeginDrawPrimMaterialBlock() override;
    LTRESULT EndDrawPrimMaterialBlock() override;
    LTRESULT DrawPrimMaterial(LT_POLYGTTS4 *pPrim, const uint32 nCount, HMATERIAL hMaterial) override;
    LTRESULT DrawPrimMaterial(LT_POLYGTTS3 *pPrim, const uint32 nCount, HMATERIAL hMaterial) override;
};

extern CFearDrawPrim g_FearDrawPrim;

#endif
