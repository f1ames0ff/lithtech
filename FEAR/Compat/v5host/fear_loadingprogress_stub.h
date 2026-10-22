#ifndef __FEAR_LOADINGPROGRESS_STUB_H__
#define __FEAR_LOADINGPROGRESS_STUB_H__

#include "ltbasedefs.h"
#include "iltloadingprogress.h"

class CFearLoadingProgress : public ILTLoadingProgress
{
public:
    declare_interface(CFearLoadingProgress)
    CFearLoadingProgress();

    void SetLoadingCallback(TLoadingProgressCallbackFn pfnCB, void *pUser) override;
    void TriggerLoadingCallback(ELoadingProgressTask eTask, float fProgress) override;
    void UpdateProgress(ELoadingProgressTask eTask, float fProgress) override;

private:
    TLoadingProgressCallbackFn m_pfnCallback;
    void *m_pUser;
};

extern CFearLoadingProgress g_FearLoadingProgress;

#endif
