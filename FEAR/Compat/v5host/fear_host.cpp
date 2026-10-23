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
#include "iltmessage.h"
#include "fear_client_facade.h"
#include "fear_object_map.h"
#include "fear_server_host.h"
#include "fear_input_stub.h"
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
typedef void (*TFearClientPreLoadWorld)(const char *);
typedef void (*TFearClientOnEnterWorld)();
typedef void (*TFearClientLogState)();
typedef int (*TFearClientIsPostload)();
typedef void (*TFearClientPressKey)(int);
typedef void (*TFearClientReplySaveFailed)();
typedef void (*TFearClientEnterWorldPreview)();
typedef void (*TFearClientRenderWorldPreview)();
typedef int (*TFearClientStartNewGame)();
typedef int (*TFearClientIsPlaying)();
typedef int (*TFearClientPreparePortPlayer)(void *);
typedef int (*TFearClientCreateLocalCharacter)(void *);
typedef void (*TFearClientRenderPortCamera)();
typedef int (*TFearClientInstallDefaultBindings)();
typedef void (*TFearClientLogInputState)();

static TFearClientMouseMove g_pfnClientMouseMove = NULL;
static TFearClientMouseButton g_pfnClientMouseButton = NULL;
static TFearClientMouseWheel g_pfnClientMouseWheel = NULL;
static TFearClientDisableWindowHook g_pfnClientDisableWindowHook = NULL;
static TFearClientPreLoadWorld g_pfnClientPreLoadWorld = NULL;
static TFearClientOnEnterWorld g_pfnClientOnEnterWorld = NULL;
static TFearClientLogState g_pfnClientLogState = NULL;
static TFearClientIsPostload g_pfnClientIsPostload = NULL;
static TFearClientPressKey g_pfnClientPressKey = NULL;
static TFearClientReplySaveFailed g_pfnClientReplySaveFailed = NULL;
static TFearClientEnterWorldPreview g_pfnClientEnterWorldPreview = NULL;
static TFearClientRenderWorldPreview g_pfnClientRenderWorldPreview = NULL;
static TFearClientStartNewGame g_pfnClientStartNewGame = NULL;
static TFearClientIsPlaying g_pfnClientIsPlaying = NULL;
static TFearClientPreparePortPlayer g_pfnClientPreparePortPlayer = NULL;
static TFearClientCreateLocalCharacter g_pfnClientCreateLocalCharacter = NULL;
static TFearClientRenderPortCamera g_pfnClientRenderPortCamera = NULL;
static TFearClientInstallDefaultBindings g_pfnClientInstallDefaultBindings = NULL;
static TFearClientLogInputState g_pfnClientLogInputState = NULL;

static bool g_bWorldPreview = false;
static bool g_bAwaitPostload = false;
static bool g_bPreviewKeys[256] = {false};
static float g_fPreviewPos[3] = { -1100.0f, -1372.0f, -370.0f };
static float g_fPreviewYaw = -0.0784588f;
static float g_fPreviewPitch = 0.0f;
static int g_nPreviewLastMouseX = 320;
static int g_nPreviewLastMouseY = 240;
static bool g_bPreviewMouseInitialized = false;
static DWORD g_nPreviewLastTick = 0;

extern "C" __declspec(dllexport) void FearHost_OnKeyDown(int key, int rep);
extern "C" __declspec(dllexport) void FearHost_OnKeyUp(int key);

static void FearHost_UpdatePreviewCamera()
{
    if (g_pEngineApi && g_pEngineApi->m_pRenderApi && g_pEngineApi->m_pRenderApi->SetWorldCamera)
        g_pEngineApi->m_pRenderApi->SetWorldCamera(g_fPreviewPos, g_fPreviewYaw, g_fPreviewPitch);
}

static unsigned int g_nServerMsgCounts[256];
static unsigned int g_nClientMsgCounts[256];

static bool FearHost_ShouldLogMsg(unsigned int *pCounts, unsigned int nId)
{
    unsigned int nCount = ++pCounts[nId];
    return nCount == 1 || (nCount % 600) == 0;
}

extern "C" void FearHost_DeliverServerMessage(ILTMessage_Read *pMessage)
{
    if (!g_pShell || !pMessage)
        return;

    uint32 nSaved = pMessage->Tell();
    unsigned char nMessageID = (unsigned char)pMessage->ReadBits(8);
    pMessage->SeekTo(nSaved);

    bool bLogMsg = FearHost_ShouldLogMsg(g_nServerMsgCounts, nMessageID);
    unsigned char nMessageState = 0;
    if (bLogMsg)
        printf("FEARPORT: server message id=%u count=%u", (unsigned)nMessageID, g_nServerMsgCounts[nMessageID]);
    if (nMessageID == 187)
    {
        if (bLogMsg)
            printf(" save-data refused\n");
        fflush(stdout);
        if (g_pfnClientReplySaveFailed)
            g_pfnClientReplySaveFailed();
        return;
    }
    if (nMessageID == 103 || nMessageID == 126 || nMessageID == 129 || nMessageID == 145 ||
        nMessageID == 229 || nMessageID == 233 || nMessageID == 138 ||
        nMessageID == 230 || nMessageID == 231)
    {
        if (bLogMsg)
        {
            printf(" skipped\n");
            fflush(stdout);
        }
        return;
    }
    pMessage->SeekTo(nSaved);
    pMessage->ReadBits(8);
    if (bLogMsg)
    {
        if (nMessageID == 160)
            printf(" pause=%u", (unsigned)pMessage->ReadBits(1));
        else if (nMessageID == 245 || nMessageID == 195 || nMessageID == 105)
        {
            nMessageState = (unsigned char)pMessage->ReadBits(8);
            printf(" state=%u", (unsigned)nMessageState);
        }
        printf("\n");
        fflush(stdout);
    }
    else if (nMessageID == 245 || nMessageID == 195 || nMessageID == 105)
    {
        nMessageState = (unsigned char)pMessage->ReadBits(8);
    }
    pMessage->SeekTo(nSaved);
    if (nMessageID == 245 && nMessageState == 12)
        g_bAwaitPostload = true;

    char szWorld[MAX_PATH] = {0};
    if (nMessageID == 186)
    {
        pMessage->SeekTo(nSaved);
        pMessage->ReadBits(8);
        pMessage->ReadString(szWorld, sizeof(szWorld));
        pMessage->SeekTo(nSaved);
    }

    unsigned short nPlayerObjectId = FearServerHost_GetPlayerObjectId();
    if (nPlayerObjectId != 0xFFFF)
        FearClient_SetLocalServerObject(nPlayerObjectId);

    FearClient_BeginServerMessage();
    g_pShell->OnMessage(pMessage);
    FearClient_EndServerMessage();

    static bool s_bWorldLoadTriggered = false;
    if (!s_bWorldLoadTriggered && (nMessageID == 186 || nMessageID == 195) && g_pfnClientPreLoadWorld)
    {
        const char *pszWorld = szWorld[0] ? szWorld : "Worlds\\Release\\Intro.World00p";
        g_pfnClientPreLoadWorld(pszWorld);
        if (g_pfnClientOnEnterWorld)
            g_pfnClientOnEnterWorld();
        s_bWorldLoadTriggered = true;
    }
}

const FearEngineApi *FearHost_GetEngineApi(void)
{
    return g_pEngineApi;
}

static void FearHost_OpenLogFile(void)
{
    char szPath[1024];
    DWORD nLen = GetEnvironmentVariableA("FEAR_HOST_LOG", szPath, (DWORD)sizeof(szPath));
    if (nLen == 0 || nLen >= (DWORD)sizeof(szPath))
        return;
    if (!freopen(szPath, "w", stdout))
        return;
    freopen(szPath, "a", stderr);
    setvbuf(stdout, NULL, _IONBF, 0);
}

extern "C" __declspec(dllexport) int FearHost_Init(const char *pszGameClientDll, const FearEngineApi *pApi)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    FearHost_OpenLogFile();
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
    g_pfnClientPreLoadWorld = (TFearClientPreLoadWorld)GetProcAddress(g_hGameClient, "FearClient_PreLoadWorld");
    g_pfnClientOnEnterWorld = (TFearClientOnEnterWorld)GetProcAddress(g_hGameClient, "FearClient_OnEnterWorld");
    g_pfnClientLogState = (TFearClientLogState)GetProcAddress(g_hGameClient, "FearClient_LogState");
    g_pfnClientInstallDefaultBindings = (TFearClientInstallDefaultBindings)GetProcAddress(g_hGameClient, "FearClient_InstallDefaultBindings");
    g_pfnClientLogInputState = (TFearClientLogInputState)GetProcAddress(g_hGameClient, "FearClient_LogInputState");
    g_pfnClientIsPostload = (TFearClientIsPostload)GetProcAddress(g_hGameClient, "FearClient_IsPostload");
    g_pfnClientPressKey = (TFearClientPressKey)GetProcAddress(g_hGameClient, "FearClient_PressKey");
    g_pfnClientReplySaveFailed = (TFearClientReplySaveFailed)GetProcAddress(g_hGameClient, "FearClient_ReplySaveFailed");
    g_pfnClientEnterWorldPreview = (TFearClientEnterWorldPreview)GetProcAddress(g_hGameClient, "FearClient_EnterWorldPreview");
    g_pfnClientRenderWorldPreview = (TFearClientRenderWorldPreview)GetProcAddress(g_hGameClient, "FearClient_RenderWorldPreview");
    g_pfnClientStartNewGame = (TFearClientStartNewGame)GetProcAddress(g_hGameClient, "FearClient_StartNewGame");
    g_pfnClientIsPlaying = (TFearClientIsPlaying)GetProcAddress(g_hGameClient, "FearClient_IsPlaying");
    g_pfnClientPreparePortPlayer = (TFearClientPreparePortPlayer)GetProcAddress(g_hGameClient, "FearClient_PreparePortPlayer");
    g_pfnClientCreateLocalCharacter = (TFearClientCreateLocalCharacter)GetProcAddress(g_hGameClient, "FearClient_CreateLocalCharacter");
    g_pfnClientRenderPortCamera = (TFearClientRenderPortCamera)GetProcAddress(g_hGameClient, "FearClient_RenderPortCamera");
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

    unsigned short nPlayerObjectId = FearServerHost_GetPlayerObjectId();
    if (nPlayerObjectId != 0xFFFF)
    {
        float position[3] = {0.0f, 0.0f, 0.0f};
        float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        FearClient_SetLocalServerObject(nPlayerObjectId);
        if (FearServerHost_GetPlayerTransform(position, rotation))
        {
            // While the client owns the local player, its predicted position is
            // authoritative for rendering; the server transform must not snap it back.
            if (!FearClient_IsLocalPlayerScope())
                FearClient_UpdateServerObjectTransform(nPlayerObjectId, position, rotation);
        }
    }

    if (!g_pShell)
        return;

    FearServerHost_DeliverMessages();

    g_pShell->PreUpdate();
    g_pShell->Update();
    g_pShell->PostUpdate();

    static bool s_bPortCameraReady = false;
    const bool bClientPlaying = g_pfnClientIsPlaying && g_pfnClientIsPlaying();
    if (!s_bPortCameraReady && bClientPlaying && g_bFearConnected && FearServerHost_GetPlayerObject() &&
        g_pfnClientPreparePortPlayer && g_pfnClientRenderPortCamera)
    {
        unsigned short nObjectId = FearServerHost_GetPlayerObjectId();
        if (nObjectId == 0xFFFF)
            nObjectId = 0;

        HOBJECT hTarget = FearClient_GetClientObject();
        if (!hTarget)
            hTarget = FearClient_ResolveServerObject(nObjectId);

        if (hTarget)
        {
            int nLocalPlayer = 0;
            if (g_pfnClientCreateLocalCharacter)
            {
                FearClient_SetLocalPlayerScope(true);
                nLocalPlayer = g_pfnClientCreateLocalCharacter(hTarget);
                FearClient_SetLocalPlayerScope(false);
            }

            if (g_pfnClientPreparePortPlayer(hTarget))
            {
                s_bPortCameraReady = true;
                printf("FEARPORT: port camera ready target=%p id=%u localplayer=%d\n", hTarget, (unsigned)nObjectId, nLocalPlayer);
                fflush(stdout);
            }
        }
    }

    if (s_bPortCameraReady && bClientPlaying && g_pfnClientRenderPortCamera)
        g_pfnClientRenderPortCamera();

    if (g_bAwaitPostload && g_pfnClientIsPostload && g_pfnClientIsPostload() && g_pfnClientPressKey)
    {
        g_pfnClientPressKey(VK_SPACE);
        g_bAwaitPostload = false;
    }

    static int s_nMenuFrames = 0;
    static int s_nAutoStartAttempts = 0;
    static int s_bAutoStart = -1;
    static int s_bBindingsInstalled = -1;
    if (s_bAutoStart < 0)
    {
        char szAutoStart[8] = {0};
        DWORD nLen = GetEnvironmentVariableA("FEAR_AUTOSTART", szAutoStart, sizeof(szAutoStart));
        s_bAutoStart = (nLen > 0 && szAutoStart[0] != '0') ? 1 : 0;
    }
    ++s_nMenuFrames;
    if (s_bBindingsInstalled != 1 && g_pfnClientInstallDefaultBindings && (s_nMenuFrames % 30) == 0)
    {
        s_bBindingsInstalled = g_pfnClientInstallDefaultBindings();
        if (s_bBindingsInstalled == 1)
        {
            printf("FEARPORT: default command bindings installed\n");
            fflush(stdout);
        }
    }
    if (g_pfnClientLogState && (s_nMenuFrames % 60) == 0)
    {
        float vSrvPos[3] = {0.0f, 0.0f, 0.0f};
        float vSrvRot[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        int nHasSrv = FearServerHost_GetPlayerTransform(vSrvPos, vSrvRot) ? 1 : 0;
        printf("FEARPORT: serverframe=%d srvplayer=%d pos=%.2f %.2f %.2f inputqueries=%d\n",
            s_nMenuFrames, nHasSrv, vSrvPos[0], vSrvPos[1], vSrvPos[2],
            FearInput_GetQueryCounter() ? *FearInput_GetQueryCounter() : -1);
        fflush(stdout);
        g_pfnClientLogState();
        if (g_pfnClientLogInputState)
            g_pfnClientLogInputState();
    }
    if (s_bAutoStart && !g_bFearConnected && g_pfnClientStartNewGame &&
        s_nMenuFrames > 120 && s_nAutoStartAttempts < 1 && (s_nMenuFrames % 60) == 0)
    {
        ++s_nAutoStartAttempts;
        int nStarted = g_pfnClientStartNewGame();
        printf("FEARPORT: auto-start new game attempt=%d result=%d\n", s_nAutoStartAttempts, nStarted);
        fflush(stdout);
    }

    static int s_nAutoWalkDown = -1;
    static int s_nAutoWalkUp = -1;
    static int s_bAutoWalkPressed = 0;
    static int s_nAutoWalkKey = 0x57;
    if (s_nAutoWalkDown < 0)
    {
        char szWalk[64] = {0};
        if (GetEnvironmentVariableA("FEAR_AUTOWALK", szWalk, sizeof(szWalk)) > 0)
        {
            int nDown = 0, nUp = 0, nKey = 0x57;
            int nFields = sscanf(szWalk, "%d,%d,%d", &nDown, &nUp, &nKey);
            if (nFields >= 2)
            {
                s_nAutoWalkDown = nDown;
                s_nAutoWalkUp = nUp;
                s_nAutoWalkKey = nKey;
                printf("FEARPORT: autowalk key=%d down=%d up=%d\n", nKey, nDown, nUp);
            }
        }
        if (s_nAutoWalkDown < 0)
            s_nAutoWalkDown = 0x7FFFFFFF;
    }

    if (!s_bAutoWalkPressed && s_nMenuFrames >= s_nAutoWalkDown && !g_bWorldPreview)
    {
        s_bAutoWalkPressed = 1;
        FearHost_OnKeyDown(s_nAutoWalkKey, 0);
        printf("FEARPORT: autowalk key down at frame %d\n", s_nMenuFrames);
        fflush(stdout);
    }
    else if (s_bAutoWalkPressed == 1 && s_nMenuFrames >= s_nAutoWalkUp && !g_bWorldPreview)
    {
        s_bAutoWalkPressed = 2;
        FearHost_OnKeyUp(s_nAutoWalkKey);
        printf("FEARPORT: autowalk key up at frame %d\n", s_nMenuFrames);
        fflush(stdout);
    }
}

extern "C" __declspec(dllexport) void FearHost_OnKeyDown(int key, int rep)
{
    FearInput_SetKeyState(key, true);
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
    FearInput_SetKeyState(key, false);
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
    FearInput_SetMousePos(x, y);
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
    FearInput_SetMouseButton(nButton, nDown != 0);
    FearInput_SetMousePos(x, y);
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
