#ifndef __FEAR_MODEL_H__
#define __FEAR_MODEL_H__

#include <d3d9.h>

struct FearModelLight
{
    float m_Pos[3];
    float m_Color[3];
    float m_Radius;
};

struct FearModel;

FearModel *FearModel_LoadFile(const char *pszPath);
FearModel *FearModel_LoadByName(const char *pszName);
void FearModel_Release(FearModel *pModel);

int FearModel_GetVertexCount(const FearModel *pModel);
int FearModel_GetTriangleCount(const FearModel *pModel);
void FearModel_GetBounds(const FearModel *pModel, float *pMin, float *pMax);
const char *FearModel_GetPath(const FearModel *pModel);

void FearModel_SetLights(const FearModelLight *pLights, int nCount, const float *pAmbient);
void FearModel_Queue(FearModel *pModel, const char *pszMaterial, const float *pMatrix);
void FearModel_QueueSkinned(FearModel *pModel, const char *pszMaterial, const float *pMatrix,
    const float *pNodeMatrices, unsigned int nNodeCount);
void FearModel_ClearQueue();
void FearModel_RenderQueued(IDirect3DDevice9 *pDevice, const float *pView, const float *pProj,
    const float *pEye, float fFovY, float fAspect);
void FearModel_Shutdown();

#endif
