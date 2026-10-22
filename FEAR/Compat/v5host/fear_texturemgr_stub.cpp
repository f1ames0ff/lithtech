#include <map>
#include <string>
#include "fear_texturemgr_stub.h"
#include "fear_render_api.h"
#include "fear_host_api.h"
#include <stdio.h>

template <typename T> struct SvcDefault { static T Get() { return T(); } };
template <> struct SvcDefault<LTRESULT> { static LTRESULT Get() { return LT_UNSUPPORTED; } };
template <> struct SvcDefault<const char*> { static const char* Get() { return ""; } };

#define FEAR_FWD_LOG(name) \
    do { static int s_nFearLogged = 0; if (!s_nFearLogged) { s_nFearLogged = 1; printf("FWD %s\n", (name)); } } while (0)
#define FEAR_UNIMPL_LOG(name) \
    do { static int s_nFearLogged = 0; if (!s_nFearLogged) { s_nFearLogged = 1; printf("UNIMPL %s\n", (name)); } } while (0)

static const FearRenderApi *FearRender(void)
{
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    return pApi ? pApi->m_pRenderApi : NULL;
}

CFearTextureMgr g_FearTextureMgr;

static std::map<std::string, HTEXTURE> g_FearTextureCache;
static std::map<HTEXTURE, int> g_FearTextureRefs;

static void FearTextureAddRef(HTEXTURE hTexture)
{
    if (hTexture)
        g_FearTextureRefs[hTexture]++;
}

static void FearTextureReleaseRef(HTEXTURE hTexture)
{
    if (!hTexture)
        return;
    std::map<HTEXTURE, int>::iterator it = g_FearTextureRefs.find(hTexture);
    if (it == g_FearTextureRefs.end())
        return;
    if (it->second > 1)
        it->second--;
}

LTRESULT CFearTextureMgr::CreateTextureFromFile(HTEXTURE &hTexture, const char *pFilename)
{
    FEAR_FWD_LOG("ILTTextureMgr::CreateTextureFromFile");
    const FearRenderApi *pRender = FearRender();
    if (!pRender || !pFilename)
        return LT_UNSUPPORTED;

    std::map<std::string, HTEXTURE>::iterator it = g_FearTextureCache.find(pFilename);
    if (it != g_FearTextureCache.end())
    {
        hTexture = it->second;
        FearTextureAddRef(hTexture);
        return LT_OK;
    }

    hTexture = (HTEXTURE)pRender->CreateTexture(pFilename, 0, 0, 0, 0);
    if (!hTexture)
        return LT_NOTFOUND;

    g_FearTextureCache[pFilename] = hTexture;
    g_FearTextureRefs[hTexture] = 2;
    return LT_OK;
}

LTRESULT CFearTextureMgr::GetTextureDims(HTEXTURE hTexture, uint32 &nWidth, uint32 &nHeight)
{
    FEAR_FWD_LOG("ILTTextureMgr::GetTextureDims(HTEXTURE)");
    const FearRenderApi *pRender = FearRender();
    if (!pRender)
        return LT_UNSUPPORTED;

    if (!hTexture)
        return LT_INVALIDPARAMS;

    int nW = 0;
    int nH = 0;
    if (!pRender->GetTextureDims((void *)hTexture, &nW, &nH))
        return LT_ERROR;

    nWidth = (uint32)nW;
    nHeight = (uint32)nH;
    return LT_OK;
}

LTRESULT CFearTextureMgr::GetTextureDims(HDYNAMICTEXTURE hTexture, uint32 &nWidth, uint32 &nHeight)
{
    FEAR_UNIMPL_LOG("ILTTextureMgr::GetTextureDims(HDYNAMICTEXTURE)");
    return LT_UNSUPPORTED;
}

LTRESULT CFearTextureMgr::ReleaseTexture(HTEXTURE hTexture)
{
    FEAR_FWD_LOG("ILTTextureMgr::ReleaseTexture");
    FearTextureReleaseRef(hTexture);
    return LT_OK;
}

LTRESULT CFearTextureMgr::AddTextureRef(HTEXTURE hTexture)
{
    if (!hTexture)
        return LT_INVALIDPARAMS;
    FearTextureAddRef(hTexture);
    return LT_OK;
}

LTRESULT CFearTextureMgr::CreateDynamicTexture(HDYNAMICTEXTURE &hTexture, const CTextureData& TextureData)
{
    FEAR_UNIMPL_LOG("ILTTextureMgr::CreateDynamicTexture(CTextureData)");
    return LT_UNSUPPORTED;
}

LTRESULT CFearTextureMgr::CreateDynamicTexture(HDYNAMICTEXTURE &hTexture, ILTInStream* pInFile)
{
    FEAR_UNIMPL_LOG("ILTTextureMgr::CreateDynamicTexture(ILTInStream)");
    return LT_UNSUPPORTED;
}

LTRESULT CFearTextureMgr::ReleaseDynamicTexture(HDYNAMICTEXTURE hTexture)
{
    FEAR_UNIMPL_LOG("ILTTextureMgr::ReleaseDynamicTexture");
    return LT_OK;
}

LTRESULT CFearTextureMgr::AddDynamicTextureRef(HDYNAMICTEXTURE hTexture)
{
    FEAR_UNIMPL_LOG("ILTTextureMgr::AddDynamicTextureRef");
    return LT_OK;
}
