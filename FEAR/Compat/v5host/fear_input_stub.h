#ifndef __FEAR_INPUT_STUB_H__
#define __FEAR_INPUT_STUB_H__

#include <windows.h>
#include "ltbasedefs.h"
#include "iltinput.h"

// Fixed device/object layout of CFearInput. Kept stable so the client shell can
// install default bindings without querying the interface.
enum
{
    FEAR_INPUT_DEVICE_KEYBOARD = 0,
    FEAR_INPUT_DEVICE_MOUSE    = 1,

    FEAR_INPUT_MOUSE_AXIS_X    = 0,
    FEAR_INPUT_MOUSE_AXIS_Y    = 1,
    FEAR_INPUT_MOUSE_BUTTON_0  = 2,
    FEAR_INPUT_MOUSE_BUTTON_1  = 3,
    FEAR_INPUT_MOUSE_BUTTON_2  = 4
};

// State feed used by the host window message pump.
void FearInput_SetKeyState(int nVKCode, bool bDown);
void FearInput_SetMousePos(int x, int y);
void FearInput_SetMouseButton(int nButton, bool bDown);
void FearInput_ClearState(void);
int *FearInput_GetQueryCounter(void);
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
