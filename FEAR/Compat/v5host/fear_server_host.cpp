#include "fear_server_host.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "ltbasedefs.h"
#include "ltguid.h"
#include "server_interface.h"

typedef SI_CREATESTATUS(__cdecl *TFearCreateServer)(int, LTGUID &, ServerInterface **);
typedef void(__cdecl *TFearDeleteServer)();
typedef void(__thiscall *TFearStringCtor)(void *);
typedef void(__thiscall *TFearStringAssign)(void *, const char *);

class CFearServerAppHandler : public ServerAppHandler
{
public:
    LTRESULT ConsoleOutputFn(const char *pMsg) override
    {
        if (pMsg)
        {
            printf("FEARSERVER: %s", pMsg);
            fflush(stdout);
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
static bool g_bLocalClientEnabled = false;

static bool FearServerLocalClientEnabled(void)
{
    char szValue[8] = {0};
    DWORD nLen = GetEnvironmentVariableA("FEAR_LOCAL_CLIENT", szValue, sizeof(szValue));
    return nLen > 0 && szValue[0] != '0';
}

static void FearServerLog(const char *pszText)
{
    printf("FEARSERVER: %s\n", pszText ? pszText : "");
    fflush(stdout);
}

static unsigned int __stdcall FearServerFakeGetClientID(void *)
{
    return 0;
}

static char __stdcall FearServerFakeGetClientData(void *, void *, unsigned int *pSize)
{
    if (pSize)
        *pSize = 0;
    return 0;
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
        pMessage->AddRef();
        g_MessageQueue.push_back(pMessage);
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
        FearServerPatchVtable(g_hEngineServer, 103, (void *)FearServerFakeGetClientData);
        *(uint32_t *)((uintptr_t)g_hEngineServer + 0xF72F0 + 56) = (uint32_t)(uintptr_t)FearServerFakeGetClientID;
        static const unsigned char s_SetClientNameOk[] = {0xB0, 0x01, 0xC3};
        static const unsigned char s_FinishClientConnectOk[] = {0x31, 0xC0, 0xC2, 0x04, 0x00};
        FearServerPatchCode(g_hEngineServer, 0x4B1B0, s_SetClientNameOk, sizeof(s_SetClientNameOk));
        FearServerPatchCode(g_hEngineServer, 0x4BAB0, s_FinishClientConnectOk, sizeof(s_FinishClientConnectOk));
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

    StartGameRequest request;
    memset(&request, 0, sizeof(request));
    request.m_Type = STARTGAME_NORMAL;
    if (pszWorldName && pszWorldName[0])
        strncpy(request.m_WorldName, pszWorldName, sizeof(request.m_WorldName) - 1);
    else
        strncpy(request.m_WorldName, "Worlds\\Release\\Intro.World00p", sizeof(request.m_WorldName) - 1);

    if (!g_pServer->StartWorld(&request))
    {
        wchar_t wszError[1024] = L"";
        g_pServer->GetErrorString(wszError, 1024);
        wprintf(L"FEARSERVER: StartWorld failed: %ls\n", wszError);
        fflush(stdout);
        FearServerHost_Shutdown();
        return false;
    }

    void *pShell = *(void **)((uintptr_t)g_hEngineServer + 0xF911C);
    if (g_bLocalClientEnabled && pShell)
    {
        void **pVtable = *(void ***)pShell;
        typedef void (__thiscall *TOnAddClient)(void *, void *);
        typedef void *(__thiscall *TOnClientEnterWorld)(void *, void *);
        TOnAddClient pfnAddClient = (TOnAddClient)pVtable[15];
        TOnClientEnterWorld pfnEnterWorld = (TOnClientEnterWorld)pVtable[5];
        pfnAddClient(pShell, (void *)1);
        void *pPlayer = pfnEnterWorld(pShell, (void *)1);
        printf("FEARSERVER: local client player=%p\n", pPlayer);
        fflush(stdout);
    }

    g_bServerRunning = true;
    FearServerLog("world started");
    return true;
}

void FearServerHost_Update(void)
{
    if (g_bServerRunning && g_pServer)
        g_pServer->Update(0);
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
    if (g_pServer)
    {
        if (g_pfnDeleteServer)
            g_pfnDeleteServer();
        g_pServer = NULL;
    }
    g_bServerRunning = false;
    g_bLocalClientEnabled = false;
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
