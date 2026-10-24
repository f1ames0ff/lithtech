#include "fear_server_host.h"
#include "fear_messageread_stub.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "ltbasedefs.h"
#include "ltguid.h"
#include "iltmessage.h"
#include "server_interface.h"

extern "C" unsigned short FearClient_GetServerObjectId(HOBJECT hObject);

typedef SI_CREATESTATUS(__cdecl *TFearCreateServer)(int, LTGUID &, ServerInterface **);
typedef void(__cdecl *TFearDeleteServer)();
typedef void(__thiscall *TFearStringCtor)(void *);
typedef void(__thiscall *TFearStringAssign)(void *, const char *);

static bool g_bServerWorldLoaded = false;

class CFearServerAppHandler : public ServerAppHandler
{
public:
    LTRESULT ConsoleOutputFn(const char *pMsg) override
    {
        if (pMsg)
        {
            printf("FEARSERVER: %s", pMsg);
            fflush(stdout);
            if (strstr(pMsg, "World loaded"))
                g_bServerWorldLoaded = true;
        }
        return LT_OK;
    }

    LTRESULT ShellMessageFn(ILTMessage_Read &) override
    {
        return LT_OK;
    }

    LTRESULT OutOfMemory() override
    {
        return LT_OK;
    }
};

static HMODULE g_hEngineServer = NULL;
static HMODULE g_hMsvcp71 = NULL;
static ServerInterface *g_pServer = NULL;
static TFearDeleteServer g_pfnDeleteServer = NULL;
static CFearServerAppHandler g_ServerHandler;
static bool g_bServerRunning = false;
static unsigned char g_NetGameInfo[176];
static TFearStringAssign g_pfnStringAssign = NULL;
static TFearServerMessageSink g_pfnMessageSink = NULL;
static std::vector<ILTMessage_Read *> g_MessageQueue;
static std::vector<ILTMessage_Read *> g_ClientMessageQueue;
static bool g_bLocalClientEnabled = false;
static void *g_hServerClient = NULL;
static void *g_hServerPlayer = NULL;

static bool FearServerLocalClientEnabled(void)
{
    char szValue[8] = {0};
    DWORD nLen = GetEnvironmentVariableA("FEAR_LOCAL_CLIENT", szValue, sizeof(szValue));
    if (nLen > 0 && szValue[0] == '0')
        return false;
    return true;
}

static void FearServerLog(const char *pszText)
{
    printf("FEARSERVER: %s\n", pszText ? pszText : "");
    fflush(stdout);
}

static int __stdcall FearServerFakeGetClientAddr(void *, unsigned char *pAddr, unsigned short *pPort)
{
    if (pAddr)
        memset(pAddr, 0, 4);
    if (pPort)
        *pPort = 0;
    return 0;
}

static LTRESULT __stdcall FearServerFakeSendToClient(ILTMessage_Read *pMessage, void *, uint32)
{
    if (pMessage && g_MessageQueue.size() < 4096)
    {
        uint32 nBits = pMessage->Size();
        uint32 nSaved = pMessage->Tell();
        std::vector<unsigned char> bytes((nBits + 7) >> 3, 0);
        pMessage->SeekTo(0);
        if (nBits)
            pMessage->ReadData(bytes.data(), nBits);
        pMessage->SeekTo(nSaved);

        CFearMessageRead *pCopy = new CFearMessageRead;
        pCopy->SetData(bytes.empty() ? NULL : bytes.data(), nBits);
        g_MessageQueue.push_back(pCopy);
    }
    return LT_OK;
}

static void FearServerPatchVtable(HMODULE hEngine, int nSlot, void *pFunction)
{
    if (!hEngine)
        return;
    uint32_t *pObject = (uint32_t *)((uintptr_t)hEngine + 0xF72F0);
    uint32_t nVtable = *pObject;
    if (!nVtable)
        return;
    DWORD nOldProtect = 0;
    if (VirtualProtect((void *)(nVtable + nSlot * 4), 4, PAGE_READWRITE, &nOldProtect))
    {
        *(uint32_t *)(nVtable + nSlot * 4) = (uint32_t)(uintptr_t)pFunction;
        DWORD nIgnored = 0;
        VirtualProtect((void *)(nVtable + nSlot * 4), 4, nOldProtect, &nIgnored);
    }
}

static void FearServerPatchCode(HMODULE hEngine, uintptr_t nOffset, const unsigned char *pBytes, size_t nSize)
{
    if (!hEngine)
        return;
    unsigned char *pCode = (unsigned char *)hEngine + nOffset;
    DWORD nOldProtect = 0;
    if (VirtualProtect(pCode, nSize, PAGE_EXECUTE_READWRITE, &nOldProtect))
    {
        memcpy(pCode, pBytes, nSize);
        DWORD nIgnored = 0;
        VirtualProtect(pCode, nSize, nOldProtect, &nIgnored);
        FlushInstructionCache(GetCurrentProcess(), pCode, nSize);
    }
}

static bool FearServerCreateLocalClient(void)
{
    if (!g_hEngineServer || !g_pServer)
        return false;

    void *pShell = *(void **)((uintptr_t)g_hEngineServer + 0xF911C);
    if (!pShell)
    {
        FearServerLog("server shell is not available");
        return false;
    }

    void **pShellVtable = *(void ***)pShell;
    typedef void(__thiscall *TOnAddClient)(void *, void *);
    TOnAddClient pfnAddClient = (TOnAddClient)pShellVtable[15];

    unsigned char *pClient = (unsigned char *)calloc(1, 0x60);
    if (!pClient)
        return false;
    *(unsigned short *)(pClient + 0x34) = 0;
    *(unsigned int *)(pClient + 0x40) = 0x0B;

    pfnAddClient(pShell, pClient);

    typedef void(__cdecl *TClientConnected)(void *);
    TClientConnected pfnConnected = (TClientConnected)((char *)g_hEngineServer + 0x41810);
    pfnConnected(pClient);

    g_hServerClient = pClient;
    g_hServerPlayer = *(void **)(pClient + 0x5C);

    printf("FEARSERVER: local client=%p state=%u flags=%08x player=%p\n",
        pClient, *(unsigned int *)(pClient + 0x38), *(unsigned int *)(pClient + 0x40), g_hServerPlayer);
    fflush(stdout);
    return g_hServerPlayer != NULL;
}

static void FearServerGetDirectory(char *pszDir, size_t nSize)
{
    if (!pszDir || nSize == 0)
        return;
    pszDir[0] = '\0';
    HMODULE hSelf = GetModuleHandleA("FearHost.dll");
    if (!hSelf)
        return;
    if (!GetModuleFileNameA(hSelf, pszDir, (DWORD)nSize))
    {
        pszDir[0] = '\0';
        return;
    }
    char *pSlash = strrchr(pszDir, '\\');
    if (pSlash)
        *pSlash = '\0';
}

static bool FearServerReadArchives(const char *pszDir, std::vector<std::string> &archives)
{
    char szPath[MAX_PATH];
    sprintf(szPath, "%s\\Default.archcfg", pszDir);
    FILE *pFile = fopen(szPath, "rt");
    if (!pFile)
        return false;
    char szLine[512];
    while (fgets(szLine, sizeof(szLine), pFile))
    {
        char *p = szLine;
        while (*p == ' ' || *p == '\t')
            ++p;
        char *pEnd = p + strlen(p);
        while (pEnd > p && (pEnd[-1] == '\n' || pEnd[-1] == '\r' || pEnd[-1] == ' ' || pEnd[-1] == '\t'))
            *--pEnd = '\0';
        if (*p && *p != ';' && *p != '#')
        {
            char szFull[MAX_PATH];
            sprintf(szFull, "%s\\%s", pszDir, p);
            archives.push_back(szFull);
        }
    }
    fclose(pFile);
    return !archives.empty();
}

static bool FearServerPatchMultiplayerFlag(HMODULE hEngine)
{
    if (!hEngine)
        return false;
    unsigned char *pCode = (unsigned char *)hEngine + 0x42F10;
    if (pCode[0] == 0x30 && pCode[1] == 0xC0 && pCode[2] == 0xC3)
        return true;
    if (pCode[0] != 0xB0 || pCode[1] != 0x01 || pCode[2] != 0xC3)
        return false;
    DWORD nOldProtect = 0;
    if (!VirtualProtect(pCode, 3, PAGE_EXECUTE_READWRITE, &nOldProtect))
        return false;
    pCode[0] = 0x30;
    pCode[1] = 0xC0;
    pCode[2] = 0xC3;
    DWORD nIgnored = 0;
    VirtualProtect(pCode, 3, nOldProtect, &nIgnored);
    FlushInstructionCache(GetCurrentProcess(), pCode, 3);
    return true;
}

static bool FearServerBuildNetGameInfo(const char *pszDir)
{
    if (!g_pfnStringAssign)
        return false;
    memset(g_NetGameInfo, 0, sizeof(g_NetGameInfo));
    typedef void(__thiscall *TStringCtor)(void *);
    TStringCtor pfnCtor = (TStringCtor)GetProcAddress(g_hMsvcp71,
        "??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ");
    if (!pfnCtor)
        return false;
    for (int i = 0; i < 6; ++i)
        pfnCtor(g_NetGameInfo + (size_t)i * 28);
    g_pfnStringAssign(g_NetGameInfo + 28, "SinglePlayer");
    g_pfnStringAssign(g_NetGameInfo + 84, "Default");
    g_pfnStringAssign(g_NetGameInfo + 112, "MissionOrder");
    return true;
}

bool FearServerHost_Start(const char *pszWorldName)
{
    if (g_bServerRunning)
        FearServerHost_Shutdown();

    g_bServerWorldLoaded = false;
    char szDir[MAX_PATH];
    FearServerGetDirectory(szDir, sizeof(szDir));
    if (!szDir[0])
    {
        FearServerLog("unable to determine stage directory");
        return false;
    }

    char szEngine[MAX_PATH];
    sprintf(szEngine, "%s\\EngineServer.dll", szDir);
    g_hEngineServer = LoadLibraryA(szEngine);
    if (!g_hEngineServer)
    {
        g_hEngineServer = LoadLibraryA("EngineServer.dll");
        if (!g_hEngineServer)
        {
            FearServerLog("unable to load EngineServer.dll");
            return false;
        }
    }

    TFearCreateServer pfnCreateServer = (TFearCreateServer)GetProcAddress(g_hEngineServer, "CreateServer");
    g_pfnDeleteServer = (TFearDeleteServer)GetProcAddress(g_hEngineServer, "DeleteServer");
    if (!pfnCreateServer || !g_pfnDeleteServer)
    {
        FearServerLog("EngineServer.dll does not export CreateServer/DeleteServer");
        FearServerHost_Shutdown();
        return false;
    }

    if (!FearServerPatchMultiplayerFlag(g_hEngineServer))
    {
        FearServerLog("failed to patch IsMultiplayerExe");
        FearServerHost_Shutdown();
        return false;
    }

    char szMsvcp[MAX_PATH];
    sprintf(szMsvcp, "%s\\msvcp71.dll", szDir);
    g_hMsvcp71 = LoadLibraryA(szMsvcp);
    if (!g_hMsvcp71)
        g_hMsvcp71 = LoadLibraryA("msvcp71.dll");
    if (!g_hMsvcp71)
    {
        FearServerLog("unable to load msvcp71.dll");
        FearServerHost_Shutdown();
        return false;
    }
    g_pfnStringAssign = (TFearStringAssign)GetProcAddress(g_hMsvcp71,
        "??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@PBD@Z");
    if (!g_pfnStringAssign || !FearServerBuildNetGameInfo(szDir))
    {
        FearServerLog("unable to build NetGameInfo");
        FearServerHost_Shutdown();
        return false;
    }

    LTGUID appGuid;
    memset(&appGuid, 0, sizeof(appGuid));
    SI_CREATESTATUS nStatus = pfnCreateServer(SI_VERSION, appGuid, &g_pServer);
    if (nStatus != SI_OK || !g_pServer)
    {
        FearServerLog("CreateServer failed");
        FearServerHost_Shutdown();
        return false;
    }

    g_bLocalClientEnabled = FearServerLocalClientEnabled();
    if (g_bLocalClientEnabled)
    {
        FearServerPatchVtable(g_hEngineServer, 105, (void *)FearServerFakeGetClientAddr);
        FearServerPatchVtable(g_hEngineServer, 94, (void *)FearServerFakeSendToClient);
    }

    g_pServer->SetAppHandler(&g_ServerHandler);

    char szExe[] = "FEARServer.exe";
    char *pArgs[] = { szExe, NULL };
    if (!g_pServer->Init(1, pArgs))
    {
        FearServerLog("server Init failed");
        FearServerHost_Shutdown();
        return false;
    }

    std::vector<std::string> resources;
    if (!FearServerReadArchives(szDir, resources))
    {
        FearServerLog("unable to read Default.archcfg");
        FearServerHost_Shutdown();
        return false;
    }
    resources.push_back(szDir);
    std::vector<const char *> resourcePtrs;
    for (size_t i = 0; i < resources.size(); ++i)
        resourcePtrs.push_back(resources[i].c_str());
    if (!g_pServer->AddResources(&resourcePtrs[0], (uint32)resourcePtrs.size()))
    {
        FearServerLog("server AddResources failed");
        FearServerHost_Shutdown();
        return false;
    }

    void *pNetGameInfo = g_NetGameInfo;
    if (!g_pServer->SetGameInfo(&pNetGameInfo, sizeof(pNetGameInfo)))
    {
        FearServerLog("server SetGameInfo failed");
        FearServerHost_Shutdown();
        return false;
    }

    if (!g_pServer->LoadBinaries())
    {
        wchar_t wszError[1024] = L"";
        g_pServer->GetErrorString(wszError, 1024);
        wprintf(L"FEARSERVER: LoadBinaries failed: %ls\n", wszError);
        fflush(stdout);
        FearServerHost_Shutdown();
        return false;
    }

    if (g_bLocalClientEnabled)
        FearServerCreateLocalClient();

    if (pszWorldName)
    {
        StartGameRequest request;
        memset(&request, 0, sizeof(request));
        request.m_Type = STARTGAME_NORMAL;
        strncpy(request.m_WorldName, pszWorldName, sizeof(request.m_WorldName) - 1);

        if (!g_pServer->StartWorld(&request))
        {
            wchar_t wszError[1024] = L"";
            g_pServer->GetErrorString(wszError, 1024);
            wprintf(L"FEARSERVER: StartWorld failed: %ls\n", wszError);
            fflush(stdout);
            FearServerHost_Shutdown();
            return false;
        }
    }

    g_bServerRunning = true;
    FearServerLog(pszWorldName ? "world started" : "server started");
    return true;
}

void FearServerHost_Update(void)
{
    if (!g_bServerRunning || !g_pServer)
        return;

    static unsigned int s_nReceivedPlayerUpdates = 0;
    static unsigned int s_nLoggedPlayerPositions = 0;
    static unsigned int s_nLoggedServerSteps = 0;

    void *pShell = *(void **)((uintptr_t)g_hEngineServer + 0xF911C);
    if (pShell && g_hServerClient)
    {
        while (!g_ClientMessageQueue.empty())
        {
            ILTMessage_Read *pMessage = g_ClientMessageQueue.front();
            g_ClientMessageQueue.erase(g_ClientMessageQueue.begin());
            if (pMessage)
            {
                pMessage->SeekTo(0);
                static unsigned int s_nServerClientMsgs = 0;
                ++s_nServerClientMsgs;
                if (s_nServerClientMsgs <= 10)
                {
                    printf("FEARSERVER: client msg #%u size=%u id=%u\n",
                        s_nServerClientMsgs, (unsigned)pMessage->Size(), (unsigned)pMessage->Readuint8());
                    fflush(stdout);
                    pMessage->SeekTo(0);
                }
                static unsigned int s_nRx100 = 0;
                bool bPlayerUpdate = false;
                {
                    uint32 nSaved = pMessage->Tell();
                    pMessage->SeekTo(0);
                    uint32 nFirst = pMessage->ReadBits(8);
                    pMessage->SeekTo(nSaved);
                    if (nFirst == 100)
                    {
                        bPlayerUpdate = true;
                        ++s_nRx100;
                        ++s_nReceivedPlayerUpdates;
                        if (s_nRx100 <= 3 || (s_nRx100 % 30) == 0)
                        {
                            printf("FEARRX: id=100 n=%u size=%u player=%p\n", s_nRx100, (unsigned)pMessage->Size(), g_hServerPlayer);
                            fflush(stdout);
                        }
                    }
                }

                float vBefore[3] = {0.0f, 0.0f, 0.0f};
                float vAfter[3] = {0.0f, 0.0f, 0.0f};
                bool bHaveBefore = bPlayerUpdate && FearServerHost_GetPlayerTransform(vBefore, NULL);

                void **pVtable = *(void ***)pShell;
                typedef void(__thiscall *TOnMessage)(void *, void *, ILTMessage_Read *);
                TOnMessage pfnOnMessage = (TOnMessage)pVtable[10];
                pfnOnMessage(pShell, g_hServerClient, pMessage);

                if (bPlayerUpdate && bHaveBefore && FearServerHost_GetPlayerTransform(vAfter, NULL))
                {
                    float fDeltaX = vAfter[0] - vBefore[0];
                    float fDeltaY = vAfter[1] - vBefore[1];
                    float fDeltaZ = vAfter[2] - vBefore[2];
                    float fDeltaSqr = fDeltaX * fDeltaX + fDeltaY * fDeltaY + fDeltaZ * fDeltaZ;
                    if ((fDeltaSqr > 0.01f) &&
                        (s_nLoggedPlayerPositions < 30 || (s_nLoggedPlayerPositions % 120) == 0))
                    {
                        printf("FEARSERVER: player update=%u size=%u moved %.2f %.2f %.2f "
                            "from=%.2f %.2f %.2f to=%.2f %.2f %.2f\n",
                            s_nReceivedPlayerUpdates, (unsigned)pMessage->Size(),
                            fDeltaX, fDeltaY, fDeltaZ,
                            vBefore[0], vBefore[1], vBefore[2],
                            vAfter[0], vAfter[1], vAfter[2]);
                        fflush(stdout);
                        ++s_nLoggedPlayerPositions;
                    }
                }
                pMessage->Release();
            }
        }
    }

    float vBeforeUpdate[3] = {0.0f, 0.0f, 0.0f};
    const bool bHaveBeforeUpdate = FearServerHost_GetPlayerTransform(vBeforeUpdate, NULL);
    g_pServer->Update(0);
    float vAfterUpdate[3] = {0.0f, 0.0f, 0.0f};
    if (bHaveBeforeUpdate && FearServerHost_GetPlayerTransform(vAfterUpdate, NULL))
    {
        float fDeltaX = vAfterUpdate[0] - vBeforeUpdate[0];
        float fDeltaY = vAfterUpdate[1] - vBeforeUpdate[1];
        float fDeltaZ = vAfterUpdate[2] - vBeforeUpdate[2];
        float fDeltaSqr = fDeltaX * fDeltaX + fDeltaY * fDeltaY + fDeltaZ * fDeltaZ;
        if ((fDeltaSqr > 0.01f) &&
            (s_nLoggedServerSteps < 30 || (s_nLoggedServerSteps % 120) == 0))
        {
            printf("FEARSERVER: step moved %.2f %.2f %.2f from=%.2f %.2f %.2f to=%.2f %.2f %.2f\n",
                fDeltaX, fDeltaY, fDeltaZ,
                vBeforeUpdate[0], vBeforeUpdate[1], vBeforeUpdate[2],
                vAfterUpdate[0], vAfterUpdate[1], vAfterUpdate[2]);
            fflush(stdout);
            ++s_nLoggedServerSteps;
        }
    }

    static bool s_bCamerasDumped = false;
    char szDumpCameras[8] = {0};
    if (!s_bCamerasDumped && g_bServerWorldLoaded &&
        GetEnvironmentVariableA("FEAR_DUMP_CAMERAS", szDumpCameras, sizeof(szDumpCameras)) > 0 &&
        szDumpCameras[0] && szDumpCameras[0] != '0')
    {
        HMODULE hGameServer = GetModuleHandleA("GameServer.dll");
        if (hGameServer)
        {
            typedef int(__cdecl *TDumpCameras)(void);
            TDumpCameras pfnDump = (TDumpCameras)GetProcAddress(hGameServer, "FearGameServer_DumpCameras");
            if (pfnDump)
                pfnDump();
        }
        s_bCamerasDumped = true;
    }

    if (g_hServerClient)
    {
        g_hServerPlayer = *(void **)((unsigned char *)g_hServerClient + 0x5C);
        if (!g_hServerPlayer)
        {
            if (g_bServerWorldLoaded)
            {
                typedef void(__cdecl *TClientEnterWorld)(void *);
                TClientEnterWorld pfnEnterWorld = (TClientEnterWorld)((char *)g_hEngineServer + 0x41750);
                pfnEnterWorld(g_hServerClient);
                g_hServerPlayer = *(void **)((unsigned char *)g_hServerClient + 0x5C);
            }
            if (!g_hServerPlayer)
            {
                typedef void(__cdecl *TClientConnected)(void *);
                TClientConnected pfnConnected = (TClientConnected)((char *)g_hEngineServer + 0x41810);
                pfnConnected(g_hServerClient);
                g_hServerPlayer = *(void **)((unsigned char *)g_hServerClient + 0x5C);
            }
            if (g_hServerPlayer)
            {
                printf("FEARSERVER: player connected=%p\n", g_hServerPlayer);
                fflush(stdout);
            }
        }
    }

    static bool s_bStartupCommandQueued = false;
    char szStartupCommand[2048] = {0};
    if (!s_bStartupCommandQueued && g_hServerPlayer &&
        GetEnvironmentVariableA("FEAR_QUEUE_COMMAND", szStartupCommand, sizeof(szStartupCommand)) > 0 &&
        szStartupCommand[0])
    {
        HMODULE hGameServer = GetModuleHandleA("GameServer.dll");
        if (hGameServer)
        {
            typedef void(__cdecl *TQueueCommand)(const char *);
            TQueueCommand pfnQueue =
                (TQueueCommand)GetProcAddress(hGameServer, "FearGameServer_QueueCommand");
            if (pfnQueue)
            {
                pfnQueue(szStartupCommand);
                printf("FEARSERVER: queued startup command '%s'\n", szStartupCommand);
                fflush(stdout);
            }
        }
        s_bStartupCommandQueued = true;
    }
}

void FearServerHost_SubmitClientMessage(ILTMessage_Read *pMessage)
{
    if (!pMessage)
        return;
    g_ClientMessageQueue.push_back(pMessage);
}

void FearServerHost_DeliverMessages(void)
{
    if (!g_pfnMessageSink)
        return;
    while (!g_MessageQueue.empty())
    {
        ILTMessage_Read *pMessage = g_MessageQueue.front();
        g_MessageQueue.erase(g_MessageQueue.begin());
        if (pMessage)
        {
            pMessage->SeekTo(0);
            g_pfnMessageSink(pMessage);
            pMessage->Release();
        }
    }
}

int FearServerHost_VisitCameras(TFearServerObjectVisitor pVisitor, void *pContext)
{
    if (!g_bServerRunning || !g_bServerWorldLoaded || !pVisitor)
        return 0;

    HMODULE hGameServer = GetModuleHandleA("GameServer.dll");
    if (!hGameServer)
        return 0;

    TFearGameServerVisitCameras pfnVisit =
        (TFearGameServerVisitCameras)GetProcAddress(hGameServer, "FearGameServer_VisitCameras");
    return pfnVisit ? pfnVisit(pVisitor, pContext) : 0;
}

int FearServerHost_VisitObjects(TFearServerObjectVisitor pVisitor, void *pContext)
{
    if (!g_bServerRunning || !g_bServerWorldLoaded || !pVisitor)
        return 0;

    HMODULE hGameServer = GetModuleHandleA("GameServer.dll");
    if (!hGameServer)
        return 0;

    TFearGameServerVisitObjects pfnVisit =
        (TFearGameServerVisitObjects)GetProcAddress(hGameServer, "FearGameServer_VisitObjects");
    return pfnVisit ? pfnVisit(pVisitor, pContext) : 0;
}

void *FearServerHost_ResolveObject(unsigned short nObjectId)
{
    if (!g_bServerRunning || !g_bServerWorldLoaded || nObjectId == FEAR_INVALID_OBJECT_ID)
        return NULL;

    HMODULE hGameServer = GetModuleHandleA("GameServer.dll");
    if (!hGameServer)
        return NULL;

    TFearGameServerResolveObject pfnResolve =
        (TFearGameServerResolveObject)GetProcAddress(hGameServer, "FearGameServer_ResolveObject");
    return pfnResolve ? pfnResolve(nObjectId) : NULL;
}

static void *FearServerHost_ResolveClientObject(void *pClientObject)
{
    HOBJECT hServerObject = (HOBJECT)FearServerHost_ResolveObject(
        FearClient_GetServerObjectId((HOBJECT)pClientObject));
    if (hServerObject)
        return hServerObject;

    HMODULE hGameClient = GetModuleHandleA("GameClient.dll");
    if (hGameClient)
    {
        typedef void *(__cdecl *TGetLocalPlayerBody)();
        TGetLocalPlayerBody pfnGetLocalPlayerBody =
            (TGetLocalPlayerBody)GetProcAddress(hGameClient, "FearClient_GetLocalPlayerBody");
        if (pfnGetLocalPlayerBody && pfnGetLocalPlayerBody() == pClientObject)
            return g_hServerPlayer;
    }

    return NULL;
}

bool FearServerHost_GetAnimKeyFrame(
    void *pClientObject, unsigned int nAnimation, unsigned int nKeyFrame,
    unsigned int *pTime, char *pszKey, unsigned int nKeyLength)
{
    void *pObject = FearServerHost_ResolveClientObject(pClientObject);
    if (!pObject || !pTime || !pszKey || !nKeyLength)
        return false;

    bool bResult = false;
    __try
    {
        unsigned char *pBytes = (unsigned char *)pObject;
        if (pBytes[87] != 1)
            __leave;

        unsigned char *pModel = *(unsigned char **)(pBytes + 272);
        if (!pModel)
            __leave;

        unsigned char *pInfo = *(unsigned char **)(pModel + 100);
        unsigned char *pInfoEnd = *(unsigned char **)(pModel + 104);
        if (!pInfo || !pInfoEnd || pInfoEnd <= pInfo)
            __leave;

        unsigned int nAnimationCount = (unsigned int)(pInfoEnd - pInfo) / 8;
        if (nAnimation >= nAnimationCount)
            __leave;

        unsigned char *pAnim = *(unsigned char **)(pInfo + nAnimation * 8 + 4);
        if (!pAnim)
            __leave;

        unsigned int nKeyFrameCount = *(unsigned int *)(pAnim + 20);
        if (nKeyFrame >= nKeyFrameCount)
            __leave;

        unsigned char *pRecord = pAnim + 40 + nKeyFrame * 8;
        const char *pszSource = *(const char **)(pRecord + 4);
        *pTime = *(unsigned int *)pRecord;
        pszKey[0] = '\0';
        if (pszSource)
        {
            unsigned int nCopy = 0;
            while (nCopy + 1 < nKeyLength && pszSource[nCopy])
            {
                pszKey[nCopy] = pszSource[nCopy];
                ++nCopy;
            }
            pszKey[nCopy] = '\0';
        }
        bResult = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        bResult = false;
    }
    return bResult;
}

static HMODULE FearServerHost_GameServerModule(void)
{
    if (!g_bServerRunning || !g_bServerWorldLoaded)
        return NULL;
    return GetModuleHandleA("GameServer.dll");
}

bool FearServerHost_GetAnimLength(
    void *pClientObject, unsigned int nAnimation, unsigned int *pLength)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !pClientObject || !pLength)
        return false;

    TFearGameServerGetAnimLength pfnGet =
        (TFearGameServerGetAnimLength)GetProcAddress(hGameServer, "FearGameServer_GetAnimLength");
    return pfnGet && pfnGet(FearServerHost_ResolveClientObject(pClientObject), nAnimation, pLength);
}

bool FearServerHost_GetAnimKeyFrames(
    void *pClientObject, unsigned int nAnimation, unsigned int *pKeyFrames)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !pClientObject || !pKeyFrames)
        return false;

    TFearGameServerGetAnimKeyFrames pfnGet =
        (TFearGameServerGetAnimKeyFrames)GetProcAddress(hGameServer, "FearGameServer_GetAnimKeyFrames");
    return pfnGet && pfnGet(FearServerHost_ResolveClientObject(pClientObject), nAnimation, pKeyFrames);
}

bool FearServerHost_GetAnimNodeTransform(
    void *pClientObject, unsigned int nAnimation, unsigned int nNode, unsigned int nKeyFrame,
    float *pPosition, float *pRotation)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !pClientObject || !pPosition || !pRotation)
        return false;

    TFearGameServerGetAnimNodeTransform pfnGet =
        (TFearGameServerGetAnimNodeTransform)GetProcAddress(
            hGameServer, "FearGameServer_GetAnimNodeTransform");
    return pfnGet && pfnGet(
        FearServerHost_ResolveClientObject(pClientObject),
        nAnimation, nNode, nKeyFrame, pPosition, pRotation);
}

bool FearServerHost_GetNodeTransform(
    void *pClientObject, unsigned int nNode, float *pPosition, float *pRotation, bool bWorldSpace)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !pClientObject || !pPosition || !pRotation)
        return false;

    TFearGameServerGetNodeTransform pfnGet =
        (TFearGameServerGetNodeTransform)GetProcAddress(
            hGameServer, "FearGameServer_GetNodeTransform");
    return pfnGet && pfnGet(
        FearServerHost_ResolveClientObject(pClientObject),
        nNode, pPosition, pRotation, bWorldSpace ? 1 : 0);
}

bool FearServerHost_GetBindPoseNodeTransform(
    void *pClientObject, unsigned int nNode, float *pPosition, float *pRotation)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !pClientObject || !pPosition || !pRotation)
        return false;

    TFearGameServerGetBindPoseNodeTransform pfnGet =
        (TFearGameServerGetBindPoseNodeTransform)GetProcAddress(
            hGameServer, "FearGameServer_GetBindPoseNodeTransform");
    return pfnGet && pfnGet(
        FearServerHost_ResolveClientObject(pClientObject),
        nNode, pPosition, pRotation);
}

static bool FearServerHost_NativeModelReady(void *pServerObject)
{
    __try
    {
        unsigned char *pObject = (unsigned char *)pServerObject;
        if (!pObject || pObject[87] != 1)
            return false;
        unsigned char *pModel = *(unsigned char **)(pObject + 272);
        if (!pModel)
            return false;
        unsigned int nNodeCount = *(unsigned int *)(pModel + 48);
        return nNodeCount > 0 && nNodeCount <= 4096;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool FearServerHost_GetNodeTransformRaw(
    void *pServerObject, unsigned int nNode, float *pPosition, float *pRotation, bool bWorldSpace)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !FearServerHost_NativeModelReady(pServerObject) ||
        !pPosition || !pRotation)
        return false;

    TFearGameServerGetNodeTransform pfnGet =
        (TFearGameServerGetNodeTransform)GetProcAddress(
            hGameServer, "FearGameServer_GetNodeTransform");
    return pfnGet && pfnGet(
        pServerObject, nNode, pPosition, pRotation, bWorldSpace ? 1 : 0);
}

bool FearServerHost_GetBindPoseNodeTransformRaw(
    void *pServerObject, unsigned int nNode, float *pPosition, float *pRotation)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !FearServerHost_NativeModelReady(pServerObject) ||
        !pPosition || !pRotation)
        return false;

    TFearGameServerGetBindPoseNodeTransform pfnGet =
        (TFearGameServerGetBindPoseNodeTransform)GetProcAddress(
            hGameServer, "FearGameServer_GetBindPoseNodeTransform");
    return pfnGet && pfnGet(pServerObject, nNode, pPosition, pRotation);
}

bool FearServerHost_GetNumNodesRaw(void *pServerObject, unsigned int *pNumNodes)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !FearServerHost_NativeModelReady(pServerObject) ||
        !pNumNodes)
        return false;

    TFearGameServerGetNumNodes pfnGet =
        (TFearGameServerGetNumNodes)GetProcAddress(
            hGameServer, "FearGameServer_GetNumNodes");
    return pfnGet && pfnGet(pServerObject, pNumNodes);
}

bool FearServerHost_GetMaterialFilenameRaw(
    void *pServerObject, unsigned int nIndex, char *pszName, unsigned int nNameLength)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !FearServerHost_NativeModelReady(pServerObject) ||
        !pszName || !nNameLength)
        return false;

    typedef int (__cdecl *TGetMaterialFilename)(
        void *, unsigned int, char *, unsigned int);
    TGetMaterialFilename pfnGet = (TGetMaterialFilename)GetProcAddress(
        hGameServer, "FearGameServer_GetMaterialFilename");
    return pfnGet && pfnGet(pServerObject, nIndex, pszName, nNameLength);
}

bool FearServerHost_DumpModelPiecesRaw(
    void *pServerObject, unsigned int nMaxPieces)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !FearServerHost_NativeModelReady(pServerObject) ||
        nMaxPieces == 0)
        return false;

    TFearGameServerDumpModelPieces pfnDump =
        (TFearGameServerDumpModelPieces)GetProcAddress(
            hGameServer, "FearGameServer_DumpModelPieces");
    return pfnDump && pfnDump(pServerObject, nMaxPieces) >= 0;
}

bool FearServerHost_GetAnimIndex(
    void *pClientObject, const char *pszName, unsigned int *pAnimation)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !pClientObject || !pszName || !pAnimation)
        return false;

    TFearGameServerGetAnimIndex pfnGet =
        (TFearGameServerGetAnimIndex)GetProcAddress(hGameServer, "FearGameServer_GetAnimIndex");
    return pfnGet && pfnGet(FearServerHost_ResolveClientObject(pClientObject), pszName, pAnimation);
}

bool FearServerHost_GetAnimName(
    void *pClientObject, unsigned int nAnimation, char *pszName, unsigned int nNameLength)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !pClientObject || !pszName || !nNameLength)
        return false;

    TFearGameServerGetAnimName pfnGet =
        (TFearGameServerGetAnimName)GetProcAddress(hGameServer, "FearGameServer_GetAnimName");
    return pfnGet && pfnGet(
        FearServerHost_ResolveClientObject(pClientObject),
        nAnimation, pszName, nNameLength);
}

bool FearServerHost_GetModelAnimUserDims(
    void *pClientObject, unsigned int nAnimation, float *pDims)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !pClientObject || !pDims)
        return false;

    TFearGameServerGetModelAnimUserDims pfnGet =
        (TFearGameServerGetModelAnimUserDims)GetProcAddress(
            hGameServer, "FearGameServer_GetModelAnimUserDims");
    return pfnGet && pfnGet(
        FearServerHost_ResolveClientObject(pClientObject),
        nAnimation, pDims);
}

bool FearServerHost_GetSocket(
    void *pClientObject, const char *pszName, unsigned int *pSocket)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !pClientObject || !pszName || !pSocket)
        return false;

    TFearGameServerGetSocket pfnGet =
        (TFearGameServerGetSocket)GetProcAddress(hGameServer, "FearGameServer_GetSocket");
    return pfnGet && pfnGet(FearServerHost_ResolveClientObject(pClientObject), pszName, pSocket);
}

bool FearServerHost_GetSocketTransform(
    void *pClientObject, unsigned int nSocket, float *pPosition, float *pRotation, bool bWorldSpace)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !pClientObject || !pPosition || !pRotation)
        return false;

    TFearGameServerGetSocketTransform pfnGet =
        (TFearGameServerGetSocketTransform)GetProcAddress(
            hGameServer, "FearGameServer_GetSocketTransform");
    return pfnGet && pfnGet(
        FearServerHost_ResolveClientObject(pClientObject),
        nSocket, pPosition, pRotation, bWorldSpace ? 1 : 0);
}

bool FearServerHost_GetNode(
    void *pClientObject, const char *pszName, unsigned int *pNode)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !pClientObject || !pszName || !pNode)
        return false;

    TFearGameServerGetNode pfnGet =
        (TFearGameServerGetNode)GetProcAddress(hGameServer, "FearGameServer_GetNode");
    return pfnGet && pfnGet(
        FearServerHost_ResolveClientObject(pClientObject), pszName, pNode);
}

bool FearServerHost_GetNumNodes(void *pClientObject, unsigned int *pNumNodes)
{
    HMODULE hGameServer = FearServerHost_GameServerModule();
    if (!hGameServer || !pClientObject || !pNumNodes)
        return false;

    TFearGameServerGetNumNodes pfnGet =
        (TFearGameServerGetNumNodes)GetProcAddress(hGameServer, "FearGameServer_GetNumNodes");
    return pfnGet && pfnGet(
        FearServerHost_ResolveClientObject(pClientObject), pNumNodes);
}

void FearServerHost_SetMessageSink(TFearServerMessageSink pSink)
{
    g_pfnMessageSink = pSink;
}

void FearServerHost_Shutdown(void)
{
    while (!g_MessageQueue.empty())
    {
        ILTMessage_Read *pMessage = g_MessageQueue.front();
        g_MessageQueue.erase(g_MessageQueue.begin());
        if (pMessage)
            pMessage->Release();
    }
    while (!g_ClientMessageQueue.empty())
    {
        ILTMessage_Read *pMessage = g_ClientMessageQueue.front();
        g_ClientMessageQueue.erase(g_ClientMessageQueue.begin());
        if (pMessage)
            pMessage->Release();
    }
    if (g_pServer)
    {
        if (g_pfnDeleteServer)
            g_pfnDeleteServer();
        g_pServer = NULL;
    }
    g_bServerRunning = false;
    g_bLocalClientEnabled = false;
    g_hServerClient = NULL;
    g_hServerPlayer = NULL;
    g_pfnDeleteServer = NULL;
    g_pfnStringAssign = NULL;
    if (g_hMsvcp71)
    {
        FreeLibrary(g_hMsvcp71);
        g_hMsvcp71 = NULL;
    }
    if (g_hEngineServer)
    {
        FreeLibrary(g_hEngineServer);
        g_hEngineServer = NULL;
    }
}

bool FearServerHost_IsRunning(void)
{
    return g_bServerRunning;
}

void *FearServerHost_GetClientHandle(void)
{
    return g_hServerClient;
}

void *FearServerHost_GetPlayerObject(void)
{
    return g_hServerPlayer;
}

unsigned short FearServerHost_GetPlayerObjectId(void)
{
    if (!g_hServerPlayer)
        return FEAR_INVALID_OBJECT_ID;

    HMODULE hGameServer = GetModuleHandleA("GameServer.dll");
    if (hGameServer)
    {
        TFearGameServerObjectWireId pfnWireId =
            (TFearGameServerObjectWireId)GetProcAddress(hGameServer, "FearGameServer_GetObjectWireId");
        if (pfnWireId)
        {
            unsigned short nWireId = pfnWireId(g_hServerPlayer);
            if (nWireId != FEAR_INVALID_OBJECT_ID)
                return nWireId;
        }
    }

    return *(unsigned short *)((unsigned char *)g_hServerPlayer + 0x54);
}

unsigned short FearServerHost_GetLocalCharacterObjectId(void)
{
    HMODULE hGameServer = GetModuleHandleA("GameServer.dll");
    if (hGameServer)
    {
        TFearGameServerGetLocalCharacterWireId pfnGetId =
            (TFearGameServerGetLocalCharacterWireId)GetProcAddress(
                hGameServer, "FearGameServer_GetLocalCharacterWireId");
        if (pfnGetId)
        {
            unsigned short nCharacterId = pfnGetId();
            if (nCharacterId != FEAR_INVALID_OBJECT_ID)
                return nCharacterId;
        }
    }
    return FearServerHost_GetPlayerObjectId();
}

bool FearServerHost_GetPlayerTransform(float *pPos, float *pRotation)
{
    if (!g_hServerPlayer || !g_hEngineServer)
        return false;

    if (pPos)
    {
        typedef int(__stdcall *TGetObjectPos)(void *, float *);
        TGetObjectPos pfnGetPos = (TGetObjectPos)((unsigned char *)g_hEngineServer + 0x4A780);
        if (pfnGetPos(g_hServerPlayer, pPos) != 0)
            return false;
    }

    if (pRotation)
    {
        typedef int(__stdcall *TGetObjectRotation)(void *, float *);
        TGetObjectRotation pfnGetRotation = (TGetObjectRotation)((unsigned char *)g_hEngineServer + 0x4A7F0);
        if (pfnGetRotation(g_hServerPlayer, pRotation) != 0)
            return false;
    }

    return true;
}

bool FearServerHost_SetPlayerTransform(
    const float *pPos, const float *pVelocity, const float *pRotation)
{
    if (!g_hServerPlayer || !pPos || !g_bServerRunning)
        return false;

    HMODULE hGameServer = GetModuleHandleA("GameServer.dll");
    if (!hGameServer)
        return false;

    TFearGameServerSetPlayerTransform pfnSetTransform =
        (TFearGameServerSetPlayerTransform)GetProcAddress(
            hGameServer, "FearGameServer_SetPlayerTransform");
    return pfnSetTransform && pfnSetTransform(pPos, pVelocity, pRotation) != 0;
}
