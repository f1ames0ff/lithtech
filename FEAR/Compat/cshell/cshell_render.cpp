#include "cshell_render.h"

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <new>

#include "cshell_filemgr.h"
#include "dds_loader.h"
#include "fear_world.h"

#include "ltbasedefs.h"
#include "ltcodes.h"
#include "ltmodule.h"
#include "iltclient.h"
#include "iltdrawprim.h"
#include "ilttexinterface.h"
#include "iltrendermgr.h"
#include <d3d9.h>

static ILTClient *g_pFearClient = NULL;
define_holder(ILTClient, g_pFearClient);

static ILTDrawPrim *g_pFearDrawPrim = NULL;
define_holder_to_instance(ILTDrawPrim, g_pFearDrawPrim, Internal);

static ILTTexInterface *g_pFearTexInterface = NULL;
define_holder(ILTTexInterface, g_pFearTexInterface);

static ILTRenderMgr *g_pFearRenderMgr = NULL;
define_holder(ILTRenderMgr, g_pFearRenderMgr);

#define FEAR_LOG_ONCE(name) \
    do { static int s_nFearLogged = 0; if (!s_nFearLogged) { s_nFearLogged = 1; printf("FEARRENDER: %s\n", (name)); } } while (0)

static int g_n2DDepth = 0;
static int g_bDrawBlockOpen = 0;
static int g_bRendererReady = 0;

static void *g_pDecalDeleteFn = NULL;
static void *g_pDecalDeleteUser = NULL;

static int g_nFrameCalls = 0;
static int g_nFrameTris = 0;

#define FEAR_MAX_RENDER_TARGETS 32

struct FearRenderTargetEntry
{
    unsigned int m_nId;
    int m_nWidth;
    int m_nHeight;
};

static FearRenderTargetEntry g_RenderTargets[FEAR_MAX_RENDER_TARGETS];
static int g_nRenderTargetCount = 0;
static unsigned int g_nNextRenderTargetId = 1;
static unsigned int g_nCurrentRenderTargetId = 0;
static int g_bDefaultTargetStored = 0;

static FearRenderTargetEntry *FearFindRenderTarget(unsigned int nId)
{
    for (int i = 0; i < g_nRenderTargetCount; ++i)
    {
        if (g_RenderTargets[i].m_nId == nId)
            return &g_RenderTargets[i];
    }
    return NULL;
}

static void FearRemoveRenderTargetEntry(unsigned int nId)
{
    for (int i = 0; i < g_nRenderTargetCount; ++i)
    {
        if (g_RenderTargets[i].m_nId == nId)
        {
            g_RenderTargets[i] = g_RenderTargets[g_nRenderTargetCount - 1];
            g_nRenderTargetCount--;
            return;
        }
    }
}

static ERenderTargetFormat FearRenderTargetFormat(int nFormat)
{
    if (nFormat == 0x7FFFFFFF)
        return RTFMT_A8R8G8B8;
    if (nFormat & (1 << 7))
        return RTFMT_R32F;
    return RTFMT_A8R8G8B8;
}

static EStencilBufferFormat FearStencilBufferFormat(int nFormat)
{
    if (nFormat & ((1 << 2) | (1 << 8) | (1 << 9)))
        return STFMT_D24S8;
    return STFMT_UNKNOWN;
}

static int FearRendererReady(void)
{
    if (g_bRendererReady)
        return 1;

    if (!g_pFearClient)
        return 0;

    RMode mode;
    memset(&mode, 0, sizeof(mode));
    g_bRendererReady = (g_pFearClient->GetRenderMode(&mode) == LT_OK) ? 1 : 0;
    return g_bRendererReady;
}

static int FearIsSmallHandle(void *hHandle)
{
    return (size_t)hHandle < 0x10000;
}

static int FearRender_SetRenderTarget(void *hTarget)
{
    FEAR_LOG_ONCE("SetRenderTarget");

    if (!hTarget)
    {
        if (g_pFearRenderMgr && g_nCurrentRenderTargetId != 0)
            g_pFearRenderMgr->RestoreDefaultRenderTarget();

        g_nCurrentRenderTargetId = 0;
        g_bDefaultTargetStored = 0;
        return 1;
    }

    if (!g_pFearRenderMgr)
        return 0;

    unsigned int nId = (unsigned int)(size_t)hTarget;
    if (!FearFindRenderTarget(nId))
        return 0;

    if (!g_bDefaultTargetStored)
    {
        if (g_pFearRenderMgr->StoreDefaultRenderTarget() != LT_OK)
            return 0;
        g_bDefaultTargetStored = 1;
    }

    if (g_pFearRenderMgr->InstallRenderTarget(nId) != LT_OK)
        return 0;

    g_nCurrentRenderTargetId = nId;
    return 1;
}

static int FearRender_Clear(unsigned int color, float depth, unsigned int stencil)
{
    FEAR_LOG_ONCE("Clear");

    if (!g_pFearClient)
        return 0;

    unsigned int nFlags = CLEARSCREEN_SCREEN;
    if (depth != 0.0f || stencil != 0)
        nFlags |= CLEARSCREEN_RENDER;

    LTRGB clearColor;
    clearColor.a = (uint8)((color >> 24) & 0xFF);
    clearColor.r = (uint8)((color >> 16) & 0xFF);
    clearColor.g = (uint8)((color >> 8) & 0xFF);
    clearColor.b = (uint8)(color & 0xFF);

    return (g_pFearClient->ClearScreen(NULL, nFlags, &clearColor) == LT_OK) ? 1 : 0;
}

static int FearRender_Start2D(void)
{
    FEAR_LOG_ONCE("Start2D");

    if (!g_pFearClient)
        return 0;

    if (g_pFearClient->Start3D() != LT_OK)
        return 0;

    FearRendererReady();

    g_n2DDepth++;
    return 1;
}

static int FearRender_End2D(void)
{
    FEAR_LOG_ONCE("End2D");

    if (g_n2DDepth <= 0)
        return 0;

    g_n2DDepth--;

    if (g_bDrawBlockOpen && g_pFearDrawPrim)
    {
        g_pFearDrawPrim->EndDrawPrim();
        g_bDrawBlockOpen = 0;
    }

    if (!g_pFearClient)
        return 0;

    g_pFearClient->End3D(END3D_CANDRAWCONSOLE);
    return 1;
}

static int FearRender_DrawQuad2D(const float *pXY, const float *pUV, unsigned int color, void *hTexture)
{
    FEAR_LOG_ONCE("DrawQuad2D");

    if (!g_pFearDrawPrim || !pXY || !pUV)
        return 0;

    LT_POLYGT4 quad;
    for (int i = 0; i < 4; ++i)
    {
        LT_VERTGT &v = quad.verts[i];
        v.x = pXY[i * 2];
        v.y = pXY[i * 2 + 1];
        v.z = SCREEN_NEAR_Z;
        v.u = pUV[i * 2];
        v.v = pUV[i * 2 + 1];
        v.rgba.a = (uint8)((color >> 24) & 0xFF);
        v.rgba.r = (uint8)((color >> 16) & 0xFF);
        v.rgba.g = (uint8)((color >> 8) & 0xFF);
        v.rgba.b = (uint8)(color & 0xFF);
    }

    if (g_n2DDepth <= 0 && g_bDrawBlockOpen)
    {
        g_pFearDrawPrim->EndDrawPrim();
        g_bDrawBlockOpen = 0;
    }

    int bTempBlock = 0;
    if (!g_bDrawBlockOpen)
    {
        if (!FearRendererReady())
            return 0;

        g_pFearDrawPrim->SetCamera(NULL);
        g_pFearDrawPrim->SetReallyClose(false);

        if (g_pFearDrawPrim->BeginDrawPrim() != LT_OK)
            return 0;

        if (g_n2DDepth > 0)
            g_bDrawBlockOpen = 1;
        else
            bTempBlock = 1;
    }

    g_pFearDrawPrim->SetTransformType(DRAWPRIM_TRANSFORM_SCREEN);
    g_pFearDrawPrim->SetZBufferMode(DRAWPRIM_NOZ);
    g_pFearDrawPrim->SetAlphaBlendMode(DRAWPRIM_BLEND_MOD_SRCALPHA);
    g_pFearDrawPrim->SetAlphaTestMode(DRAWPRIM_NOALPHATEST);
    g_pFearDrawPrim->SetClipMode(DRAWPRIM_NOCLIP);
    g_pFearDrawPrim->SetFillMode(DRAWPRIM_FILL);
    g_pFearDrawPrim->SetCullMode(DRAWPRIM_CULL_NONE);
    g_pFearDrawPrim->SetFogEnable(false);
    int bHasTexture = hTexture && !FearIsSmallHandle(hTexture);

    g_pFearDrawPrim->SetColorOp(bHasTexture ? DRAWPRIM_MODULATE : DRAWPRIM_NOCOLOROP);
    g_pFearDrawPrim->SetTexture(bHasTexture ? (HTEXTURE)hTexture : NULL);

    LTRESULT result = g_pFearDrawPrim->DrawPrim(&quad, 1);

    if (bTempBlock)
        g_pFearDrawPrim->EndDrawPrim();

    return (result == LT_OK) ? 1 : 0;
}

static int FearHasDDSExtension(const char *pszFilename)
{
    if (!pszFilename)
        return 0;

    size_t nLength = strlen(pszFilename);
    if (nLength < 4)
        return 0;

    const char *pszExtension = pszFilename + nLength - 4;
    if (pszExtension[0] != '.')
        return 0;

    return (pszExtension[1] == 'd' || pszExtension[1] == 'D') &&
        (pszExtension[2] == 'd' || pszExtension[2] == 'D') &&
        (pszExtension[3] == 's' || pszExtension[3] == 'S');
}

#define FEAR_MAX_DDS_LOGGED_PATHS 64

static char g_DDSLoggedPaths[FEAR_MAX_DDS_LOGGED_PATHS][96];
static int g_nDDSLoggedPathCount = 0;

static int FearShouldLogDDS(const char *pszFilename)
{
    for (int i = 0; i < g_nDDSLoggedPathCount; ++i)
    {
        if (strcmp(g_DDSLoggedPaths[i], pszFilename) == 0)
            return 0;
    }

    if (g_nDDSLoggedPathCount >= FEAR_MAX_DDS_LOGGED_PATHS)
        return 0;

    strncpy(g_DDSLoggedPaths[g_nDDSLoggedPathCount], pszFilename, sizeof(g_DDSLoggedPaths[0]) - 1);
    g_DDSLoggedPaths[g_nDDSLoggedPathCount][sizeof(g_DDSLoggedPaths[0]) - 1] = 0;
    ++g_nDDSLoggedPathCount;
    return 1;
}

static int FearIsValidTextureSize(unsigned int nSize)
{
    return nSize == 8 || nSize == 16 || nSize == 32 || nSize == 64 ||
        nSize == 128 || nSize == 256 || nSize == 512 || nSize == 1024;
}

static unsigned int FearFitTextureSize(unsigned int nSize)
{
    if (nSize <= 8)
        return 8;
    if (nSize >= 1024)
        return 1024;

    unsigned int nFitted = 8;
    while (nFitted < nSize)
        nFitted <<= 1;
    return nFitted;
}

static unsigned char *FearResamplePixels(const unsigned char *pSource, unsigned int nSourceWidth,
    unsigned int nSourceHeight, unsigned int nWidth, unsigned int nHeight)
{
    if (!pSource || nSourceWidth == 0 || nSourceHeight == 0 || nWidth == 0 || nHeight == 0)
        return NULL;

    size_t nBytes = (size_t)nWidth * (size_t)nHeight * 4;
    unsigned char *pDestination = new (std::nothrow) unsigned char[nBytes];
    if (!pDestination)
        return NULL;

    int bDownscale = nWidth < nSourceWidth || nHeight < nSourceHeight;

    for (unsigned int nY = 0; nY < nHeight; ++nY)
    {
        unsigned char *pDestinationRow = pDestination + (size_t)nY * nWidth * 4;

        for (unsigned int nX = 0; nX < nWidth; ++nX)
        {
            unsigned char *pDestinationPixel = pDestinationRow + (size_t)nX * 4;

            if (bDownscale)
            {
                unsigned int nSourceX0 = (unsigned int)((unsigned long long)nX * nSourceWidth / nWidth);
                unsigned int nSourceX1 = (unsigned int)(((unsigned long long)(nX + 1) * nSourceWidth + nWidth - 1) / nWidth);
                unsigned int nSourceY0 = (unsigned int)((unsigned long long)nY * nSourceHeight / nHeight);
                unsigned int nSourceY1 = (unsigned int)(((unsigned long long)(nY + 1) * nSourceHeight + nHeight - 1) / nHeight);

                if (nSourceX1 <= nSourceX0)
                    nSourceX1 = nSourceX0 + 1;
                if (nSourceY1 <= nSourceY0)
                    nSourceY1 = nSourceY0 + 1;
                if (nSourceX1 > nSourceWidth)
                    nSourceX1 = nSourceWidth;
                if (nSourceY1 > nSourceHeight)
                    nSourceY1 = nSourceHeight;

                unsigned long long nSumRed = 0;
                unsigned long long nSumGreen = 0;
                unsigned long long nSumBlue = 0;
                unsigned long long nSumAlpha = 0;

                for (unsigned int nSourceY = nSourceY0; nSourceY < nSourceY1; ++nSourceY)
                {
                    const unsigned char *pSourcePixel = pSource + ((size_t)nSourceY * nSourceWidth + nSourceX0) * 4;
                    for (unsigned int nSourceX = nSourceX0; nSourceX < nSourceX1; ++nSourceX)
                    {
                        nSumRed += pSourcePixel[0];
                        nSumGreen += pSourcePixel[1];
                        nSumBlue += pSourcePixel[2];
                        nSumAlpha += pSourcePixel[3];
                        pSourcePixel += 4;
                    }
                }

                unsigned long long nCount = (unsigned long long)(nSourceX1 - nSourceX0) * (nSourceY1 - nSourceY0);
                pDestinationPixel[0] = (unsigned char)(nSumRed / nCount);
                pDestinationPixel[1] = (unsigned char)(nSumGreen / nCount);
                pDestinationPixel[2] = (unsigned char)(nSumBlue / nCount);
                pDestinationPixel[3] = (unsigned char)(nSumAlpha / nCount);
            }
            else
            {
                float fSourceX = ((float)nX + 0.5f) * (float)nSourceWidth / (float)nWidth - 0.5f;
                float fSourceY = ((float)nY + 0.5f) * (float)nSourceHeight / (float)nHeight - 0.5f;

                if (fSourceX < 0.0f)
                    fSourceX = 0.0f;
                if (fSourceY < 0.0f)
                    fSourceY = 0.0f;

                float fMaxX = (float)(nSourceWidth - 1);
                float fMaxY = (float)(nSourceHeight - 1);
                if (fSourceX > fMaxX)
                    fSourceX = fMaxX;
                if (fSourceY > fMaxY)
                    fSourceY = fMaxY;

                unsigned int nSourceX0 = (unsigned int)fSourceX;
                unsigned int nSourceY0 = (unsigned int)fSourceY;
                unsigned int nSourceX1 = nSourceX0 + 1 < nSourceWidth ? nSourceX0 + 1 : nSourceX0;
                unsigned int nSourceY1 = nSourceY0 + 1 < nSourceHeight ? nSourceY0 + 1 : nSourceY0;

                float fFractionX = fSourceX - (float)nSourceX0;
                float fFractionY = fSourceY - (float)nSourceY0;

                const unsigned char *pTopLeft = pSource + ((size_t)nSourceY0 * nSourceWidth + nSourceX0) * 4;
                const unsigned char *pTopRight = pSource + ((size_t)nSourceY0 * nSourceWidth + nSourceX1) * 4;
                const unsigned char *pBottomLeft = pSource + ((size_t)nSourceY1 * nSourceWidth + nSourceX0) * 4;
                const unsigned char *pBottomRight = pSource + ((size_t)nSourceY1 * nSourceWidth + nSourceX1) * 4;

                for (int i = 0; i < 4; ++i)
                {
                    float fTop = (float)pTopLeft[i] + ((float)pTopRight[i] - (float)pTopLeft[i]) * fFractionX;
                    float fBottom = (float)pBottomLeft[i] + ((float)pBottomRight[i] - (float)pBottomLeft[i]) * fFractionX;
                    float fValue = fTop + (fBottom - fTop) * fFractionY;
                    pDestinationPixel[i] = (unsigned char)(fValue + 0.5f);
                }
            }
        }
    }

    return pDestination;
}

static void *FearRender_CreateTextureFromDDS(const char *pszFilename)
{
    int bShouldLog = FearShouldLogDDS(pszFilename);

    printf("DDSDBG open %s\n", pszFilename);
    void *hFile = FearFile_Open(pszFilename);
    printf("DDSDBG handle %p\n", hFile);
    if (!hFile)
    {
        if (bShouldLog)
            printf("FEARRENDER: DDS %s failed\n", pszFilename);
        return NULL;
    }

    unsigned long long nFileSize = FearFile_Size(hFile);
    printf("DDSDBG size %llu\n", nFileSize);
    if (nFileSize < 128 || nFileSize > 134217728ull)
    {
        FearFile_Close(hFile);
        if (bShouldLog)
            printf("FEARRENDER: DDS %s failed\n", pszFilename);
        return NULL;
    }

    unsigned int nDataSize = (unsigned int)nFileSize;
    unsigned char *pData = new (std::nothrow) unsigned char[nDataSize];
    if (!pData)
    {
        FearFile_Close(hFile);
        if (bShouldLog)
            printf("FEARRENDER: DDS %s failed\n", pszFilename);
        return NULL;
    }

    int bRead = FearFile_Read(hFile, pData, nDataSize);
    FearFile_Close(hFile);

    printf("DDSDBG read %d\n", bRead);
    FearDDSImage image = { 0, 0, NULL };
    if (!bRead || !FearDDS_Load(pData, nDataSize, image))
    {
        delete [] pData;
        if (bShouldLog)
            printf("FEARRENDER: DDS %s failed\n", pszFilename);
        return NULL;
    }

    delete [] pData;

    unsigned int nUploadWidth = image.m_Width;
    unsigned int nUploadHeight = image.m_Height;
    unsigned char *pResizedPixels = NULL;

    if (!FearIsValidTextureSize(nUploadWidth) || !FearIsValidTextureSize(nUploadHeight))
    {
        unsigned int nFittedWidth = FearFitTextureSize(nUploadWidth);
        unsigned int nFittedHeight = FearFitTextureSize(nUploadHeight);

        pResizedPixels = FearResamplePixels(image.m_pPixels, image.m_Width, image.m_Height,
            nFittedWidth, nFittedHeight);

        if (pResizedPixels)
        {
            nUploadWidth = nFittedWidth;
            nUploadHeight = nFittedHeight;
        }
    }

    unsigned char *pUploadPixels = pResizedPixels ? pResizedPixels : image.m_pPixels;

    size_t nPixels = (size_t)nUploadWidth * (size_t)nUploadHeight;
    for (size_t i = 0; i < nPixels; ++i)
    {
        unsigned char *pPixel = pUploadPixels + i * 4;
        unsigned char nRed = pPixel[0];
        pPixel[0] = pPixel[2];
        pPixel[2] = nRed;
    }

    printf("DDSDBG decode %ux%u pixels=%p iface=%p\n", image.m_Width, image.m_Height, image.m_pPixels, (void*)g_pFearTexInterface);
    HTEXTURE hTexture = NULL;
    printf("DDSDBG upload %ux%u\n", nUploadWidth, nUploadHeight);
    LTRESULT result = g_pFearTexInterface->CreateTextureFromData(hTexture, TEXTURETYPE_ARGB8888,
        TEXTUREFLAG_NOSYSCACHE, pUploadPixels, nUploadWidth, nUploadHeight);

    printf("DDSDBG result %d handle %p\n", (int)result, (void*)hTexture);
    if (bShouldLog)
    {
        if (result == LT_OK)
            printf("FEARRENDER: DDS %s %ux%u\n", pszFilename, image.m_Width, image.m_Height);
        else
            printf("FEARRENDER: DDS %s %ux%u failed\n", pszFilename, image.m_Width, image.m_Height);
    }

    if (bShouldLog && pResizedPixels)
        printf("FEARRENDER: DDS %s uploaded at %ux%u\n", pszFilename, nUploadWidth, nUploadHeight);

    delete [] pResizedPixels;
    FearDDS_Free(image);

    return (result == LT_OK) ? hTexture : NULL;
}

static void *FearRender_CreateTexture(const char *pszFilename, int nWidth, int nHeight, int nFormat, int nFlags)
{
    static int s_nTextureLogCount = 0;
    if (s_nTextureLogCount < 32)
    {
        ++s_nTextureLogCount;
        printf("FEARRENDER: CreateTexture(%s, %d, %d, %d, %d)\n",
            pszFilename ? pszFilename : "(null)", nWidth, nHeight, nFormat, nFlags);
    }

    if (!g_pFearTexInterface)
        return NULL;

    HTEXTURE hTexture = NULL;

    if (pszFilename && pszFilename[0])
    {
        if (FearHasDDSExtension(pszFilename))
            return FearRender_CreateTextureFromDDS(pszFilename);

        if (g_pFearTexInterface->CreateTextureFromName(hTexture, pszFilename) == LT_OK)
            return hTexture;

        static int s_nLoadFailLogged = 0;
        if (!s_nLoadFailLogged)
        {
            s_nLoadFailLogged = 1;
            printf("FEARRENDER: CreateTexture failed for %s\n", pszFilename);
        }
        return NULL;
    }

    if (nWidth <= 0 || nHeight <= 0)
        return NULL;

    size_t nBytes = (size_t)nWidth * (size_t)nHeight * 4;
    uint8 *pData = new uint8[nBytes];
    if (!pData)
        return NULL;
    memset(pData, 0, nBytes);

    uint32 nTextureFlags = TEXTUREFLAG_NOSYSCACHE;
    LTRESULT result = g_pFearTexInterface->CreateTextureFromData(hTexture, TEXTURETYPE_ARGB8888,
        nTextureFlags, pData, (uint32)nWidth, (uint32)nHeight);

    delete [] pData;
    return (result == LT_OK) ? hTexture : NULL;
}

static void FearRender_ReleaseTexture(void *hTexture)
{
    FEAR_LOG_ONCE("ReleaseTexture");

    if (g_pFearTexInterface && hTexture && !FearIsSmallHandle(hTexture))
        g_pFearTexInterface->ReleaseTextureHandle((HTEXTURE)hTexture);
}

static int FearRender_GetTextureDims(void *hTexture, int *pWidth, int *pHeight)
{
    FEAR_LOG_ONCE("GetTextureDims");

    if (!pWidth || !pHeight)
        return 0;

    if (!hTexture)
    {
        RMode mode;
        memset(&mode, 0, sizeof(mode));
        if (!g_pFearClient || g_pFearClient->GetRenderMode(&mode) != LT_OK)
            return 0;
        *pWidth = (int)mode.m_Width;
        *pHeight = (int)mode.m_Height;
        return 1;
    }

    FearRenderTargetEntry *pTarget = FearFindRenderTarget((unsigned int)(size_t)hTexture);
    if (pTarget)
    {
        *pWidth = pTarget->m_nWidth;
        *pHeight = pTarget->m_nHeight;
        return 1;
    }

    if (FearIsSmallHandle(hTexture))
        return 0;

    if (!g_pFearTexInterface)
        return 0;

    uint32 nWidth = 0;
    uint32 nHeight = 0;
    if (g_pFearTexInterface->GetTextureDims((HTEXTURE)hTexture, nWidth, nHeight) != LT_OK)
        return 0;

    *pWidth = (int)nWidth;
    *pHeight = (int)nHeight;
    return 1;
}

static void *FearRender_CreateRenderTarget(int nWidth, int nHeight, int nFormat)
{
    FEAR_LOG_ONCE("CreateRenderTarget");

    if (!g_pFearRenderMgr || nWidth <= 0 || nHeight <= 0)
        return NULL;

    if (g_nRenderTargetCount >= FEAR_MAX_RENDER_TARGETS)
        return NULL;

    unsigned int nId = g_nNextRenderTargetId++;

    if (g_pFearRenderMgr->CreateRenderTarget((uint32)nWidth, (uint32)nHeight,
        FearRenderTargetFormat(nFormat), FearStencilBufferFormat(nFormat), nId) != LT_OK)
    {
        return NULL;
    }

    FearRenderTargetEntry &entry = g_RenderTargets[g_nRenderTargetCount++];
    entry.m_nId = nId;
    entry.m_nWidth = nWidth;
    entry.m_nHeight = nHeight;

    return (void *)(size_t)nId;
}

static void FearRender_ReleaseRenderTarget(void *hTarget)
{
    FEAR_LOG_ONCE("ReleaseRenderTarget");

    if (!hTarget)
        return;

    unsigned int nId = (unsigned int)(size_t)hTarget;
    if (!FearFindRenderTarget(nId))
        return;

    if (g_nCurrentRenderTargetId == nId)
    {
        if (g_pFearRenderMgr)
            g_pFearRenderMgr->RestoreDefaultRenderTarget();
        g_nCurrentRenderTargetId = 0;
        g_bDefaultTargetStored = 0;
    }

    if (g_pFearRenderMgr)
        g_pFearRenderMgr->RemoveRenderTarget(nId);

    FearRemoveRenderTargetEntry(nId);
}

static void FearRender_SetModelDecalDeleteFn(void *pFn, void *pUser)
{
    FEAR_LOG_ONCE("SetModelDecalDeleteFn");

    g_pDecalDeleteFn = pFn;
    g_pDecalDeleteUser = pUser;
}

static void FearRender_GetModelDecalDeleteFn(void **ppFn, void **ppUser)
{
    FEAR_LOG_ONCE("GetModelDecalDeleteFn");

    if (ppFn)
        *ppFn = g_pDecalDeleteFn;
    if (ppUser)
        *ppUser = g_pDecalDeleteUser;
}

static void FearRender_SetFrameStats(int nNumCalls, int nNumTris)
{
    FEAR_LOG_ONCE("SetFrameStats");

    g_nFrameCalls = nNumCalls;
    g_nFrameTris = nNumTris;
}

static int FearRender_GetFrameStats(int *pNumCalls, int *pNumTris)
{
    FEAR_LOG_ONCE("GetFrameStats");

    if (pNumCalls)
        *pNumCalls = g_nFrameCalls;
    if (pNumTris)
        *pNumTris = g_nFrameTris;
    return 1;
}

static int FearRender_IsReady(void)
{
    FEAR_LOG_ONCE("IsReady");

    if (!g_pFearClient || !g_pFearDrawPrim)
        return 0;

    RMode mode;
    memset(&mode, 0, sizeof(mode));
    return (g_pFearClient->GetRenderMode(&mode) == LT_OK) ? 1 : 0;
}

static int FearRender_Present(void)
{
    FEAR_LOG_ONCE("Present");

    if (!g_pFearClient)
        return 0;

    return (g_pFearClient->FlipScreen(0) == LT_OK) ? 1 : 0;
}

static int FearRender_MakeScreenShot(const char *pszFilename)
{
    if (!g_pFearClient || !pszFilename || !pszFilename[0])
        return 0;

    void *pDeviceValue = NULL;
    if (g_pFearClient->GetEngineHook("d3ddevice", &pDeviceValue) != LT_OK || !pDeviceValue)
    {
        printf("FEARRENDER: screenshot failed device\n");
        fflush(stdout);
        return 0;
    }

    IDirect3DDevice9 *pDevice = (IDirect3DDevice9 *)pDeviceValue;
    IDirect3DSurface9 *pBackBuffer = NULL;
    IDirect3DSurface9 *pOffscreen = NULL;
    if (FAILED(pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer)) || !pBackBuffer)
    {
        printf("FEARRENDER: screenshot failed backbuffer\n");
        fflush(stdout);
        return 0;
    }

    D3DSURFACE_DESC desc;
    memset(&desc, 0, sizeof(desc));
    if (FAILED(pBackBuffer->GetDesc(&desc)) ||
        FAILED(pDevice->CreateOffscreenPlainSurface(desc.Width, desc.Height, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &pOffscreen, NULL)) ||
        !pOffscreen)
    {
        printf("FEARRENDER: screenshot failed surface\n");
        fflush(stdout);
        pBackBuffer->Release();
        return 0;
    }

    if (FAILED(pDevice->GetRenderTargetData(pBackBuffer, pOffscreen)) &&
        FAILED(pDevice->GetFrontBufferData(0, pOffscreen)))
    {
        printf("FEARRENDER: screenshot failed readback\n");
        fflush(stdout);
        pOffscreen->Release();
        pBackBuffer->Release();
        return 0;
    }

    D3DLOCKED_RECT locked;
    memset(&locked, 0, sizeof(locked));
    if (FAILED(pOffscreen->LockRect(&locked, NULL, D3DLOCK_READONLY)))
    {
        pOffscreen->Release();
        pBackBuffer->Release();
        return 0;
    }

    const unsigned int nWidth = desc.Width;
    const unsigned int nHeight = desc.Height;
    const unsigned int nImageBytes = nWidth * nHeight * 4;
    unsigned char *pPixels = new (std::nothrow) unsigned char[nImageBytes];
    if (!pPixels)
    {
        pOffscreen->UnlockRect();
        pOffscreen->Release();
        pBackBuffer->Release();
        return 0;
    }

    for (unsigned int y = 0; y < nHeight; ++y)
    {
        const unsigned char *pSource = (const unsigned char *)locked.pBits + (nHeight - 1 - y) * locked.Pitch;
        unsigned char *pDest = pPixels + y * nWidth * 4;
        for (unsigned int x = 0; x < nWidth; ++x)
        {
            pDest[x * 4 + 0] = pSource[x * 4 + 0];
            pDest[x * 4 + 1] = pSource[x * 4 + 1];
            pDest[x * 4 + 2] = pSource[x * 4 + 2];
            pDest[x * 4 + 3] = 255;
        }
    }

    pOffscreen->UnlockRect();
    pOffscreen->Release();
    pBackBuffer->Release();

    FILE *pFile = fopen(pszFilename, "wb");
    if (!pFile)
    {
        delete [] pPixels;
        return 0;
    }

    unsigned char header[54];
    memset(header, 0, sizeof(header));
    header[0] = 'B';
    header[1] = 'M';
    const unsigned int nFileSize = 54 + nImageBytes;
    header[2] = (unsigned char)(nFileSize);
    header[3] = (unsigned char)(nFileSize >> 8);
    header[4] = (unsigned char)(nFileSize >> 16);
    header[5] = (unsigned char)(nFileSize >> 24);
    header[10] = 54;
    header[14] = 40;
    header[18] = (unsigned char)(nWidth);
    header[19] = (unsigned char)(nWidth >> 8);
    header[20] = (unsigned char)(nWidth >> 16);
    header[21] = (unsigned char)(nWidth >> 24);
    header[22] = (unsigned char)(nHeight);
    header[23] = (unsigned char)(nHeight >> 8);
    header[24] = (unsigned char)(nHeight >> 16);
    header[25] = (unsigned char)(nHeight >> 24);
    header[26] = 1;
    header[28] = 32;
    header[34] = (unsigned char)(nImageBytes);
    header[35] = (unsigned char)(nImageBytes >> 8);
    header[36] = (unsigned char)(nImageBytes >> 16);
    header[37] = (unsigned char)(nImageBytes >> 24);

    fwrite(header, 1, sizeof(header), pFile);
    fwrite(pPixels, 1, nImageBytes, pFile);
    fclose(pFile);
    delete [] pPixels;

    printf("FEARRENDER: screenshot %s %ux%u\n", pszFilename, nWidth, nHeight);
    fflush(stdout);
    return 1;
}

static int FearRender_RenderWorld(void)
{
    FEAR_LOG_ONCE("RenderWorld");

    if (!g_pFearClient)
        return 0;

    void *pDevice = NULL;
    if (g_pFearClient->GetEngineHook("d3ddevice", &pDevice) != LT_OK || !pDevice)
        return 0;

    FearWorld_Render((IDirect3DDevice9 *)pDevice);
    return 1;
}

static void FearRender_SetWorldCamera(const float *pPos, float fYaw, float fPitch)
{
    FearWorld_SetCamera(pPos, fYaw, fPitch);
}

static void *FearRender_CreateTextureString(const wchar_t *pszString, const char *pszTypeface, int nHeight, int nStyle)
{
    (void)pszString;
    (void)pszTypeface;
    (void)nHeight;
    (void)nStyle;
    return NULL;
}

static void FearRender_ReleaseTextureString(void *hString)
{
    (void)hString;
}

static int FearRender_GetTextureStringExtents(void *hString, int *pWidth, int *pHeight)
{
    (void)hString;
    if (pWidth)
        *pWidth = 0;
    if (pHeight)
        *pHeight = 0;
    return 0;
}

static int FearRender_RenderTextureString(void *hString, float x, float y, unsigned int color, float fAnchorScaleX, float fAnchorScaleY)
{
    (void)hString;
    (void)x;
    (void)y;
    (void)color;
    (void)fAnchorScaleX;
    (void)fAnchorScaleY;
    return 0;
}

static void *FearRender_CreateTextureFromRGBA(const unsigned int *pPixels, int nWidth, int nHeight)
{
    if (!g_pFearTexInterface || !pPixels || nWidth <= 0 || nHeight <= 0)
        return NULL;

    HTEXTURE hTexture = NULL;
    LTRESULT result = g_pFearTexInterface->CreateTextureFromData(hTexture, TEXTURETYPE_ARGB8888,
        TEXTUREFLAG_NOSYSCACHE, (uint8 *)pPixels, (uint32)nWidth, (uint32)nHeight);
    return (result == LT_OK) ? hTexture : NULL;
}

extern "C" const FearRenderApi g_FearRenderApi =
{
    FearRender_SetRenderTarget,
    FearRender_Clear,
    FearRender_Start2D,
    FearRender_End2D,
    FearRender_DrawQuad2D,
    FearRender_CreateTexture,
    FearRender_ReleaseTexture,
    FearRender_GetTextureDims,
    FearRender_CreateRenderTarget,
    FearRender_ReleaseRenderTarget,
    FearRender_SetModelDecalDeleteFn,
    FearRender_GetModelDecalDeleteFn,
    FearRender_SetFrameStats,
    FearRender_GetFrameStats,
    FearRender_IsReady,
    FearRender_Present,
    FearRender_CreateTextureString,
    FearRender_ReleaseTextureString,
    FearRender_GetTextureStringExtents,
    FearRender_RenderTextureString,
    FearRender_CreateTextureFromRGBA,
    FearRender_RenderWorld,
    FearRender_SetWorldCamera,
    FearRender_MakeScreenShot
};
