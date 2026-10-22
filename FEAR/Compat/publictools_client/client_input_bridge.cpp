#include "stdafx.h"
#include "GameClientShell.h"
#include "InterfaceMgr.h"
#include "MissionMgr.h"
#include "ClientConnectionMgr.h"
#include "PlayerMgr.h"
#include "PlayerCamera.h"
#include "MsgIDs.h"
#include "AutoMessage.h"

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

extern "C" __declspec(dllexport) void FearClient_PreLoadWorld(const char *pszWorldName)
{
    if (g_pMissionMgr && pszWorldName)
        g_pMissionMgr->PreLoadWorld(pszWorldName);
}

extern "C" __declspec(dllexport) void FearClient_OnEnterWorld()
{
    if (g_pGameClientShell)
        g_pGameClientShell->OnEnterWorld();
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

    static int s_nPortFrame = 0;
    ++s_nPortFrame;
    const char *pszCapturePath = getenv("FEAR_CAPTURE_PATH");
    if (pszCapturePath && pszCapturePath[0] && s_nPortFrame == 5)
        pRenderer->MakeScreenShot(pszCapturePath);

    pRenderer->End3D();
    pRenderer->FlipScreen();
}

extern "C" __declspec(dllexport) void FearClient_LogState()
{
    int nGameState = g_pInterfaceMgr ? (int)g_pInterfaceMgr->GetGameState() : -1;
    int nRecvState = g_pClientConnectionMgr ? (int)g_pClientConnectionMgr->GetRecvClientConnectionState() : -1;
    int nSentState = g_pClientConnectionMgr ? (int)g_pClientConnectionMgr->GetSentClientConnectionState() : -1;
    int nPlayerState = g_pPlayerMgr ? (int)g_pPlayerMgr->GetPlayerState() : -1;
    const char *pszWorld = g_pMissionMgr ? g_pMissionMgr->GetCurrentWorldName() : "";
    printf("FEARPORT: clientstate game=%d recv=%d sent=%d player=%d world='%s'\n",
        nGameState, nRecvState, nSentState, nPlayerState, pszWorld ? pszWorld : "");
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
