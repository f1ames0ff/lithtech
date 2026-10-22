#include "fear_cursor_stub.h"
#include <stdio.h>

template <typename T> struct SvcDefault { static T Get() { return T(); } };
template <> struct SvcDefault<LTRESULT> { static LTRESULT Get() { return LT_UNSUPPORTED; } };
template <> struct SvcDefault<const char*> { static const char* Get() { return ""; } };

CFearCursor g_FearCursor;

LTRESULT CFearCursor::SetCursorMode(CursorMode cMode, bool bForce)
{
    printf("STUB ILTCursor::SetCursorMode\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCursor::GetCursorMode(CursorMode &cMode)
{
    printf("STUB ILTCursor::GetCursorMode\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCursor::IsCursorModeAvailable(CursorMode cMode)
{
    printf("STUB ILTCursor::IsCursorModeAvailable\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCursor::LoadCursorFromFile(const char *pszFilename, HLTCURSOR &hCursor)
{
    printf("STUB ILTCursor::LoadCursorFromFile\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCursor::FreeCursor(const HLTCURSOR hCursor)
{
    printf("STUB ILTCursor::FreeCursor\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCursor::SetCursor(HLTCURSOR hCursor)
{
    printf("STUB ILTCursor::SetCursor\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCursor::RefreshCursor()
{
    printf("STUB ILTCursor::RefreshCursor\n");
    return SvcDefault<LTRESULT>::Get();
}
