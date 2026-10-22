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
} FearEngineApi;

#ifdef __cplusplus
extern "C" {
#endif

const FearEngineApi *FearHost_GetEngineApi(void);

#ifdef __cplusplus
}
#endif

#endif
