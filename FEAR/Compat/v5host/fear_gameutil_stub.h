#ifndef __FEAR_GAMEUTIL_STUB_H__
#define __FEAR_GAMEUTIL_STUB_H__

#include "ltbasedefs.h"
#include "iltgameutil.h"
#include "ipunkbuster.h"

class CFearGameUtil : public ILTGameUtil
{
public:
    declare_interface(CFearGameUtil)
    CFearGameUtil() = default;
    ~CFearGameUtil() {}

    IGameSpyServer* CreateGameSpyServer(IGameSpyServer::StartupInfo const& startupInfo) override;
    void DestroyGameSpyServer(IGameSpyServer* pGameSpyServer) override;
    IGameSpyBrowser* CreateGameSpyBrowser(IGameSpyBrowser::StartupInfo const& startupInfo) override;
    void DestroyGameSpyBrowser(IGameSpyBrowser* pGameSpyBrowser) override;
    IGameSpyPatchInfo* CreateGameSpyPatchInfo(IGameSpyPatchInfo::StartupInfo const& startupInfo) override;
    void DestroyGameSpyPatchInfo(IGameSpyPatchInfo* pGameSpyPatchInfo) override;
    bool IsCDKeyValid(char const* pszCDKey) override;
    bool CDKeyComputeChallengeResponse(char const* pszCDKey, char const* pszChallenge, char* pszChallengeResponse, uint32& nChallengeResponseLen) override;
    void WriteHandshakingVersion(ILTMessage_Write& msg, uint16 nHandshakingVersion, uint16 nHandshakingVersionMask) override;
    uint16 ReadHandshakingVersion(ILTMessage_Read& msg, uint16 nHandshakingVersionMask) override;
    void WriteClientKey1(ILTMessage_Write& msg, uint32 nClientKey1) override;
    uint32 ReadClientKey1(ILTMessage_Read& msg) override;
    void WriteServerKey(ILTMessage_Write& msg, uint32 nClientKey1, LTGUID const& guid, uint32 nServerKey) override;
    uint32 ReadServerKey(ILTMessage_Read& msg, uint32 nClientKey1, LTGUID const& guid) override;
    bool WriteCRCMessage(ILTMessage_Write& msg, LTGUID const& guid, uint32 nServerKey, void* pGameClient, char const* pszDBFile) override;
    bool ReadCRCMessage(ILTMessage_Read& msg, LTGUID const& guid, uint32 nServerKey, char const* pszDBFile) override;
    void WriteServerKeyData(ILTMessage_Write& msg, LTGUID const& guid, uint32 nServerKey, uint32 nData) override;
    uint32 ReadServerKeyData(ILTMessage_Read& msg, LTGUID const& guid, uint32 nServerKey) override;
    void WritePunkBusterState(ILTMessage_Write& msg, LTGUID const& guid, uint32 nServerKey, IPunkBusterClient* pPunkBusterClient) override;
    bool ReadPunkBusterState(ILTMessage_Read& msg, LTGUID const& guid, uint32 nServerKey) override;
    IPunkBusterServer* CreatePunkBusterServer(IPunkBusterServer::StartupInfo const& startupInfo) override;
    void DestroyPunkBusterServer(IPunkBusterServer* pPunkBusterServer) override;
    IPunkBusterClient* CreatePunkBusterClient(IPunkBusterClient::StartupInfo const& startupInfo) override;
    void DestroyPunkBusterClient(IPunkBusterClient* pPunkBusterClient) override;
};


class CFearPunkBusterClient : public IPunkBusterClient
{
public:
    CFearPunkBusterClient() {}
    ~CFearPunkBusterClient() {}

    void Initialize() override;
    void SetPointers() override;
    bool IsEnabled() override;
    bool IsEnabledRequested() override;
    bool Enable() override;
    bool Disable() override;
    void ProcessEvents() override;
    bool HandleNetMessage(uint8* pData, uint32 nDataLen, uint8 senderAddr[4], uint16 nPort) override;
    void ClearCallbacks() override;
    void ProcessCmd(char* cmd) override;
    void CaptureConsoleOutput(char* msg, int msglen) override;
};

class CFearPunkBusterServer : public IPunkBusterServer
{
public:
    CFearPunkBusterServer() {}
    ~CFearPunkBusterServer() {}

    void Initialize() override;
    void SetPointers() override;
    bool IsEnabled() override;
    bool IsEnabledRequested() override;
    bool Enable() override;
    bool Disable() override;
    void ProcessEvents() override;
    bool HandleNetMessage(uint8* pData, uint32 nDataLen, uint8 senderAddr[4], uint16 nPort) override;
    void ProcessCmd(char* cmd) override;
    void ClearCallbacks() override;
    char* IsClientBanned(char* fromAddr, int cl_pb, const char* cl_guid) override;
    void CaptureConsoleOutput(char* msg, int msglen) override;
};

extern CFearPunkBusterClient g_FearPunkBusterClient;
extern CFearPunkBusterServer g_FearPunkBusterServer;

extern CFearGameUtil g_FearGameUtil;

#endif
