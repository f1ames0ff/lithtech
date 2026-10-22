#include "fear_loadingprogress_stub.h"

CFearLoadingProgress g_FearLoadingProgress;

CFearLoadingProgress::CFearLoadingProgress() :
    m_pfnCallback(NULL),
    m_pUser(NULL)
{
}

void CFearLoadingProgress::SetLoadingCallback(TLoadingProgressCallbackFn pfnCB, void *pUser)
{
    m_pfnCallback = pfnCB;
    m_pUser = pUser;
}

void CFearLoadingProgress::TriggerLoadingCallback(ELoadingProgressTask eTask, float fProgress)
{
    if (m_pfnCallback)
        m_pfnCallback(eTask, fProgress, m_pUser);
}

void CFearLoadingProgress::UpdateProgress(ELoadingProgressTask eTask, float fProgress)
{
    TriggerLoadingCallback(eTask, fProgress);
}
