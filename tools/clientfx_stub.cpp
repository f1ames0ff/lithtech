#include <stdint.h>

struct FX_REF
{
    const char *m_sName;
    void *m_pfnCreate;
    void *m_pfnCreateProps;
    void *m_pfnGetPropSize;
};

extern "C"
{
__declspec(dllexport) void fxInitDLLRuntime()
{
}

__declspec(dllexport) void fxTermDLLRuntime()
{
}

__declspec(dllexport) uint32_t fxGetNum()
{
    return 0;
}

__declspec(dllexport) uint32_t fxGetVersion()
{
    return 1;
}

__declspec(dllexport) FX_REF fxGetRef(int nFx)
{
    (void)nFx;
    FX_REF ref;
    ref.m_sName = 0;
    ref.m_pfnCreate = 0;
    ref.m_pfnCreateProps = 0;
    ref.m_pfnGetPropSize = 0;
    return ref;
}

__declspec(dllexport) void fxDelete(void *pFx)
{
    (void)pFx;
}

__declspec(dllexport) void fxFreePropList(void *pProps)
{
    (void)pProps;
}
}
