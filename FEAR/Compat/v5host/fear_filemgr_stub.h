#ifndef __FEAR_FILEMGR_STUB_H__
#define __FEAR_FILEMGR_STUB_H__

#include "ltbasedefs.h"
#include "iltfilemgr.h"

class CFearFileMgr : public ILTFileMgr
{
public:
    declare_interface(CFearFileMgr)
    CFearFileMgr() = default;
    ~CFearFileMgr() {}

    ILTInStream* OpenFile(const char* pszFilename) override;
    LTRESULT ExtractFile(const char* pszFilename, char* pszFullFilename, uint32 nBufferLen, bool& bExtracted) override;
    LTRESULT DeleteExtractedFile(const char* pszExtractedFile) override;
    LTRESULT GetAbsoluteFilename(const char* pszFilename, char* pszAbsolute, uint32 nBufferLen) override;
    FileEntry* GetFileList(const char* pszDirectory) override;
    void FreeFileList(FileEntry* pFileList) override;
    ILTOutStream* OpenUserFileForWriting(const char* pszFilename) override;
    ILTInStream* OpenUserFileForReading(const char* pszFilename) override;
    LTRESULT CopyUserFile(const char* pszSourceFile, const char* pszDestFile) override;
    LTRESULT DeleteUserFile(const char* pszRelativeName) override;
    LTRESULT GetAbsoluteUserFileName(const char* pszRelative, char* pszAbsolute, uint32 nAbsoluteLen) override;
};

extern CFearFileMgr g_FearFileMgr;

#endif
