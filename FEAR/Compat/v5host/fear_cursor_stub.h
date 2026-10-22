#ifndef __FEAR_CURSOR_STUB_H__
#define __FEAR_CURSOR_STUB_H__

#include <windows.h>
#include "ltbasedefs.h"
#include "iltcursor.h"

class CFearCursor : public ILTCursor
{
public:
    declare_interface(CFearCursor)
    CFearCursor() = default;
    ~CFearCursor() {}

    LTRESULT SetCursorMode(CursorMode cMode, bool bForce) override;
    LTRESULT GetCursorMode(CursorMode &cMode) override;
    LTRESULT IsCursorModeAvailable(CursorMode cMode) override;
    LTRESULT LoadCursorFromFile(const char *pszFilename, HLTCURSOR &hCursor) override;
    LTRESULT FreeCursor(const HLTCURSOR hCursor) override;
    LTRESULT SetCursor(HLTCURSOR hCursor) override;
    LTRESULT RefreshCursor() override;
};

extern CFearCursor g_FearCursor;

#endif
