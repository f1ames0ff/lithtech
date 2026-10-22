#ifndef __FEAR_SOUNDMGR_STUB_H__
#define __FEAR_SOUNDMGR_STUB_H__

#include <windows.h>
#include "ltbasedefs.h"
#include "iltsoundmgr.h"

class CFearSoundMgr : public ILTClientSoundMgr
{
public:
    declare_interface(CFearSoundMgr)
    CFearSoundMgr() = default;
    ~CFearSoundMgr() {}

    LTRESULT PlaySound(PlaySoundInfo *pPlaySoundInfo, HLTSOUND &hResult) override;
    LTRESULT GetSoundDuration(HLTSOUND hSound, LTFLOAT &fDuration) override;
    LTRESULT IsSoundDone(HLTSOUND hSound, bool &bDone) override;
    LTRESULT KillSound(HLTSOUND hSound) override;
    LTRESULT KillSoundLoop(HLTSOUND hSound) override;
    LTRESULT SetSoundVolume(HLTSOUND hSound, uint8 nVolume, LTFLOAT fFadeTime) override;
    LTRESULT GetSoundVolume(HLTSOUND hSound, uint8 &nVolume, LTFLOAT &fFadeTime) override;
    LTRESULT GetSoundCapabilities(SoundCapabilities* pSoundCaps) override;
    LTRESULT SetSoundCapabilityPreferences(SoundCapabilities SoundCaps) override;
    bool GetSoundInitError(char* pszSoundInitError, int32 nStringSize) override;
    LTRESULT GetSound3DProviderLists(Sound3DProvider *&pSound3DProviderList, bool bVerify, uint32 uiMax3DVoices) override;
    LTRESULT ReleaseSound3DProviderList(Sound3DProvider *pSound3DProviderList) override;
    LTRESULT InitSound(InitSoundInfo *pSoundInfo) override;
    LTRESULT SetListenerDoppler(float fVolume) override;
    LTRESULT GetVolume(uint16 &nVolume) override;
    LTRESULT SetVolume(uint16 nVolume) override;
    LTRESULT UpdateVolumeSettings() override;
    LTRESULT SetReverbProperties(ReverbProperties *pReverbProperties) override;
    LTRESULT GetReverbProperties(ReverbProperties *pReverbProperties) override;
    LTRESULT SetSoundOcclusion(HLTSOUND hSound, LTFLOAT fLevel) override;
    LTRESULT GetSoundOcclusion(HLTSOUND hSound, LTFLOAT *pLevel) override;
    LTRESULT SetSoundObstruction(HLTSOUND hSound, LTFLOAT fLevel) override;
    LTRESULT GetSoundObstruction(HLTSOUND hSound, LTFLOAT *pLevel) override;
    LTRESULT SetSoundFilter(HLTSOUND hSound, const char *pFilter) override;
    LTRESULT SetSoundFilterParam(HLTSOUND hSound, const char *pParam, float fValue) override;
    LTRESULT SetSoundFilter(const char *pFilter) override;
    LTRESULT SetSoundFilterParam(const char *pParam, float fValue) override;
    LTRESULT SetSoundFilterName(const char *pParam) override;
    LTRESULT EnableSoundFilter(bool bEnable) override;
    LTRESULT SetSoundPosition(HLTSOUND hSound, LTVector *pPos, bool bTeleport) override;
    LTRESULT GetSoundPosition(HLTSOUND hSound, LTVector *pPos) override;
    LTRESULT PauseSounds() override;
    LTRESULT ResumeSounds() override;
    LTRESULT StopAllSounds(bool bReturningToFE) override;
    LTRESULT RemoveAllUnusedSoundInstances() override;
    LTRESULT SetSoundClassMultiplier(uint8 nSoundClass, float fMult, bool bUseGlobalVolume) override;
    LTRESULT GetSoundClassMultiplier(uint8 nSoundClass, float* pfMult) override;
    LTRESULT SetListener(bool bListenerInClient, LTVector *pPos, LTRotation *pRot, bool bTeleport) override;
    LTRESULT SetSoundMixer(SoundMixerInfo* mixer, int32 OverrideTime, int32 KillLayer) override;
    LTRESULT SetSoundMixerMainLevels(SoundMixerSliders* pMainMix) override;
    LTRESULT SetMixerChannelLimits(int32* pChannelLimits, int32 nNumChannelLimits) override;
    LTRESULT ClearTempSoundMixers() override;
    LTRESULT DeactivateSoundRegion(char* pszRegionName) override;
};

extern CFearSoundMgr g_FearSoundMgr;
void FearPatchSoundListenerOnly(void);
void FearPatchSoundVtable(void);

#endif
