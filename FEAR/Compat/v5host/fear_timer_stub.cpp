#include <windows.h>
#include "fear_timer_stub.h"

CFearTimer g_FearTimer;

static uint32 g_nTimerSentinel = 0;
static LARGE_INTEGER g_nTimerFrequency = {0};
static LARGE_INTEGER g_nTimerStart = {0};

static void FearTimerInit()
{
    if (g_nTimerFrequency.QuadPart == 0)
    {
        QueryPerformanceFrequency(&g_nTimerFrequency);
        QueryPerformanceCounter(&g_nTimerStart);
    }
}

double FearTimer_GetSeconds()
{
    FearTimerInit();
    LARGE_INTEGER nNow;
    QueryPerformanceCounter(&nNow);
    return (double)(nNow.QuadPart - g_nTimerStart.QuadPart) / (double)g_nTimerFrequency.QuadPart;
}

static uint32 FearTimerGetMS()
{
    return (uint32)(FearTimer_GetSeconds() * 1000.0);
}

static HENGINETIMER FearTimerHandle()
{
    return reinterpret_cast<HENGINETIMER>(&g_nTimerSentinel);
}

HENGINETIMER CFearTimer::CreateTimer()
{
    return FearTimerHandle();
}

LTRESULT CFearTimer::AddTimerRef(HENGINETIMER hTimer)
{
    return LT_OK;
}

LTRESULT CFearTimer::ReleaseTimer(HENGINETIMER hTimer)
{
    return LT_OK;
}

LTRESULT CFearTimer::AddChildTimer(HENGINETIMER hParent, HENGINETIMER hChild)
{
    return LT_OK;
}

LTRESULT CFearTimer::DetachTimer(HENGINETIMER hTimer)
{
    return LT_OK;
}

LTRESULT CFearTimer::AddSystemChildTimer(HENGINETIMER hChild)
{
    return LT_OK;
}

HENGINETIMER CFearTimer::GetSimulationTimer()
{
    return FearTimerHandle();
}

LTRESULT CFearTimer::SetObjectTimer(HOBJECT hObject, HENGINETIMER hTimer)
{
    return LT_OK;
}

HENGINETIMER CFearTimer::GetObjectTimer(HOBJECT hObject)
{
    return FearTimerHandle();
}

LTRESULT CFearTimer::SetTimerTimeScale(HENGINETIMER hTimer, uint32 nNumerator, uint32 nDenominator)
{
    return LT_OK;
}

LTRESULT CFearTimer::GetTimerTimeScale(HENGINETIMER hTimer, uint32& nNumerator, uint32& nDenominator)
{
    nNumerator = 1;
    nDenominator = 1;
    return LT_OK;
}

LTRESULT CFearTimer::SetTimerUpdateRange(HENGINETIMER hTimer, uint32 nMinMS, uint32 nMaxMS)
{
    return LT_OK;
}

LTRESULT CFearTimer::GetTimerUpdateRange(HENGINETIMER hTimer, uint32& nMinMS, uint32& nMaxMS)
{
    nMinMS = 0;
    nMaxMS = 0;
    return LT_OK;
}

LTRESULT CFearTimer::GetTimerElapsedS(HENGINETIMER hTimer, float& fElapsedS)
{
    fElapsedS = (float)FearTimer_GetSeconds();
    return LT_OK;
}

LTRESULT CFearTimer::GetTimerElapsedMS(HENGINETIMER hTimer, uint32& nElapsedMS)
{
    nElapsedMS = FearTimerGetMS();
    return LT_OK;
}

LTRESULT CFearTimer::GetTimerAccumulatedS(HENGINETIMER hTimer, double& fAccumulatedS)
{
    fAccumulatedS = FearTimer_GetSeconds();
    return LT_OK;
}

LTRESULT CFearTimer::GetTimerAccumulatedMS(HENGINETIMER hTimer, uint32& nAccumulatedMS)
{
    nAccumulatedMS = FearTimerGetMS();
    return LT_OK;
}

LTRESULT CFearTimer::PauseTimer(HENGINETIMER hTimer, bool bPause)
{
    return LT_OK;
}

LTRESULT CFearTimer::IsTimerPaused(HENGINETIMER hTimer, bool& bPaused)
{
    bPaused = false;
    return LT_OK;
}
