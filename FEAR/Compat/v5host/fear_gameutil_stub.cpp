#include "fear_gameutil_stub.h"
#include <stdio.h>

template <typename T> struct SvcDefault { static T Get() { return T(); } };
template <> struct SvcDefault<LTRESULT> { static LTRESULT Get() { return LT_UNSUPPORTED; } };
template <> struct SvcDefault<const char*> { static const char* Get() { return ""; } };

CFearGameUtil g_FearGameUtil;
CFearPunkBusterClient g_FearPunkBusterClient;
CFearPunkBusterServer g_FearPunkBusterServer;

IGameSpyServer* CFearGameUtil::CreateGameSpyServer(IGameSpyServer::StartupInfo const& startupInfo)
{
    printf("STUB ILTGameUtil::CreateGameSpyServer\n");
    return SvcDefault<IGameSpyServer*>::Get();
}

void CFearGameUtil::DestroyGameSpyServer(IGameSpyServer* pGameSpyServer)
{
    printf("STUB ILTGameUtil::DestroyGameSpyServer\n");
}

IGameSpyBrowser* CFearGameUtil::CreateGameSpyBrowser(IGameSpyBrowser::StartupInfo const& startupInfo)
{
    printf("STUB ILTGameUtil::CreateGameSpyBrowser\n");
    return SvcDefault<IGameSpyBrowser*>::Get();
}

void CFearGameUtil::DestroyGameSpyBrowser(IGameSpyBrowser* pGameSpyBrowser)
{
    printf("STUB ILTGameUtil::DestroyGameSpyBrowser\n");
}

IGameSpyPatchInfo* CFearGameUtil::CreateGameSpyPatchInfo(IGameSpyPatchInfo::StartupInfo const& startupInfo)
{
    return SvcDefault<IGameSpyPatchInfo*>::Get();
}

void CFearGameUtil::DestroyGameSpyPatchInfo(IGameSpyPatchInfo* pGameSpyPatchInfo)
{
    printf("STUB ILTGameUtil::DestroyGameSpyPatchInfo\n");
}

bool CFearGameUtil::IsCDKeyValid(char const* pszCDKey)
{
    printf("STUB ILTGameUtil::IsCDKeyValid\n");
    return SvcDefault<bool>::Get();
}

bool CFearGameUtil::CDKeyComputeChallengeResponse(char const* pszCDKey, char const* pszChallenge, char* pszChallengeResponse, uint32& nChallengeResponseLen)
{
    printf("STUB ILTGameUtil::CDKeyComputeChallengeResponse\n");
    return SvcDefault<bool>::Get();
}

void CFearGameUtil::WriteHandshakingVersion(ILTMessage_Write& msg, uint16 nHandshakingVersion, uint16 nHandshakingVersionMask)
{
    printf("STUB ILTGameUtil::WriteHandshakingVersion\n");
}

uint16 CFearGameUtil::ReadHandshakingVersion(ILTMessage_Read& msg, uint16 nHandshakingVersionMask)
{
    printf("STUB ILTGameUtil::ReadHandshakingVersion\n");
    return SvcDefault<uint16>::Get();
}

void CFearGameUtil::WriteClientKey1(ILTMessage_Write& msg, uint32 nClientKey1)
{
    printf("STUB ILTGameUtil::WriteClientKey1\n");
}

uint32 CFearGameUtil::ReadClientKey1(ILTMessage_Read& msg)
{
    printf("STUB ILTGameUtil::ReadClientKey1\n");
    return SvcDefault<uint32>::Get();
}

void CFearGameUtil::WriteServerKey(ILTMessage_Write& msg, uint32 nClientKey1, LTGUID const& guid, uint32 nServerKey)
{
    printf("STUB ILTGameUtil::WriteServerKey\n");
}

uint32 CFearGameUtil::ReadServerKey(ILTMessage_Read& msg, uint32 nClientKey1, LTGUID const& guid)
{
    printf("STUB ILTGameUtil::ReadServerKey\n");
    return SvcDefault<uint32>::Get();
}

bool CFearGameUtil::WriteCRCMessage(ILTMessage_Write& msg, LTGUID const& guid, uint32 nServerKey, void* pGameClient, char const* pszDBFile)
{
    printf("STUB ILTGameUtil::WriteCRCMessage\n");
    return SvcDefault<bool>::Get();
}

bool CFearGameUtil::ReadCRCMessage(ILTMessage_Read& msg, LTGUID const& guid, uint32 nServerKey, char const* pszDBFile)
{
    printf("STUB ILTGameUtil::ReadCRCMessage\n");
    return SvcDefault<bool>::Get();
}

void CFearGameUtil::WriteServerKeyData(ILTMessage_Write& msg, LTGUID const& guid, uint32 nServerKey, uint32 nData)
{
    printf("STUB ILTGameUtil::WriteServerKeyData\n");
}

uint32 CFearGameUtil::ReadServerKeyData(ILTMessage_Read& msg, LTGUID const& guid, uint32 nServerKey)
{
    printf("STUB ILTGameUtil::ReadServerKeyData\n");
    return SvcDefault<uint32>::Get();
}

void CFearGameUtil::WritePunkBusterState(ILTMessage_Write& msg, LTGUID const& guid, uint32 nServerKey, IPunkBusterClient* pPunkBusterClient)
{
    printf("STUB ILTGameUtil::WritePunkBusterState\n");
}

bool CFearGameUtil::ReadPunkBusterState(ILTMessage_Read& msg, LTGUID const& guid, uint32 nServerKey)
{
    printf("STUB ILTGameUtil::ReadPunkBusterState\n");
    return SvcDefault<bool>::Get();
}

IPunkBusterServer* CFearGameUtil::CreatePunkBusterServer(IPunkBusterServer::StartupInfo const& startupInfo)
{
    return &g_FearPunkBusterServer;
}

void CFearGameUtil::DestroyPunkBusterServer(IPunkBusterServer* pPunkBusterServer)
{
    printf("STUB ILTGameUtil::DestroyPunkBusterServer\n");
}

IPunkBusterClient* CFearGameUtil::CreatePunkBusterClient(IPunkBusterClient::StartupInfo const& startupInfo)
{
    return &g_FearPunkBusterClient;
}

void CFearGameUtil::DestroyPunkBusterClient(IPunkBusterClient* pPunkBusterClient)
{
    printf("STUB ILTGameUtil::DestroyPunkBusterClient\n");
}

void CFearPunkBusterClient::Initialize() {}
void CFearPunkBusterClient::SetPointers() {}
bool CFearPunkBusterClient::IsEnabled() { return false; }
bool CFearPunkBusterClient::IsEnabledRequested() { return false; }
bool CFearPunkBusterClient::Enable() { return false; }
bool CFearPunkBusterClient::Disable() { return false; }
void CFearPunkBusterClient::ProcessEvents() {}
bool CFearPunkBusterClient::HandleNetMessage(uint8* pData, uint32 nDataLen, uint8 senderAddr[4], uint16 nPort) { return false; }
void CFearPunkBusterClient::ClearCallbacks() {}
void CFearPunkBusterClient::ProcessCmd(char* cmd) {}
void CFearPunkBusterClient::CaptureConsoleOutput(char* msg, int msglen) {}

void CFearPunkBusterServer::Initialize() {}
void CFearPunkBusterServer::SetPointers() {}
bool CFearPunkBusterServer::IsEnabled() { return false; }
bool CFearPunkBusterServer::IsEnabledRequested() { return false; }
bool CFearPunkBusterServer::Enable() { return false; }
bool CFearPunkBusterServer::Disable() { return false; }
void CFearPunkBusterServer::ProcessEvents() {}
bool CFearPunkBusterServer::HandleNetMessage(uint8* pData, uint32 nDataLen, uint8 senderAddr[4], uint16 nPort) { return false; }
void CFearPunkBusterServer::ProcessCmd(char* cmd) {}
void CFearPunkBusterServer::ClearCallbacks() {}
char* CFearPunkBusterServer::IsClientBanned(char* fromAddr, int cl_pb, const char* cl_guid) { return NULL; }
void CFearPunkBusterServer::CaptureConsoleOutput(char* msg, int msglen) {}
