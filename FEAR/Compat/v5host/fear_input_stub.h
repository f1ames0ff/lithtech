#ifndef __FEAR_INPUT_STUB_H__
#define __FEAR_INPUT_STUB_H__

#include <windows.h>
#include "ltbasedefs.h"
#include "iltinput.h"

class CFearInput : public ILTInput
{
public:
    declare_interface(CFearInput)
    CFearInput() = default;
    ~CFearInput() {}

    LTRESULT GetNumDevices(uint32 *pNumDevices) override;
    LTRESULT GetNumDevicesByCategory(EDeviceCategory eCategory, uint32 *pNumDevices) override;
    LTRESULT GetDeviceDesc(uint32 nDeviceIndex, SDeviceDesc *pDevice) override;
    LTRESULT FindDeviceByName(const wchar_t *pName, uint32 *pDeviceIndex) override;
    LTRESULT FindFirstDeviceByCategory(EDeviceCategory eCategory, uint32 *pDeviceIndex) override;
    LTRESULT IsDeviceReady(uint32 nDeviceIndex, bool *pReady) override;
    LTRESULT GetDeviceObjectDesc(uint32 nDeviceIndex, uint32 nObjectIndex, SDeviceObjectDesc *pObject) override;
    LTRESULT GetNumDeviceObjectsByCategory(uint32 nDeviceIndex, EDeviceObjectCategory eCategory, uint32 *pNumObjects) override;
    LTRESULT FindDeviceObjectByName(uint32 nDeviceIndex, const wchar_t *pName, uint32 *pObjectIndex) override;
    LTRESULT FindFirstDeviceObjectByCategory(uint32 nDeviceIndex, EDeviceObjectCategory eCategory, uint32 *pObjectIndex) override;
    LTRESULT GetDeviceObjectValue(uint32 nDeviceIndex, uint32 nObjectIndex, float *pValue) override;
    LTRESULT GetDeviceValues(uint32 nDeviceIndex, uint32 nFirstObjectIndex, uint32 nNumObjects, float *pValues) override;
    LTRESULT SetDeviceObjectValue(uint32 nDeviceIndex, uint32 nObjectIndex, float fValue) override;
    LTRESULT SetDeviceValues(uint32 nDeviceIndex, uint32 nFirstObjectIndex, uint32 nNumObjects, float *pValues) override;
};

extern CFearInput g_FearInput;

#endif
