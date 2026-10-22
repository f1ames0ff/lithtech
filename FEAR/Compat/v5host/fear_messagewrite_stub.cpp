#include "fear_messagewrite_stub.h"
#include "fear_messageread_stub.h"
#include "fear_object_map.h"
#include <string.h>
#include <wchar.h>

static uint32 FearFloatToBits(float fValue)
{
    uint32 nValue;
    memcpy(&nValue, &fValue, sizeof(nValue));
    return nValue;
}

CFearMessageWrite g_FearMessageWrite;

void CFearMessageWrite::WriteBit(uint32 nPos, uint32 nValue)
{
    uint32 nByte = nPos >> 3;
    if (nByte >= m_Data.size())
        m_Data.resize(nByte + 1, 0);

    uint8 nMask = (uint8)(1 << (nPos & 7));
    if (nValue)
        m_Data[nByte] = (uint8)(m_Data[nByte] | nMask);
    else
        m_Data[nByte] = (uint8)(m_Data[nByte] & (uint8)(~nMask));
}

void CFearMessageWrite::Reset()
{
    m_Data.clear();
    m_nBitPos = 0;
}

ILTMessage_Read * CFearMessageWrite::Read()
{
    g_FearMessageRead.SetData(m_Data.empty() ? NULL : &m_Data[0], m_nBitPos);
    return &g_FearMessageRead;
}

uint32 CFearMessageWrite::Size() const
{
    return m_nBitPos;
}

void CFearMessageWrite::WriteBits(uint32 nValue, uint32 nSize)
{
    if (nSize > 32)
        nSize = 32;

    for (uint32 i = 0; i < nSize; i++)
        WriteBit(m_nBitPos + i, (nValue >> i) & 1);

    m_nBitPos += nSize;
}

void CFearMessageWrite::WriteBits64(uint64 nValue, uint32 nSize)
{
    if (nSize > 64)
        nSize = 64;

    for (uint32 i = 0; i < nSize; i++)
        WriteBit(m_nBitPos + i, (uint32)((nValue >> i) & 1));

    m_nBitPos += nSize;
}

void CFearMessageWrite::WriteData(const void *pData, uint32 nSize)
{
    if (!pData || (nSize == 0))
        return;

    const uint8 *pBytes = (const uint8 *)pData;
    for (uint32 i = 0; i < nSize; i++)
        WriteBit(m_nBitPos + i, (pBytes[i >> 3] >> (i & 7)) & 1);

    m_nBitPos += nSize;
}

void CFearMessageWrite::WriteMessage(const ILTMessage_Read *pMsg)
{
    if (!pMsg)
    {
        WriteBits(0, 16);
        return;
    }
    uint32 nSize = pMsg->Size();
    WriteBits(nSize, 16);
    WriteMessageRaw(pMsg);
}

void CFearMessageWrite::WriteMessageRaw(const ILTMessage_Read *pMsg)
{
    if (!pMsg)
        return;
    ILTMessage_Read *pRead = const_cast<ILTMessage_Read *>(pMsg);
    uint32 nSaved = pRead->Tell();
    pRead->SeekTo(0);
    uint32 nRemaining = pMsg->Size();
    while (nRemaining >= 32)
    {
        WriteBits(pRead->ReadBits(32), 32);
        nRemaining -= 32;
    }
    if (nRemaining)
        WriteBits(pRead->ReadBits(nRemaining), nRemaining);
    pRead->SeekTo(nSaved);
}

void CFearMessageWrite::WriteString(const char *pString)
{
    if (!pString)
        pString = "";
    while (*pString)
    {
        WriteBits((uint32)(uint8)*pString, 8);
        ++pString;
    }
    WriteBits(0, 8);
}

void CFearMessageWrite::WriteWString(const wchar_t *pwString)
{
    if (!pwString)
        pwString = L"";
    while (*pwString)
    {
        WriteBits((uint32)(uint16)*pwString, 16);
        ++pwString;
    }
    WriteBits(0, 16);
}

void CFearMessageWrite::WriteCompLTVector(const LTVector &vVec)
{
    WriteBits(FearFloatToBits(vVec.x), 32);
    WriteBits(FearFloatToBits(vVec.y), 32);
    WriteBits(FearFloatToBits(vVec.z), 32);
}

void CFearMessageWrite::WriteCompPos(const LTVector &vPos)
{
    WriteCompLTVector(vPos);
}

void CFearMessageWrite::WriteCompLTRotation(const LTRotation &cRotation)
{
    WriteBits(FearFloatToBits(cRotation.m_Quat[0]), 32);
    WriteBits(FearFloatToBits(cRotation.m_Quat[1]), 32);
    WriteBits(FearFloatToBits(cRotation.m_Quat[2]), 32);
    WriteBits(FearFloatToBits(cRotation.m_Quat[3]), 32);
}

void CFearMessageWrite::WriteObject(HOBJECT hObj)
{
    unsigned short nServerId = FearClient_GetServerObjectId(hObj);
    WriteBits(nServerId, 16);
}

void CFearMessageWrite::WriteYRotation(const LTRotation &cRotation)
{
    WriteCompLTRotation(cRotation);
}

void CFearMessageWrite::WriteDatabaseRecord(IDatabaseMgr *pDatabase, HRECORD hRecord)
{
    WriteBits64(reinterpret_cast<uint64>(hRecord), 64);
}

void CFearMessageWrite::WriteTimer(HENGINETIMER hTimer)
{
    WriteBits64(reinterpret_cast<uint64>(hTimer), 64);
}

void CFearMessageWrite::WriteCompLTPolarCoord(const LTPolarCoord &vVec)
{
    WriteData(&vVec, (uint32)sizeof(vVec) * 8);
}
