#ifndef __FEAR_HOST_API_H__
#define __FEAR_HOST_API_H__

#include <stdint.h>

typedef struct FearMode
{
    uint32_t m_Width;
    uint32_t m_Height;
    uint32_t m_BitDepth;
    uint32_t m_Flags;
    int32_t  m_bHWTnL;
    char     m_DeviceName[128];
} FearMode;

typedef struct FearFileInfo
{
    char m_Name[260];
    char m_FullName[520];
    int  m_Type;
} FearFileInfo;

typedef struct FearObjectCreate
{
    uint16_t m_ObjectType;
    uint16_t m_ContainerCode;
    uint32_t m_Flags;
    uint32_t m_Flags2;
    float    m_Pos[3];
    float    m_Rotation[4];
    float    m_Scale;
    char     m_Filename[260];
    char     m_Name[128];
    uint32_t m_UserData;
} FearObjectCreate;

typedef struct FearPerformanceStats
{
    float m_fCPUFrequency;
    uint32_t m_nCPUCount;
    uint32_t m_nCPUMemory;
    float m_fGPUMaxPS;
    float m_fGPUMaxVS;
    uint32_t m_nGPUMemory;
    float m_fGPUPS11Fill;
    float m_fGPUPS20Fill;
} FearPerformanceStats;

struct FearRenderApi;

typedef struct FearEngineApi
{
    int  (*SetRenderMode)(const FearMode *pMode);
    int  (*GetRenderMode)(FearMode *pMode);
    void (*ShutdownRender)(uint32_t flags);
    void *(*GetEngineHook)(const char *pszName);
    void *(*OpenFile)(const char *pszFilename);
    void *(*FileOpen)(const char *pszFilename);
    int   (*FileRead)(void *hFile, void *pBuffer, unsigned int nSize);
    int   (*FileSeek)(void *hFile, unsigned long long nOffset);
    unsigned long long (*FileTell)(void *hFile);
    unsigned long long (*FileSize)(void *hFile);
    void  (*FileClose)(void *hFile);
    const FearFileInfo *(*FileList)(const char *pszDirectory, int *pnCount, void **ppHandle);
    void  (*FileListFree)(void *hHandle);
    void *(*CreateObject)(const FearObjectCreate *pStruct);
    const struct FearRenderApi *m_pRenderApi;
    int   (*IntersectWorld)(const float *pFrom, const float *pTo, float *pHit, float *pNormal, float *pDist);
    int   (*GetRenderModes)(FearMode *pModes, int nMaxModes);
    int   (*QueryPerformanceStats)(FearPerformanceStats *pStats, int bQuick);
    int   (*SetObjectFlags)(void *hObject, uint32_t nFlagType, uint32_t nFlags, uint32_t nMask);
    int   (*GetObjectFlags)(void *hObject, uint32_t nFlagType, uint32_t *pFlags);
    int   (*ModelSetCurAnim)(void *hObject, uint32_t nTracker, uint32_t nAnim, int bInterpolate);
    int   (*ModelSetPlaying)(void *hObject, uint32_t nTracker, int bPlaying);
    int   (*ModelSetLooping)(void *hObject, uint32_t nTracker, int bLooping);
    int   (*ModelSetCurAnimTime)(void *hObject, uint32_t nTracker, uint32_t nTime);
    int   (*ModelResetAnim)(void *hObject, uint32_t nTracker);
    int   (*ModelGetCurAnim)(void *hObject, uint32_t nTracker, uint32_t *pAnim);
    int   (*ModelGetCurAnimLength)(void *hObject, uint32_t nTracker, uint32_t *pLength);
    int   (*ModelGetCurAnimTime)(void *hObject, uint32_t nTracker, uint32_t *pTime);
    int   (*ModelGetPlaying)(void *hObject, uint32_t nTracker);
    int   (*ModelGetLooping)(void *hObject, uint32_t nTracker);
    int   (*ModelSetAnimRate)(void *hObject, uint32_t nTracker, float fRate);
    int   (*ModelGetAnimRate)(void *hObject, uint32_t nTracker, float *pRate);
    int   (*ModelGetAnimIndex)(void *hObject, const char *pszName, uint32_t *pAnim);
    int   (*ModelGetAnimName)(void *hObject, uint32_t nAnim, char *pszName, uint32_t nNameLen);
} FearEngineApi;

#ifdef __cplusplus
extern "C" {
#endif

const FearEngineApi *FearHost_GetEngineApi(void);
void FearHost_SetFrameDelta(float fDelta);
float FearHost_GetFrameDelta(void);

#ifdef __cplusplus
}
#endif

#endif
