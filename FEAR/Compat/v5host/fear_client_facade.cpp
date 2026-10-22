#include "fear_client_facade.h"
#include "iltmessage.h"
#include "fear_host_api.h"
#include "fear_renderer_stub.h"
#include "fear_drawprim_stub.h"
#include "fear_texturemgr_stub.h"
#include "fear_texturestring_stub.h"
#include "fear_input_stub.h"
#include "fear_gameutil_stub.h"
#include "fear_loadingprogress_stub.h"
#include "fear_resourcemgr_stub.h"
#include "fear_common_stub.h"
#include "fear_server_host.h"
#include "fear_physics_stub.h"
#include "fear_physicssim_stub.h"
#include "fear_model_stub.h"
#include "fear_timer_stub.h"
#include "fear_soundmgr_stub.h"
#include "fear_cursor_stub.h"
#include "fear_filemgr_stub.h"
#include "fear_resourcemgr_stub.h"
#include "ltobjectcreate.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

template <typename T> struct StubDefault { static T Get() { return T(); } };
template <> struct StubDefault<LTRESULT> { static LTRESULT Get() { return LT_UNSUPPORTED; } };
template <> struct StubDefault<const char*> { static const char* Get() { return ""; } };

static RMode g_FearMode;
static bool g_FearModeSet = false;

static void FearCopyObjectName(char *pDest, uint32 nDestSize, const char *pSrc, uint32 nSrcSize)
{
    if (!pDest || (nDestSize == 0))
        return;

    pDest[0] = '\0';
    if (!pSrc)
        return;

    uint32 nLen = 0;
    while ((nLen < nSrcSize) && (pSrc[nLen] != '\0'))
        nLen++;

    if (nLen >= nDestSize)
        nLen = nDestSize - 1;

    if (nLen > 0)
        memcpy(pDest, pSrc, nLen);

    pDest[nLen] = '\0';
}

static LTRESULT Stub_WriteConfigFileFields_0(const char *pFilename, uint32 nNumValues, const char** pValues)
{
    printf("STUB ILTClient::WriteConfigFileFields\n");
    return StubDefault<LTRESULT>::Get();
}

static LTRESULT Stub_WriteConfigFile_1(const char *pFilename)
{
    printf("STUB ILTClient::WriteConfigFile\n");
    return StubDefault<LTRESULT>::Get();
}

static LTRESULT Stub_ReadConfigFile_2(const char *pFilename)
{
    printf("STUB ILTClient::ReadConfigFile\n");
    return StubDefault<LTRESULT>::Get();
}

static LTRESULT Stub_GetSoundTimer_3(HLTSOUND hSound, LTFLOAT *fTimer)
{
    printf("STUB ILTClient::GetSoundTimer\n");
    return StubDefault<LTRESULT>::Get();
}

static LTRESULT Stub_GetSoundData_4(HLTSOUND hSound, int16 *&pSixteenBitData, int8 *&pEightBitData, uint32 *dwSamplesPerSecond, uint32 *dwChannels)
{
    printf("STUB ILTClient::GetSoundData\n");
    return StubDefault<LTRESULT>::Get();
}

static bool Stub_IsDone_5(HLTSOUND pSoundHandle)
{
    printf("STUB ILTClient::IsDone\n");
    return StubDefault<bool>::Get();
}

static void Stub_GetListener_6(bool *bListenerInClient, LTVector *pPos, LTRotation *pRot)
{
    printf("STUB ILTClient::GetListener\n");
}

static LTRESULT Stub_GetGameMode_7(int *mode)
{
    if (!mode)
        return LT_INVALIDPARAMS;
    *mode = FearServerHost_IsRunning() ? STARTGAME_NORMAL : GAMEMODE_NONE;
    return LT_OK;
}

bool g_bFearConnected = false;

static LTRESULT Stub_StartGame_8(StartGameRequest *pRequest)
{
    printf("STUB ILTClient::StartGame type=%d world='%s'\n", pRequest ? pRequest->m_Type : -1, pRequest ? pRequest->m_WorldName : "");
    fflush(stdout);
    g_bFearConnected = true;
    if (pRequest && pRequest->m_WorldName[0])
        FearServerHost_Start(pRequest->m_WorldName);
    else
        FearServerHost_Start(NULL);
    return LT_OK;
}

static LTRESULT Stub_GetLocalClientID_9(uint32 *pID)
{
    if (pID)
        *pID = 0;
    return LT_OK;
}

static void Stub_Disconnect_10()
{
    printf("STUB ILTClient::Disconnect\n");
    g_bFearConnected = false;
    FearServerHost_Shutdown();
}

static bool Stub_IsConnected_11()
{
    return g_bFearConnected;
}

static void Stub_Shutdown_12()
{
    printf("STUB ILTClient::Shutdown\n");
    FearServerHost_Shutdown();
}

static void Stub_ShutdownWithMessage_13(const wchar_t *pMsg, ...)
{
    wprintf(L"SHUTDOWN: %ls\n", pMsg);
}

static LTRESULT Stub_GetRenderMode_14(RMode *pMode)
{
    if (!pMode) return LT_INVALIDPARAMS;

    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (pApi && pApi->GetRenderMode)
    {
        FearMode mode;
        memset(&mode, 0, sizeof(mode));
        if (pApi->GetRenderMode(&mode))
        {
            memset(pMode, 0, sizeof(*pMode));
            pMode->m_Width = mode.m_Width;
            pMode->m_Height = mode.m_Height;
            pMode->m_BitDepth = mode.m_BitDepth;
            pMode->m_bHWTnL = mode.m_bHWTnL != 0;
            strncpy(pMode->m_DeviceName, mode.m_DeviceName, sizeof(pMode->m_DeviceName) - 1);
            g_FearMode = *pMode;
            g_FearModeSet = true;
            return LT_OK;
        }
    }

    if (g_FearModeSet) { *pMode = g_FearMode; return LT_OK; }
    memset(pMode, 0, sizeof(*pMode));
    pMode->m_Width = 1024;
    pMode->m_Height = 768;
    pMode->m_BitDepth = 32;
    return LT_OK;
}

static LTRESULT Stub_SetRenderMode_15(RMode *pMode)
{
    if (!pMode) return LT_INVALIDPARAMS;

    FearMode mode;
    memset(&mode, 0, sizeof(mode));
    mode.m_Width = pMode->m_Width;
    mode.m_Height = pMode->m_Height;
    mode.m_BitDepth = pMode->m_BitDepth;
    mode.m_bHWTnL = pMode->m_bHWTnL ? 1 : 0;
    strncpy(mode.m_DeviceName, pMode->m_DeviceName, sizeof(mode.m_DeviceName) - 1);

    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (pApi && pApi->SetRenderMode)
    {
        if (!pApi->SetRenderMode(&mode))
        {
            printf("FEARPORT: SetRenderMode failed %ux%u@%u\n", mode.m_Width, mode.m_Height, mode.m_BitDepth);
            return LT_ERROR;
        }
    }

    g_FearMode = *pMode;
    g_FearModeSet = true;
    printf("FEARPORT: SetRenderMode %ux%u@%u\n", mode.m_Width, mode.m_Height, mode.m_BitDepth);
    return LT_OK;
}

static LTRESULT Stub_ShutdownRender_16(uint32 flags)
{
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (pApi && pApi->ShutdownRender)
        pApi->ShutdownRender(flags);
    return LT_OK;
}

static HLOCALOBJ Stub_CreateObject_17(ObjectCreateStruct *pStruct)
{
    if (!pStruct)
        return NULL;

    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (!pApi || !pApi->CreateObject)
        return NULL;

    FearObjectCreate foc;
    memset(&foc, 0, sizeof(foc));

    foc.m_ObjectType = pStruct->m_ObjectType;
    switch (pStruct->m_ObjectType)
    {
    case OT_LIGHT:
        foc.m_ObjectType = 4;
        break;
    case OT_CAMERA:
        foc.m_ObjectType = 5;
        break;
    case OT_CONTAINER:
        foc.m_ObjectType = 9;
        break;
    case OT_CUSTOMRENDER:
        foc.m_ObjectType = 10;
        break;
    default:
        break;
    }
    foc.m_ContainerCode = pStruct->m_ContainerCode;
    foc.m_Flags = pStruct->m_Flags;
    foc.m_Flags2 = pStruct->m_Flags2;
    foc.m_UserData = pStruct->m_UserData;

    foc.m_Pos[0] = pStruct->m_Pos.x;
    foc.m_Pos[1] = pStruct->m_Pos.y;
    foc.m_Pos[2] = pStruct->m_Pos.z;

    foc.m_Rotation[0] = pStruct->m_Rotation.m_Quat[0];
    foc.m_Rotation[1] = pStruct->m_Rotation.m_Quat[1];
    foc.m_Rotation[2] = pStruct->m_Rotation.m_Quat[2];
    foc.m_Rotation[3] = pStruct->m_Rotation.m_Quat[3];

    foc.m_Scale = pStruct->m_Scale;

    FearCopyObjectName(foc.m_Filename, (uint32)sizeof(foc.m_Filename), pStruct->m_Filename, (uint32)sizeof(pStruct->m_Filename));
    FearCopyObjectName(foc.m_Name, (uint32)sizeof(foc.m_Name), pStruct->m_Name, (uint32)sizeof(pStruct->m_Name));

    void *hObject = pApi->CreateObject(&foc);
    if (!hObject && foc.m_ObjectType != 0)
    {
        foc.m_ObjectType = 0;
        hObject = pApi->CreateObject(&foc);
    }
    printf("FHOST CreateObject type=%u -> %p\n", (unsigned)pStruct->m_ObjectType, hObject);
    fflush(stdout);
    return static_cast<HLOCALOBJ>(hObject);
}

static void Stub_GetCameraFOV_18(HLOCALOBJ hObj, float *pX, float *pY)
{
    printf("STUB ILTClient::GetCameraFOV\n");
}

static void Stub_SetCameraFOV_19(HLOCALOBJ hObj, float fovX, float fovY)
{
    printf("STUB ILTClient::SetCameraFOV\n");
}

static void Stub_GetCameraRect_20(HLOCALOBJ hObj, LTRect2f& rCameraRect)
{
    printf("STUB ILTClient::GetCameraRect\n");
}

static void Stub_SetCameraRect_21(HLOCALOBJ hObj, const LTRect2f& rCameraRect)
{
    printf("STUB ILTClient::SetCameraRect\n");
}

static bool Stub_GetParticleBlockersInAABB_22(const LTVector& pos, const LTVector& dims, uint32* pIndices, uint32& nNumIndices)
{
    printf("STUB ILTClient::GetParticleBlockersInAABB\n");
    return StubDefault<bool>::Get();
}

static bool Stub_GetParticleBlockerEdgesXZ_23(uint32 index, LTPlane& blockerPlane, uint32& numEdges, LTPlane*& edgePlanes)
{
    printf("STUB ILTClient::GetParticleBlockerEdgesXZ\n");
    return StubDefault<bool>::Get();
}

static LTRESULT Stub_InitNetworking_24(const char *pDriver, uint32 dwFlags)
{
    printf("STUB ILTClient::InitNetworking\n");
    return StubDefault<LTRESULT>::Get();
}

static LTRESULT Stub_GetServiceList_25(NetService* &pListHead)
{
    printf("STUB ILTClient::GetServiceList\n");
    return StubDefault<LTRESULT>::Get();
}

static LTRESULT Stub_GetLobbyLaunchInfo_26(const char* sDriver, void** ppLobbyLaunchData)
{
    printf("STUB ILTClient::GetLobbyLaunchInfo\n");
    return StubDefault<LTRESULT>::Get();
}

static LTRESULT Stub_GetVersionInfoExt_27(LTVERSIONINFOEXT* pVersionInfo)
{
    printf("STUB ILTClient::GetVersionInfoExt\n");
    return StubDefault<LTRESULT>::Get();
}

static LTRESULT Stub_SetConsoleEnable_28(bool bEnable)
{
    printf("STUB ILTClient::SetConsoleEnable\n");
    return StubDefault<LTRESULT>::Get();
}

static double Stub_GetGameTime_29()
{
    printf("STUB ILTClient::GetGameTime\n");
    return StubDefault<double>::Get();
}

static void Stub_DebugOut_30(const char *pMsg, ...)
{
    printf("STUB ILTClient::DebugOut\n");
}

static HLOCALOBJ Stub_GetClientObject_31()
{
    return NULL;
}

static LTRESULT Stub_FreeServiceList_32(NetService *pListHead)
{
    printf("STUB ILTClient::FreeServiceList\n");
    return StubDefault<LTRESULT>::Get();
}

static LTRESULT Stub_SelectService_33(HNETSERVICE hNetService)
{
    printf("STUB ILTClient::SelectService\n");
    return StubDefault<LTRESULT>::Get();
}

static LTRESULT Stub_GetSessionList_34(NetSession* &pListHead, const char *pInfo)
{
    printf("STUB ILTClient::GetSessionList\n");
    return StubDefault<LTRESULT>::Get();
}

static LTRESULT Stub_FreeSessionList_35(NetSession *pListHead)
{
    printf("STUB ILTClient::FreeSessionList\n");
    return StubDefault<LTRESULT>::Get();
}

static LTRESULT Stub_AddInternetDriver_36()
{
    printf("STUB ILTClient::AddInternetDriver\n");
    return StubDefault<LTRESULT>::Get();
}

static LTRESULT Stub_RemoveInternetDriver_37()
{
    printf("STUB ILTClient::RemoveInternetDriver\n");
    return StubDefault<LTRESULT>::Get();
}

static LTRESULT Stub_IsLobbyLaunched_38(const char* sDriver)
{
    printf("STUB ILTClient::IsLobbyLaunched\n");
    return StubDefault<LTRESULT>::Get();
}

CFearClient::CFearClient()
{
    WriteConfigFileFields = &Stub_WriteConfigFileFields_0;
    WriteConfigFile = &Stub_WriteConfigFile_1;
    ReadConfigFile = &Stub_ReadConfigFile_2;
    GetSoundTimer = &Stub_GetSoundTimer_3;
    GetSoundData = &Stub_GetSoundData_4;
    IsDone = &Stub_IsDone_5;
    GetListener = &Stub_GetListener_6;
    GetGameMode = &Stub_GetGameMode_7;
    StartGame = &Stub_StartGame_8;
    GetLocalClientID = &Stub_GetLocalClientID_9;
    Disconnect = &Stub_Disconnect_10;
    IsConnected = &Stub_IsConnected_11;
    Shutdown = &Stub_Shutdown_12;
    ShutdownWithMessage = &Stub_ShutdownWithMessage_13;
    GetRenderMode = &Stub_GetRenderMode_14;
    SetRenderMode = &Stub_SetRenderMode_15;
    ShutdownRender = &Stub_ShutdownRender_16;
    CreateObject = &Stub_CreateObject_17;
    GetCameraFOV = &Stub_GetCameraFOV_18;
    SetCameraFOV = &Stub_SetCameraFOV_19;
    GetCameraRect = &Stub_GetCameraRect_20;
    SetCameraRect = &Stub_SetCameraRect_21;
    GetParticleBlockersInAABB = &Stub_GetParticleBlockersInAABB_22;
    GetParticleBlockerEdgesXZ = &Stub_GetParticleBlockerEdgesXZ_23;
    InitNetworking = &Stub_InitNetworking_24;
    GetServiceList = &Stub_GetServiceList_25;
    GetLobbyLaunchInfo = &Stub_GetLobbyLaunchInfo_26;
    GetVersionInfoExt = &Stub_GetVersionInfoExt_27;
    SetConsoleEnable = &Stub_SetConsoleEnable_28;
    GetGameTime = &Stub_GetGameTime_29;
    DebugOut = &Stub_DebugOut_30;
    GetClientObject = &Stub_GetClientObject_31;
    FreeServiceList = &Stub_FreeServiceList_32;
    SelectService = &Stub_SelectService_33;
    GetSessionList = &Stub_GetSessionList_34;
    FreeSessionList = &Stub_FreeSessionList_35;
    AddInternetDriver = &Stub_AddInternetDriver_36;
    RemoveInternetDriver = &Stub_RemoveInternetDriver_37;
    IsLobbyLaunched = &Stub_IsLobbyLaunched_38;
}

CFearClient::~CFearClient() {}

LTRESULT CFearClient::SendTo(ILTMessage_Read *pMsg, const char *pAddr, uint16 nPort)
{
    printf("STUB ILTClient::SendTo\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::StartPing(const char *pAddr, uint16 nPort, uint32 *pPingID)
{
    printf("STUB ILTClient::StartPing\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetPingStatus(uint32 nPingID, uint32 *pStatus, uint32 *pLatency)
{
    printf("STUB ILTClient::GetPingStatus\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::RemovePing(uint32 nPingID)
{
    printf("STUB ILTClient::RemovePing\n");
    return StubDefault<LTRESULT>::Get();
}

HOBJECT CFearClient::GetMainWorldModel()
{
    printf("STUB ILTClient::GetMainWorldModel\n");
    return StubDefault<HOBJECT>::Get();
}

LTRESULT CFearClient::GetObjectRotation(HLOCALOBJ hObj, LTRotation *pRotation)
{
    printf("STUB ILTClient::GetObjectRotation\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetObjectRotation(HOBJECT hObj, const LTRotation& rRotation)
{
    printf("STUB ILTClient::SetObjectRotation\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetObjectPos(HLOCALOBJ hObj, LTVector *pPos)
{
    printf("STUB ILTClient::GetObjectPos\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetObjectPos(HLOCALOBJ hObj, const LTVector& vPos)
{
    printf("STUB ILTClient::SetObjectPos\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::ForceCurrentObjectPos(HLOCALOBJ hObj)
{
    printf("STUB ILTClient::ForceCurrentObjectPos\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetObjectTransform(HLOCALOBJ hObj, LTRigidTransform *pTransform)
{
    printf("STUB ILTClient::GetObjectTransform\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetObjectTransform(HLOCALOBJ hObj, const LTRigidTransform& tTransform)
{
    printf("STUB ILTClient::SetObjectTransform\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetObjectTransform(HLOCALOBJ hObj, LTTransform *pTransform)
{
    printf("STUB ILTClient::GetObjectTransform\n");
    return StubDefault<LTRESULT>::Get();
}

ILTResourceMgr* CFearClient::ResourceMgr()
{
    return &g_FearResourceMgr;
}

ILTTimer * CFearClient::Timer()
{
    return &g_FearTimer;
}

ILTModel * CFearClient::GetModelLT()
{
    return &g_FearModel;
}

ILTSoundMgr * CFearClient::SoundMgr()
{
    void **ppVt = *reinterpret_cast<void ***>(reinterpret_cast<void *>(&g_FearSoundMgr));
    return &g_FearSoundMgr;
}

ILTFileMgr * CFearClient::FileMgr()
{
    return &g_FearFileMgr;
}

ILTCommon * CFearClient::Common()
{
    return &g_FearCommon;
}

ILTPhysics * CFearClient::Physics()
{
    return &g_FearPhysics;
}

ILTPhysicsSim * CFearClient::PhysicsSim()
{
    return &g_FearPhysicsSim;
}

ILTLoadingProgress * CFearClient::LoadingProgress()
{
    return &g_FearLoadingProgress;
}

LTRESULT CFearClient::SetObjectTransform(HLOCALOBJ hObj, const LTTransform& tTransform)
{
    printf("STUB ILTClient::SetObjectTransform\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetObjectScale(HOBJECT hObj, float* pScale)
{
    printf("STUB ILTClient::GetObjectScale\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetObjectScale(HOBJECT hObj, float fScale)
{
    printf("STUB ILTClient::SetObjectScale\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetObjectColor(HLOCALOBJ hObject, float *r, float *g, float *b, float *a)
{
    printf("STUB ILTClient::GetObjectColor\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetObjectColor(HLOCALOBJ hObject, float r, float g, float b, float a)
{
    printf("STUB ILTClient::SetObjectColor\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetObjectShadowLOD(HLOCALOBJ hObject, EEngineLOD& eShadowLOD)
{
    printf("STUB ILTClient::GetObjectShadowLOD\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetObjectShadowLOD(HLOCALOBJ hObject, EEngineLOD eShadowLOD)
{
    printf("STUB ILTClient::SetObjectShadowLOD\n");
    return StubDefault<LTRESULT>::Get();
}

HMODELANIM CFearClient::GetAnimIndex(HOBJECT hObj, const char *pAnimName)
{
    printf("STUB ILTClient::GetAnimIndex\n");
    return StubDefault<HMODELANIM>::Get();
}

HMODELANIM CFearClient::GetModelAnimation(HOBJECT hObj)
{
    printf("STUB ILTClient::GetModelAnimation\n");
    return StubDefault<HMODELANIM>::Get();
}

void CFearClient::SetModelLooping(HOBJECT hObj, bool bLoop)
{
    printf("STUB ILTClient::SetModelLooping\n");
}

bool CFearClient::GetModelLooping(HOBJECT hObj)
{
    printf("STUB ILTClient::GetModelLooping\n");
    return StubDefault<bool>::Get();
}

uint32 CFearClient::GetModelPlaybackState(HOBJECT hObj)
{
    printf("STUB ILTClient::GetModelPlaybackState\n");
    return StubDefault<uint32>::Get();
}

LTRESULT CFearClient::SetLightType(HOBJECT hLight, EEngineLightType eLightType)
{
    printf("STUB ILTClient::SetLightType\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetLightType(HOBJECT hLight, EEngineLightType& eLightType)
{
    printf("STUB ILTClient::GetLightType\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetLightRadius(HOBJECT hLight, float fRadius)
{
    printf("STUB ILTClient::SetLightRadius\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetLightRadius(HOBJECT hLight, float& fRadius)
{
    printf("STUB ILTClient::GetLightRadius\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::RegisterConsoleProgram(const char *pName, ConsoleProgramFn fn)
{
    printf("STUB ILTClient::RegisterConsoleProgram\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::UnregisterConsoleProgram(const char *pName)
{
    printf("STUB ILTClient::UnregisterConsoleProgram\n");
    return StubDefault<LTRESULT>::Get();
}

bool CFearClient::GetContainerCode(HOBJECT hObj, uint16 *pCode)
{
    printf("STUB ILTClient::GetContainerCode\n");
    return StubDefault<bool>::Get();
}

uint32 CFearClient::GetObjectContainers(HOBJECT hObj, HOBJECT *pContainerList, uint32 maxListSize)
{
    printf("STUB ILTClient::GetObjectContainers\n");
    return StubDefault<uint32>::Get();
}

uint32 CFearClient::GetContainedObjects(HOBJECT hContainer, HOBJECT *pObjectList, uint32 maxListSize)
{
    printf("STUB ILTClient::GetContainedObjects\n");
    return StubDefault<uint32>::Get();
}

LTRESULT CFearClient::OpenSocket(uint32 *phSocket)
{
    printf("STUB ILTClient::OpenSocket\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetLightIntensityScale(HOBJECT hLight, float fIntensity)
{
    printf("STUB ILTClient::SetLightIntensityScale\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetLightIntensityScale(HOBJECT hLight, float& fIntensity)
{
    printf("STUB ILTClient::GetLightIntensityScale\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetLightTexture(HOBJECT hLight, const char* pszTexture)
{
    printf("STUB ILTClient::SetLightTexture\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetLightTexture(HOBJECT hLight, char* pszTexture, uint32 nBufferLen)
{
    printf("STUB ILTClient::GetLightTexture\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetLightAttenuationTexture(HOBJECT hLight, const char* pszTexture)
{
    printf("STUB ILTClient::SetLightAttenuationTexture\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetLightAttenuationTexture(HOBJECT hLight, char* pszTexture, uint32 nBufferLen)
{
    printf("STUB ILTClient::GetLightAttenuationTexture\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetLightTranslucentColor(HOBJECT hLight, const LTVector& vTransColor)
{
    printf("STUB ILTClient::SetLightTranslucentColor\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::DetachObjectStanding(HLOCALOBJ hObject)
{
    printf("STUB ILTClient::DetachObjectStanding\n");
    return StubDefault<LTRESULT>::Get();
}

bool CFearClient::IntersectSegment(const IntersectQuery& Query, IntersectInfo *pInfo)
{
    printf("STUB ILTClient::IntersectSegment\n");
    return StubDefault<bool>::Get();
}

bool CFearClient::IntersectSegmentAgainst(const IntersectQuery& Query, IntersectInfo *pInfo, HOBJECT hObject)
{
    printf("STUB ILTClient::IntersectSegmentAgainst\n");
    return StubDefault<bool>::Get();
}

LTRESULT CFearClient::GetLightTranslucentColor(HOBJECT hLight, LTVector& vTransColor)
{
    printf("STUB ILTClient::GetLightTranslucentColor\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetLightSpecularColor(HOBJECT hLight, const LTVector& vSpecularColor)
{
    printf("STUB ILTClient::SetLightSpecularColor\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetLightSpecularColor(HOBJECT hLight, LTVector& vSpecularColor)
{
    printf("STUB ILTClient::GetLightSpecularColor\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetLightDirectionalDims(HOBJECT hLight, const LTVector& vDims)
{
    printf("STUB ILTClient::SetLightDirectionalDims\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetLightDirectionalDims(HOBJECT hLight, LTVector& vDims)
{
    printf("STUB ILTClient::GetLightDirectionalDims\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetLightSpotInfo(HOBJECT hLight, float fFovX, float fFovY, float fNearClip)
{
    printf("STUB ILTClient::SetLightSpotInfo\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetLightSpotInfo(HOBJECT hLight, float& fFovX, float& fFovY, float& fNearClip)
{
    printf("STUB ILTClient::GetLightSpotInfo\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetLightDetailSettings(HOBJECT hLight, EEngineLOD eLightEnable, EEngineLOD eWorldShadows, EEngineLOD eObjectShadows)
{
    printf("STUB ILTClient::SetLightDetailSettings\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetLightDetailSettings(HOBJECT hLight, EEngineLOD& eLightEnable, EEngineLOD& eWorldShadows, EEngineLOD& eObjectShadows)
{
    printf("STUB ILTClient::GetLightDetailSettings\n");
    return StubDefault<LTRESULT>::Get();
}

uint32 CFearClient::GetPointContainers(const LTVector *pPoint, HOBJECT *pList, uint32 maxListSize)
{
    printf("STUB ILTClient::GetPointContainers\n");
    return StubDefault<uint32>::Get();
}

void CFearClient::CPrintNoArgs(const char *pMsg)
{
    if (pMsg)
        printf("%s", pMsg);
}

void CFearClient::CPrint(const char *pMsg, ...)
{
    if (!pMsg)
        return;
    va_list args;
    va_start(args, pMsg);
    vprintf(pMsg, args);
    va_end(args);
}

struct FearConsoleVar
{
    char m_Name[64];
    float m_Value;
    char m_String[128];
};

static FearConsoleVar g_FearConsoleVars[256];
static int g_nFearConsoleVars = 0;

static FearConsoleVar *FearFindConsoleVar(const char *pszName, bool bCreate)
{
    if (!pszName || !pszName[0])
        return NULL;

    for (int i = 0; i < g_nFearConsoleVars; i++)
    {
        if (_stricmp(g_FearConsoleVars[i].m_Name, pszName) == 0)
            return &g_FearConsoleVars[i];
    }

    if (!bCreate || g_nFearConsoleVars >= 256)
        return NULL;

    FearConsoleVar *pVar = &g_FearConsoleVars[g_nFearConsoleVars++];
    memset(pVar, 0, sizeof(*pVar));
    strncpy(pVar->m_Name, pszName, sizeof(pVar->m_Name) - 1);
    if (_stricmp(pszName, "StreamResources") == 0 || _stricmp(pszName, "LoadUnprefetchedResources") == 0)
        pVar->m_Value = 1.0f;
    return pVar;
}

static bool g_bFearConsoleVarsInit = false;

static void FearEnsureConsoleVars()
{
    if (g_bFearConsoleVarsInit)
        return;
    g_bFearConsoleVarsInit = true;

    FearConsoleVar *pVar = FearFindConsoleVar("StreamResources", true);
    if (pVar)
        pVar->m_Value = 1.0f;
    pVar = FearFindConsoleVar("LoadUnprefetchedResources", true);
    if (pVar)
        pVar->m_Value = 1.0f;
    pVar = FearFindConsoleVar("skiptitle", true);
    if (pVar)
    {
        pVar->m_Value = 1.0f;
        strncpy(pVar->m_String, "1", sizeof(pVar->m_String) - 1);
    }
}

HCONSOLEVAR CFearClient::GetConsoleVariable(const char* pszVarName)
{
    FearEnsureConsoleVars();
    if (!pszVarName)
        return NULL;
    if (_stricmp(pszVarName, "host") == 0 || _stricmp(pszVarName, "runworld") == 0)
        return NULL;
    return (HCONSOLEVAR)FearFindConsoleVar(pszVarName, false);
}

LTRESULT CFearClient::RunConsoleCommand(const char* pszCommand)
{
    printf("STUB ILTClient::RunConsoleCommand\n");
    return StubDefault<LTRESULT>::Get();
}

float CFearClient::GetConsoleVariableFloat(HCONSOLEVAR hVar)
{
    FearConsoleVar *pVar = (FearConsoleVar *)hVar;
    return pVar ? pVar->m_Value : 0.0f;
}

LTRESULT CFearClient::SetConsoleVariableFloat(const char* pszVarName, float fValue)
{
    FearEnsureConsoleVars();
    FearConsoleVar *pVar = FearFindConsoleVar(pszVarName, true);
    if (pVar)
        pVar->m_Value = fValue;
    return LT_OK;
}

const char* CFearClient::GetConsoleVariableString(HCONSOLEVAR hVar)
{
    FearConsoleVar *pVar = (FearConsoleVar *)hVar;
    return pVar ? pVar->m_String : "";
}

LTRESULT CFearClient::SetConsoleVariableString(const char* pszVarName, const char* pszValue)
{
    FearEnsureConsoleVars();
    FearConsoleVar *pVar = FearFindConsoleVar(pszVarName, true);
    if (pVar && pszValue)
    {
        strncpy(pVar->m_String, pszValue, sizeof(pVar->m_String) - 1);
        pVar->m_String[sizeof(pVar->m_String) - 1] = '\0';
    }
    return LT_OK;
}

LTDOUBLE CFearClient::GetTime()
{
    return FearTimer_GetSeconds();
}

uint32 CFearClient::GetTimeMS()
{
    printf("STUB ILTClient::GetTimeMS\n");
    return StubDefault<uint32>::Get();
}

LTRESULT CFearClient::GetObjectUnguaranteedData(HLOCALOBJ hObj, uint32 *pData)
{
    printf("STUB ILTClient::GetObjectUnguaranteedData\n");
    return StubDefault<LTRESULT>::Get();
}

LTFLOAT CFearClient::GetFrameTime()
{
    printf("STUB ILTClient::GetFrameTime\n");
    return StubDefault<LTFLOAT>::Get();
}

LTRESULT CFearClient::GetSourceWorldOffset(LTVector& vVector)
{
    vVector.Init();
    return LT_OK;
}

LTRESULT CFearClient::RemoveObject(HOBJECT hObj)
{
    printf("STUB ILTClient::RemoveObject\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetSectorID(const char *pName, uint32 *pResult) const
{
    printf("STUB ILTClient::GetSectorID\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::LinkObjRef(HOBJECT hObj, LTObjRef *pRef)
{
    printf("STUB ILTClient::LinkObjRef\n");
    if (!hObj || !pRef)
        return LT_INVALIDPARAMS;
    return LT_OK;
}

LTRESULT CFearClient::GetBlindObjectData(uint32 nNum, uint32 nId, uint8*& pData, uint32& nSize)
{
    printf("STUB ILTClient::GetBlindObjectData\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetSkyCameraTransform(LTRigidTransform& tSkyCamera)
{
    printf("STUB ILTClient::GetSkyCameraTransform\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::OpenStreamFromBuffer(uint8* pData, const uint32 nDataSize, bool bCopyData, ILTInStream** ppStream)
{
    printf("STUB ILTClient::OpenStreamFromBuffer\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetCompressedBufferMaxSize(const uint32 nSourceBufferSize, uint32& nCompressedBufferMaxSize)
{
    printf("STUB ILTClient::GetCompressedBufferMaxSize\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::CompressBuffer(const uint8* pSourceData, const uint32 nDataLen, const uint8* pCompressedBuffer, const uint32 nCompressedBufferSize, uint32& nCompressedSize)
{
    printf("STUB ILTClient::CompressBuffer\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::DecompressBuffer(const uint8* pSourceData, const uint32 nDataLen, const uint8* pDecompressedBuffer, const uint32 nDeompressedBufferSize, uint32& nDecompressedSize)
{
    printf("STUB ILTClient::DecompressBuffer\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetWorldCRC(uint32& nWorldCRC)
{
    printf("STUB ILTClient::GetWorldCRC\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetSoundOffset(HLTSOUND hSound, uint32 * dwOffset, uint32 * dwSize)
{
    printf("STUB ILTClient::GetSoundOffset\n");
    return StubDefault<LTRESULT>::Get();
}

ILTVideoTexture * CFearClient::GetVideoTexture()
{
    printf("STUB ILTClient::GetVideoTexture\n");
    return StubDefault<ILTVideoTexture *>::Get();
}

ILTTextureMgr * CFearClient::GetTextureMgr()
{
    return &g_FearTextureMgr;
}

ILTRenderer * CFearClient::GetRenderer()
{
    return &g_FearRenderer;
}

ILTCustomRender * CFearClient::GetCustomRender()
{
    return NULL;
}

ILTTextureString * CFearClient::GetTextureString()
{
    return &g_FearTextureString;
}

ILTGraphMgr* CFearClient::GetGraphMgr()
{
    printf("STUB ILTClient::GetGraphMgr\n");
    return StubDefault<ILTGraphMgr*>::Get();
}

LTRESULT CFearClient::FindObjectsInBox(const LTVector& vCenter, const LTVector& vHalfDims, TQueryRegionObjectsCB pfnQueryCB, void* pUserData)
{
    printf("STUB ILTClient::FindObjectsInBox\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::FindPolysInSphere(HOBJECT hWorldModel, const LTVector& vSphereCenter, float fSphereRadius, uint32 nNumClipPlanes, const LTPlane* pClipPlanes, TQueryRegionPolysCB pfnPolyCB, void* pCBUser)
{
    printf("STUB ILTClient::FindPolysInSphere\n");
    return StubDefault<LTRESULT>::Get();
}

ILTDrawPrim * CFearClient::GetDrawPrim()
{
    return &g_FearDrawPrim;
}

ILTCursor * CFearClient::Cursor()
{
    return &g_FearCursor;
}

LTRESULT CFearClient::RenderConsoleToRenderTarget()
{
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetServerIPAddress(uint8 pAddr[4], uint16 *pPort)
{
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::ProcessAttachments(HOBJECT hObj)
{
    printf("STUB ILTClient::ProcessAttachments\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetSectorVisibility(uint32 nID, bool *pSectorEnabled, bool *pPortalsEnabled)
{
    printf("STUB ILTClient::GetSectorVisibility\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetSectorVisibility(uint32 nID, bool bPortalsEnabled)
{
    printf("STUB ILTClient::SetSectorVisibility\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::EnableRegionSectors(HSTREAMINGREGION hRegion, bool bEnable)
{
    printf("STUB ILTClient::EnableRegionSectors\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetUDPSocket(SOCKET& socket)
{
    printf("STUB ILTClient::GetUDPSocket\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetSConValueFloat(const char *pName, float &val)
{
    printf("STUB ILTClient::GetSConValueFloat\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetSConValueString(const char *pName, char *valBuf, uint32 bufLen)
{
    printf("STUB ILTClient::GetSConValueString\n");
    return StubDefault<LTRESULT>::Get();
}

LTDOUBLE CFearClient::GetServerRealTime()
{
    return FearTimer_GetSeconds();
}

LTRESULT CFearClient::SendToServer(ILTMessage_Read *pMsg, uint32 flags)
{
    if (!pMsg || !FearServerHost_IsRunning())
        return LT_ERROR;

    uint32 nSaved = pMsg->Tell();
    unsigned char nMessageID = (unsigned char)pMsg->ReadBits(8);
    pMsg->SeekTo(nSaved);
    printf("FEARPORT: client message id=%u", (unsigned)nMessageID);
    pMsg->SeekTo(nSaved);
    pMsg->ReadBits(8);
    if (nMessageID == 160)
        printf(" pause=%u", (unsigned)pMsg->ReadBits(1));
    else if (nMessageID == 245)
        printf(" state=%u", (unsigned)pMsg->ReadBits(8));
    printf("\n");
    fflush(stdout);
    pMsg->SeekTo(nSaved);

    ILTMessage_Read *pCopy = pMsg->Clone();
    if (!pCopy)
        return LT_ERROR;

    FearServerHost_SubmitClientMessage(pCopy);
    return LT_OK;
}

LTRESULT CFearClient::StartQuery(const char *pInfo)
{
    printf("STUB ILTClient::StartQuery\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::UpdateQuery()
{
    printf("STUB ILTClient::UpdateQuery\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetQueryResults(NetSession* &pListHead)
{
    printf("STUB ILTClient::GetQueryResults\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::EndQuery()
{
    printf("STUB ILTClient::EndQuery\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::IsLocalToServer(bool *bLocal)
{
    if (bLocal)
        *bLocal = FearServerHost_IsRunning();
    return (bLocal != NULL) ? LT_OK : LT_INVALIDPARAMS;
}

LTRESULT CFearClient::GetHostUDPSocket(SOCKET& socket)
{
    printf("STUB ILTClient::GetHostUDPSocket\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetEngineHook(char *pName, void **pData)
{
    if (!pName || !pData)
        return LT_INVALIDPARAMS;
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (!pApi || !pApi->GetEngineHook)
        return LT_ERROR;
    *pData = pApi->GetEngineHook(pName);
    return (*pData != NULL) ? LT_OK : LT_ERROR;
}

LTRESULT CFearClient::QueryPerformanceStats(SPerformanceStats *pStats, bool bQuick)
{
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::GetObjectGameData(HOBJECT hObj, void **pData)
{
    printf("STUB ILTClient::GetObjectGameData\n");
    return StubDefault<LTRESULT>::Get();
}

LTRESULT CFearClient::SetObjectGameData(HOBJECT hObj, void *pData)
{
    printf("STUB ILTClient::SetObjectGameData\n");
    return StubDefault<LTRESULT>::Get();
}

bool CFearClient::IsMultiplayerExe()
{
    return StubDefault<bool>::Get();
}
