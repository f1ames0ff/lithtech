#include "fear_input_stub.h"
#include <string.h>
#include <stdlib.h>

CFearInput g_FearInput;

namespace
{
	const uint32 kNumKeyboardObjects = 256;
	const uint32 kNumMouseButtons = 8;
	const uint32 kNumMouseObjects = 2 + kNumMouseButtons;

	const wchar_t * const kKeyboardName = L"Keyboard";
	const wchar_t * const kKeyboardDisplay = L"Keyboard";
	const wchar_t * const kMouseName = L"Mouse";
	const wchar_t * const kMouseDisplay = L"Mouse";

	bool s_bKeyDown[256];
	bool s_bMouseButton[kNumMouseButtons];

	bool s_bMouseInit = false;
	int s_nLastMouseX = 0;
	int s_nLastMouseY = 0;
	float s_fAccumDx = 0.0f;
	float s_fAccumDy = 0.0f;
	float s_fFrameDx = 0.0f;
	float s_fFrameDy = 0.0f;

	LONGLONG s_nFrameStamp = 0;
	LONGLONG s_nQpcFreq = 0;
	float s_fMouseScale = 0.01f;

	LONGLONG Now(void)
	{
		if (s_nQpcFreq == 0)
		{
			LARGE_INTEGER liFreq;
			QueryPerformanceFrequency(&liFreq);
			s_nQpcFreq = (liFreq.QuadPart > 0) ? liFreq.QuadPart : 1;
		}
		LARGE_INTEGER liNow;
		QueryPerformanceCounter(&liNow);
		return liNow.QuadPart;
	}

	// Converts accumulated mouse movement into a per-frame delta. Readings inside a
	// single frame must agree, so the delta is latched and only refreshed once the
	// stamp is older than a couple of milliseconds.
	void LatchMouseDelta(void)
	{
		LONGLONG nNow = Now();
		const LONGLONG nMinTicks = s_nQpcFreq / 250;
		if (s_nFrameStamp == 0 || (nNow - s_nFrameStamp) >= nMinTicks)
		{
			s_fFrameDx = s_fAccumDx;
			s_fFrameDy = s_fAccumDy;
			s_fAccumDx = 0.0f;
			s_fAccumDy = 0.0f;
			s_nFrameStamp = nNow;
		}
	}

	float AxisValue(int nAxis)
	{
		LatchMouseDelta();
		const float fDelta = (nAxis == FEAR_INPUT_MOUSE_AXIS_X) ? s_fFrameDx : s_fFrameDy;
		float fValue = 0.5f + fDelta * s_fMouseScale;
		if (fValue < 0.0f)
			fValue = 0.0f;
		if (fValue > 1.0f)
			fValue = 1.0f;
		return fValue;
	}

	struct SKeyName
	{
		int nCode;
		const wchar_t *pszName;
	};

	const SKeyName kKeyNames[] =
	{
		{ VK_LBUTTON, L"Left Mouse Button" },
		{ VK_RBUTTON, L"Right Mouse Button" },
		{ VK_MBUTTON, L"Middle Mouse Button" },
		{ VK_BACK, L"Backspace" },
		{ VK_TAB, L"Tab" },
		{ VK_RETURN, L"Enter" },
		{ VK_PAUSE, L"Pause" },
		{ VK_CAPITAL, L"Caps Lock" },
		{ VK_ESCAPE, L"Escape" },
		{ VK_SPACE, L"Space" },
		{ VK_PRIOR, L"Page Up" },
		{ VK_NEXT, L"Page Down" },
		{ VK_END, L"End" },
		{ VK_HOME, L"Home" },
		{ VK_LEFT, L"Left Arrow" },
		{ VK_UP, L"Up Arrow" },
		{ VK_RIGHT, L"Right Arrow" },
		{ VK_DOWN, L"Down Arrow" },
		{ VK_INSERT, L"Insert" },
		{ VK_DELETE, L"Delete" },
		{ VK_LSHIFT, L"Left Shift" },
		{ VK_RSHIFT, L"Right Shift" },
		{ VK_LCONTROL, L"Left Ctrl" },
		{ VK_RCONTROL, L"Right Ctrl" },
		{ VK_LMENU, L"Left Alt" },
		{ VK_RMENU, L"Right Alt" },
		{ VK_NUMPAD0, L"Numpad 0" },
		{ VK_NUMPAD1, L"Numpad 1" },
		{ VK_NUMPAD2, L"Numpad 2" },
		{ VK_NUMPAD3, L"Numpad 3" },
		{ VK_NUMPAD4, L"Numpad 4" },
		{ VK_NUMPAD5, L"Numpad 5" },
		{ VK_NUMPAD6, L"Numpad 6" },
		{ VK_NUMPAD7, L"Numpad 7" },
		{ VK_NUMPAD8, L"Numpad 8" },
		{ VK_NUMPAD9, L"Numpad 9" },
		{ VK_MULTIPLY, L"Numpad *" },
		{ VK_ADD, L"Numpad +" },
		{ VK_SUBTRACT, L"Numpad -" },
		{ VK_DECIMAL, L"Numpad ." },
		{ VK_DIVIDE, L"Numpad /" },
		{ VK_F1, L"F1" }, { VK_F2, L"F2" }, { VK_F3, L"F3" }, { VK_F4, L"F4" },
		{ VK_F5, L"F5" }, { VK_F6, L"F6" }, { VK_F7, L"F7" }, { VK_F8, L"F8" },
		{ VK_F9, L"F9" }, { VK_F10, L"F10" }, { VK_F11, L"F11" }, { VK_F12, L"F12" },
		{ 0, NULL }
	};

	bool EqualsNoCase(const wchar_t *pA, const wchar_t *pB)
	{
		if (!pA || !pB)
			return false;
		while (*pA && *pB)
		{
			wchar_t a = *pA;
			wchar_t b = *pB;
			if (a >= L'A' && a <= L'Z')
				a = (wchar_t)(a - L'A' + L'a');
			if (b >= L'A' && b <= L'Z')
				b = (wchar_t)(b - L'A' + L'b');
			if (a != b)
				return false;
			++pA;
			++pB;
		}
		return (*pA == 0) && (*pB == 0);
	}

	void CopyName(wchar_t *pDst, const wchar_t *pSrc)
	{
		const size_t nMax = ILTInput::SDeviceObjectDesc::k_nMaxNameLength;
		size_t i = 0;
		if (pSrc)
		{
			for (; pSrc[i] && i < nMax; ++i)
				pDst[i] = pSrc[i];
		}
		pDst[i] = 0;
	}

	// Resolves a virtual key code to its retail-friendly name.
	void KeyName(int nCode, wchar_t *pszOut)
	{
		if (nCode >= '0' && nCode <= '9')
		{
			pszOut[0] = (wchar_t)nCode;
			pszOut[1] = 0;
			return;
		}
		if (nCode >= 'A' && nCode <= 'Z')
		{
			pszOut[0] = (wchar_t)nCode;
			pszOut[1] = 0;
			return;
		}
		for (const SKeyName *pCur = kKeyNames; pCur->pszName; ++pCur)
		{
			if (pCur->nCode == nCode)
			{
				wcscpy(pszOut, pCur->pszName);
				return;
			}
		}
		_snwprintf(pszOut, ILTInput::SDeviceObjectDesc::k_nMaxNameLength, L"Key%d", nCode);
		pszOut[ILTInput::SDeviceObjectDesc::k_nMaxNameLength] = 0;
	}

	int FindKeyByName(const wchar_t *pszName)
	{
		if (!pszName)
			return -1;
		for (int nCode = 'A'; nCode <= 'Z'; ++nCode)
		{
			wchar_t szName[2] = { (wchar_t)nCode, 0 };
			if (EqualsNoCase(pszName, szName))
				return nCode;
		}
		for (int nCode = '0'; nCode <= '9'; ++nCode)
		{
			wchar_t szName[2] = { (wchar_t)nCode, 0 };
			if (EqualsNoCase(pszName, szName))
				return nCode;
		}
		for (const SKeyName *pCur = kKeyNames; pCur->pszName; ++pCur)
		{
			if (EqualsNoCase(pszName, pCur->pszName))
				return pCur->nCode;
		}

		// Common short forms used by LithTech control databases.
		struct SAlias { const wchar_t *pszAlias; int nCode; };
		static const SAlias kAliases[] =
		{
			{ L"Esc", VK_ESCAPE },
			{ L"Shift", VK_LSHIFT },
			{ L"Ctrl", VK_LCONTROL },
			{ L"Control", VK_LCONTROL },
			{ L"Alt", VK_LMENU },
			{ L"Up", VK_UP },
			{ L"Down", VK_DOWN },
			{ L"Left", VK_LEFT },
			{ L"Right", VK_RIGHT },
			{ L"Mouse0", VK_LBUTTON },
			{ L"Mouse1", VK_RBUTTON },
			{ L"Mouse2", VK_MBUTTON },
			{ NULL, 0 }
		};
		for (const SAlias *pCur = kAliases; pCur->pszAlias; ++pCur)
		{
			if (EqualsNoCase(pszName, pCur->pszAlias))
				return pCur->nCode;
		}
		return -1;
	}

	const wchar_t *MouseObjectName(uint32 nObjectIndex)
	{
		switch (nObjectIndex)
		{
		case FEAR_INPUT_MOUSE_AXIS_X: return L"X Axis";
		case FEAR_INPUT_MOUSE_AXIS_Y: return L"Y Axis";
		case FEAR_INPUT_MOUSE_BUTTON_0: return L"Left Mouse Button";
		case FEAR_INPUT_MOUSE_BUTTON_1: return L"Right Mouse Button";
		case FEAR_INPUT_MOUSE_BUTTON_2: return L"Middle Mouse Button";
		default: return L"Button";
		}
	}

	int FindMouseObjectByName(const wchar_t *pszName)
	{
		if (!pszName)
			return -1;

		struct SAlias { const wchar_t *pszAlias; int nIndex; };
		static const SAlias kAliases[] =
		{
			{ L"X Axis", FEAR_INPUT_MOUSE_AXIS_X },
			{ L"XAxis", FEAR_INPUT_MOUSE_AXIS_X },
			{ L"X", FEAR_INPUT_MOUSE_AXIS_X },
			{ L"Y Axis", FEAR_INPUT_MOUSE_AXIS_Y },
			{ L"YAxis", FEAR_INPUT_MOUSE_AXIS_Y },
			{ L"Y", FEAR_INPUT_MOUSE_AXIS_Y },
			{ L"Left Mouse Button", FEAR_INPUT_MOUSE_BUTTON_0 },
			{ L"Left Button", FEAR_INPUT_MOUSE_BUTTON_0 },
			{ L"Mouse0", FEAR_INPUT_MOUSE_BUTTON_0 },
			{ L"Right Mouse Button", FEAR_INPUT_MOUSE_BUTTON_1 },
			{ L"Right Button", FEAR_INPUT_MOUSE_BUTTON_1 },
			{ L"Mouse1", FEAR_INPUT_MOUSE_BUTTON_1 },
			{ L"Middle Mouse Button", FEAR_INPUT_MOUSE_BUTTON_2 },
			{ L"Middle Button", FEAR_INPUT_MOUSE_BUTTON_2 },
			{ L"Mouse2", FEAR_INPUT_MOUSE_BUTTON_2 },
			{ NULL, -1 }
		};
		for (const SAlias *pCur = kAliases; pCur->pszAlias; ++pCur)
		{
			if (EqualsNoCase(pszName, pCur->pszAlias))
				return pCur->nIndex;
		}
		return -1;
	}

	bool IsMouseObjectAxis(uint32 nObjectIndex)
	{
		return (nObjectIndex == FEAR_INPUT_MOUSE_AXIS_X) || (nObjectIndex == FEAR_INPUT_MOUSE_AXIS_Y);
	}
}

void FearInput_SetKeyState(int nVKCode, bool bDown)
{
	if (nVKCode < 0 || nVKCode > 255)
		return;
	if (nVKCode == VK_LBUTTON)
		s_bMouseButton[0] = bDown;
	else if (nVKCode == VK_RBUTTON)
		s_bMouseButton[1] = bDown;
	else if (nVKCode == VK_MBUTTON)
		s_bMouseButton[2] = bDown;
	s_bKeyDown[nVKCode] = bDown;
}

void FearInput_SetMousePos(int x, int y)
{
	if (!s_bMouseInit)
	{
		s_bMouseInit = true;
		s_nLastMouseX = x;
		s_nLastMouseY = y;
		return;
	}
	s_fAccumDx += (float)(x - s_nLastMouseX);
	s_fAccumDy += (float)(y - s_nLastMouseY);
	s_nLastMouseX = x;
	s_nLastMouseY = y;
}

void FearInput_SetMouseButton(int nButton, bool bDown)
{
	if (nButton < 0 || nButton >= (int)kNumMouseButtons)
		return;
	s_bMouseButton[nButton] = bDown;
	const int nVK = (nButton == 0) ? VK_LBUTTON : ((nButton == 1) ? VK_RBUTTON : ((nButton == 2) ? VK_MBUTTON : -1));
	if (nVK >= 0)
		s_bKeyDown[nVK] = bDown;
}

int *FearInput_GetQueryCounter(void)
{
	static int s_nQueries = 0;
	return &s_nQueries;
}

void FearInput_ClearState(void)
{
	memset(s_bKeyDown, 0, sizeof(s_bKeyDown));
	memset(s_bMouseButton, 0, sizeof(s_bMouseButton));
	s_bMouseInit = false;
	s_fAccumDx = s_fAccumDy = 0.0f;
	s_fFrameDx = s_fFrameDy = 0.0f;
	s_nFrameStamp = 0;
}

LTRESULT CFearInput::GetNumDevices(uint32 *pNumDevices)
{
	if (!pNumDevices)
		return LT_INVALIDPARAMS;

	*pNumDevices = 2;
	return LT_OK;
}

LTRESULT CFearInput::GetNumDevicesByCategory(EDeviceCategory eCategory, uint32 *pNumDevices)
{
	if (!pNumDevices)
		return LT_INVALIDPARAMS;

	*pNumDevices = (eCategory == eDC_Keyboard || eCategory == eDC_Mouse) ? 1 : 0;
	return LT_OK;
}

LTRESULT CFearInput::GetDeviceDesc(uint32 nDeviceIndex, SDeviceDesc *pDevice)
{
	if (!pDevice)
		return LT_INVALIDPARAMS;
	if (nDeviceIndex > FEAR_INPUT_DEVICE_MOUSE)
		return LT_INVALIDPARAMS;

	memset(pDevice, 0, sizeof(*pDevice));
	if (nDeviceIndex == FEAR_INPUT_DEVICE_KEYBOARD)
	{
		wcscpy(pDevice->m_sName, kKeyboardName);
		wcscpy(pDevice->m_sDisplayName, kKeyboardDisplay);
		pDevice->m_eCategory = eDC_Keyboard;
		pDevice->m_nNumObjects = kNumKeyboardObjects;
	}
	else
	{
		wcscpy(pDevice->m_sName, kMouseName);
		wcscpy(pDevice->m_sDisplayName, kMouseDisplay);
		pDevice->m_eCategory = eDC_Mouse;
		pDevice->m_nNumObjects = kNumMouseObjects;
	}
	return LT_OK;
}

LTRESULT CFearInput::FindDeviceByName(const wchar_t *pName, uint32 *pDeviceIndex)
{
	if (!pName || !pDeviceIndex)
		return LT_INVALIDPARAMS;

	if (EqualsNoCase(pName, kKeyboardName) || EqualsNoCase(pName, L"Kbd"))
	{
		*pDeviceIndex = FEAR_INPUT_DEVICE_KEYBOARD;
		return LT_OK;
	}
	if (EqualsNoCase(pName, kMouseName) || EqualsNoCase(pName, L"Mse"))
	{
		*pDeviceIndex = FEAR_INPUT_DEVICE_MOUSE;
		return LT_OK;
	}
	return LT_NOTFOUND;
}

LTRESULT CFearInput::FindFirstDeviceByCategory(EDeviceCategory eCategory, uint32 *pDeviceIndex)
{
	if (!pDeviceIndex)
		return LT_INVALIDPARAMS;

	if (eCategory == eDC_Keyboard)
	{
		*pDeviceIndex = FEAR_INPUT_DEVICE_KEYBOARD;
		return LT_OK;
	}
	if (eCategory == eDC_Mouse)
	{
		*pDeviceIndex = FEAR_INPUT_DEVICE_MOUSE;
		return LT_OK;
	}
	return LT_NOTFOUND;
}

LTRESULT CFearInput::IsDeviceReady(uint32 nDeviceIndex, bool *pReady)
{
	if (!pReady)
		return LT_INVALIDPARAMS;
	if (nDeviceIndex > FEAR_INPUT_DEVICE_MOUSE)
	{
		*pReady = false;
		return LT_INVALIDPARAMS;
	}

	*pReady = true;
	return LT_OK;
}

LTRESULT CFearInput::GetDeviceObjectDesc(uint32 nDeviceIndex, uint32 nObjectIndex, SDeviceObjectDesc *pObject)
{
	if (!pObject)
		return LT_INVALIDPARAMS;
	if (nDeviceIndex > FEAR_INPUT_DEVICE_MOUSE)
		return LT_INVALIDPARAMS;

	memset(pObject, 0, sizeof(*pObject));
	if (nDeviceIndex == FEAR_INPUT_DEVICE_KEYBOARD)
	{
		if (nObjectIndex >= kNumKeyboardObjects)
			return LT_INVALIDPARAMS;
		wchar_t szName[ILTInput::SDeviceObjectDesc::k_nMaxNameLength + 1];
		KeyName((int)nObjectIndex, szName);
		CopyName(pObject->m_sName, szName);
		CopyName(pObject->m_sDisplayName, szName);
		pObject->m_eCategory = eDOC_Button;
		pObject->m_bWriteable = false;
		pObject->m_fResolution = 1.0f;
		pObject->m_nControlCode = nObjectIndex;
		return LT_OK;
	}

	if (nObjectIndex >= kNumMouseObjects)
		return LT_INVALIDPARAMS;

	const wchar_t *pszName = MouseObjectName(nObjectIndex);
	CopyName(pObject->m_sName, pszName);
	CopyName(pObject->m_sDisplayName, pszName);
	if (IsMouseObjectAxis(nObjectIndex))
	{
		pObject->m_eCategory = eDOC_Axis;
		pObject->m_fResolution = 0.01f;
		pObject->m_nControlCode = nObjectIndex;
	}
	else
	{
		pObject->m_eCategory = eDOC_Button;
		pObject->m_fResolution = 1.0f;
		pObject->m_nControlCode = nObjectIndex - FEAR_INPUT_MOUSE_BUTTON_0;
	}
	pObject->m_bWriteable = false;
	return LT_OK;
}

LTRESULT CFearInput::GetNumDeviceObjectsByCategory(uint32 nDeviceIndex, EDeviceObjectCategory eCategory, uint32 *pNumObjects)
{
	if (!pNumObjects)
		return LT_INVALIDPARAMS;
	if (nDeviceIndex > FEAR_INPUT_DEVICE_MOUSE)
		return LT_INVALIDPARAMS;

	if (nDeviceIndex == FEAR_INPUT_DEVICE_KEYBOARD)
		*pNumObjects = (eCategory == eDOC_Button) ? kNumKeyboardObjects : 0;
	else
		*pNumObjects = (eCategory == eDOC_Axis) ? 2 : ((eCategory == eDOC_Button) ? kNumMouseButtons : 0);
	return LT_OK;
}

LTRESULT CFearInput::FindDeviceObjectByName(uint32 nDeviceIndex, const wchar_t *pName, uint32 *pObjectIndex)
{
	if (!pName || !pObjectIndex)
		return LT_INVALIDPARAMS;
	if (nDeviceIndex > FEAR_INPUT_DEVICE_MOUSE)
		return LT_INVALIDPARAMS;

	if (nDeviceIndex == FEAR_INPUT_DEVICE_KEYBOARD)
	{
		const int nCode = FindKeyByName(pName);
		if (nCode < 0)
			return LT_NOTFOUND;
		*pObjectIndex = (uint32)nCode;
		return LT_OK;
	}

	const int nIndex = FindMouseObjectByName(pName);
	if (nIndex < 0)
		return LT_NOTFOUND;
	*pObjectIndex = (uint32)nIndex;
	return LT_OK;
}

LTRESULT CFearInput::FindFirstDeviceObjectByCategory(uint32 nDeviceIndex, EDeviceObjectCategory eCategory, uint32 *pObjectIndex)
{
	if (!pObjectIndex)
		return LT_INVALIDPARAMS;
	if (nDeviceIndex > FEAR_INPUT_DEVICE_MOUSE)
		return LT_INVALIDPARAMS;

	if (nDeviceIndex == FEAR_INPUT_DEVICE_KEYBOARD)
	{
		if (eCategory != eDOC_Button)
			return LT_NOTFOUND;
		*pObjectIndex = VK_BACK;
		return LT_OK;
	}

	if (eCategory == eDOC_Axis)
	{
		*pObjectIndex = FEAR_INPUT_MOUSE_AXIS_X;
		return LT_OK;
	}
	if (eCategory == eDOC_Button)
	{
		*pObjectIndex = FEAR_INPUT_MOUSE_BUTTON_0;
		return LT_OK;
	}
	return LT_NOTFOUND;
}

LTRESULT CFearInput::GetDeviceObjectValue(uint32 nDeviceIndex, uint32 nObjectIndex, float *pValue)
{
	int *pQueryCount = FearInput_GetQueryCounter();
	if (pQueryCount)
		++*pQueryCount;

	if (!pValue)
		return LT_INVALIDPARAMS;
	if (nDeviceIndex > FEAR_INPUT_DEVICE_MOUSE)
		return LT_INVALIDPARAMS;

	if (nDeviceIndex == FEAR_INPUT_DEVICE_KEYBOARD)
	{
		if (nObjectIndex >= kNumKeyboardObjects)
			return LT_INVALIDPARAMS;
		const uint32 nCode = nObjectIndex;
		*pValue = s_bKeyDown[nCode] ? 1.0f : 0.0f;
		if (nCode == VK_LBUTTON)
			*pValue = s_bMouseButton[0] ? 1.0f : 0.0f;
		else if (nCode == VK_RBUTTON)
			*pValue = s_bMouseButton[1] ? 1.0f : 0.0f;
		else if (nCode == VK_MBUTTON)
			*pValue = s_bMouseButton[2] ? 1.0f : 0.0f;
		return LT_OK;
	}

	if (nObjectIndex >= kNumMouseObjects)
		return LT_INVALIDPARAMS;

	if (IsMouseObjectAxis(nObjectIndex))
	{
		*pValue = AxisValue((int)nObjectIndex);
		return LT_OK;
	}

	const uint32 nButton = nObjectIndex - FEAR_INPUT_MOUSE_BUTTON_0;
	*pValue = s_bMouseButton[nButton] ? 1.0f : 0.0f;
	return LT_OK;
}

LTRESULT CFearInput::GetDeviceValues(uint32 nDeviceIndex, uint32 nFirstObjectIndex, uint32 nNumObjects, float *pValues)
{
	if (!pValues)
		return LT_INVALIDPARAMS;

	for (uint32 i = 0; i < nNumObjects; ++i)
	{
		float fValue = 0.0f;
		if (GetDeviceObjectValue(nDeviceIndex, nFirstObjectIndex + i, &fValue) != LT_OK)
			return LT_INVALIDPARAMS;
		pValues[i] = fValue;
	}
	return LT_OK;
}

LTRESULT CFearInput::SetDeviceObjectValue(uint32 nDeviceIndex, uint32 nObjectIndex, float fValue)
{
	if (nDeviceIndex > FEAR_INPUT_DEVICE_MOUSE)
		return LT_INVALIDPARAMS;

	if (nDeviceIndex == FEAR_INPUT_DEVICE_KEYBOARD)
	{
		if (nObjectIndex >= kNumKeyboardObjects)
			return LT_INVALIDPARAMS;
		s_bKeyDown[nObjectIndex] = (fValue >= 0.5f);
		return LT_OK;
	}

	if (nObjectIndex >= kNumMouseObjects)
		return LT_INVALIDPARAMS;
	if (!IsMouseObjectAxis(nObjectIndex))
		s_bMouseButton[nObjectIndex - FEAR_INPUT_MOUSE_BUTTON_0] = (fValue >= 0.5f);
	return LT_OK;
}

LTRESULT CFearInput::SetDeviceValues(uint32 nDeviceIndex, uint32 nFirstObjectIndex, uint32 nNumObjects, float *pValues)
{
	if (!pValues && (nNumObjects > 0))
		return LT_INVALIDPARAMS;

	for (uint32 i = 0; i < nNumObjects; ++i)
	{
		if (SetDeviceObjectValue(nDeviceIndex, nFirstObjectIndex + i, pValues[i]) != LT_OK)
			return LT_INVALIDPARAMS;
	}
	return LT_OK;
}
