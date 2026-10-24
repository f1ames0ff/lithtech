#include "stdafx.h"
#include "GameClientShell.h"
#include "InterfaceMgr.h"
#include "MissionMgr.h"
#include "ClientConnectionMgr.h"
#include "PlayerMgr.h"
#include "PlayerBodyMgr.h"
#include "PlayerCamera.h"
#include "SFXMgr.h"
#include "CharacterFX.h"
#include "SharedFXStructs.h"
#include "MsgIDs.h"
#include "AutoMessage.h"
#include "ModelsDB.h"
#include "NetDefs.h"
#include "CMoveMgr.h"
#include "BindMgr.h"
#include "CommandIDs.h"
#include "DamageFXMgr.h"
#include "LadderMgr.h"
#include "SpecialMoveMgr.h"
#include "VehicleMgr.h"
#include "ClientWeaponMgr.h"
#include "ClientWeapon.h"
#include "PlayerStats.h"
#include "HUDMgr.h"

extern void UnhookWindow();

extern "C" __declspec(dllexport) void FearClient_DisableWindowHook()
{
    UnhookWindow();
}

extern "C" __declspec(dllexport) int FearClient_StartNewGame()
{
    if (!g_pMissionMgr)
        return 0;
    return g_pMissionMgr->StartGameNew() ? 1 : 0;
}

extern "C" __declspec(dllexport) void *FearClient_GetLocalPlayerBody()
{
    return CPlayerBodyMgr::Instance().GetObject();
}

extern "C" __declspec(dllexport) void FearClient_PreLoadWorld(const char *pszWorldName)
{
    if (g_pMissionMgr && pszWorldName)
        g_pMissionMgr->PreLoadWorld(pszWorldName);
}

extern "C" __declspec(dllexport) void FearClient_OnEnterWorld()
{
    if (g_pGameClientShell)
        g_pGameClientShell->OnEnterWorld();

    if (g_pMissionMgr && getenv("FEAR_LOCAL_CLIENT"))
    {
        CAutoMessage cMsg;
        cMsg.Writeuint8(EServerGameState_Playing);
        CLTMsgRef_Read cRead = cMsg.Read();
        if (cRead)
        {
            printf("FEARPORT: local client server-state Playing\n");
            fflush(stdout);
            g_pMissionMgr->OnMessage(MID_SERVERGAMESTATE, *cRead);
        }
    }
}

extern "C" __declspec(dllexport) void FearClient_EnterWorldPreview()
{
    FearClient_OnEnterWorld();
    if (g_pInterfaceMgr)
    {
        g_pInterfaceMgr->HideLoadScreen();
        g_pInterfaceMgr->ChangeState(GS_PLAYING);
    }
}

extern "C" __declspec(dllexport) int FearClient_IsPostload()
{
    return (g_pInterfaceMgr && g_pInterfaceMgr->GetCurrentScreen() == SCREEN_ID_POSTLOAD) ? 1 : 0;
}

extern "C" __declspec(dllexport) void FearClient_PressKey(int key)
{
    if (!g_pInterfaceMgr)
        return;
    g_pInterfaceMgr->OnKeyDown(key, 1);
    g_pInterfaceMgr->OnKeyUp(key);
}

extern "C" __declspec(dllexport) void FearClient_ReplySaveFailed()
{
    if (!g_pLTClient)
        return;
    CAutoMessage cMsg;
    cMsg.Writeuint8(MID_SAVE_DATA);
    cMsg.Writebool(true);
    g_pLTClient->SendToServer(cMsg.Read(), MESSAGE_GUARANTEED);
}

extern "C" __declspec(dllexport) int FearClient_IsPlaying()
{
    return (g_pInterfaceMgr && g_pInterfaceMgr->GetGameState() == GS_PLAYING) ? 1 : 0;
}

extern "C" __declspec(dllexport) int FearClient_GetGameState()
{
    return g_pInterfaceMgr ? (int)g_pInterfaceMgr->GetGameState() : -1;
}

extern "C" __declspec(dllexport) void FearClient_CloseUserMenu()
{
    if (g_pInterfaceMgr)
        g_pInterfaceMgr->GetUserMenuMgr().Close();
}

extern "C" __declspec(dllexport) void *FearClient_GetMoveObject()
{
    if (!g_pPlayerMgr)
        return NULL;

    CMoveMgr *pMoveMgr = g_pPlayerMgr->GetMoveMgr();
    return pMoveMgr ? (void *)pMoveMgr->GetObject() : NULL;
}

extern "C" __declspec(dllexport) int FearClient_IsPlayerMovementAllowed()
{
    if (!g_pPlayerMgr || !g_pInterfaceMgr)
        return 0;
    return g_pPlayerMgr->IsPlayerMovementAllowed() &&
        g_pInterfaceMgr->AllowCameraMovement() ? 1 : 0;
}

extern "C" __declspec(dllexport) int FearClient_GetMovementState(
    float *pPosition, float *pVelocity, float *pRotation)
{
    if (!g_pPlayerMgr || !g_pLTClient || !pPosition || !pVelocity || !pRotation)
        return 0;

    CMoveMgr *pMoveMgr = g_pPlayerMgr->GetMoveMgr();
    if (!pMoveMgr)
        return 0;

    HOBJECT hObject = pMoveMgr->GetObject();
    if (!hObject)
        return 0;

    LTVector vPosition;
    LTRotation rRotation;
    if (g_pLTClient->GetObjectPos(hObject, &vPosition) != LT_OK ||
        g_pLTClient->GetObjectRotation(hObject, &rRotation) != LT_OK)
        return 0;

    LTVector vVelocity = pMoveMgr->GetVelocity();
    pPosition[0] = vPosition.x;
    pPosition[1] = vPosition.y;
    pPosition[2] = vPosition.z;
    pVelocity[0] = vVelocity.x;
    pVelocity[1] = vVelocity.y;
    pVelocity[2] = vVelocity.z;
    for (int i = 0; i < 4; ++i)
        pRotation[i] = rRotation.m_Quat[i];
    return 1;
}

extern "C" __declspec(dllexport) void FearClient_ApplyServerCamera(
    void *pServerObject,
    unsigned int nFlags,
    unsigned int nFlags2,
    unsigned int nUserFlags,
    const float *pPosition,
    const float *pRotation,
    const unsigned char *pSFXData,
    unsigned int nSFXBits,
    int bNotify)
{
    HOBJECT hObject = (HOBJECT)pServerObject;
    if (!hObject || !g_pLTClient || !g_pCommonLT)
        return;

    if (pPosition)
    {
        LTVector vPosition(pPosition[0], pPosition[1], pPosition[2]);
        g_pLTClient->SetObjectPos(hObject, vPosition);
    }
    if (pRotation)
    {
        LTRotation rRotation;
        rRotation.m_Quat[0] = pRotation[0];
        rRotation.m_Quat[1] = pRotation[1];
        rRotation.m_Quat[2] = pRotation[2];
        rRotation.m_Quat[3] = pRotation[3];
        g_pLTClient->SetObjectRotation(hObject, rRotation);
    }

    g_pCommonLT->SetObjectFlags(hObject, OFT_Flags, nFlags, 0xFFFFFFFFu);
    g_pCommonLT->SetObjectFlags(hObject, OFT_Flags2, nFlags2, 0xFFFFFFFFu);
    g_pCommonLT->SetObjectFlags(hObject, OFT_User, nUserFlags, 0xFFFFFFFFu);

    if (bNotify && pSFXData && nSFXBits && g_pGameClientShell)
    {
        CAutoMessage cMsg;
        cMsg.WriteData(pSFXData, nSFXBits);
        g_pGameClientShell->SpecialEffectNotify(hObject, cMsg.Read());

        CSFXMgr *pSFXMgr = g_pGameClientShell->GetSFXMgr();
        if (pSFXMgr && pSFXData[0] == SFX_CHARACTER_ID)
        {
            CCharacterFX *pCharacterFX = pSFXMgr->GetCharacterFX(hObject);
            printf("FEARPORT: authoritative character sfx object=%p fx=%p bits=%u\n",
                (void *)hObject, (void *)pCharacterFX, nSFXBits);
            fflush(stdout);
        }
    }
}

extern "C" __declspec(dllexport) int FearClient_PreparePortPlayer(void *hTarget)
{
    if (!g_pPlayerMgr || !hTarget)
        return 0;

    CPlayerCamera *pCamera = g_pPlayerMgr->GetPlayerCamera();
    if (!pCamera || !pCamera->GetCamera())
        return 0;

    pCamera->SetCameraMode(CPlayerCamera::kCM_FirstPerson);
    pCamera->SetTargetObject((HOBJECT)hTarget);
    pCamera->Update();
    return 1;
}

static HOBJECT g_hLocalCharacter = NULL;

static int FearClientCreateCharacterFX(HOBJECT hServerObj, bool bInitializeLocalPlayer)
{
    if (!g_pGameClientShell || !hServerObj)
        return 0;

    CSFXMgr *pSFXMgr = g_pGameClientShell->GetSFXMgr();
    if (!pSFXMgr)
        return 0;

    if (pSFXMgr->GetCharacterFX(hServerObj))
        return 1;

    CHARCREATESTRUCT charCreate;
    charCreate.hServerObj = hServerObj;
    charCreate.bIsPlayer = true;
    charCreate.bUseDefaultHitboxDims = true;
    charCreate.nTimeScaleNumerator = 1;
    charCreate.nTimeScaleDenominator = 1;

    charCreate.hModel = g_pModelsDB ? g_pModelsDB->GetModelByRecordName(DEFAULT_PLAYERNAME) : NULL;

    printf("FEARPORT: character modelsdb=%p model=%p local=%d\n",
        (void *)g_pModelsDB, (void *)charCreate.hModel, bInitializeLocalPlayer ? 1 : 0);
    fflush(stdout);

    if (!pSFXMgr->CreateSFX(SFX_CHARACTER_ID, &charCreate))
    {
        printf("FEARPORT: character fx creation failed for %p\n", (void *)hServerObj);
        fflush(stdout);
        return 0;
    }

    CCharacterFX *pCharacterFX = pSFXMgr->GetCharacterFX(hServerObj);
    if (pCharacterFX && bInitializeLocalPlayer && g_pPlayerMgr)
        g_pPlayerMgr->InitLocalPlayer(*pCharacterFX);

    printf("FEARPORT: character fx created for %p initlocal=%d result=%d\n",
        (void *)hServerObj, bInitializeLocalPlayer ? 1 : 0, pCharacterFX ? 1 : 0);
    fflush(stdout);
    if (bInitializeLocalPlayer)
        g_hLocalCharacter = hServerObj;
    return 1;
}

extern "C" __declspec(dllexport) int FearClient_CreateLocalCharacter(void *hObject)
{
    return FearClientCreateCharacterFX((HOBJECT)hObject, true);
}

extern "C" __declspec(dllexport) int FearClient_EnsureDialogueCharacter(void *hObject)
{
    return FearClientCreateCharacterFX((HOBJECT)hObject, false);
}

extern "C" __declspec(dllexport) void FearClient_ResetObjectFX(void *hObject)
{
    if (!g_pGameClientShell || !hObject)
        return;

    CSFXMgr *pSFXMgr = g_pGameClientShell->GetSFXMgr();
    if (pSFXMgr)
        pSFXMgr->RemoveSpecialFX((HOBJECT)hObject);
}

extern "C" __declspec(dllexport) void FearClient_RenderPortCamera()
{
    if (!g_pPlayerMgr || !g_pLTClient)
        return;

    CPlayerCamera *pCamera = g_pPlayerMgr->GetPlayerCamera();
    if (!pCamera || !pCamera->GetCamera())
        return;

    ILTRenderer *pRenderer = g_pLTClient->GetRenderer();
    if (!pRenderer || pRenderer->Start3D() != LT_OK)
        return;

    pCamera->Update();
    pCamera->Render();
    if (g_pInterfaceMgr && g_pInterfaceMgr->IsInGame())
        g_pInterfaceMgr->Draw();

    pRenderer->End3D();
    pRenderer->FlipScreen();

    static int s_nPortFrame = 0;
    ++s_nPortFrame;
    const char *pszCapturePath = getenv("FEAR_CAPTURE_PATH");
    if (pszCapturePath && pszCapturePath[0])
    {
        static int s_nCaptureFrame = -1;
        if (s_nCaptureFrame < 0)
        {
            const char *pszFrame = getenv("FEAR_CAPTURE_FRAME");
            s_nCaptureFrame = pszFrame && pszFrame[0] ? atoi(pszFrame) : 5;
        }

        const char *pszEvery = getenv("FEAR_CAPTURE_EVERY");
        const int nEvery = pszEvery && pszEvery[0] ? atoi(pszEvery) : 0;
        const char *pszLimit = getenv("FEAR_CAPTURE_LIMIT");
        static int s_nCaptureCount = 0;
        const int nLimit = pszLimit && pszLimit[0] ? atoi(pszLimit) : 0;
        if (nEvery > 0)
        {
            if ((nLimit <= 0 || s_nCaptureCount < nLimit) &&
                s_nPortFrame >= s_nCaptureFrame &&
                ((s_nPortFrame - s_nCaptureFrame) % nEvery) == 0)
            {
                char szSeq[512];
                _snprintf(szSeq, sizeof(szSeq), "%s_%04d.bmp", pszCapturePath, s_nPortFrame);
                szSeq[sizeof(szSeq) - 1] = 0;
                printf("FEARPORT: capture frame=%d file='%s'\n", s_nPortFrame, szSeq);
                fflush(stdout);
                pRenderer->MakeScreenShot(szSeq);
                ++s_nCaptureCount;
            }
        }
        else if (s_nPortFrame == s_nCaptureFrame && s_nCaptureCount == 0)
        {
            printf("FEARPORT: capture frame=%d file='%s'\n", s_nPortFrame, pszCapturePath);
            fflush(stdout);
            pRenderer->MakeScreenShot(pszCapturePath);
            ++s_nCaptureCount;
        }
    }
}

extern "C" __declspec(dllexport) void FearClient_LogState()
{
    int nGameState = g_pInterfaceMgr ? (int)g_pInterfaceMgr->GetGameState() : -1;
    int nRecvState = g_pClientConnectionMgr ? (int)g_pClientConnectionMgr->GetRecvClientConnectionState() : -1;
    int nSentState = g_pClientConnectionMgr ? (int)g_pClientConnectionMgr->GetSentClientConnectionState() : -1;
    int nPlayerState = g_pPlayerMgr ? (int)g_pPlayerMgr->GetPlayerState() : -1;
    const char *pszWorld = g_pMissionMgr ? g_pMissionMgr->GetCurrentWorldName() : "";
    printf("FEARPORT: clientstate game=%d servergame=%d recv=%d sent=%d player=%d world='%s'\n",
        nGameState,
        g_pMissionMgr ? (int)g_pMissionMgr->GetServerGameState() : -1,
        nRecvState,
        nSentState,
        nPlayerState,
        pszWorld ? pszWorld : "");

    int nHudLevel = -1;
    int nHudVisible = -1;
    int nStoryMode = -1;
    if (g_pInterfaceMgr && g_pInterfaceMgr->GetHUDMgr())
    {
        nHudLevel = (int)g_pInterfaceMgr->GetHUDMgr()->GetRenderLevel();
        nHudVisible = g_pInterfaceMgr->GetHUDMgr()->IsShown() ? 1 : 0;
    }
    if (g_pPlayerMgr)
        nStoryMode = g_pPlayerMgr->InStoryMode() ? 1 : 0;

    static int s_nLastHudLevel = -2;
    static int s_nLastHudVisible = -2;
    static int s_nLastStoryMode = -2;
    if (nHudLevel != s_nLastHudLevel || nHudVisible != s_nLastHudVisible ||
        nStoryMode != s_nLastStoryMode)
    {
        s_nLastHudLevel = nHudLevel;
        s_nLastHudVisible = nHudVisible;
        s_nLastStoryMode = nStoryMode;
        printf("FEARPORT: hud level=%d visible=%d story=%d\n",
            nHudLevel, nHudVisible, nStoryMode);
    }

    if (g_pLTClient)
    {
        uint32 nRealTime = g_pGameClientShell ? g_pGameClientShell->GetServerRealTimeMS() : 0;
        int nInWorld = (g_pPlayerMgr && g_pPlayerMgr->GetMoveMgr() && g_pPlayerMgr->GetMoveMgr()->IsInWorld()) ? 1 : 0;
        int nMoveAllowed = (g_pPlayerMgr && g_pPlayerMgr->IsPlayerMovementAllowed()) ? 1 : 0;
        int nCamMode = (g_pPlayerMgr && g_pPlayerMgr->GetPlayerCamera()) ? (int)g_pPlayerMgr->GetPlayerCamera()->GetCameraMode() : -1;
        printf("FEARPORT: timing sec=%.3f real=%u inworld=%d moveallowed=%d cammode=%d\n",
            (double)g_pLTClient->GetTime(), nRealTime, nInWorld, nMoveAllowed, nCamMode);
    }

    if (g_pLTClient)
    {
        HOBJECT hCamera = NULL;
        if (g_pPlayerMgr && g_pPlayerMgr->GetPlayerCamera())
            hCamera = g_pPlayerMgr->GetPlayerCamera()->GetCamera();

        LTVector vCamPos(0.0f, 0.0f, 0.0f);
        LTRotation rCamRot;
        if (hCamera)
            g_pLTClient->GetObjectPos(hCamera, &vCamPos);
        if (hCamera)
            g_pLTClient->GetObjectRotation(hCamera, &rCamRot);

        uint32 nObjects = 0;
        HOBJECT hClientObj = g_hLocalCharacter;
        LTVector vPlayerPos(0.0f, 0.0f, 0.0f);
        if (hClientObj)
        {
            g_pLTClient->GetObjectPos(hClientObj, &vPlayerPos);
            ++nObjects;
        }

        LTVector vCamFwd = hCamera ? rCamRot.Forward() : LTVector(0.0f, 0.0f, 1.0f);
        printf("FEARPORT: cam obj=%p pos=%.2f %.2f %.2f fwd=%.3f %.3f %.3f char=%p cpos=%.2f %.2f %.2f\n",
            (void *)hCamera, vCamPos.x, vCamPos.y, vCamPos.z,
            vCamFwd.x, vCamFwd.y, vCamFwd.z,
            (void *)hClientObj, vPlayerPos.x, vPlayerPos.y, vPlayerPos.z);
        (void)nObjects;
    }

    fflush(stdout);
}

extern "C" __declspec(dllexport) void FearClient_RenderWorldPreview()
{
    if (!g_pLTClient)
        return;
    ILTRenderer *pRenderer = g_pLTClient->GetRenderer();
    if (!pRenderer)
        return;
    if (pRenderer->Start3D() != LT_OK)
        return;
    pRenderer->RenderCamera((HLOCALOBJ)1, NULL);
    pRenderer->End3D();
    pRenderer->FlipScreen();
}

extern "C" __declspec(dllexport) void FearClient_MouseMove(int x, int y)
{
    if (g_pInterfaceMgr)
        g_pInterfaceMgr->OnMouseMove(x, y);
}

extern "C" __declspec(dllexport) void FearClient_MouseButton(int nButton, int nDown, int x, int y)
{
    if (!g_pInterfaceMgr)
        return;

    if (g_pPlayerMgr && g_pInterfaceMgr->GetGameState() == GS_PLAYING &&
        (nButton == 0 || nButton == 1))
    {
        const int nCommand = (nButton == 0) ? COMMAND_ID_FIRING : COMMAND_ID_ALT_FIRING;
        bool bHandled = false;
        if (nDown)
            bHandled = g_pPlayerMgr->OnCommandOn(nCommand);
        else
            bHandled = g_pPlayerMgr->OnCommandOff(nCommand);
        printf("FEARPORT: mouse command=%d down=%d handled=%d flags=%08x canfire=%d state=%d damageallow=%d\n",
            nCommand,
            nDown ? 1 : 0,
            bHandled ? 1 : 0,
            g_pPlayerMgr->GetPlayerFlags(),
            g_pPlayerMgr->CanFireWeapon() ? 1 : 0,
            g_pClientWeaponMgr ? (int)g_pClientWeaponMgr->GetCurrentWeaponState() : -1,
            g_pDamageFXMgr && g_pDamageFXMgr->AllowWeapons() ? 1 : 0);
        fflush(stdout);
    }

    if (nButton == 0)
    {
        if (nDown)
            g_pInterfaceMgr->OnLButtonDown(x, y);
        else
            g_pInterfaceMgr->OnLButtonUp(x, y);
    }
    else if (nButton == 1)
    {
        if (nDown)
            g_pInterfaceMgr->OnRButtonDown(x, y);
        else
            g_pInterfaceMgr->OnRButtonUp(x, y);
    }
}

extern "C" __declspec(dllexport) void FearClient_MouseWheel(int x, int y, int nDelta)
{
    if (g_pInterfaceMgr)
        g_pInterfaceMgr->OnMouseWheel(x, y, nDelta);
}

// Device/object indices of the port's ILTInput implementation (see
// FEAR/Compat/v5host/fear_input_stub.cpp). The retail profile database is not
// available on disk in this port, so the default command bindings are built here.
static const uint32 kPortDeviceKeyboard = 0;
static const uint32 kPortDeviceMouse = 1;
static const uint32 kPortMouseAxisX = 0;
static const uint32 kPortMouseAxisY = 1;
static const uint32 kPortMouseButton0 = 2;
static const uint32 kPortMouseButton1 = 3;

int g_nFearMoveUpd = 0;
int g_nFearMoveUpdBail = 0;
int g_nFearControlFlags = 0;
int g_nFearNormalMotion = 0;
int g_nFearVelocityUpd = 0;
int g_nFearShellPlaying = 0;
int g_nFearPlayingEnter = 0;
int g_nFearPlayingPaused = 0;
int g_nFearShellUpd = 0;
int g_nFearShellIface = 0;

static void PortBindButton(uint32 nCommand, uint32 nDevice, uint32 nObject){
    CBindMgr::SBinding sBinding;
    sBinding.m_nCommand = nCommand;
    sBinding.m_nDevice = nDevice;
    sBinding.m_nObject = nObject;
    CBindMgr::GetSingleton().SetBinding(sBinding);
}

extern "C" __declspec(dllexport) int FearClient_InstallDefaultBindings()
{
    CBindMgr::TBindingList aForward;
    CBindMgr::GetSingleton().GetCommandBindings(COMMAND_ID_FORWARD, &aForward);

    CBindMgr::TBindingList aYaw;
    CBindMgr::GetSingleton().GetCommandBindings(COMMAND_ID_YAW, &aYaw);

    printf("FEARPORT: existing bindings forward=%u yaw=%u\n",
        (unsigned)aForward.size(), (unsigned)aYaw.size());
    fflush(stdout);

    PortBindButton(COMMAND_ID_FORWARD, kPortDeviceKeyboard, 'W');
    PortBindButton(COMMAND_ID_REVERSE, kPortDeviceKeyboard, 'S');
    PortBindButton(COMMAND_ID_STRAFE_LEFT, kPortDeviceKeyboard, 'A');
    PortBindButton(COMMAND_ID_STRAFE_RIGHT, kPortDeviceKeyboard, 'D');
    PortBindButton(COMMAND_ID_JUMP, kPortDeviceKeyboard, VK_SPACE);
    PortBindButton(COMMAND_ID_DUCK, kPortDeviceKeyboard, VK_LCONTROL);
    PortBindButton(COMMAND_ID_RUN, kPortDeviceKeyboard, VK_LSHIFT);
    PortBindButton(COMMAND_ID_STAND, kPortDeviceKeyboard, VK_LSHIFT);
    PortBindButton(COMMAND_ID_FIRING, kPortDeviceMouse, kPortMouseButton0);
    PortBindButton(COMMAND_ID_ALT_FIRING, kPortDeviceMouse, kPortMouseButton1);
    PortBindButton(COMMAND_ID_ACTIVATE, kPortDeviceKeyboard, 'E');
    PortBindButton(COMMAND_ID_RELOAD, kPortDeviceKeyboard, 'R');
    PortBindButton(COMMAND_ID_FLASHLIGHT, kPortDeviceKeyboard, 'F');
    PortBindButton(COMMAND_ID_TOOLS, kPortDeviceKeyboard, 'L');
    PortBindButton(COMMAND_ID_FOCUS, kPortDeviceKeyboard, 'Z');
    PortBindButton(COMMAND_ID_TOGGLEMELEE, kPortDeviceKeyboard, 'V');
    PortBindButton(COMMAND_ID_THROW_GRENADE, kPortDeviceKeyboard, 'G');
    PortBindButton(COMMAND_ID_MENU, kPortDeviceKeyboard, VK_ESCAPE);

    // Mouse axes: map the [0..1] device range onto [-1..1], matching ApplyMouse().
    CBindMgr::SBinding sAxis;
    sAxis.m_fDefaultValue = 0.0f;
    sAxis.m_fOffset = -1.0f;
    sAxis.m_fScale = 2.0f;
    sAxis.m_nDevice = kPortDeviceMouse;
    sAxis.m_nObject = kPortMouseAxisX;
    sAxis.m_nCommand = COMMAND_ID_YAW;
    CBindMgr::GetSingleton().SetBinding(sAxis);
    sAxis.m_nObject = kPortMouseAxisY;
    sAxis.m_nCommand = COMMAND_ID_PITCH;
    CBindMgr::GetSingleton().SetBinding(sAxis);

    return 1;
}



extern "C" __declspec(dllexport) void FearClient_LogInputState()
{
    CBindMgr &cBindMgr = CBindMgr::GetSingleton();
    CBindMgr::TBindingList aForward;
    cBindMgr.GetCommandBindings(COMMAND_ID_FORWARD, &aForward);

    printf("FEARPORT: gatestate cameramove=%d mp=%d allowinput=%d\n",
        (g_pInterfaceMgr && g_pInterfaceMgr->AllowCameraMovement()) ? 1 : 0,
        IsMultiplayerGameClient() ? 1 : 0,
        (g_pPlayerMgr && g_pPlayerMgr->IsPlayerMovementAllowed()) ? 1 : 0);
    printf("FEARPORT: input bindings=%u forward=%d yaw=%.3f pitch=%.3f wdown=%d\n",
        (unsigned)aForward.size(),
        cBindMgr.IsCommandOn(COMMAND_ID_FORWARD) ? 1 : 0,
        cBindMgr.GetExtremalCommandValue(COMMAND_ID_YAW),
        cBindMgr.GetExtremalCommandValue(COMMAND_ID_PITCH),
        (GetAsyncKeyState('W') & 0x8000) ? 1 : 0);

    CMoveMgr *pMoveMgr = g_pPlayerMgr ? g_pPlayerMgr->GetMoveMgr() : NULL;
    if (pMoveMgr)
    {
        LTVector vVel = pMoveMgr->GetVelocity();
        HOBJECT hMoveObj = pMoveMgr->GetObject();
        LTVector vPos(0.0f, 0.0f, 0.0f);
        LTVector vAccel(0.0f, 0.0f, 0.0f);
        if (hMoveObj && g_pLTClient)
        {
            g_pLTClient->GetObjectPos(hMoveObj, &vPos);
            g_pPhysicsLT->GetAcceleration(hMoveObj, &vAccel);
        }
        printf("FEARPORT: movemgr obj=%p inworld=%d onground=%d vel=%.2f %.2f %.2f pos=%.2f %.2f %.2f allowed=%d dead=%d\n",
            hMoveObj,
            pMoveMgr->IsInWorld() ? 1 : 0,
            pMoveMgr->IsOnGround() ? 1 : 0,
            vVel.x, vVel.y, vVel.z,
            vPos.x, vPos.y, vPos.z,
            g_pPlayerMgr->IsPlayerMovementAllowed() ? 1 : 0,
            g_pPlayerMgr->IsPlayerDead() ? 1 : 0);
        printf("FEARPORT: movegate allow=%d damage=%d flags=%08x accel=%.2f %.2f %.2f\n",
            pMoveMgr->GetAllowMovement() ? 1 : 0,
            g_pDamageFXMgr->AllowMovement() ? 1 : 0,
            pMoveMgr->GetControlFlags(),
            vAccel.x, vAccel.y, vAccel.z);
        printf("FEARPORT: movehold physics=%d node=%p ladder=%d special=%d turret=%d vehicle=%d cam=%d allowcam=%d spectator=%d\n",
            (int)pMoveMgr->GetVehicleMgr()->GetPhysicsModel(),
            (void *)g_pPlayerMgr->GetPlayerNodeGoto(),
            LadderMgr::Instance().IsClimbing() ? 1 : 0,
            SpecialMoveMgr::Instance().IsActive() ? 1 : 0,
            g_pPlayerMgr->IsOperatingTurret() ? 1 : 0,
            pMoveMgr->GetVehicleMgr()->IsVehiclePhysics() ? 1 : 0,
            g_pPlayerMgr->GetPlayerCamera() ? (int)g_pPlayerMgr->GetPlayerCamera()->GetCameraMode() : -1,
            g_pInterfaceMgr->AllowCameraMovement() ? 1 : 0,
            (int)g_pPlayerMgr->GetSpectatorMode());
    }
    else
    {
        printf("FEARPORT: movemgr null playermgr=%d\n", g_pPlayerMgr ? 1 : 0);
    }

    printf("FEARPORT: movepipe update=%d bail=%d flags=%d normal=%d vel=%d playing=%d pent=%d ppause=%d shellupd=%d iface=%d clientobj=%p paused=%d\n",
        g_nFearMoveUpd, g_nFearMoveUpdBail, g_nFearControlFlags, g_nFearNormalMotion, g_nFearVelocityUpd,
        g_nFearShellPlaying, g_nFearPlayingEnter, g_nFearPlayingPaused,
        g_nFearShellUpd, g_nFearShellIface,
        g_pLTClient ? g_pLTClient->GetClientObject() : NULL,
        (g_pGameClientShell && g_pGameClientShell->IsServerPaused()) ? 1 : 0);

    if (g_pPlayerMgr)
    {
        printf("FEARPORT: playerstate=%d inworld=%d gs=%d\n",
            (int)g_pPlayerMgr->GetPlayerState(),
            g_pPlayerMgr->IsPlayerInWorld() ? 1 : 0,
            g_pInterfaceMgr ? (int)g_pInterfaceMgr->GetGameState() : -1);
    }
    fflush(stdout);
}

extern "C" __declspec(dllexport) void FearClient_LogWeaponState()
{
    CClientWeapon *pWeapon =
        g_pClientWeaponMgr ? g_pClientWeaponMgr->GetCurrentClientWeapon() : NULL;
    HWEAPON hWeapon = pWeapon ? pWeapon->GetWeaponRecord() : NULL;
    HAMMO hAmmo = pWeapon ? pWeapon->GetAmmoRecord() : NULL;
    const char *pszWeapon =
        (g_pLTDatabase && hWeapon) ? g_pLTDatabase->GetRecordName(hWeapon) : "";
    const char *pszAmmo =
        (g_pLTDatabase && hAmmo) ? g_pLTDatabase->GetRecordName(hAmmo) : "";
    HWEAPON hStatsWeapon = g_pPlayerStats ? g_pPlayerStats->GetCurrentWeaponRecord() : NULL;
    HAMMO hStatsAmmo = g_pPlayerStats ? g_pPlayerStats->GetCurrentAmmoRecord() : NULL;
    const char *pszStatsWeapon =
        (g_pLTDatabase && hStatsWeapon) ? g_pLTDatabase->GetRecordName(hStatsWeapon) : "";
    const char *pszStatsAmmo =
        (g_pLTDatabase && hStatsAmmo) ? g_pLTDatabase->GetRecordName(hStatsAmmo) : "";
    HWEAPON hPistol = g_pWeaponDB ? g_pWeaponDB->GetWeaponRecord("Pistol") : NULL;
    const bool bHavePistol = g_pPlayerStats && hPistol && g_pPlayerStats->HaveWeapon(hPistol);
    const bool bCanPistol =
        hPistol && g_pClientWeaponMgr && g_pClientWeaponMgr->CanChangeToWeapon(hPistol);

    printf("FEARWEAPON: stats=%p mgr=%p current=%p weapon=%p name='%s' ammo=%p ammo_name='%s' clip=%d reserve=%u firing=%d\n",
        (void *)g_pPlayerStats,
        (void *)g_pClientWeaponMgr,
        (void *)pWeapon,
        (void *)hWeapon,
        pszWeapon ? pszWeapon : "",
        (void *)hAmmo,
        pszAmmo ? pszAmmo : "",
        pWeapon ? pWeapon->GetAmmoInClips() : -1,
        (g_pPlayerStats && hAmmo) ? g_pPlayerStats->GetAmmoCount(hAmmo) : 0,
        CBindMgr::GetSingleton().IsCommandOn(COMMAND_ID_FIRING) ? 1 : 0);
    if (g_pPlayerStats)
    {
        printf("FEARSTATS: health=%u/%u armor=%u/%u weapons=%u air=%.3f\n",
            g_pPlayerStats->GetHealth(),
            g_pPlayerStats->GetMaxHealth(),
            g_pPlayerStats->GetArmor(),
            g_pPlayerStats->GetMaxArmor(),
            g_pPlayerStats->GetNumHaveWeapons(),
            g_pPlayerStats->GetAirPercent());
        printf("FEARAMMO: current_weapon=%p name='%s' current_ammo=%p name='%s' count=%u\n",
            (void *)hStatsWeapon,
            pszStatsWeapon ? pszStatsWeapon : "",
            (void *)hStatsAmmo,
            pszStatsAmmo ? pszStatsAmmo : "",
            g_pPlayerStats->GetCurrentAmmoCount());
        printf("FEARPISTOL: record=%p have=%d can=%d special=%d enabled=%d visible=%d\n",
            (void *)hPistol,
            bHavePistol ? 1 : 0,
            bCanPistol ? 1 : 0,
            CPlayerBodyMgr::Instance().IsPlayingSpecial() ? 1 : 0,
            g_pClientWeaponMgr && g_pClientWeaponMgr->WeaponsEnabled() ? 1 : 0,
            g_pClientWeaponMgr && g_pClientWeaponMgr->WeaponsVisible() ? 1 : 0);
    }
    fflush(stdout);
}





