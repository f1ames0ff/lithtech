#include "fear_messageread_stub.h"
#include <string.h>
#include <wchar.h>

CFearMessageRead g_FearMessageRead;

void CFearMessageRead::SetData(const uint8 *pData, uint32 nBits)
{
    m_Storage.clear();
    m_nDataBits = nBits;
    m_nReadPos = 0;
    if (!pData || nBits == 0)
    {
        m_pData = NULL;
        return;
    }

    uint32 nBytes = (nBits + 7) >> 3;
    m_Storage.assign(pData, pData + nBytes);
    uint32 nRemainder = nBits & 7;
    if (nRemainder)
        m_Storage[nBytes - 1] &= (uint8)((1u << nRemainder) - 1u);
    m_pData = m_Storage.data();
}

CFearMessageRead *CFearMessageRead::CreateFromBits(const uint8 *pData, uint32 nStart, uint32 nBits) const
{
    CFearMessageRead *pResult = new CFearMessageRead;
    if (!pData || nBits == 0)
    {
        pResult->SetData(NULL, 0);
        return pResult;
    }

    std::vector<uint8> bits((nBits + 7) >> 3, 0);
    for (uint32 i = 0; i < nBits; ++i)
    {
        uint32 nSource = nStart + i;
        if (nSource >= m_nDataBits || (pData[nSource >> 3] & (1u << (nSource & 7))))
            bits[i >> 3] |= (uint8)(1u << (i & 7));
    }
    pResult->SetData(bits.data(), nBits);
    return pResult;
}

ILTMessage_Read *CFearMessageRead::Clone() const
{
    return CreateFromBits(m_pData, 0, m_nDataBits);
}

ILTMessage_Read *CFearMessageRead::SubMsg(uint32 nPos) const
{
    uint32 nStart = (nPos < m_nDataBits) ? nPos : m_nDataBits;
    return CreateFromBits(m_pData, nStart, m_nDataBits - nStart);
}

ILTMessage_Read *CFearMessageRead::SubMsg(uint32 nPos, uint32 nLength) const
{
    uint32 nStart = (nPos < m_nDataBits) ? nPos : m_nDataBits;
    uint32 nAvailable = m_nDataBits - nStart;
    return CreateFromBits(m_pData, nStart, (nLength < nAvailable) ? nLength : nAvailable);
}

uint32 CFearMessageRead::Size() const
{
    return m_nDataBits;
}

void CFearMessageRead::Seek(int32 nOffset)
{
    int64 nPos = (int64)m_nReadPos + nOffset;
    if (nPos < 0)
        nPos = 0;
    if (nPos > m_nDataBits)
        nPos = m_nDataBits;
    m_nReadPos = (uint32)nPos;
}

void CFearMessageRead::SeekTo(uint32 nPos)
{
    m_nReadPos = (nPos < m_nDataBits) ? nPos : m_nDataBits;
}

uint32 CFearMessageRead::Tell() const
{
    return m_nReadPos;
}

uint32 CFearMessageRead::TellEnd() const
{
    return m_nDataBits - m_nReadPos;
}

bool CFearMessageRead::EOM() const
{
    return m_nReadPos >= m_nDataBits;
}

uint32 CFearMessageRead::ReadBits(uint32 nBits)
{
    if (nBits > 32)
        nBits = 32;
    uint32 nValue = 0;
    for (uint32 i = 0; i < nBits; ++i)
    {
        uint32 nPos = m_nReadPos + i;
        if (nPos < m_nDataBits && m_pData && (m_pData[nPos >> 3] & (1u << (nPos & 7))))
            nValue |= (1u << i);
    }
    m_nReadPos += nBits;
    if (m_nReadPos > m_nDataBits)
        m_nReadPos = m_nDataBits;
    return nValue;
}

uint64 CFearMessageRead::ReadBits64(uint32 nBits)
{
    if (nBits > 64)
        nBits = 64;
    uint64 nValue = 0;
    for (uint32 i = 0; i < nBits; ++i)
    {
        uint32 nPos = m_nReadPos + i;
        if (nPos < m_nDataBits && m_pData && (m_pData[nPos >> 3] & (1u << (nPos & 7))))
            nValue |= ((uint64)1 << i);
    }
    m_nReadPos += nBits;
    if (m_nReadPos > m_nDataBits)
        m_nReadPos = m_nDataBits;
    return nValue;
}

void CFearMessageRead::ReadData(void *pData, uint32 nBits)
{
    if (!pData)
        return;
    uint8 *pDest = (uint8 *)pData;
    memset(pDest, 0, (nBits + 7) >> 3);
    for (uint32 i = 0; i < nBits; ++i)
    {
        uint32 nPos = m_nReadPos + i;
        if (nPos < m_nDataBits && m_pData && (m_pData[nPos >> 3] & (1u << (nPos & 7))))
            pDest[i >> 3] |= (uint8)(1u << (i & 7));
    }
    m_nReadPos += nBits;
    if (m_nReadPos > m_nDataBits)
        m_nReadPos = m_nDataBits;
}

ILTMessage_Read *CFearMessageRead::ReadMessage()
{
    uint32 nSize = ReadBits(16);
    ILTMessage_Read *pResult = SubMsg(m_nReadPos, nSize);
    m_nReadPos += nSize;
    if (m_nReadPos > m_nDataBits)
        m_nReadPos = m_nDataBits;
    return pResult;
}

uint32 CFearMessageRead::ReadString(char *pDest, uint32 nMaxLen)
{
    uint32 nWritten = 0;
    for (;;)
    {
        char c = (char)ReadBits(8);
        if (pDest && nWritten + 1 < nMaxLen)
            pDest[nWritten] = c;
        if (c == 0)
            break;
        ++nWritten;
        if (EOM())
            break;
    }
    if (pDest && nMaxLen)
        pDest[(nWritten < nMaxLen) ? nWritten : (nMaxLen - 1)] = 0;
    return nWritten;
}

uint32 CFearMessageRead::ReadWString(wchar_t *pwDest, uint32 nMaxLen)
{
    uint32 nWritten = 0;
    for (;;)
    {
        wchar_t c = (wchar_t)ReadBits(16);
        if (pwDest && nWritten + 1 < nMaxLen)
            pwDest[nWritten] = c;
        if (c == 0)
            break;
        ++nWritten;
        if (EOM())
            break;
    }
    if (pwDest && nMaxLen)
        pwDest[(nWritten < nMaxLen) ? nWritten : (nMaxLen - 1)] = 0;
    return nWritten;
}

LTVector CFearMessageRead::ReadCompLTVector()
{
    LTVector v;
    uint32 nBits = ReadBits(32);
    memcpy(&v.x, &nBits, sizeof(float));
    nBits = ReadBits(32);
    memcpy(&v.y, &nBits, sizeof(float));
    nBits = ReadBits(32);
    memcpy(&v.z, &nBits, sizeof(float));
    return v;
}

LTVector CFearMessageRead::ReadCompPos()
{
    return ReadCompLTVector();
}

LTRotation CFearMessageRead::ReadCompLTRotation()
{
    LTRotation r;
    for (int i = 0; i < 4; ++i)
    {
        uint32 nBits = ReadBits(32);
        memcpy(&r.m_Quat[i], &nBits, sizeof(float));
    }
    return r;
}

HOBJECT CFearMessageRead::ReadObject()
{
    uint32 nID = ReadBits(16);
    return (nID == 0xFFFF) ? NULL : (HOBJECT)(uintptr_t)nID;
}

LTRotation CFearMessageRead::ReadYRotation()
{
    return ReadCompLTRotation();
}

HRECORD CFearMessageRead::ReadDatabaseRecord(IDatabaseMgr *, HCATEGORY)
{
    return (HRECORD)(uintptr_t)ReadBits(16);
}

LTPolarCoord CFearMessageRead::ReadCompLTPolarCoord()
{
    LTPolarCoord p;
    memset(&p, 0, sizeof(p));
    ReadData(&p, sizeof(p) * 8);
    return p;
}

HENGINETIMER CFearMessageRead::ReadTimer()
{
    return (HENGINETIMER)(uintptr_t)ReadBits(16);
}

uint32 CFearMessageRead::PeekBits(uint32 nBits) const
{
    CFearMessageRead *pSelf = const_cast<CFearMessageRead *>(this);
    uint32 nSaved = pSelf->m_nReadPos;
    uint32 nValue = pSelf->ReadBits(nBits);
    pSelf->m_nReadPos = nSaved;
    return nValue;
}

uint64 CFearMessageRead::PeekBits64(uint32 nBits) const
{
    CFearMessageRead *pSelf = const_cast<CFearMessageRead *>(this);
    uint32 nSaved = pSelf->m_nReadPos;
    uint64 nValue = pSelf->ReadBits64(nBits);
    pSelf->m_nReadPos = nSaved;
    return nValue;
}

void CFearMessageRead::PeekData(void *pData, uint32 nBits) const
{
    CFearMessageRead *pSelf = const_cast<CFearMessageRead *>(this);
    uint32 nSaved = pSelf->m_nReadPos;
    pSelf->ReadData(pData, nBits);
    pSelf->m_nReadPos = nSaved;
}

ILTMessage_Read *CFearMessageRead::PeekMessage() const
{
    CFearMessageRead *pSelf = const_cast<CFearMessageRead *>(this);
    uint32 nSaved = pSelf->m_nReadPos;
    ILTMessage_Read *pResult = pSelf->ReadMessage();
    pSelf->m_nReadPos = nSaved;
    return pResult;
}

uint32 CFearMessageRead::PeekString(char *pDest, uint32 nMaxLen) const
{
    CFearMessageRead *pSelf = const_cast<CFearMessageRead *>(this);
    uint32 nSaved = pSelf->m_nReadPos;
    uint32 nResult = pSelf->ReadString(pDest, nMaxLen);
    pSelf->m_nReadPos = nSaved;
    return nResult;
}

uint32 CFearMessageRead::PeekWString(wchar_t *pwDest, uint32 nMaxLen) const
{
    CFearMessageRead *pSelf = const_cast<CFearMessageRead *>(this);
    uint32 nSaved = pSelf->m_nReadPos;
    uint32 nResult = pSelf->ReadWString(pwDest, nMaxLen);
    pSelf->m_nReadPos = nSaved;
    return nResult;
}

LTVector CFearMessageRead::PeekCompLTVector() const
{
    CFearMessageRead *pSelf = const_cast<CFearMessageRead *>(this);
    uint32 nSaved = pSelf->m_nReadPos;
    LTVector v = pSelf->ReadCompLTVector();
    pSelf->m_nReadPos = nSaved;
    return v;
}

LTVector CFearMessageRead::PeekCompPos() const
{
    return PeekCompLTVector();
}

LTRotation CFearMessageRead::PeekCompLTRotation() const
{
    CFearMessageRead *pSelf = const_cast<CFearMessageRead *>(this);
    uint32 nSaved = pSelf->m_nReadPos;
    LTRotation r = pSelf->ReadCompLTRotation();
    pSelf->m_nReadPos = nSaved;
    return r;
}

HOBJECT CFearMessageRead::PeekObject() const
{
    CFearMessageRead *pSelf = const_cast<CFearMessageRead *>(this);
    uint32 nSaved = pSelf->m_nReadPos;
    HOBJECT hObj = pSelf->ReadObject();
    pSelf->m_nReadPos = nSaved;
    return hObj;
}

LTRotation CFearMessageRead::PeekYRotation() const
{
    return PeekCompLTRotation();
}

HRECORD CFearMessageRead::PeekDatabaseRecord(IDatabaseMgr *pDatabase, HCATEGORY hCategory)
{
    uint32 nSaved = m_nReadPos;
    HRECORD hRecord = ReadDatabaseRecord(pDatabase, hCategory);
    m_nReadPos = nSaved;
    return hRecord;
}
