#ifndef __FEAR_CONTENTTRANSFER_STUB_H__
#define __FEAR_CONTENTTRANSFER_STUB_H__

#include <windows.h>
#include "ltbasedefs.h"
#include "iltclientcontenttransfer.h"

class CFearClientContentTransfer : public ILTClientContentTransfer
{
public:
    declare_interface(CFearClientContentTransfer)

    CFearClientContentTransfer() {}
    ~CFearClientContentTransfer() {}

    LTRESULT SetNotificationSink(ILTContentTransferNotification *pNotificationSink, uint32 nNotificationOptions) override;
    LTRESULT SetOptions(const SClientContentTransferOptions &cOptions) override;
    LTRESULT OnMessage(CPacket_Read &cPacket) override;
    LTRESULT OnUpdate() override;
    LTRESULT AbortContentDownload() override;
    LTRESULT OnDisconnect() override;
    LTRESULT CheckForMissingArchiveFiles(SClientContentTransferArchiveFileInfo *ppArchiveFileList,
                                         uint32 nNumberOfArchiveFiles,
                                         bool &bIsMissingFiles,
                                         uint32 &nTotalMissingFilesSize) override;
};

extern CFearClientContentTransfer g_FearClientContentTransfer;

#endif
