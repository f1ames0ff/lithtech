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
                void **pVtable = *(void ***)pShell;
                typedef void(__thiscall *TOnMessage)(void *, void *, ILTMessage_Read *);
                TOnMessage pfnOnMessage = (TOnMessage)pVtable[10];
                pfnOnMessage(pShell, g_hServerClient, pMessage);
                pMessage->Release();
            }
        }
    }

    g_pServer->Update(0);

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
        return 0xFFFF;
    return *(unsigned short *)((unsigned char *)g_hServerPlayer + 0x54);
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
