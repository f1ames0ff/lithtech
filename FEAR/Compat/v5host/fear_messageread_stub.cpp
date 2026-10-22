#include "fear_messageread_stub.h"
#include <stdio.h>

template <typename T> struct MsgDefault { static T Get() { return T(); } };
template <> struct MsgDefault<LTRESULT> { static LTRESULT Get() { return LT_OK; } };
template <> struct MsgDefault<const char*> { static const char* Get() { return ""; } };

CFearMessageRead g_FearMessageRead;

void CFearMessageRead::SetData(const uint8 *pData, uint32 nBits)
{
    m_pData = pData;
    m_nDataBits = nBits;
    m_nReadPos = 0;
}

ILTMessage_Read * CFearMessageRead::Clone() const
{
    printf("STUB ILTMessage_Read::Clone\n");
    return MsgDefault<ILTMessage_Read *>::Get();
}

ILTMessage_Read * CFearMessageRead::SubMsg(uint32 nPos) const
{
    printf("STUB ILTMessage_Read::SubMsg\n");
    return MsgDefault<ILTMessage_Read *>::Get();
}

ILTMessage_Read * CFearMessageRead::SubMsg(uint32 nPos, uint32 nLength) const
{
    printf("STUB ILTMessage_Read::SubMsg\n");
    return MsgDefault<ILTMessage_Read *>::Get();
}

uint32 CFearMessageRead::Size() const
{
    printf("STUB ILTMessage_Read::Size\n");
    return MsgDefault<uint32>::Get();
}

void CFearMessageRead::Seek(int32 nOffset)
{
    printf("STUB ILTMessage_Read::Seek\n");
}

void CFearMessageRead::SeekTo(uint32 nPos)
{
    printf("STUB ILTMessage_Read::SeekTo\n");
}

uint32 CFearMessageRead::Tell() const
{
    printf("STUB ILTMessage_Read::Tell\n");
    return MsgDefault<uint32>::Get();
}

uint32 CFearMessageRead::TellEnd() const
{
    printf("STUB ILTMessage_Read::TellEnd\n");
    return MsgDefault<uint32>::Get();
}

bool CFearMessageRead::EOM() const
{
    printf("STUB ILTMessage_Read::EOM\n");
    return MsgDefault<bool>::Get();
}

uint32 CFearMessageRead::ReadBits(uint32 nBits)
{
    printf("STUB ILTMessage_Read::ReadBits\n");
    return MsgDefault<uint32>::Get();
}

uint64 CFearMessageRead::ReadBits64(uint32 nBits)
{
    printf("STUB ILTMessage_Read::ReadBits64\n");
    return MsgDefault<uint64>::Get();
}

void CFearMessageRead::ReadData(void *pData, uint32 nBits)
{
    printf("STUB ILTMessage_Read::ReadData\n");
}

ILTMessage_Read * CFearMessageRead::ReadMessage()
{
    printf("STUB ILTMessage_Read::ReadMessage\n");
    return MsgDefault<ILTMessage_Read *>::Get();
}

uint32 CFearMessageRead::ReadString(char *pDest, uint32 nMaxLen)
{
    printf("STUB ILTMessage_Read::ReadString\n");
    return MsgDefault<uint32>::Get();
}

uint32 CFearMessageRead::ReadWString(wchar_t *pwDest, uint32 nMaxLen)
{
    printf("STUB ILTMessage_Read::ReadWString\n");
    return MsgDefault<uint32>::Get();
}

LTVector CFearMessageRead::ReadCompLTVector()
{
    printf("STUB ILTMessage_Read::ReadCompLTVector\n");
    return MsgDefault<LTVector>::Get();
}

LTVector CFearMessageRead::ReadCompPos()
{
    printf("STUB ILTMessage_Read::ReadCompPos\n");
    return MsgDefault<LTVector>::Get();
}

LTRotation CFearMessageRead::ReadCompLTRotation()
{
    printf("STUB ILTMessage_Read::ReadCompLTRotation\n");
    return MsgDefault<LTRotation>::Get();
}

HOBJECT CFearMessageRead::ReadObject()
{
    printf("STUB ILTMessage_Read::ReadObject\n");
    return MsgDefault<HOBJECT>::Get();
}

LTRotation CFearMessageRead::ReadYRotation()
{
    printf("STUB ILTMessage_Read::ReadYRotation\n");
    return MsgDefault<LTRotation>::Get();
}

HRECORD CFearMessageRead::ReadDatabaseRecord(IDatabaseMgr *pDatabase, HCATEGORY hCategory)
{
    printf("STUB ILTMessage_Read::ReadDatabaseRecord\n");
    return MsgDefault<HRECORD>::Get();
}

LTPolarCoord CFearMessageRead::ReadCompLTPolarCoord()
{
    printf("STUB ILTMessage_Read::ReadCompLTPolarCoord\n");
    return MsgDefault<LTPolarCoord>::Get();
}

HENGINETIMER CFearMessageRead::ReadTimer()
{
    printf("STUB ILTMessage_Read::ReadTimer\n");
    return MsgDefault<HENGINETIMER>::Get();
}

uint32 CFearMessageRead::PeekBits(uint32 nBits) const
{
    printf("STUB ILTMessage_Read::PeekBits\n");
    return MsgDefault<uint32>::Get();
}

uint64 CFearMessageRead::PeekBits64(uint32 nBits) const
{
    printf("STUB ILTMessage_Read::PeekBits64\n");
    return MsgDefault<uint64>::Get();
}

void CFearMessageRead::PeekData(void *pData, uint32 nBits) const
{
    printf("STUB ILTMessage_Read::PeekData\n");
}

ILTMessage_Read * CFearMessageRead::PeekMessage() const
{
    printf("STUB ILTMessage_Read::PeekMessage\n");
    return MsgDefault<ILTMessage_Read *>::Get();
}

uint32 CFearMessageRead::PeekString(char *pDest, uint32 nMaxLen) const
{
    printf("STUB ILTMessage_Read::PeekString\n");
    return MsgDefault<uint32>::Get();
}

uint32 CFearMessageRead::PeekWString(wchar_t *pwDest, uint32 nMaxLen) const
{
    printf("STUB ILTMessage_Read::PeekWString\n");
    return MsgDefault<uint32>::Get();
}

LTVector CFearMessageRead::PeekCompLTVector() const
{
    printf("STUB ILTMessage_Read::PeekCompLTVector\n");
    return MsgDefault<LTVector>::Get();
}

LTVector CFearMessageRead::PeekCompPos() const
{
    printf("STUB ILTMessage_Read::PeekCompPos\n");
    return MsgDefault<LTVector>::Get();
}

LTRotation CFearMessageRead::PeekCompLTRotation() const
{
    printf("STUB ILTMessage_Read::PeekCompLTRotation\n");
    return MsgDefault<LTRotation>::Get();
}

HOBJECT CFearMessageRead::PeekObject() const
{
    printf("STUB ILTMessage_Read::PeekObject\n");
    return MsgDefault<HOBJECT>::Get();
}

LTRotation CFearMessageRead::PeekYRotation() const
{
    printf("STUB ILTMessage_Read::PeekYRotation\n");
    return MsgDefault<LTRotation>::Get();
}

HRECORD CFearMessageRead::PeekDatabaseRecord(IDatabaseMgr *pDatabase, HCATEGORY hCategory)
{
    printf("STUB ILTMessage_Read::PeekDatabaseRecord\n");
    return MsgDefault<HRECORD>::Get();
}
