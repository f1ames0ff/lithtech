#include "stdafx.h"
#include "GameClientShell.h"
#include "InterfaceMgr.h"
#include "MissionMgr.h"

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

extern "C" __declspec(dllexport) void FearClient_EnterWorldPreview()
{
    if (g_pGameClientShell)
        g_pGameClientShell->OnEnterWorld();
    if (g_pInterfaceMgr)
    {
        g_pInterfaceMgr->HideLoadScreen();
        g_pInterfaceMgr->ChangeState(GS_PLAYING);
    }
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
