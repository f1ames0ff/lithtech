#include "fear_filemgr_stub.h"
#include "fear_host_api.h"
#include "iltinstream.h"
#include "iltoutstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <windows.h>

static const int kFearFileInfoTypeDirectory = 2;

class CFearInStreamBase : public ILTInStream
{
public:
    CFearInStreamBase() : m_bError(false) {}

    void Release() override
    {
        delete this;
    }

    LTRESULT Read(void *pData, uint32 size) override
    {
        if (size == 0)
            return LT_OK;

        if (!pData)
        {
            m_bError = true;
            return LT_ERROR;
        }

        return ReadRaw(pData, size) ? LT_OK : LT_ERROR;
    }

    LTRESULT ReadString(char *pStr, uint32 maxBytes) override
    {
        if (!pStr && maxBytes > 0)
        {
            m_bError = true;
            return LT_ERROR;
        }

        uint16 nStoredLength = 0;
        if (!ReadRaw(&nStoredLength, (uint32)sizeof(nStoredLength)))
        {
            if (pStr)
                memset(pStr, 0, maxBytes);
            return LT_ERROR;
        }

        uint32 nLength = nStoredLength;

        if (maxBytes == 0)
            return Discard(nLength) ? LT_OK : LT_ERROR;

        uint32 nMaxChars = maxBytes - 1;
        if (nLength > nMaxChars)
        {
            if (!ReadRaw(pStr, nMaxChars))
            {
                memset(pStr, 0, maxBytes);
                return LT_ERROR;
            }

            pStr[nMaxChars] = 0;

            if (!Discard(nLength - nMaxChars))
            {
                memset(pStr, 0, maxBytes);
                return LT_ERROR;
            }

            return LT_ERROR;
        }

        if (nLength > 0 && !ReadRaw(pStr, nLength))
        {
            memset(pStr, 0, maxBytes);
            return LT_ERROR;
        }

        pStr[nLength] = 0;
        return LT_OK;
    }

    bool HasErrorOccurred() override
    {
        return m_bError;
    }

    bool CanSeek() override
    {
        return true;
    }

    LTRESULT SeekTo(uint64 offset) override
    {
        if (!SeekRaw(offset))
        {
            m_bError = true;
            return LT_ERROR;
        }

        return LT_OK;
    }

    uint64 GetPos() override
    {
        return Position();
    }

    uint64 GetLen() override
    {
        return Length();
    }

protected:
    bool m_bError;

private:
    bool Discard(uint32 nSize)
    {
        char szScratch[128];
        while (nSize > 0)
        {
            uint32 nChunk = nSize > sizeof(szScratch) ? (uint32)sizeof(szScratch) : nSize;
            if (!ReadRaw(szScratch, nChunk))
                return false;
            nSize -= nChunk;
        }
        return true;
    }

    virtual bool ReadRaw(void *pData, uint32 nSize) = 0;
    virtual bool SeekRaw(uint64 offset) = 0;
    virtual uint64 Position() = 0;
    virtual uint64 Length() = 0;
};

class CFearEngineInStream : public CFearInStreamBase
{
public:
    CFearEngineInStream(const FearEngineApi *pApi, void *hFile)
        : m_pApi(pApi), m_hFile(hFile)
    {
    }

private:
    ~CFearEngineInStream() override
    {
        if (m_hFile && m_pApi && m_pApi->FileClose)
            m_pApi->FileClose(m_hFile);
    }

    bool ReadRaw(void *pData, uint32 nSize) override
    {
        if (!m_hFile || !m_pApi || !m_pApi->FileRead)
        {
            m_bError = true;
            return false;
        }

        if (nSize == 0)
            return true;

        if (!m_pApi->FileRead(m_hFile, pData, nSize))
        {
            memset(pData, 0, nSize);
            m_bError = true;
            return false;
        }

        return true;
    }

    bool SeekRaw(uint64 offset) override
    {
        if (!m_hFile || !m_pApi || !m_pApi->FileSeek)
        {
            m_bError = true;
            return false;
        }

        if (!m_pApi->FileSeek(m_hFile, offset))
        {
            m_bError = true;
            return false;
        }

        return true;
    }

    uint64 Position() override
    {
        if (!m_hFile || !m_pApi || !m_pApi->FileTell)
        {
            m_bError = true;
            return 0;
        }

        return m_pApi->FileTell(m_hFile);
    }

    uint64 Length() override
    {
        if (!m_hFile || !m_pApi || !m_pApi->FileSize)
        {
            m_bError = true;
            return 0;
        }

        return m_pApi->FileSize(m_hFile);
    }

    const FearEngineApi *m_pApi;
    void *m_hFile;
};

class CFearLocalInStream : public CFearInStreamBase
{
public:
    explicit CFearLocalInStream(FILE *pFile) : m_pFile(pFile)
    {
    }

private:
    ~CFearLocalInStream() override
    {
        if (m_pFile)
            fclose(m_pFile);
    }

    bool ReadRaw(void *pData, uint32 nSize) override
    {
        if (!m_pFile)
        {
            m_bError = true;
            return false;
        }

        if (nSize == 0)
            return true;

        if (fread(pData, 1, nSize, m_pFile) != nSize)
        {
            memset(pData, 0, nSize);
            m_bError = true;
            return false;
        }

        return true;
    }

    bool SeekRaw(uint64 offset) override
    {
        if (!m_pFile || offset > 0x7FFFFFFFFFFFFFFFull)
        {
            m_bError = true;
            return false;
        }

        if (_fseeki64(m_pFile, (__int64)offset, SEEK_SET) != 0)
        {
            m_bError = true;
            return false;
        }

        return true;
    }

    uint64 Position() override
    {
        if (!m_pFile)
        {
            m_bError = true;
            return 0;
        }

        __int64 nPosition = _ftelli64(m_pFile);
        if (nPosition < 0)
        {
            m_bError = true;
            return 0;
        }

        return (uint64)nPosition;
    }

    uint64 Length() override
    {
        if (!m_pFile)
        {
            m_bError = true;
            return 0;
        }

        __int64 nCur = _ftelli64(m_pFile);
        if (nCur < 0 || _fseeki64(m_pFile, 0, SEEK_END) != 0)
        {
            m_bError = true;
            return 0;
        }

        __int64 nLength = _ftelli64(m_pFile);
        if (nLength < 0 || _fseeki64(m_pFile, nCur, SEEK_SET) != 0)
        {
            m_bError = true;
            return 0;
        }

        return (uint64)nLength;
    }

    FILE *m_pFile;
};

class CFearOutStream : public ILTOutStream
{
public:
    explicit CFearOutStream(FILE *pFile) : m_pFile(pFile), m_bError(false) {}
    void Release() override { delete this; }
    LTRESULT Write(const void *pData, uint32 size) override
    {
        if (!m_pFile || (!pData && size > 0)) { m_bError = true; return LT_ERROR; }
        if (size == 0) return LT_OK;
        if (fwrite(pData, 1, size, m_pFile) != size) { m_bError = true; return LT_ERROR; }
        return LT_OK;
    }
    LTRESULT WriteString(const char *pStr) override
    {
        if (!pStr || !m_pFile) { m_bError = true; return LT_ERROR; }
        uint32 nLength = (uint32)strlen(pStr);
        if (nLength > 0xFFFF) { m_bError = true; return LT_ERROR; }
        uint16 nStoredLength = (uint16)nLength;
        if (Write(&nStoredLength, (uint32)sizeof(nStoredLength)) != LT_OK) return LT_ERROR;
        return Write(pStr, nLength);
    }
    bool HasErrorOccurred() override { return m_bError; }
    bool CanSeek() override { return true; }
    LTRESULT SeekTo(uint64 offset) override
    {
        if (!m_pFile || offset > 0x7FFFFFFFFFFFFFFFull) { m_bError = true; return LT_ERROR; }
        if (_fseeki64(m_pFile, (__int64)offset, SEEK_SET) != 0) { m_bError = true; return LT_ERROR; }
        return LT_OK;
    }
    uint64 GetPos() override
    {
        if (!m_pFile) { m_bError = true; return 0; }
        __int64 nPosition = _ftelli64(m_pFile);
        if (nPosition < 0) { m_bError = true; return 0; }
        return (uint64)nPosition;
    }
private:
    ~CFearOutStream() { if (m_pFile) fclose(m_pFile); }
    FILE *m_pFile;
    bool m_bError;
};

static void FearFreeEntryList(FileEntry *pList)
{
    while (pList)
    {
        FileEntry *pNext = pList->m_pNext;
        delete[] pList->m_pBaseFilename;
        delete[] pList->m_pFullFilename;
        delete pList;
        pList = pNext;
    }
}

CFearFileMgr g_FearFileMgr;

ILTInStream *CFearFileMgr::OpenFile(const char *pszFilename)
{
    if (!pszFilename)
        return NULL;

    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (!pApi || !pApi->FileOpen)
        return NULL;

    void *hFile = pApi->FileOpen(pszFilename);
    if (!hFile)
    {
        printf("FILEMGR OpenFile failed: %s\n", pszFilename);
        return NULL;
    }

    CFearEngineInStream *pStream = new CFearEngineInStream(pApi, hFile);
    if (!pStream)
    {
        if (pApi->FileClose)
            pApi->FileClose(hFile);
        return NULL;
    }

    return pStream;
}

LTRESULT CFearFileMgr::ExtractFile(const char *pszFilename, char *pszFullFilename, uint32 nBufferLen, bool &bExtracted)
{
    bExtracted = false;
    if (!pszFilename || !pszFullFilename || nBufferLen == 0)
        return LT_ERROR;
    if (!_fullpath(pszFullFilename, pszFilename, (size_t)nBufferLen)) return LT_ERROR;
    return LT_OK;
}

LTRESULT CFearFileMgr::DeleteExtractedFile(const char *pszExtractedFile)
{
    if (!pszExtractedFile)
        return LT_ERROR;
    return DeleteFileA(pszExtractedFile) ? LT_OK : LT_ERROR;
}

LTRESULT CFearFileMgr::GetAbsoluteFilename(const char *pszFilename, char *pszAbsolute, uint32 nBufferLen)
{
    if (!pszFilename || !pszAbsolute || nBufferLen == 0)
        return LT_ERROR;
    if (!_fullpath(pszAbsolute, pszFilename, (size_t)nBufferLen)) return LT_ERROR;
    return LT_OK;
}

FileEntry *CFearFileMgr::GetFileList(const char *pszDirectory)
{
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (!pApi || !pApi->FileList)
        return NULL;

    int nCount = 0;
    void *hHandle = NULL;
    const FearFileInfo *pInfos = pApi->FileList(pszDirectory ? pszDirectory : "", &nCount, &hHandle);

    if (!pInfos || nCount <= 0)
    {
        if (hHandle && pApi->FileListFree)
            pApi->FileListFree(hHandle);
        return NULL;
    }

    FileEntry *pHead = NULL;
    FileEntry *pTail = NULL;

    for (int i = 0; i < nCount; ++i)
    {
        FileEntry *pEntry = new FileEntry;
        pEntry->m_Type = (pInfos[i].m_Type == kFearFileInfoTypeDirectory) ? eLTFileEntryType_Directory : eLTFileEntryType_File;
        pEntry->m_pBaseFilename = LTStrDup(pInfos[i].m_Name);
        pEntry->m_pFullFilename = LTStrDup(pInfos[i].m_FullName);
        pEntry->m_pNext = NULL;

        if (!pEntry->m_pBaseFilename || !pEntry->m_pFullFilename)
        {
            delete[] pEntry->m_pBaseFilename;
            delete[] pEntry->m_pFullFilename;
            delete pEntry;
            FearFreeEntryList(pHead);
            pHead = NULL;
            pTail = NULL;
            break;
        }

        if (pTail)
            pTail->m_pNext = pEntry;
        else
            pHead = pEntry;
        pTail = pEntry;
    }

    if (hHandle && pApi->FileListFree)
        pApi->FileListFree(hHandle);

    return pHead;
}

void CFearFileMgr::FreeFileList(FileEntry *pFileList)
{
    FearFreeEntryList(pFileList);
}

ILTOutStream *CFearFileMgr::OpenUserFileForWriting(const char *pszFilename)
{
    if (!pszFilename)
        return NULL;
    FILE *pFile = fopen(pszFilename, "wb");
    if (!pFile) return NULL;
    CFearOutStream *pStream = new CFearOutStream(pFile);
    if (!pStream)
    {
        fclose(pFile);
        return NULL;
    }
    return pStream;
}

ILTInStream *CFearFileMgr::OpenUserFileForReading(const char *pszFilename)
{
    if (!pszFilename)
        return NULL;
    FILE *pFile = fopen(pszFilename, "rb");
    if (!pFile)
        return NULL;
    CFearLocalInStream *pStream = new CFearLocalInStream(pFile);
    if (!pStream)
    {
        fclose(pFile);
        return NULL;
    }
    return pStream;
}

LTRESULT CFearFileMgr::CopyUserFile(const char *pszSourceFile, const char *pszDestFile)
{
    if (!pszSourceFile || !pszDestFile)
        return LT_ERROR;
    return CopyFileA(pszSourceFile, pszDestFile, FALSE) ? LT_OK : LT_ERROR;
}

LTRESULT CFearFileMgr::DeleteUserFile(const char *pszRelativeName)
{
    if (!pszRelativeName)
        return LT_ERROR;
    return DeleteFileA(pszRelativeName) ? LT_OK : LT_ERROR;
}

LTRESULT CFearFileMgr::GetAbsoluteUserFileName(const char *pszRelative, char *pszAbsolute, uint32 nAbsoluteLen)
{
    return GetAbsoluteFilename(pszRelative, pszAbsolute, nAbsoluteLen);
}
