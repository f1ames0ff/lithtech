#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "fear_host_api.h"
#include "fear_render_api.h"
#include "ltbasedefs.h"
#include "ltassert.h"
#include "ltmodule.h"
#include "iclientshell.h"
#include "iltclient.h"
#include "fear_client_facade.h"
#include "fear_server_host.h"
#include "fear_resourcemgr_stub.h"
#include "fear_common_stub.h"
#include "fear_physics_stub.h"
#include "fear_model_stub.h"
#include "fear_timer_stub.h"
#include "fear_soundmgr_stub.h"
#include "fear_cursor_stub.h"
#include "fear_filemgr_stub.h"
#include "fear_contenttransfer_stub.h"
#include "fear_renderer_stub.h"
#include "fear_drawprim_stub.h"
#include "fear_texturemgr_stub.h"
#include "fear_texturestring_stub.h"
#include "fear_input_stub.h"
#include "fear_gameutil_stub.h"

int LTAssert::RunAssert(const char*, const char*, int, const char*, bool, const char*, ...)
{
    return 0;
}

ILTClient *g_pLTClient = NULL;
define_holder(ILTClient, g_pLTClient);
IClientShell *g_pShell = NULL;
define_holder(IClientShell, g_pShell);

define_interface(CFearClient, ILTClient);
define_interface(CFearResourceMgr, ILTResourceMgr);
define_interface(CFearCommon, ILTCommon);
define_interface(CFearPhysics, ILTPhysics);
define_interface(CFearModel, ILTModel);
define_interface(CFearTimer, ILTTimer);
define_interface(CFearSoundMgr, ILTSoundMgr);
define_interface(CFearCursor, ILTCursor);
define_interface(CFearRenderer, ILTRenderer);
define_interface(CFearDrawPrim, ILTDrawPrim);
define_interface(CFearTextureMgr, ILTTextureMgr);
define_interface(CFearTextureString, ILTTextureString);
define_interface(CFearInput, ILTInput);
define_interface(CFearGameUtil, ILTGameUtil);
define_interface(CFearClientContentTransfer, ILTClientContentTransfer);

static HMODULE g_hGameClient = NULL;
static const FearEngineApi *g_pEngineApi = NULL;

typedef void (*TFearClientMouseMove)(int, int);
typedef void (*TFearClientMouseButton)(int, int, int, int);
typedef void (*TFearClientMouseWheel)(int, int, int);
typedef void (*TFearClientDisableWindowHook)();
typedef void (*TFearClientEnterWorldPreview)();
typedef void (*TFearClientRenderWorldPreview)();
typedef int (*TFearClientStartNewGame)();

static TFearClientMouseMove g_pfnClientMouseMove = NULL;
static TFearClientMouseButton g_pfnClientMouseButton = NULL;
static TFearClientMouseWheel g_pfnClientMouseWheel = NULL;
static TFearClientDisableWindowHook g_pfnClientDisableWindowHook = NULL;
static TFearClientEnterWorldPreview g_pfnClientEnterWorldPreview = NULL;
static TFearClientRenderWorldPreview g_pfnClientRenderWorldPreview = NULL;
static TFearClientStartNewGame g_pfnClientStartNewGame = NULL;

static bool g_bWorldPreview = false;
static bool g_bPreviewKeys[256] = {false};
static float g_fPreviewPos[3] = { -1100.0f, -1372.0f, -370.0f };
static float g_fPreviewYaw = -0.0784588f;
static float g_fPreviewPitch = 0.0f;
static int g_nPreviewLastMouseX = 320;
static int g_nPreviewLastMouseY = 240;
static bool g_bPreviewMouseInitialized = false;
static DWORD g_nPreviewLastTick = 0;

static void FearHost_UpdatePreviewCamera()
{
    if (g_pEngineApi && g_pEngineApi->m_pRenderApi && g_pEngineApi->m_pRenderApi->SetWorldCamera)
        g_pEngineApi->m_pRenderApi->SetWorldCamera(g_fPreviewPos, g_fPreviewYaw, g_fPreviewPitch);
}

extern "C" void FearHost_DeliverServerMessage(ILTMessage_Read *pMessage)
{
    if (!g_pShell || !pMessage)
        return;
    if (!g_bWorldPreview)
    {
        g_bWorldPreview = true;
        g_bPreviewMouseInitialized = false;
        g_nPreviewLastTick = GetTickCount();
        if (g_pfnClientEnterWorldPreview)
            g_pfnClientEnterWorldPreview();
    }
    g_pShell->OnMessage(pMessage);
}

const FearEngineApi *FearHost_GetEngineApi(void)
{
    return g_pEngineApi;
}

extern "C" __declspec(dllexport) int FearHost_Init(const char *pszGameClientDll, const FearEngineApi *pApi)
{
    g_pEngineApi = pApi;
    CInterfaceDatabase *pMaster = GetMasterDatabase();
    g_hGameClient = LoadLibraryA(pszGameClientDll);
    if (!g_hGameClient)
        return 0;
    typedef void (*TSetMaster)(CInterfaceDatabase*);
    TSetMaster pfnSetMaster = (TSetMaster)GetProcAddress(g_hGameClient, "SetMasterDatabase");
    if (pfnSetMaster)
        pfnSetMaster(pMaster);
    g_pfnClientMouseMove = (TFearClientMouseMove)GetProcAddress(g_hGameClient, "FearClient_MouseMove");
    g_pfnClientMouseButton = (TFearClientMouseButton)GetProcAddress(g_hGameClient, "FearClient_MouseButton");
    g_pfnClientMouseWheel = (TFearClientMouseWheel)GetProcAddress(g_hGameClient, "FearClient_MouseWheel");
    g_pfnClientDisableWindowHook = (TFearClientDisableWindowHook)GetProcAddress(g_hGameClient, "FearClient_DisableWindowHook");
    g_pfnClientEnterWorldPreview = (TFearClientEnterWorldPreview)GetProcAddress(g_hGameClient, "FearClient_EnterWorldPreview");
    g_pfnClientRenderWorldPreview = (TFearClientRenderWorldPreview)GetProcAddress(g_hGameClient, "FearClient_RenderWorldPreview");
    g_pfnClientStartNewGame = (TFearClientStartNewGame)GetProcAddress(g_hGameClient, "FearClient_StartNewGame");
    FearServerHost_SetMessageSink(FearHost_DeliverServerMessage);
    char szRetailAbi[16] = {0};
    DWORD nRetailAbi = GetEnvironmentVariableA("FEAR_RETAIL_ABI", szRetailAbi, sizeof(szRetailAbi));
    char szToolsClient[16] = {0};
    DWORD nToolsClient = GetEnvironmentVariableA("FEAR_TOOLS_CLIENT", szToolsClient, sizeof(szToolsClient));
    if (nRetailAbi > 0 && szRetailAbi[0] != '0')
        FearPatchSoundVtable();
    else if (nToolsClient > 0 && szToolsClient[0] != '0')
        FearPatchSoundListenerOnly();
    return (g_pShell != NULL) ? 1 : 0;
}

extern "C" __declspec(dllexport) int FearHost_OnEngineInitialized(const FearMode *pMode, const unsigned char *pGuid)
{
    if (!g_pShell || !pMode)
        return 0;
    RMode mode;
    memset(&mode, 0, sizeof(mode));
    mode.m_Width = pMode->m_Width;
    mode.m_Height = pMode->m_Height;
    mode.m_BitDepth = pMode->m_BitDepth;
    mode.m_bHWTnL = (pMode->m_bHWTnL != 0);
    strncpy(mode.m_DeviceName, pMode->m_DeviceName, sizeof(mode.m_DeviceName) - 1);
    LTGUID guid;
    memset(&guid, 0, sizeof(guid));
    if (pGuid)
        memcpy(&guid, pGuid, sizeof(guid) < 16 ? sizeof(guid) : 16);
    int nResult = (int)g_pShell->OnEngineInitialized(&guid, &mode);
    if (g_pfnClientDisableWindowHook)
        g_pfnClientDisableWindowHook();
    return nResult;
}

extern "C" __declspec(dllexport) void FearHost_Update(void)
{
    if (FearServerHost_IsRunning())
        FearServerHost_Update();

    if (!g_pShell)
        return;

    static int s_nConnectedFrames = 0;
    if (g_bFearConnected)
        ++s_nConnectedFrames;

    if (s_nConnectedFrames > 120)
    {
        g_pShell->PreUpdate();
        g_pShell->Update();
        g_pShell->PostUpdate();
        FearServerHost_DeliverMessages();

        DWORD nNow = GetTickCount();
        float fDelta = (float)(nNow - g_nPreviewLastTick) * 0.001f;
        g_nPreviewLastTick = nNow;
        if (fDelta > 0.1f)
            fDelta = 0.1f;

        float fForward = (g_bPreviewKeys[0x26] ? 1.0f : 0.0f) - (g_bPreviewKeys[0x28] ? 1.0f : 0.0f);
        float fStrafe = (g_bPreviewKeys[0x27] ? 1.0f : 0.0f) - (g_bPreviewKeys[0x25] ? 1.0f : 0.0f);
        float fUp = (g_bPreviewKeys[0x20] ? 1.0f : 0.0f) - (g_bPreviewKeys[0x11] ? 1.0f : 0.0f);
        float fSpeed = (g_bPreviewKeys[0x10] ? 1200.0f : 500.0f) * fDelta;

        float fSin = sinf(g_fPreviewYaw);
        float fCos = cosf(g_fPreviewYaw);
        g_fPreviewPos[0] += (fSin * fForward + fCos * fStrafe) * fSpeed;
        g_fPreviewPos[1] += fUp * fSpeed;
        g_fPreviewPos[2] += (fCos * fForward - fSin * fStrafe) * fSpeed;
        FearHost_UpdatePreviewCamera();

        if (g_pfnClientRenderWorldPreview)
            g_pfnClientRenderWorldPreview();
        return;
    }

    g_pShell->PreUpdate();
    g_pShell->Update();
    g_pShell->PostUpdate();
    FearServerHost_DeliverMessages();

    static int s_nMenuFrames = 0;
    static int s_nAutoStartAttempts = 0;
    static int s_bAutoStart = -1;
    if (s_bAutoStart < 0)
    {
        char szAutoStart[8] = {0};
        DWORD nLen = GetEnvironmentVariableA("FEAR_AUTOSTART", szAutoStart, sizeof(szAutoStart));
        s_bAutoStart = (nLen > 0 && szAutoStart[0] != '0') ? 1 : 0;
    }
    ++s_nMenuFrames;
    if (s_bAutoStart && !g_bFearConnected && g_pfnClientStartNewGame &&
        s_nMenuFrames > 120 && s_nAutoStartAttempts < 5 && (s_nMenuFrames % 60) == 0)
    {
        ++s_nAutoStartAttempts;
        int nStarted = g_pfnClientStartNewGame();
        printf("FEARPORT: auto-start new game attempt=%d result=%d\n", s_nAutoStartAttempts, nStarted);
        fflush(stdout);
    }

    if (!g_bWorldPreview && s_nConnectedFrames == 120 && g_pfnClientEnterWorldPreview)
    {
        printf("FEARPORT: entering world preview\n");
        fflush(stdout);
        g_bWorldPreview = true;
        g_bPreviewMouseInitialized = false;
        g_nPreviewLastTick = GetTickCount();
        FearHost_UpdatePreviewCamera();
        g_pfnClientEnterWorldPreview();
    }
}

extern "C" __declspec(dllexport) void FearHost_OnKeyDown(int key, int rep)
{
    if (g_bWorldPreview)
    {
        if (key >= 0 && key < 256)
            g_bPreviewKeys[key] = true;
        return;
    }
    if (g_pShell)
        g_pShell->OnKeyDown(key, rep);
}

extern "C" __declspec(dllexport) void FearHost_OnKeyUp(int key)
{
    if (g_bWorldPreview)
    {
        if (key >= 0 && key < 256)
            g_bPreviewKeys[key] = false;
        return;
    }
    if (g_pShell)
        g_pShell->OnKeyUp(key);
}

extern "C" __declspec(dllexport) void FearHost_OnMouseMove(int x, int y)
{
    if (g_bWorldPreview)
    {
        if (!g_bPreviewMouseInitialized)
        {
            g_bPreviewMouseInitialized = true;
            g_nPreviewLastMouseX = x;
            g_nPreviewLastMouseY = y;
            return;
        }
        int dx = x - g_nPreviewLastMouseX;
        int dy = y - g_nPreviewLastMouseY;
        g_nPreviewLastMouseX = x;
        g_nPreviewLastMouseY = y;
        g_fPreviewYaw += (float)dx * 0.0025f;
        g_fPreviewPitch -= (float)dy * 0.0025f;
        if (g_fPreviewPitch > 1.35f)
            g_fPreviewPitch = 1.35f;
        if (g_fPreviewPitch < -1.35f)
            g_fPreviewPitch = -1.35f;
        FearHost_UpdatePreviewCamera();
        return;
    }
    if (g_pfnClientMouseMove)
        g_pfnClientMouseMove(x, y);
}

extern "C" __declspec(dllexport) void FearHost_OnMouseButton(int nButton, int nDown, int x, int y)
{
    if (g_bWorldPreview)
        return;
    if (g_pfnClientMouseButton)
        g_pfnClientMouseButton(nButton, nDown, x, y);
}

extern "C" __declspec(dllexport) void FearHost_OnMouseWheel(int x, int y, int nDelta)
{
    if (g_pfnClientMouseWheel)
        g_pfnClientMouseWheel(x, y, nDelta);
}

extern "C" __declspec(dllexport) void FearHost_OnEngineTerm(void)
{
    FearServerHost_Shutdown();
    if (g_pShell)
        g_pShell->OnEngineTerm();
}

extern "C" __declspec(dllexport) void FearHost_Shutdown(void)
{
    FearServerHost_Shutdown();
    if (g_hGameClient)
    {
        FreeLibrary(g_hGameClient);
        g_hGameClient = NULL;
    }
}
