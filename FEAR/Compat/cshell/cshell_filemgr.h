#ifndef __FEAR_CSHELL_FILEMGR_H__
#define __FEAR_CSHELL_FILEMGR_H__

#include "fear_host_api.h"

#ifdef __cplusplus
extern "C" {
#endif

void *FearFile_Open(const char *pszFilename);
int FearFile_Read(void *hFile, void *pBuffer, unsigned int nSize);
int FearFile_Seek(void *hFile, unsigned long long nOffset);
unsigned long long FearFile_Tell(void *hFile);
unsigned long long FearFile_Size(void *hFile);
void FearFile_Close(void *hFile);
const FearFileInfo *FearFile_List(const char *pszDirectory, int *pnCount, void **ppHandle);
void FearFile_FreeList(void *hHandle);

#ifdef __cplusplus
}
#endif

#endif
