#ifndef __FEAR_WORLD_H__
#define __FEAR_WORLD_H__

#include <d3d9.h>

int FearWorld_Load(const char *pszFilename);
void FearWorld_Render(IDirect3DDevice9 *pDevice);
void FearWorld_SetCamera(const float *pPos, float fYaw, float fPitch);
void FearWorld_SetFov(float fFovX, float fFovY);
void FearWorld_SetViewport(const float *pRect);
IDirect3DTexture9 *FearWorld_LoadMaterialTexture(
    IDirect3DDevice9 *pDevice, const char *pszMaterial);
bool FearWorld_GetMaterialDiffuse(
    const char *pszMaterial, char *pszDiffuse, unsigned int nLength);
void FearWorld_Shutdown();
extern "C" int FearWorld_IntersectSegment(const float *pFrom, const float *pTo, float *pHit, float *pNormal, float *pDist);

#endif
