#ifndef __FEAR_RENDER_API_H__
#define __FEAR_RENDER_API_H__

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

struct FearRenderApi
{
    int   (*SetRenderTarget)(void *hTarget);
    int   (*Clear)(unsigned int color, float depth, unsigned int stencil);
    int   (*Start2D)(void);
    int   (*End2D)(void);
    int   (*DrawQuad2D)(const float *pXY, const float *pUV, unsigned int color, void *hTexture);
    void *(*CreateTexture)(const char *pszFilename, int nWidth, int nHeight, int nFormat, int nFlags);
    void  (*ReleaseTexture)(void *hTexture);
    int   (*GetTextureDims)(void *hTexture, int *pWidth, int *pHeight);
    void *(*CreateRenderTarget)(int nWidth, int nHeight, int nFormat);
    void  (*ReleaseRenderTarget)(void *hTarget);
    void  (*SetModelDecalDeleteFn)(void *pFn, void *pUser);
    void  (*GetModelDecalDeleteFn)(void **ppFn, void **ppUser);
    void  (*SetFrameStats)(int nNumCalls, int nNumTris);
    int   (*GetFrameStats)(int *pNumCalls, int *pNumTris);
    int   (*IsReady)(void);
    int   (*Present)(void);
    void *(*CreateTextureString)(const wchar_t *pszString, const char *pszTypeface, int nHeight, int nStyle);
    void  (*ReleaseTextureString)(void *hString);
    int   (*GetTextureStringExtents)(void *hString, int *pWidth, int *pHeight);
    int   (*RenderTextureString)(void *hString, float x, float y, unsigned int color, float fAnchorScaleX, float fAnchorScaleY);
    void *(*CreateTextureFromRGBA)(const unsigned int *pPixels, int nWidth, int nHeight);
    int   (*RenderWorld)(void);
    void  (*SetWorldCamera)(const float *pPos, float fYaw, float fPitch);
    void  (*SetCameraFov)(float fFovX, float fFovY);
    int   (*MakeScreenShot)(const char *pszFilename);
    void  (*SetViewportRect)(const float *pRect);
    int   (*QueueModel)(const char *pszFilename, const char *pszMaterial, const float *pTransform);
    int   (*QueueSkinnedModel)(const char *pszFilename, const char *pszMaterial, const float *pTransform,
                               const float *pNodeMatrices, unsigned int nNodeCount);
    void *(*CreateMaterialTexture)(const char *pszMaterial);
};

typedef struct FearRenderApi FearRenderApi;

#ifdef __cplusplus
}
#endif

#endif
