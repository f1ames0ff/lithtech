#include <windows.h>
#include <dbghelp.h>
#include <d3d9.h>
#include <dxgi.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "fear_host_api.h"
#include "ltbasedefs.h"
#include "ltmodule.h"
#include "iclientshell.h"
#include "iltclient.h"
#include "iltcommon.h"
#include "iltmodel.h"
#include "client_filemgr.h"
#include "cshell_filemgr.h"
#include "cshell_object.h"
#include "cshell_render.h"
#include "fear_world.h"

static HINSTANCE g_hThisModule = NULL;
static HMODULE g_hFearHost = NULL;

static int (*g_pfnFearHostInit)(const char*, const FearEngineApi*) = NULL;
static int (*g_pfnFearHostOnEngineInitialized)(const FearMode*, const unsigned char*) = NULL;
static void (*g_pfnFearHostUpdate)(void) = NULL;
static void (*g_pfnFearHostOnKeyDown)(int, int) = NULL;
static void (*g_pfnFearHostOnKeyUp)(int) = NULL;
static void (*g_pfnFearHostOnEvent)(uint32, uint32) = NULL;
static void (*g_pfnFearHostOnMouseMove)(int, int) = NULL;
static void (*g_pfnFearHostOnMouseButton)(int, int, int, int) = NULL;
static void (*g_pfnFearHostOnModelKey)(void *, uint32, void *) = NULL;
static void (*g_pfnFearHostOnEngineTerm)(void) = NULL;

static int g_nFearMouseX = 320;
static int g_nFearMouseY = 240;
static bool g_bFearMouseInitialized = false;

static int FearSdlKeyToVk(SDL_Keycode sym)
{
    if (sym >= SDLK_a && sym <= SDLK_z)
        return (int)(sym - SDLK_a) + 'A';
    if (sym >= SDLK_0 && sym <= SDLK_9)
        return (int)sym;
    switch (sym)
    {
    case SDLK_ESCAPE: return VK_ESCAPE;
    case SDLK_SPACE: return VK_SPACE;
    case SDLK_RETURN: return VK_RETURN;
    case SDLK_TAB: return VK_TAB;
    case SDLK_BACKSPACE: return VK_BACK;
    case SDLK_LSHIFT: return VK_LSHIFT;
    case SDLK_RSHIFT: return VK_RSHIFT;
    case SDLK_LCTRL: return VK_LCONTROL;
    case SDLK_RCTRL: return VK_RCONTROL;
    case SDLK_LALT: return VK_LMENU;
    case SDLK_UP: return VK_UP;
    case SDLK_DOWN: return VK_DOWN;
    case SDLK_LEFT: return VK_LEFT;
    case SDLK_RIGHT: return VK_RIGHT;
    case SDLK_DELETE: return VK_DELETE;
    case SDLK_HOME: return VK_HOME;
    case SDLK_END: return VK_END;
    default: break;
    }
    if (sym > 0 && sym < 128)
        return (int)sym;
    return 0;
}

ILTClient *g_pLTClientV4 = NULL;
define_holder(ILTClient, g_pLTClientV4);
IClientFileMgr *g_pClientFileMgr = NULL;
define_holder(IClientFileMgr, g_pClientFileMgr);
static ILTCommon *g_pCommonClientV4 = NULL;
define_holder_to_instance(ILTCommon, g_pCommonClientV4, Client);
static ILTModel *g_pModelClientV4 = NULL;
define_holder_to_instance(ILTModel, g_pModelClientV4, Client);

static PVOID g_hFearCrashHandler = NULL;
static volatile LONG g_nFearCrashActive = 0;
static volatile LONG g_nFearSymbolsReady = 0;

static void FearGetModuleName(ULONG_PTR nAddress, char *pszName, size_t nNameLen)
{
    if (!pszName || !nNameLen)
        return;
    pszName[0] = '\0';
    HMODULE hModule = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(nAddress), &hModule) && hModule)
    {
        char szPath[MAX_PATH] = {0};
        if (GetModuleFileNameA(hModule, szPath, MAX_PATH))
        {
            const char *pszBase = strrchr(szPath, '\\');
            pszBase = pszBase ? pszBase + 1 : szPath;
            strncpy(pszName, pszBase, nNameLen - 1);
            pszName[nNameLen - 1] = '\0';
            return;
        }
    }
    strncpy(pszName, "<unknown>", nNameLen - 1);
    pszName[nNameLen - 1] = '\0';
}

static PVOID g_hFearWatchHandler = NULL;
static volatile LONG g_nFearWatchActive = 0;

static LONG CALLBACK FearWatchHandler(EXCEPTION_POINTERS *pExceptionInfo)
{
    if (!pExceptionInfo || !pExceptionInfo->ExceptionRecord || !pExceptionInfo->ContextRecord)
        return EXCEPTION_CONTINUE_SEARCH;
    if (pExceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP)
        return EXCEPTION_CONTINUE_SEARCH;

    CONTEXT *pCtx = pExceptionInfo->ContextRecord;
    if (!(pCtx->Dr6 & 1))
        return EXCEPTION_CONTINUE_SEARCH;

    if (InterlockedCompareExchange(&g_nFearWatchActive, 1, 0) != 0)
        return EXCEPTION_CONTINUE_SEARCH;

    unsigned long nValue = 0;
    __try { nValue = *(volatile unsigned long *)0x101A4500; } __except (EXCEPTION_EXECUTE_HANDLER) {}
    ULONG_PTR nReturn = 0;
    if (pCtx->Esp)
    {
        __try { nReturn = *(ULONG_PTR *)pCtx->Esp; } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
    char szModule[MAX_PATH] = {0};
    FearGetModuleName(pCtx->Eip, szModule, sizeof(szModule));
    printf("WATCH pc=%p module=%s ret=%p value=%08lx esp=%p\n",
           reinterpret_cast<void *>(pCtx->Eip), szModule,
           reinterpret_cast<void *>(nReturn), nValue,
           reinterpret_cast<void *>(pCtx->Esp));
    fflush(stdout);

    pCtx->Dr6 = 0;
    InterlockedExchange(&g_nFearWatchActive, 0);
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void FearInstallWatchpoint(void)
{
    CONTEXT ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    HANDLE hThread = GetCurrentThread();
    if (!GetThreadContext(hThread, &ctx))
    {
        printf("WATCH: GetThreadContext failed %lu\n", GetLastError());
        return;
    }
    ctx.Dr0 = 0x101A4500;
    ctx.Dr1 = 0;
    ctx.Dr2 = 0;
    ctx.Dr3 = 0;
    ctx.Dr7 = (ctx.Dr7 & ~0xF0003ul) | 0x1ul | (0x1ul << 16) | (0x3ul << 18);
    if (!SetThreadContext(hThread, &ctx))
    {
        printf("WATCH: SetThreadContext failed %lu\n", GetLastError());
        return;
    }
    printf("WATCH: installed on 0x101A4500\n");
}

static volatile LONG g_nFearGwStop = 0;

static DWORD WINAPI FearGlobalWatchThread(LPVOID)
{
    unsigned long nLast = 0xFFFFFFFFul;
    while (!g_nFearGwStop)
    {
        unsigned long nValue = 0;
        __try { nValue = *(volatile unsigned long *)0x101A4500; } __except (EXCEPTION_EXECUTE_HANDLER) { nValue = 0; }
        if (nValue != nLast)
        {
            printf("GWATCH 101A4500=%08lx\n", nValue);
            fflush(stdout);
            nLast = nValue;
        }
        Sleep(1);
    }
    return 0;
}

static void FearPatchGameClientGates(void)
{
    struct FearPatchByte { unsigned char *m_pAddress; unsigned char m_nExpected; unsigned char m_nReplace; };
    static const FearPatchByte s_Patches[] =
    {
        { reinterpret_cast<unsigned char *>(0x10057E5A), 0x74, 0x90 },
        { reinterpret_cast<unsigned char *>(0x10057E5B), 0x24, 0x90 },
        { reinterpret_cast<unsigned char *>(0x10057E65), 0x75, 0xEB },
        { reinterpret_cast<unsigned char *>(0x10057F05), 0x74, 0xEB },
        { reinterpret_cast<unsigned char *>(0x10057FE1), 0x75, 0xEB },
        { reinterpret_cast<unsigned char *>(0x10057881), 0x75, 0xEB },
        { reinterpret_cast<unsigned char *>(0x10057911), 0x75, 0xEB },
        { reinterpret_cast<unsigned char *>(0x1005802B), 0x75, 0xEB },
        { reinterpret_cast<unsigned char *>(0x10058074), 0x74, 0x90 },
        { reinterpret_cast<unsigned char *>(0x10058075), 0x34, 0x90 },
        { reinterpret_cast<unsigned char *>(0x1005809E), 0x75, 0xEB },
        { reinterpret_cast<unsigned char *>(0x100580E5), 0x75, 0xEB },
    };

    for (size_t i = 0; i < sizeof(s_Patches) / sizeof(s_Patches[0]); i++)
    {
        unsigned char *pAddress = s_Patches[i].m_pAddress;
        DWORD nOldProtect = 0;
        if (!VirtualProtect(pAddress, 1, PAGE_EXECUTE_READWRITE, &nOldProtect))
            continue;
        if (*pAddress == s_Patches[i].m_nExpected)
        {
            *pAddress = s_Patches[i].m_nReplace;
            printf("PATCH: %p %02x -> %02x\n", pAddress, s_Patches[i].m_nExpected, s_Patches[i].m_nReplace);
        }
        DWORD nTmp = 0;
        VirtualProtect(pAddress, 1, nOldProtect, &nTmp);
    }

    unsigned char *pClientFxChecks[] = { reinterpret_cast<unsigned char *>(0x1005815E), reinterpret_cast<unsigned char *>(0x100581AA) };
    for (size_t i = 0; i < sizeof(pClientFxChecks) / sizeof(pClientFxChecks[0]); i++)
    {
        unsigned char *pAddress = pClientFxChecks[i];
        DWORD nOldProtect = 0;
        if (!VirtualProtect(pAddress, 6, PAGE_EXECUTE_READWRITE, &nOldProtect))
            continue;
        if (pAddress[0] == 0x0F && pAddress[1] == 0x84)
        {
            memset(pAddress, 0x90, 6);
            printf("PATCH: nop clientfx check %p\n", pAddress);
        }
        DWORD nTmp = 0;
        VirtualProtect(pAddress, 6, nOldProtect, &nTmp);
    }
}

static void FearEnsureSymbols(void)
{
    if (InterlockedCompareExchange(&g_nFearSymbolsReady, 1, 0) != 0)
        return;
    if (!SymInitialize(GetCurrentProcess(), NULL, TRUE))
        InterlockedExchange(&g_nFearSymbolsReady, 2);
}

static void FearPrintStackFrame(DWORD nFrame, ULONG_PTR nPC)
{
    char szModule[MAX_PATH] = {0};
    FearGetModuleName(nPC, szModule, sizeof(szModule));

    char szSymbol[512] = {0};
    char szSymbolBuffer[sizeof(SYMBOL_INFO) + 256] = {0};
    SYMBOL_INFO *pSymbol = reinterpret_cast<SYMBOL_INFO *>(szSymbolBuffer);
    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = 255;
    DWORD64 nDisplacement = 0;
    if (SymFromAddr(GetCurrentProcess(), nPC, &nDisplacement, pSymbol))
        _snprintf_s(szSymbol, sizeof(szSymbol), _TRUNCATE, "%s+0x%llx", pSymbol->Name, nDisplacement);

    printf("  #%02lu pc=%p module=%s %s\n", (unsigned long)nFrame, reinterpret_cast<void *>(nPC), szModule, szSymbol);
    fflush(stdout);
}

static void FearPrintCallStack(EXCEPTION_POINTERS *pExceptionInfo)
{
    CONTEXT context;
    memcpy(&context, pExceptionInfo->ContextRecord, sizeof(context));

    STACKFRAME64 frame;
    memset(&frame, 0, sizeof(frame));
    frame.AddrPC.Offset = context.Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrStack.Mode = AddrModeFlat;

    for (DWORD i = 0; i < 20; i++)
    {
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(), &frame, &context,
                         NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
            break;
        if (!frame.AddrPC.Offset)
            break;
        FearPrintStackFrame(i, (ULONG_PTR)frame.AddrPC.Offset);
    }
}

static void __cdecl FearAbortHandler(int)
{
    static volatile LONG s_bAbortActive = 0;
    if (InterlockedCompareExchange(&s_bAbortActive, 1, 0) != 0)
        return;

    void *frames[32] = {0};
    USHORT nFrames = CaptureStackBackTrace(0, 32, frames, NULL);
    printf("FEARABORT: SIGABRT frames=%u\n", (unsigned)nFrames);
    for (USHORT i = 0; i < nFrames; ++i)
    {
        ULONG_PTR nPC = reinterpret_cast<ULONG_PTR>(frames[i]);
        char szModule[MAX_PATH] = {0};
        FearGetModuleName(nPC, szModule, sizeof(szModule));
        HMODULE hModule = szModule[0] ? GetModuleHandleA(szModule) : NULL;
        ULONG_PTR nRva = hModule ? nPC - reinterpret_cast<ULONG_PTR>(hModule) : 0;
        printf("ABORT %02u pc=%p module=%s base=%p rva=%p\n",
               (unsigned)i, reinterpret_cast<void *>(nPC), szModule,
               reinterpret_cast<void *>(hModule), reinterpret_cast<void *>(nRva));
    }
    fflush(stdout);
    signal(SIGABRT, SIG_DFL);
}

static LONG CALLBACK FearCrashHandler(EXCEPTION_POINTERS *pExceptionInfo)
{
    if (!pExceptionInfo || !pExceptionInfo->ExceptionRecord || !pExceptionInfo->ContextRecord)
        return EXCEPTION_CONTINUE_SEARCH;

    const DWORD nCode = pExceptionInfo->ExceptionRecord->ExceptionCode;
    if (nCode != EXCEPTION_ACCESS_VIOLATION && nCode != EXCEPTION_ILLEGAL_INSTRUCTION)
        return EXCEPTION_CONTINUE_SEARCH;

    if (InterlockedCompareExchange(&g_nFearCrashActive, 1, 0) != 0)
        return EXCEPTION_CONTINUE_SEARCH;

    char szModule[MAX_PATH] = {0};
    FearGetModuleName(reinterpret_cast<ULONG_PTR>(pExceptionInfo->ExceptionRecord->ExceptionAddress),
                      szModule, sizeof(szModule));
    const ULONG_PTR nReadWrite = pExceptionInfo->ExceptionRecord->NumberParameters > 0 ? pExceptionInfo->ExceptionRecord->ExceptionInformation[0] : 0;
    const ULONG_PTR nDataAddress = pExceptionInfo->ExceptionRecord->NumberParameters > 1 ? pExceptionInfo->ExceptionRecord->ExceptionInformation[1] : 0;
    printf("CRASH code=%08lx address=%p instr=%p esp=%p ebp=%p module=%s rw=%lu data=%p\n",
           static_cast<unsigned long>(nCode),
           pExceptionInfo->ExceptionRecord->ExceptionAddress,
           reinterpret_cast<void *>(pExceptionInfo->ContextRecord->Eip),
           reinterpret_cast<void *>(pExceptionInfo->ContextRecord->Esp),
           reinterpret_cast<void *>(pExceptionInfo->ContextRecord->Ebp),
           szModule,
           static_cast<unsigned long>(nReadWrite),
           reinterpret_cast<void *>(nDataAddress));
    ULONG_PTR nReturn = 0;
    if (pExceptionInfo->ContextRecord->Esp)
        nReturn = *reinterpret_cast<ULONG_PTR *>(pExceptionInfo->ContextRecord->Esp);
    char szReturnModule[MAX_PATH] = {0};
    FearGetModuleName(nReturn, szReturnModule, sizeof(szReturnModule));
    HMODULE hReturnModule = szReturnModule[0] ? GetModuleHandleA(szReturnModule) : NULL;
    printf("CRASH return=%p module=%s base=%p rva=%p\n", reinterpret_cast<void *>(nReturn), szReturnModule,
           reinterpret_cast<void *>(hReturnModule),
           reinterpret_cast<void *>(hReturnModule ? (nReturn - reinterpret_cast<ULONG_PTR>(hReturnModule)) : 0));
    {
        CONTEXT *pCtx = pExceptionInfo->ContextRecord;
        unsigned long nSlot54 = 0;
        __try { nSlot54 = *(volatile unsigned long *)(pCtx->Edx + 0x54); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        unsigned long nObj = 0;
        __try { nObj = *(volatile unsigned long *)(pCtx->Esi + 0x5418); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        char szEdxModule[MAX_PATH] = {0};
        char szObjModule[MAX_PATH] = {0};
        FearGetModuleName(pCtx->Edx, szEdxModule, sizeof(szEdxModule));
        FearGetModuleName(nObj, szObjModule, sizeof(szObjModule));
        printf("REGS eax=%p ebx=%p ecx=%p edx=%p esi=%p edi=%p ebp=%p\n",
               (void *)pCtx->Eax, (void *)pCtx->Ebx, (void *)pCtx->Ecx, (void *)pCtx->Edx,
               (void *)pCtx->Esi, (void *)pCtx->Edi, (void *)pCtx->Ebp);
        printf("VTBL edx=%p module=%s slot54=%p obj5418=%p objmodule=%s\n",
               (void *)pCtx->Edx, szEdxModule, (void *)nSlot54, (void *)nObj, szObjModule);
    }
    unsigned long nClientPtr = 0;
    __try { nClientPtr = *(volatile unsigned long *)0x101A5418; } __except (EXCEPTION_EXECUTE_HANDLER) {}
    printf("CLIENTPTR %08lx\n", nClientPtr);
    if (nClientPtr > 0x1000)
    {
        for (int i = 0; i < 24; i++)
        {
            unsigned long nValue = 0;
            __try { nValue = *(volatile unsigned long *)(nClientPtr + i * 4); } __except (EXCEPTION_EXECUTE_HANDLER) { break; }
            printf("CLIENT[%02d] %08lx\n", i, nValue);
        }
    }
    if (pExceptionInfo->ContextRecord->Esp)
    {
        ULONG_PTR *pStack = reinterpret_cast<ULONG_PTR *>(pExceptionInfo->ContextRecord->Esp);
        for (int i = 0; i < 20; i++)
        {
            ULONG_PTR nValue = 0;
            __try { nValue = pStack[i]; } __except (EXCEPTION_EXECUTE_HANDLER) { break; }
            char szValueModule[MAX_PATH] = {0};
            FearGetModuleName(nValue, szValueModule, sizeof(szValueModule));
            printf("STACK %02d %p module=%s\n", i, reinterpret_cast<void *>(nValue), szValueModule);
        }
    }
    fflush(stdout);

    FearEnsureSymbols();
    FearPrintCallStack(pExceptionInfo);

    InterlockedExchange(&g_nFearCrashActive, 0);
    return EXCEPTION_CONTINUE_SEARCH;
}

static int EngineSetRenderMode(const FearMode *pMode)
{
    if (!g_pLTClientV4 || !pMode)
        return 0;
    RMode mode;
    memset(&mode, 0, sizeof(mode));
    mode.m_Width = pMode->m_Width;
    mode.m_Height = pMode->m_Height;
    mode.m_BitDepth = pMode->m_BitDepth;
    mode.m_bHWTnL = (pMode->m_bHWTnL != 0);
    strncpy(mode.m_InternalName, pMode->m_DeviceName, sizeof(mode.m_InternalName) - 1);
    FearWorld_Shutdown();
    return (g_pLTClientV4->SetRenderMode(&mode, NULL) == LT_OK) ? 1 : 0;
}

static int EngineGetRenderMode(FearMode *pMode)
{
    if (!g_pLTClientV4 || !pMode)
        return 0;
    RMode mode;
    memset(&mode, 0, sizeof(mode));
    if (g_pLTClientV4->GetRenderMode(&mode) != LT_OK)
        return 0;
    pMode->m_Width = mode.m_Width;
    pMode->m_Height = mode.m_Height;
    pMode->m_BitDepth = mode.m_BitDepth;
    pMode->m_bHWTnL = mode.m_bHWTnL ? 1 : 0;
    strncpy(pMode->m_DeviceName, mode.m_InternalName, sizeof(pMode->m_DeviceName) - 1);
    return 1;
}

static void EngineShutdownRender(uint32_t flags)
{
    if (g_pLTClientV4)
        g_pLTClientV4->ShutdownRender(flags);
}

static void *EngineGetEngineHook(const char *pszName)
{
    if (!g_pLTClientV4)
        return NULL;
    void *pValue = NULL;
    if (g_pLTClientV4->GetEngineHook(pszName, &pValue) != LT_OK)
        return NULL;
    return pValue;
}

static int EngineSetObjectFlags(void *hObject, uint32_t nFlagType, uint32_t nFlags, uint32_t nMask)
{
    if (!hObject || !g_pCommonClientV4 || nFlagType > OFT_Client)
        return 0;
    return g_pCommonClientV4->SetObjectFlags(
        (HOBJECT)hObject, (ObjFlagType)nFlagType, nFlags, nMask) == LT_OK;
}

static int EngineGetObjectFlags(void *hObject, uint32_t nFlagType, uint32_t *pFlags)
{
    if (!hObject || !pFlags || !g_pCommonClientV4 || nFlagType > OFT_Client)
        return 0;
    return g_pCommonClientV4->GetObjectFlags(
        (HOBJECT)hObject, (ObjFlagType)nFlagType, *pFlags) == LT_OK;
}

static bool EngineModelObjectIsValid(void *hObject, const char *pszOperation)
{
    if (!hObject || !g_pCommonClientV4)
        return false;

    uint32 nObjectType = 0;
    LTRESULT nResult = g_pCommonClientV4->GetObjectType(
        (HOBJECT)hObject, &nObjectType);
    if (nResult != LT_OK || nObjectType != OT_MODEL)
    {
        static int s_nInvalidModelLogs = 0;
        if (s_nInvalidModelLogs < 32)
        {
            ++s_nInvalidModelLogs;
            printf("FEARMODEL: engine %s rejected obj=%p type=%u result=%d\n",
                pszOperation, hObject, (unsigned)nObjectType, (int)nResult);
            fflush(stdout);
        }
        return false;
    }
    return true;
}

static int EngineModelSetCurAnim(
    void *hObject, uint32_t nTracker, uint32_t nAnim, int bInterpolate)
{
    if (!EngineModelObjectIsValid(hObject, "SetCurAnim") || !g_pModelClientV4)
        return 0;
    return g_pModelClientV4->SetCurAnim(
        (HOBJECT)hObject, (ANIMTRACKERID)nTracker, (HMODELANIM)nAnim) == LT_OK;
}

static int EngineModelSetPlaying(void *hObject, uint32_t nTracker, int bPlaying)
{
    if (!EngineModelObjectIsValid(hObject, "SetPlaying") || !g_pModelClientV4)
        return 0;
    return g_pModelClientV4->SetPlaying(
        (HOBJECT)hObject, (ANIMTRACKERID)nTracker, bPlaying != 0) == LT_OK;
}

static int EngineModelSetLooping(void *hObject, uint32_t nTracker, int bLooping)
{
    if (!EngineModelObjectIsValid(hObject, "SetLooping") || !g_pModelClientV4)
        return 0;
    return g_pModelClientV4->SetLooping(
        (HOBJECT)hObject, (ANIMTRACKERID)nTracker, bLooping != 0) == LT_OK;
}

static int EngineModelSetCurAnimTime(void *hObject, uint32_t nTracker, uint32_t nTime)
{
    if (!EngineModelObjectIsValid(hObject, "SetCurAnimTime") || !g_pModelClientV4)
        return 0;
    return g_pModelClientV4->SetCurAnimTime(
        (HOBJECT)hObject, (ANIMTRACKERID)nTracker, nTime) == LT_OK;
}

static int EngineModelResetAnim(void *hObject, uint32_t nTracker)
{
    if (!EngineModelObjectIsValid(hObject, "ResetAnim") || !g_pModelClientV4)
        return 0;
    return g_pModelClientV4->ResetAnim(
        (HOBJECT)hObject, (ANIMTRACKERID)nTracker) == LT_OK;
}

static int EngineModelGetCurAnim(void *hObject, uint32_t nTracker, uint32_t *pAnim)
{
    if (!pAnim || !EngineModelObjectIsValid(hObject, "GetCurAnim") || !g_pModelClientV4)
        return 0;
    HMODELANIM nAnim = INVALID_MODEL_ANIM;
    LTRESULT nResult = g_pModelClientV4->GetCurAnim(
        (HOBJECT)hObject, (ANIMTRACKERID)nTracker, nAnim);
    *pAnim = (uint32_t)nAnim;
    return nResult == LT_OK;
}

static int EngineModelGetCurAnimLength(void *hObject, uint32_t nTracker, uint32_t *pLength)
{
    if (!pLength || !EngineModelObjectIsValid(hObject, "GetCurAnimLength") ||
        !g_pModelClientV4)
        return 0;
    return g_pModelClientV4->GetCurAnimLength(
        (HOBJECT)hObject, (ANIMTRACKERID)nTracker, *pLength) == LT_OK;
}

static int EngineModelGetCurAnimTime(void *hObject, uint32_t nTracker, uint32_t *pTime)
{
    if (!pTime || !EngineModelObjectIsValid(hObject, "GetCurAnimTime") ||
        !g_pModelClientV4)
        return 0;
    return g_pModelClientV4->GetCurAnimTime(
        (HOBJECT)hObject, (ANIMTRACKERID)nTracker, *pTime) == LT_OK;
}

static int EngineModelGetPlaying(void *hObject, uint32_t nTracker)
{
    if (!EngineModelObjectIsValid(hObject, "GetPlaying") || !g_pModelClientV4)
        return 0;
    return g_pModelClientV4->GetPlaying(
        (HOBJECT)hObject, (ANIMTRACKERID)nTracker) == LT_YES;
}

static int EngineModelGetLooping(void *hObject, uint32_t nTracker)
{
    if (!EngineModelObjectIsValid(hObject, "GetLooping") || !g_pModelClientV4)
        return 0;
    return g_pModelClientV4->GetLooping(
        (HOBJECT)hObject, (ANIMTRACKERID)nTracker) == LT_YES;
}

static int EngineModelSetAnimRate(void *hObject, uint32_t nTracker, float fRate)
{
    if (!EngineModelObjectIsValid(hObject, "SetAnimRate") || !g_pModelClientV4)
        return 0;
    return g_pModelClientV4->SetAnimRate(
        (HOBJECT)hObject, (ANIMTRACKERID)nTracker, fRate) == LT_OK;
}

static int EngineModelGetAnimRate(void *hObject, uint32_t nTracker, float *pRate)
{
    if (!pRate || !EngineModelObjectIsValid(hObject, "GetAnimRate") ||
        !g_pModelClientV4)
        return 0;
    return g_pModelClientV4->GetAnimRate(
        (HOBJECT)hObject, (ANIMTRACKERID)nTracker, *pRate) == LT_OK;
}

static int EngineModelGetAnimIndex(void *hObject, const char *pszName, uint32_t *pAnim)
{
    if (!pszName || !pAnim || !EngineModelObjectIsValid(hObject, "GetAnimIndex") ||
        !g_pModelClientV4)
        return 0;
    return g_pModelClientV4->GetAnimIndex(
        (HOBJECT)hObject, pszName, *pAnim) == LT_OK;
}

static int EngineGetRenderModes(FearMode *pModes, int nMaxModes)
{
    if (!g_pLTClientV4)
        return -1;

    RMode *pHead = g_pLTClientV4->GetRenderModes();
    int nCount = 0;
    for (RMode *pMode = pHead; pMode; pMode = pMode->m_pNext)
    {
        if (pModes && nCount < nMaxModes)
        {
            FearMode &mode = pModes[nCount];
            memset(&mode, 0, sizeof(mode));
            mode.m_Width = pMode->m_Width;
            mode.m_Height = pMode->m_Height;
            mode.m_BitDepth = pMode->m_BitDepth;
            mode.m_bHWTnL = pMode->m_bHWTnL ? 1 : 0;
            strncpy(mode.m_DeviceName, pMode->m_InternalName, sizeof(mode.m_DeviceName) - 1);
        }
        ++nCount;
    }

    g_pLTClientV4->RelinquishRenderModes(pHead);
    return nCount;
}

static int EngineQueryPerformanceStats(FearPerformanceStats *pStats, int bQuick)
{
    (void)bQuick;
    if (!pStats)
        return 0;

    static FearPerformanceStats s_Stats;
    static bool s_bStatsInitialized = false;
    if (!s_bStatsInitialized)
    {
        s_bStatsInitialized = true;
        memset(&s_Stats, 0, sizeof(s_Stats));

        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        s_Stats.m_nCPUCount = systemInfo.dwNumberOfProcessors;

        MEMORYSTATUSEX memoryStatus;
        memset(&memoryStatus, 0, sizeof(memoryStatus));
        memoryStatus.dwLength = sizeof(memoryStatus);
        if (GlobalMemoryStatusEx(&memoryStatus))
            s_Stats.m_nCPUMemory = (uint32_t)(memoryStatus.ullTotalPhys / (1024ull * 1024ull));

        DWORD nMHz = 0;
        DWORD nMHzSize = sizeof(nMHz);
        if (RegGetValueA(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "~MHz",
            RRF_RT_REG_DWORD, NULL, &nMHz, &nMHzSize) == ERROR_SUCCESS)
            s_Stats.m_fCPUFrequency = (float)nMHz / 1000.0f;
        if (s_Stats.m_fCPUFrequency <= 0.0f)
            s_Stats.m_fCPUFrequency = 1.0f;

        IDirect3D9 *pD3D = Direct3DCreate9(D3D_SDK_VERSION);
        if (pD3D)
        {
            D3DCAPS9 caps;
            memset(&caps, 0, sizeof(caps));
            HRESULT nCapsResult = pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
            if (SUCCEEDED(nCapsResult))
            {
                s_Stats.m_fGPUMaxPS = (float)((caps.PixelShaderVersion >> 8) & 0xFF) +
                    (float)(caps.PixelShaderVersion & 0xFF) / 100.0f;
                s_Stats.m_fGPUMaxVS = (float)((caps.VertexShaderVersion >> 8) & 0xFF) +
                    (float)(caps.VertexShaderVersion & 0xFF) / 100.0f;
            }
            else
            {
                printf("FEARPERF: D3D9 GetDeviceCaps failed hr=0x%08lx\n", (unsigned long)nCapsResult);
            }
            pD3D->Release();
        }

        IDXGIFactory1 *pFactory = NULL;
        if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)&pFactory)) && pFactory)
        {
            unsigned long long nBestMemory = 0;
            for (UINT i = 0;; ++i)
            {
                IDXGIAdapter1 *pAdapter = NULL;
                if (pFactory->EnumAdapters1(i, &pAdapter) == DXGI_ERROR_NOT_FOUND)
                    break;
                if (!pAdapter)
                    continue;

                DXGI_ADAPTER_DESC1 desc;
                memset(&desc, 0, sizeof(desc));
                if (SUCCEEDED(pAdapter->GetDesc1(&desc)))
                {
                    unsigned long long nMemory = desc.DedicatedVideoMemory;
                    if (nMemory == 0)
                        nMemory = desc.SharedSystemMemory;
                    if (nMemory > nBestMemory)
                        nBestMemory = nMemory;
                }
                pAdapter->Release();
            }
            pFactory->Release();
            s_Stats.m_nGPUMemory = (uint32_t)(nBestMemory / (1024ull * 1024ull));
        }

        if (s_Stats.m_fGPUMaxPS <= 0.0f || s_Stats.m_fGPUMaxVS <= 0.0f)
        {
            s_Stats.m_fGPUMaxPS = 3.0f;
            s_Stats.m_fGPUMaxVS = 3.0f;
            printf("FEARPERF: D3D9 shader caps unavailable; using D3D9 feature baseline\n");
        }

        if (s_Stats.m_nGPUMemory == 0)
            s_Stats.m_nGPUMemory = 1024;
        if (s_Stats.m_nGPUMemory >= 4096)
            s_Stats.m_fGPUPS11Fill = s_Stats.m_fGPUPS20Fill = 20000.0f;
        else if (s_Stats.m_nGPUMemory >= 2048)
            s_Stats.m_fGPUPS11Fill = s_Stats.m_fGPUPS20Fill = 10000.0f;
        else if (s_Stats.m_nGPUMemory >= 1024)
            s_Stats.m_fGPUPS11Fill = s_Stats.m_fGPUPS20Fill = 5000.0f;
        else
            s_Stats.m_fGPUPS11Fill = s_Stats.m_fGPUPS20Fill = 2000.0f;

        printf("FEARPERF: detected cpu=%.2fGHz cpus=%u ram=%uMB ps=%.2f vs=%.2f gpu=%uMB\n",
            s_Stats.m_fCPUFrequency, s_Stats.m_nCPUCount, s_Stats.m_nCPUMemory,
            s_Stats.m_fGPUMaxPS, s_Stats.m_fGPUMaxVS, s_Stats.m_nGPUMemory);
        fflush(stdout);
    }

    *pStats = s_Stats;
    return 1;
}

static int EngineIntersectWorld(const float *pFrom, const float *pTo, float *pHit, float *pNormal, float *pDist)
{
    return FearWorld_IntersectSegment(pFrom, pTo, pHit, pNormal, pDist);
}

static const FearEngineApi g_EngineApi =
{
    EngineSetRenderMode,
    EngineGetRenderMode,
    EngineShutdownRender,
    EngineGetEngineHook,
    NULL,
    FearFile_Open,
    FearFile_Read,
    FearFile_Seek,
    FearFile_Tell,
    FearFile_Size,
    FearFile_Close,
    FearFile_List,
    FearFile_FreeList,
    FearObject_Create,
    &g_FearRenderApi,
    EngineIntersectWorld,
    EngineGetRenderModes,
    EngineQueryPerformanceStats,
    EngineSetObjectFlags,
    EngineGetObjectFlags,
    EngineModelSetCurAnim,
    EngineModelSetPlaying,
    EngineModelSetLooping,
    EngineModelSetCurAnimTime,
    EngineModelResetAnim,
    EngineModelGetCurAnim,
    EngineModelGetCurAnimLength,
    EngineModelGetCurAnimTime,
    EngineModelGetPlaying,
    EngineModelGetLooping,
    EngineModelSetAnimRate,
    EngineModelGetAnimRate,
    EngineModelGetAnimIndex,
    NULL
};

static bool LoadFearHost(void)
{
    if (g_hFearHost)
        return true;
    char szPath[MAX_PATH] = {0};
    if (!GetModuleFileNameA(NULL, szPath, MAX_PATH))
        return false;
    char *pSlash = strrchr(szPath, '\\');
    if (!pSlash)
        return false;
    strcpy(pSlash + 1, "FearHost.dll");
    g_hFearHost = LoadLibraryA(szPath);
    if (!g_hFearHost)
    {
        printf("FEAR_PORT: FearHost.dll load failed (%lu)\n", GetLastError());
        return false;
    }
    g_pfnFearHostInit = (int (*)(const char*, const FearEngineApi*))GetProcAddress(g_hFearHost, "FearHost_Init");
    g_pfnFearHostOnEngineInitialized = (int (*)(const FearMode*, const unsigned char*))GetProcAddress(g_hFearHost, "FearHost_OnEngineInitialized");
    g_pfnFearHostUpdate = (void (*)(void))GetProcAddress(g_hFearHost, "FearHost_Update");
    g_pfnFearHostOnKeyDown = (void (*)(int, int))GetProcAddress(g_hFearHost, "FearHost_OnKeyDown");
    g_pfnFearHostOnKeyUp = (void (*)(int))GetProcAddress(g_hFearHost, "FearHost_OnKeyUp");
    g_pfnFearHostOnEvent = (void (*)(uint32, uint32))GetProcAddress(g_hFearHost, "FearHost_OnEvent");
    g_pfnFearHostOnMouseMove = (void (*)(int, int))GetProcAddress(g_hFearHost, "FearHost_OnMouseMove");
    g_pfnFearHostOnMouseButton = (void (*)(int, int, int, int))GetProcAddress(g_hFearHost, "FearHost_OnMouseButton");
    g_pfnFearHostOnModelKey = (void (*)(void *, uint32, void *))GetProcAddress(g_hFearHost, "FearHost_OnModelKey");
    g_pfnFearHostOnEngineTerm = (void (*)(void))GetProcAddress(g_hFearHost, "FearHost_OnEngineTerm");
    return g_pfnFearHostInit && g_pfnFearHostOnEngineInitialized && g_pfnFearHostUpdate &&
        g_pfnFearHostOnEvent;
}

static bool FearRetailAbiEnabled(void)
{
    char szValue[16] = {0};
    DWORD nLen = GetEnvironmentVariableA("FEAR_RETAIL_ABI", szValue, sizeof(szValue));
    return nLen > 0 && szValue[0] != '0';
}

static bool ExtractGameClient(char *pszPath, uint32 nPathLen)
{
    char szLocalPath[MAX_PATH] = {0};
    if (GetCurrentDirectoryA(MAX_PATH, szLocalPath))
    {
        if (szLocalPath[0] && szLocalPath[strlen(szLocalPath) - 1] != '\\')
            strncat(szLocalPath, "\\", sizeof(szLocalPath) - strlen(szLocalPath) - 1);
        strncat(szLocalPath, "GameClient.dll", sizeof(szLocalPath) - strlen(szLocalPath) - 1);
        if (GetFileAttributesA(szLocalPath) != INVALID_FILE_ATTRIBUTES)
        {
            strncpy(pszPath, szLocalPath, nPathLen - 1);
            printf("FEAR_PORT: using local GameClient.dll %s\n", szLocalPath);
            return true;
        }
    }

    char szTempPath[MAX_PATH] = {0};
    char szTempFile[MAX_PATH] = {0};
    if (!GetTempPathA(MAX_PATH, szTempPath))
        return false;
    if (!GetTempFileNameA(szTempPath, "fear", 0, szTempFile))
        return false;
    bool bCopied = false;
    if (g_pClientFileMgr)
        bCopied = (g_pClientFileMgr->CopyFile("GameClient.dll", szTempFile) == LT_OK);
    if (!bCopied)
        bCopied = CopyFileA("GameClient.dll", szTempFile, FALSE) != FALSE;
    if (!bCopied)
        return false;
    strncpy(pszPath, szTempFile, nPathLen - 1);
    return true;
}

class CFearShellProxy : public IClientShellStub
{
public:
    declare_interface(CFearShellProxy)

    LTRESULT OnEngineInitialized(RMode *pMode, LTGUID *pAppGuid) override
    {
        if (!LoadFearHost())
            return LT_ERROR;
        char szGameClient[MAX_PATH] = {0};
        if (!ExtractGameClient(szGameClient, MAX_PATH))
        {
            printf("FEAR_PORT: GameClient.dll extraction failed\n");
            return LT_ERROR;
        }
        if (!g_pfnFearHostInit(szGameClient, &g_EngineApi))
        {
            printf("FEAR_PORT: FearHost_Init failed\n");
            return LT_ERROR;
        }
        if (FearRetailAbiEnabled())
        {
            FearInstallWatchpoint();
            FearPatchGameClientGates();
            g_nFearGwStop = 0;
            CreateThread(NULL, 0, FearGlobalWatchThread, NULL, 0, NULL);
        }
        FearMode mode;
        memset(&mode, 0, sizeof(mode));
        mode.m_Width = pMode->m_Width;
        mode.m_Height = pMode->m_Height;
        mode.m_BitDepth = pMode->m_BitDepth;
        mode.m_bHWTnL = pMode->m_bHWTnL ? 1 : 0;
        strncpy(mode.m_DeviceName, pMode->m_InternalName, sizeof(mode.m_DeviceName) - 1);
        unsigned char guid[16] = {0};
        if (pAppGuid)
            memcpy(guid, pAppGuid, sizeof(guid));
        int nResult = g_pfnFearHostOnEngineInitialized(&mode, guid);
        printf("FEAR_PORT: OnEngineInitialized returned %d\n", nResult);
        return (LTRESULT)nResult;
    }

    void Update() override
    {
        if (g_pfnFearHostUpdate)
            g_pfnFearHostUpdate();
    }

    void OnKeyDown(int key, int rep) override
    {
        if (g_pfnFearHostOnKeyDown)
            g_pfnFearHostOnKeyDown(key, rep);
    }

    void OnKeyUp(int key) override
    {
        if (g_pfnFearHostOnKeyUp)
            g_pfnFearHostOnKeyUp(key);
    }

    void OnModelKey(HLOCALOBJ hObj, ArgList *pArgs) override
    {
        if (g_pfnFearHostOnModelKey)
            g_pfnFearHostOnModelKey(hObj, MAIN_TRACKER, pArgs);
    }

    void OnEvent(uint32 dwEventID, uint32 dwParam) override
    {
        printf("FEARPORT: OnEvent id=%u param=%u\n", (unsigned)dwEventID, (unsigned)dwParam);
        fflush(stdout);
        if (g_pfnFearHostOnEvent)
            g_pfnFearHostOnEvent(dwEventID, dwParam);
    }

    void HandleEvent(SDL_Event e) override
    {
        if (e.type == SDL_KEYDOWN)
        {
            int nKey = FearSdlKeyToVk(e.key.keysym.sym);
            if (nKey != 0 && g_pfnFearHostOnKeyDown)
                g_pfnFearHostOnKeyDown(nKey, e.key.repeat ? 1 : 0);
        }
        else if (e.type == SDL_KEYUP)
        {
            int nKey = FearSdlKeyToVk(e.key.keysym.sym);
            if (nKey != 0 && g_pfnFearHostOnKeyUp)
                g_pfnFearHostOnKeyUp(nKey);
        }
        else if (e.type == SDL_MOUSEMOTION)
        {
            if (!g_bFearMouseInitialized)
            {
                g_bFearMouseInitialized = true;
                g_nFearMouseX = 320;
                g_nFearMouseY = 240;
            }
            else
            {
                g_nFearMouseX += e.motion.xrel;
                g_nFearMouseY += e.motion.yrel;
            }
            if (g_nFearMouseX < 0)
                g_nFearMouseX = 0;
            if (g_nFearMouseX > 639)
                g_nFearMouseX = 639;
            if (g_nFearMouseY < 0)
                g_nFearMouseY = 0;
            if (g_nFearMouseY > 479)
                g_nFearMouseY = 479;
            if (g_pfnFearHostOnMouseMove)
                g_pfnFearHostOnMouseMove(g_nFearMouseX, g_nFearMouseY);
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
        {
            int nButton = (e.button.button == SDL_BUTTON_LEFT) ? 0 : ((e.button.button == SDL_BUTTON_RIGHT) ? 1 : 2);
            int nDown = (e.type == SDL_MOUSEBUTTONDOWN) ? 1 : 0;
            if (g_pfnFearHostOnMouseButton)
                g_pfnFearHostOnMouseButton(nButton, nDown, g_nFearMouseX, g_nFearMouseY);
        }
    }

    void OnEngineTerm() override
    {
        if (g_pfnFearHostOnEngineTerm)
            g_pfnFearHostOnEngineTerm();
    }
};

define_interface(CFearShellProxy, IClientShell);

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hThisModule = hInstance;
        DisableThreadLibraryCalls(hInstance);
        g_hFearCrashHandler = AddVectoredExceptionHandler(1, FearCrashHandler);
        g_hFearWatchHandler = AddVectoredExceptionHandler(1, FearWatchHandler);
        char szStackDiag[8] = {0};
        if (GetEnvironmentVariableA("FEAR_STACK_DIAG", szStackDiag, sizeof(szStackDiag)) > 0 &&
            szStackDiag[0] && szStackDiag[0] != '0')
        {
            signal(SIGABRT, FearAbortHandler);
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        if (g_hFearCrashHandler)
        {
            RemoveVectoredExceptionHandler(g_hFearCrashHandler);
            g_hFearCrashHandler = NULL;
        }
    }
    return TRUE;
}
