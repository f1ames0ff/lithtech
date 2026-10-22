#include <windows.h>
#include <intrin.h>
#include "fear_soundmgr_stub.h"
#include <stdio.h>

template <typename T> struct SvcDefault { static T Get() { return T(); } };
template <> struct SvcDefault<LTRESULT> { static LTRESULT Get() { return LT_UNSUPPORTED; } };
template <> struct SvcDefault<const char*> { static const char* Get() { return ""; } };

CFearSoundMgr g_FearSoundMgr;

static LTRESULT __stdcall FearSoundNoArgShim()
{
    static int s_nLogged = 0;
    if (!s_nLogged)
    {
        s_nLogged = 1;
        printf("FEARSOUND: no-arg vtable slot shim\n");
        fflush(stdout);
    }
    return LT_OK;
}

static LTRESULT __stdcall FearSetListenerShim(void *pListener)
{
    static int s_nLogged = 0;
    if (!s_nLogged)
    {
        s_nLogged = 1;
        printf("FEARSOUND: SetListener one-arg shim\n");
        fflush(stdout);
    }
    return LT_OK;
}

void FearPatchSoundListenerOnly(void)
{
    void **pVtable = *reinterpret_cast<void ***>(reinterpret_cast<void *>(&g_FearSoundMgr));
    if (!pVtable)
        return;

    DWORD nOldProtect = 0;
    if (!VirtualProtect(&pVtable[38], sizeof(void *), PAGE_EXECUTE_READWRITE, &nOldProtect))
    {
        printf("FEARSOUND: SetListener patch failed %lu\n", GetLastError());
        return;
    }
    pVtable[38] = reinterpret_cast<void *>(&FearSetListenerShim);
    DWORD nTmp = 0;
    VirtualProtect(&pVtable[38], sizeof(void *), nOldProtect, &nTmp);
    printf("FEARSOUND: SetListener one-arg vtable slot patched\n");
    fflush(stdout);
}

void FearPatchSoundVtable(void)
{
    void **pVtable = *reinterpret_cast<void ***>(reinterpret_cast<void *>(&g_FearSoundMgr));
    if (!pVtable)
        return;

    DWORD nOldProtect = 0;
    if (!VirtualProtect(&pVtable[30], sizeof(void *) * 9, PAGE_EXECUTE_READWRITE, &nOldProtect))
    {
        printf("FEARSOUND: SetListener patch failed %lu\n", GetLastError());
        return;
    }
    pVtable[30] = reinterpret_cast<void *>(&FearSoundNoArgShim);
    pVtable[38] = reinterpret_cast<void *>(&FearSetListenerShim);
    DWORD nTmp = 0;
    VirtualProtect(&pVtable[30], sizeof(void *) * 9, nOldProtect, &nTmp);
    printf("FEARSOUND: SetListener vtable slot patched\n");
    fflush(stdout);
}

LTRESULT CFearSoundMgr::PlaySound(PlaySoundInfo *pPlaySoundInfo, HLTSOUND &hResult)
{
    printf("STUB ILTSoundMgr::PlaySound\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::GetSoundDuration(HLTSOUND hSound, LTFLOAT &fDuration)
{
    printf("STUB ILTSoundMgr::GetSoundDuration\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::IsSoundDone(HLTSOUND hSound, bool &bDone)
{
    printf("STUB ILTSoundMgr::IsSoundDone\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::KillSound(HLTSOUND hSound)
{
    printf("STUB ILTSoundMgr::KillSound\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::KillSoundLoop(HLTSOUND hSound)
{
    printf("STUB ILTSoundMgr::KillSoundLoop\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetSoundVolume(HLTSOUND hSound, uint8 nVolume, LTFLOAT fFadeTime)
{
    printf("STUB ILTSoundMgr::SetSoundVolume\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::GetSoundVolume(HLTSOUND hSound, uint8 &nVolume, LTFLOAT &fFadeTime)
{
    printf("STUB ILTSoundMgr::GetSoundVolume\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::GetSoundCapabilities(SoundCapabilities* pSoundCaps)
{
    printf("STUB ILTSoundMgr::GetSoundCapabilities\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetSoundCapabilityPreferences(SoundCapabilities SoundCaps)
{
    printf("STUB ILTSoundMgr::SetSoundCapabilityPreferences ret=%p this=%p esp=%p sizeof=%u\n", _ReturnAddress(), this, _AddressOfReturnAddress(), (unsigned)sizeof(SoundCapabilities));
    fflush(stdout);
    return SvcDefault<LTRESULT>::Get();
}

bool CFearSoundMgr::GetSoundInitError(char* pszSoundInitError, int32 nStringSize)
{
    return SvcDefault<bool>::Get();
}

LTRESULT CFearSoundMgr::GetSound3DProviderLists(Sound3DProvider *&pSound3DProviderList, bool bVerify, uint32 uiMax3DVoices)
{
    static Sound3DProvider s_EmptyProvider;
    memset(&s_EmptyProvider, 0, sizeof(s_EmptyProvider));
    s_EmptyProvider.m_dwProviderID = 0;
    s_EmptyProvider.m_dwCaps = 0;
    s_EmptyProvider.m_pNextProvider = NULL;
    pSound3DProviderList = &s_EmptyProvider;
    printf("FEARSOUND: GetSound3DProviderLists -> empty\n");
    return LT_OK;
}

LTRESULT CFearSoundMgr::ReleaseSound3DProviderList(Sound3DProvider *pSound3DProviderList)
{
    return LT_OK;
}

LTRESULT CFearSoundMgr::InitSound(InitSoundInfo *pSoundInfo)
{
    printf("STUB ILTSoundMgr::InitSound\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetListenerDoppler(float fVolume)
{
    printf("STUB ILTSoundMgr::SetListenerDoppler\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::GetVolume(uint16 &nVolume)
{
    printf("STUB ILTSoundMgr::GetVolume\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetVolume(uint16 nVolume)
{
    printf("STUB ILTSoundMgr::SetVolume\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::UpdateVolumeSettings()
{
    printf("STUB ILTSoundMgr::UpdateVolumeSettings\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetReverbProperties(ReverbProperties *pReverbProperties)
{
    printf("STUB ILTSoundMgr::SetReverbProperties\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::GetReverbProperties(ReverbProperties *pReverbProperties)
{
    printf("STUB ILTSoundMgr::GetReverbProperties\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetSoundOcclusion(HLTSOUND hSound, LTFLOAT fLevel)
{
    printf("STUB ILTSoundMgr::SetSoundOcclusion\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::GetSoundOcclusion(HLTSOUND hSound, LTFLOAT *pLevel)
{
    printf("STUB ILTSoundMgr::GetSoundOcclusion\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetSoundObstruction(HLTSOUND hSound, LTFLOAT fLevel)
{
    printf("STUB ILTSoundMgr::SetSoundObstruction\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::GetSoundObstruction(HLTSOUND hSound, LTFLOAT *pLevel)
{
    printf("STUB ILTSoundMgr::GetSoundObstruction\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetSoundFilter(HLTSOUND hSound, const char *pFilter)
{
    printf("STUB ILTSoundMgr::SetSoundFilter\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetSoundFilterParam(HLTSOUND hSound, const char *pParam, float fValue)
{
    printf("STUB ILTSoundMgr::SetSoundFilterParam\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetSoundFilter(const char *pFilter)
{
    printf("STUB ILTSoundMgr::SetSoundFilter\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetSoundFilterParam(const char *pParam, float fValue)
{
    printf("STUB ILTSoundMgr::SetSoundFilterParam\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetSoundFilterName(const char *pParam)
{
    printf("STUB ILTSoundMgr::SetSoundFilterName\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::EnableSoundFilter(bool bEnable)
{
    printf("STUB ILTSoundMgr::EnableSoundFilter\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetSoundPosition(HLTSOUND hSound, LTVector *pPos, bool bTeleport)
{
    printf("STUB ILTSoundMgr::SetSoundPosition ret=%p this=%p esp=%p\n", _ReturnAddress(), this, _AddressOfReturnAddress());
    fflush(stdout);
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::GetSoundPosition(HLTSOUND hSound, LTVector *pPos)
{
    printf("STUB ILTSoundMgr::GetSoundPosition\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::PauseSounds()
{
    printf("STUB ILTSoundMgr::PauseSounds\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::ResumeSounds()
{
    printf("STUB ILTSoundMgr::ResumeSounds\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::StopAllSounds(bool bReturningToFE)
{
    printf("STUB ILTSoundMgr::StopAllSounds\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::RemoveAllUnusedSoundInstances()
{
    printf("STUB ILTSoundMgr::RemoveAllUnusedSoundInstances\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetSoundClassMultiplier(uint8 nSoundClass, float fMult, bool bUseGlobalVolume)
{
    printf("STUB ILTSoundMgr::SetSoundClassMultiplier\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::GetSoundClassMultiplier(uint8 nSoundClass, float* pfMult)
{
    printf("STUB ILTSoundMgr::GetSoundClassMultiplier\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetListener(bool bListenerInClient, LTVector *pPos, LTRotation *pRot, bool bTeleport)
{
    printf("STUB ILTSoundMgr::SetListener ret=%p this=%p esp=%p\n", _ReturnAddress(), this, _AddressOfReturnAddress());
    fflush(stdout);
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetSoundMixer(SoundMixerInfo* mixer, int32 OverrideTime, int32 KillLayer)
{
    printf("STUB ILTSoundMgr::SetSoundMixer\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetSoundMixerMainLevels(SoundMixerSliders* pMainMix)
{
    printf("STUB ILTSoundMgr::SetSoundMixerMainLevels\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::SetMixerChannelLimits(int32* pChannelLimits, int32 nNumChannelLimits)
{
    printf("STUB ILTSoundMgr::SetMixerChannelLimits\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::ClearTempSoundMixers()
{
    printf("STUB ILTSoundMgr::ClearTempSoundMixers\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearSoundMgr::DeactivateSoundRegion(char* pszRegionName)
{
    printf("STUB ILTSoundMgr::DeactivateSoundRegion\n");
    return SvcDefault<LTRESULT>::Get();
}
