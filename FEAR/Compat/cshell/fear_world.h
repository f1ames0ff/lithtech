#ifndef __FEAR_WORLD_H__
#define __FEAR_WORLD_H__

#include <d3d9.h>

int FearWorld_Load(const char *pszFilename);
void FearWorld_Render(IDirect3DDevice9 *pDevice);
void FearWorld_SetCamera(const float *pPos, float fYaw, float fPitch);
void FearWorld_SetFov(float fFovX, float fFovY);
void FearWorld_Shutdown();

#endif
