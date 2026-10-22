#include "fear_contenttransfer_stub.h"
#include <stdio.h>
#include <string.h>

LTRESULT ILTClientContentTransfer::SetNotificationSink(ILTContentTransferNotification *pNotificationSink, uint32 nNotificationOptions)
{
    return LT_OK;
}

LTRESULT ILTClientContentTransfer::SetOptions(const SClientContentTransferOptions &cOptions)
{
    return LT_OK;
}

LTRESULT ILTClientContentTransfer::OnMessage(CPacket_Read &cPacket)
{
    return LT_OK;
}

LTRESULT ILTClientContentTransfer::OnUpdate()
{
    return LT_OK;
}

LTRESULT ILTClientContentTransfer::AbortContentDownload()
{
    return LT_OK;
}

LTRESULT ILTClientContentTransfer::OnDisconnect()
{
    return LT_OK;
}

LTRESULT ILTClientContentTransfer::CheckForMissingArchiveFiles(SClientContentTransferArchiveFileInfo *ppArchiveFileList,
                                                                uint32 nNumberOfArchiveFiles,
                                                                bool &bIsMissingFiles,
                                                                uint32 &nTotalMissingFilesSize)
{
    bIsMissingFiles = false;
    nTotalMissingFilesSize = 0;
    return LT_OK;
}

CFearClientContentTransfer g_FearClientContentTransfer;

LTRESULT CFearClientContentTransfer::SetNotificationSink(ILTContentTransferNotification *pNotificationSink, uint32 nNotificationOptions)
{
    return LT_OK;
}

LTRESULT CFearClientContentTransfer::SetOptions(const SClientContentTransferOptions &cOptions)
{
    static int s_nLogged = 0;
    if (!s_nLogged)
    {
        s_nLogged = 1;
        printf("FEARTRANSFER: SetOptions dir=%s download=%d redirect=%d max=%u\n",
               cOptions.m_szCustomContentDirectory,
               cOptions.m_bAllowContentDownload ? 1 : 0,
               cOptions.m_bAllowContentDownloadRedirect ? 1 : 0,
               cOptions.m_nMaxDownloadSize);
        fflush(stdout);
    }
    return LT_OK;
}

LTRESULT CFearClientContentTransfer::OnMessage(CPacket_Read &cPacket)
{
    return LT_OK;
}

LTRESULT CFearClientContentTransfer::OnUpdate()
{
    return LT_OK;
}

LTRESULT CFearClientContentTransfer::AbortContentDownload()
{
    return LT_OK;
}

LTRESULT CFearClientContentTransfer::OnDisconnect()
{
    return LT_OK;
}

LTRESULT CFearClientContentTransfer::CheckForMissingArchiveFiles(SClientContentTransferArchiveFileInfo *ppArchiveFileList,
                                                                 uint32 nNumberOfArchiveFiles,
                                                                 bool &bIsMissingFiles,
                                                                 uint32 &nTotalMissingFilesSize)
{
    bIsMissingFiles = false;
    nTotalMissingFilesSize = 0;
    return LT_OK;
}
