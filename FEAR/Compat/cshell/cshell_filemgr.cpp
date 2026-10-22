#include "cshell_filemgr.h"
#include <stdlib.h>
#include <string.h>
#include "ltbasedefs.h"
#include "ltmodule.h"
#include "iltstream.h"
#include "iltclient.h"
#include "client_filemgr.h"

static IClientFileMgr *g_pFileMgr = NULL;
define_holder(IClientFileMgr, g_pFileMgr);

static ILTClient *g_pLTClientFileLists = NULL;
define_holder(ILTClient, g_pLTClientFileLists);

static ILTStream *FearStreamFromHandle(void *hFile)
{
    return (ILTStream *)hFile;
}

static void FearFreeEngineFileList(FileEntry *pList)
{
    if (!pList)
        return;
    if (g_pLTClientFileLists && g_pLTClientFileLists->FreeFileList)
        g_pLTClientFileLists->FreeFileList(pList);
}

void *FearFile_Open(const char *pszFilename)
{
    if (!g_pFileMgr || !pszFilename)
        return NULL;

    FileRef ref;
    ref.m_FileType = FILE_ANYFILE;
    ref.m_pFilename = pszFilename;
    return g_pFileMgr->OpenFile(&ref);
}

int FearFile_Read(void *hFile, void *pBuffer, unsigned int nSize)
{
    if (!hFile)
        return 0;
    if (!pBuffer)
        return nSize == 0 ? 1 : 0;

    return FearStreamFromHandle(hFile)->Read(pBuffer, nSize) == LT_OK ? 1 : 0;
}

int FearFile_Seek(void *hFile, unsigned long long nOffset)
{
    if (!hFile || nOffset > 0xFFFFFFFFull)
        return 0;

    return FearStreamFromHandle(hFile)->SeekTo((uint32)nOffset) == LT_OK ? 1 : 0;
}

unsigned long long FearFile_Tell(void *hFile)
{
    if (!hFile)
        return 0;

    uint32 nPosition = 0;
    if (FearStreamFromHandle(hFile)->GetPos(&nPosition) != LT_OK)
        return 0;
    return nPosition;
}

unsigned long long FearFile_Size(void *hFile)
{
    if (!hFile)
        return 0;

    uint32 nLength = 0;
    if (FearStreamFromHandle(hFile)->GetLen(&nLength) != LT_OK)
        return 0;
    return nLength;
}

void FearFile_Close(void *hFile)
{
    if (hFile)
        FearStreamFromHandle(hFile)->Release();
}

const FearFileInfo *FearFile_List(const char *pszDirectory, int *pnCount, void **ppHandle)
{
    if (pnCount)
        *pnCount = 0;
    if (ppHandle)
        *ppHandle = NULL;
    if (!g_pFileMgr)
        return NULL;

    FileEntry *pList = g_pFileMgr->GetFileList(pszDirectory ? pszDirectory : "");
    if (!pList)
        return NULL;

    int nCount = 0;
    for (FileEntry *pEntry = pList; pEntry; pEntry = pEntry->m_pNext)
        ++nCount;

    FearFileInfo *pInfos = (FearFileInfo *)malloc(sizeof(FearFileInfo) * (size_t)nCount);
    if (pInfos)
    {
        int nIndex = 0;
        for (FileEntry *pEntry = pList; pEntry && nIndex < nCount; pEntry = pEntry->m_pNext, ++nIndex)
        {
            const char *pszName = pEntry->m_pBaseFilename ? pEntry->m_pBaseFilename : "";
            const char *pszFullName = pEntry->m_pFullFilename ? pEntry->m_pFullFilename : "";

            strncpy(pInfos[nIndex].m_Name, pszName, sizeof(pInfos[nIndex].m_Name) - 1);
            pInfos[nIndex].m_Name[sizeof(pInfos[nIndex].m_Name) - 1] = 0;
            strncpy(pInfos[nIndex].m_FullName, pszFullName, sizeof(pInfos[nIndex].m_FullName) - 1);
            pInfos[nIndex].m_FullName[sizeof(pInfos[nIndex].m_FullName) - 1] = 0;
            pInfos[nIndex].m_Type = pEntry->m_Type;
        }

        if (pnCount)
            *pnCount = nCount;
        if (ppHandle)
            *ppHandle = pInfos;
    }

    FearFreeEngineFileList(pList);
    return pInfos;
}

void FearFile_FreeList(void *hHandle)
{
    if (hHandle)
        free(hHandle);
}
