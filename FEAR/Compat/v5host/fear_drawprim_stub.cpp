#include "fear_drawprim_stub.h"
#include "fear_renderer_stub.h"
#include "fear_render_api.h"
#include "fear_host_api.h"
#include <stdio.h>
#include <string.h>

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

static void *g_pCurrentTexture = NULL;
static HOBJECT g_hCurrentCamera = NULL;
static ELTDrawPrimRenderMode g_eRenderMode = eLTDrawPrimRenderMode_Modulate_NoBlend;
static ELTDrawPrimTransformMode g_eTransformMode = eLTDrawPrimTransformMode_Screen;
static ELTDrawPrimZMode g_eZMode = eLTDrawPrimZMode_None;

static LTRESULT FearDrawTexturedQuad(const LT_VERTGT *pVerts)
{
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    float xy[8];
    float uv[8];
    for (int i = 0; i < 4; ++i)
    {
        xy[i * 2] = pVerts[i].pos.x;
        xy[i * 2 + 1] = pVerts[i].pos.y;
        uv[i * 2] = pVerts[i].uv.x;
        uv[i * 2 + 1] = pVerts[i].uv.y;
    }

    return pRender->DrawQuad2D(xy, uv, pVerts[0].rgba.m_nColor, g_pCurrentTexture) ? LT_OK : LT_ERROR;
}

static LTRESULT FearDrawColorQuad(const LT_VERTG *pVerts)
{
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    float xy[8];
    float uv[8];
    for (int i = 0; i < 4; ++i)
    {
        xy[i * 2] = pVerts[i].pos.x;
        xy[i * 2 + 1] = pVerts[i].pos.y;
        uv[i * 2] = 0.0f;
        uv[i * 2 + 1] = 0.0f;
    }

    return pRender->DrawQuad2D(xy, uv, pVerts[0].rgba.m_nColor, NULL) ? LT_OK : LT_ERROR;
}

CFearDrawPrim g_FearDrawPrim;

LTRESULT CFearDrawPrim::BeginDrawPrimBlock()
{
    FEAR_UNIMPL_LOG("ILTDrawPrim::BeginDrawPrimBlock");
    return LT_OK;
}

LTRESULT CFearDrawPrim::EndDrawPrimBlock()
{
    FEAR_UNIMPL_LOG("ILTDrawPrim::EndDrawPrimBlock");
    return LT_OK;
}

LTRESULT CFearDrawPrim::SetCamera(const HOBJECT hCamera)
{
    FEAR_LOCAL_LOG("ILTDrawPrim::SetCamera");
    g_hCurrentCamera = hCamera;
    return LT_OK;
}

HOBJECT CFearDrawPrim::GetCamera()
{
    FEAR_LOCAL_LOG("ILTDrawPrim::GetCamera");
    return g_hCurrentCamera;
}

LTRESULT CFearDrawPrim::SetZMode(const ELTDrawPrimZMode eZMode)
{
    FEAR_LOCAL_LOG("ILTDrawPrim::SetZMode");
    g_eZMode = eZMode;
    return LT_OK;
}

ELTDrawPrimZMode CFearDrawPrim::GetZMode()
{
    FEAR_LOCAL_LOG("ILTDrawPrim::GetZMode");
    return g_eZMode;
}

LTRESULT CFearDrawPrim::SetTexture(HTEXTURE hTexture)
{
    FEAR_FWD_LOG("ILTDrawPrim::SetTexture");
    g_pCurrentTexture = (void *)hTexture;
    return LT_OK;
}

LTRESULT CFearDrawPrim::SetDynamicTexture(HDYNAMICTEXTURE hTexture)
{
    FEAR_UNIMPL_LOG("ILTDrawPrim::SetDynamicTexture");
    return LT_UNSUPPORTED;
}

LTRESULT CFearDrawPrim::SetVideoTexture(HVIDEOTEXTURE hVideoTexture)
{
    FEAR_UNIMPL_LOG("ILTDrawPrim::SetVideoTexture");
    return LT_UNSUPPORTED;
}

LTRESULT CFearDrawPrim::SetRenderTarget(HRENDERTARGET hRenderTarget)
{
    FEAR_UNIMPL_LOG("ILTDrawPrim::SetRenderTarget");
    return LT_UNSUPPORTED;
}

LTRESULT CFearDrawPrim::SetRenderMode(const ELTDrawPrimRenderMode eRenderMode)
{
    FEAR_LOCAL_LOG("ILTDrawPrim::SetRenderMode");
    g_eRenderMode = eRenderMode;
    return LT_OK;
}

ELTDrawPrimRenderMode CFearDrawPrim::GetRenderMode()
{
    FEAR_LOCAL_LOG("ILTDrawPrim::GetRenderMode");
    return g_eRenderMode;
}

LTRESULT CFearDrawPrim::SetTransformMode(const ELTDrawPrimTransformMode eTransformMode)
{
    FEAR_LOCAL_LOG("ILTDrawPrim::SetTransformMode");
    g_eTransformMode = eTransformMode;
    return LT_OK;
}

ELTDrawPrimTransformMode CFearDrawPrim::GetTransformMode()
{
    FEAR_LOCAL_LOG("ILTDrawPrim::GetTransformMode");
    return g_eTransformMode;
}

LTRESULT CFearDrawPrim::DrawPrim(LT_POLYGT3 *pPrim, const uint32 nCount)
{
    FEAR_FWD_LOG("ILTDrawPrim::DrawPrim(POLYGT3)");
    if (!pPrim)
        return LT_INVALIDPARAMS;

    for (uint32 i = 0; i < nCount; ++i)
    {
        LT_VERTGT quad[4] = { pPrim[i].verts[0], pPrim[i].verts[1], pPrim[i].verts[2], pPrim[i].verts[2] };
        LTRESULT result = FearDrawTexturedQuad(quad);
        if (result != LT_OK)
            return result;
    }
    return LT_OK;
}

LTRESULT CFearDrawPrim::DrawPrim(LT_POLYG3 *pPrim, const uint32 nCount)
{
    FEAR_FWD_LOG("ILTDrawPrim::DrawPrim(POLYG3)");
    if (!pPrim)
        return LT_INVALIDPARAMS;

    for (uint32 i = 0; i < nCount; ++i)
    {
        LT_VERTG quad[4] = { pPrim[i].verts[0], pPrim[i].verts[1], pPrim[i].verts[2], pPrim[i].verts[2] };
        LTRESULT result = FearDrawColorQuad(quad);
        if (result != LT_OK)
            return result;
    }
    return LT_OK;
}

LTRESULT CFearDrawPrim::DrawPrim(LT_POLYGT4 *pPrim, const uint32 nCount)
{
    FEAR_FWD_LOG("ILTDrawPrim::DrawPrim(POLYGT4)");
    if (!pPrim)
        return LT_INVALIDPARAMS;

    for (uint32 i = 0; i < nCount; ++i)
    {
        LTRESULT result = FearDrawTexturedQuad(pPrim[i].verts);
        if (result != LT_OK)
            return result;
    }
    return LT_OK;
}

LTRESULT CFearDrawPrim::DrawPrim(LT_POLYG4 *pPrim, const uint32 nCount)
{
    FEAR_FWD_LOG("ILTDrawPrim::DrawPrim(POLYG4)");
    if (!pPrim)
        return LT_INVALIDPARAMS;

    for (uint32 i = 0; i < nCount; ++i)
    {
        LTRESULT result = FearDrawColorQuad(pPrim[i].verts);
        if (result != LT_OK)
            return result;
    }
    return LT_OK;
}

LTRESULT CFearDrawPrim::DrawPrim(LT_LINEG *pPrim, const uint32 nCount)
{
    FEAR_UNIMPL_LOG("ILTDrawPrim::DrawPrim(LINEG)");
    return LT_UNSUPPORTED;
}

LTRESULT CFearDrawPrim::BeginDrawPrimMaterialBlock()
{
    FEAR_UNIMPL_LOG("ILTDrawPrim::BeginDrawPrimMaterialBlock");
    return LT_OK;
}

LTRESULT CFearDrawPrim::EndDrawPrimMaterialBlock()
{
    FEAR_UNIMPL_LOG("ILTDrawPrim::EndDrawPrimMaterialBlock");
    return LT_OK;
}

LTRESULT CFearDrawPrim::DrawPrimMaterial(LT_POLYGTTS4 *pPrim, const uint32 nCount, HMATERIAL hMaterial)
{
    FEAR_FWD_LOG("ILTDrawPrim::DrawPrimMaterial(POLYGTTS4)");
    if (!pPrim || !hMaterial)
        return LT_INVALIDPARAMS;
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;
    void *pTexture = FearRenderer_GetMaterialTexture(hMaterial);

    for (uint32 i = 0; i < nCount; ++i)
    {
        float xy[8];
        float uv[8];
        for (int k = 0; k < 4; ++k)
        {
            xy[k * 2] = pPrim[i].verts[k].pos.x;
            xy[k * 2 + 1] = pPrim[i].verts[k].pos.y;
            uv[k * 2] = pPrim[i].verts[k].uv.x;
            uv[k * 2 + 1] = pPrim[i].verts[k].uv.y;
        }
        if (!pRender->DrawQuad2D(
            xy, uv, pPrim[i].verts[0].rgba.m_nColor, pTexture))
            return LT_ERROR;
    }
    return LT_OK;
}

LTRESULT CFearDrawPrim::DrawPrimMaterial(LT_POLYGTTS3 *pPrim, const uint32 nCount, HMATERIAL hMaterial)
{
    FEAR_FWD_LOG("ILTDrawPrim::DrawPrimMaterial(POLYGTTS3)");
    if (!pPrim || !hMaterial)
        return LT_INVALIDPARAMS;
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;
    void *pTexture = FearRenderer_GetMaterialTexture(hMaterial);

    for (uint32 i = 0; i < nCount; ++i)
    {
        float xy[8];
        float uv[8];
        for (int k = 0; k < 4; ++k)
        {
            const LT_VERTGTTS &vertex = pPrim[i].verts[k < 3 ? k : 2];
            xy[k * 2] = vertex.pos.x;
            xy[k * 2 + 1] = vertex.pos.y;
            uv[k * 2] = vertex.uv.x;
            uv[k * 2 + 1] = vertex.uv.y;
        }
        if (!pRender->DrawQuad2D(
            xy, uv, pPrim[i].verts[0].rgba.m_nColor, pTexture))
            return LT_ERROR;
    }
    return LT_OK;
}
