#include "fear_input_stub.h"
#include <string.h>

CFearInput g_FearInput;

LTRESULT CFearInput::GetNumDevices(uint32 *pNumDevices)
{
    if (!pNumDevices)
        return LT_INVALIDPARAMS;

    *pNumDevices = 0;
    return LT_OK;
}

LTRESULT CFearInput::GetNumDevicesByCategory(EDeviceCategory eCategory, uint32 *pNumDevices)
{
    if (!pNumDevices)
        return LT_INVALIDPARAMS;

    *pNumDevices = 0;
    return LT_OK;
}

LTRESULT CFearInput::GetDeviceDesc(uint32 nDeviceIndex, SDeviceDesc *pDevice)
{
    if (!pDevice)
        return LT_INVALIDPARAMS;

    memset(pDevice, 0, sizeof(*pDevice));
    pDevice->m_eCategory = eDC_Keyboard;
    pDevice->m_nNumObjects = 0;
    return LT_INVALIDPARAMS;
}

LTRESULT CFearInput::FindDeviceByName(const wchar_t *pName, uint32 *pDeviceIndex)
{
    if (!pName || !pDeviceIndex)
        return LT_INVALIDPARAMS;

    return LT_NOTFOUND;
}

LTRESULT CFearInput::FindFirstDeviceByCategory(EDeviceCategory eCategory, uint32 *pDeviceIndex)
{
    if (!pDeviceIndex)
        return LT_INVALIDPARAMS;

    return LT_NOTFOUND;
}

LTRESULT CFearInput::IsDeviceReady(uint32 nDeviceIndex, bool *pReady)
{
    if (!pReady)
        return LT_INVALIDPARAMS;

    *pReady = false;
    return LT_OK;
}

LTRESULT CFearInput::GetDeviceObjectDesc(uint32 nDeviceIndex, uint32 nObjectIndex, SDeviceObjectDesc *pObject)
{
    if (!pObject)
        return LT_INVALIDPARAMS;

    memset(pObject, 0, sizeof(*pObject));
    pObject->m_eCategory = eDOC_Button;
    pObject->m_bWriteable = false;
    pObject->m_fResolution = 0.0f;
    pObject->m_nControlCode = 0;
    return LT_INVALIDPARAMS;
}

LTRESULT CFearInput::GetNumDeviceObjectsByCategory(uint32 nDeviceIndex, EDeviceObjectCategory eCategory, uint32 *pNumObjects)
{
    if (!pNumObjects)
        return LT_INVALIDPARAMS;

    *pNumObjects = 0;
    return LT_OK;
}

LTRESULT CFearInput::FindDeviceObjectByName(uint32 nDeviceIndex, const wchar_t *pName, uint32 *pObjectIndex)
{
    if (!pName || !pObjectIndex)
        return LT_INVALIDPARAMS;

    return LT_NOTFOUND;
}

LTRESULT CFearInput::FindFirstDeviceObjectByCategory(uint32 nDeviceIndex, EDeviceObjectCategory eCategory, uint32 *pObjectIndex)
{
    if (!pObjectIndex)
        return LT_INVALIDPARAMS;

    return LT_NOTFOUND;
}

LTRESULT CFearInput::GetDeviceObjectValue(uint32 nDeviceIndex, uint32 nObjectIndex, float *pValue)
{
    if (!pValue)
        return LT_INVALIDPARAMS;

    *pValue = 0.0f;
    return LT_OK;
}

LTRESULT CFearInput::GetDeviceValues(uint32 nDeviceIndex, uint32 nFirstObjectIndex, uint32 nNumObjects, float *pValues)
{
    if (!pValues)
        return LT_INVALIDPARAMS;

    if (nNumObjects > 0)
        memset(pValues, 0, sizeof(float) * nNumObjects);

    return LT_OK;
}

LTRESULT CFearInput::SetDeviceObjectValue(uint32 nDeviceIndex, uint32 nObjectIndex, float fValue)
{
    return LT_OK;
}

LTRESULT CFearInput::SetDeviceValues(uint32 nDeviceIndex, uint32 nFirstObjectIndex, uint32 nNumObjects, float *pValues)
{
    if (!pValues && (nNumObjects > 0))
        return LT_INVALIDPARAMS;

    return LT_OK;
}
