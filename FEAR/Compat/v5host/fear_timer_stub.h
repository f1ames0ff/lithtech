#ifndef __FEAR_TIMER_STUB_H__
#define __FEAR_TIMER_STUB_H__

#include "ltbasedefs.h"
#include "ilttimer.h"

class CFearTimer : public ILTTimer
{
public:
    declare_interface(CFearTimer)
    CFearTimer() = default;
    ~CFearTimer() {}

    HENGINETIMER CreateTimer() override;
    LTRESULT AddTimerRef(HENGINETIMER hTimer) override;
    LTRESULT ReleaseTimer(HENGINETIMER hTimer) override;
    LTRESULT AddChildTimer(HENGINETIMER hParent, HENGINETIMER hChild) override;
    LTRESULT DetachTimer(HENGINETIMER hTimer) override;
    LTRESULT AddSystemChildTimer(HENGINETIMER hChild) override;
    HENGINETIMER GetSimulationTimer() override;
    LTRESULT SetObjectTimer(HOBJECT hObject, HENGINETIMER hTimer) override;
    HENGINETIMER GetObjectTimer(HOBJECT hObject) override;
    LTRESULT SetTimerTimeScale(HENGINETIMER hTimer, uint32 nNumerator, uint32 nDenominator) override;
    LTRESULT GetTimerTimeScale(HENGINETIMER hTimer, uint32& nNumerator, uint32& nDenominator) override;
    LTRESULT SetTimerUpdateRange(HENGINETIMER hTimer, uint32 nMinMS, uint32 nMaxMS) override;
    LTRESULT GetTimerUpdateRange(HENGINETIMER hTimer, uint32& nMinMS, uint32& nMaxMS) override;
    LTRESULT GetTimerElapsedS(HENGINETIMER hTimer, float& fElapsedS) override;
    LTRESULT GetTimerElapsedMS(HENGINETIMER hTimer, uint32& nElapsedMS) override;
    LTRESULT GetTimerAccumulatedS(HENGINETIMER hTimer, double& fAccumulatedS) override;
    LTRESULT GetTimerAccumulatedMS(HENGINETIMER hTimer, uint32& nAccumulatedMS) override;
    LTRESULT PauseTimer(HENGINETIMER hTimer, bool bPause) override;
    LTRESULT IsTimerPaused(HENGINETIMER hTimer, bool& bPaused) override;
};

extern CFearTimer g_FearTimer;
double FearTimer_GetSeconds();

#endif
