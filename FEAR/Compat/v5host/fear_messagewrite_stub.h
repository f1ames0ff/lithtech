#ifndef __FEAR_MESSAGEWRITE_STUB_H__
#define __FEAR_MESSAGEWRITE_STUB_H__

#include <vector>
#include "ltbasedefs.h"
#include "iltmessage.h"

class CFearMessageWrite : public ILTMessage_Write
{
public:
    declare_interface(CFearMessageWrite)
    CFearMessageWrite() : m_nBitPos(0) {}
    ~CFearMessageWrite() {}
    void AddRef() override {}
    void Release() override {}
    void StaticRelease() override {}
    void Free() override {}

    void Reset() override;
    ILTMessage_Read * Read() override;
    uint32 Size() const override;
    void WriteBits(uint32 nValue, uint32 nSize) override;
    void WriteBits64(uint64 nValue, uint32 nSize) override;
    void WriteData(const void *pData, uint32 nSize) override;
    void WriteMessage(const ILTMessage_Read *pMsg) override;
    void WriteMessageRaw(const ILTMessage_Read *pMsg) override;
    void WriteString(const char *pString) override;
    void WriteWString(const wchar_t *pwString) override;
    void WriteCompLTVector(const LTVector &vVec) override;
    void WriteCompPos(const LTVector &vPos) override;
    void WriteCompLTRotation(const LTRotation &cRotation) override;
    void WriteObject(HOBJECT hObj) override;
    void WriteYRotation(const LTRotation &cRotation) override;
    void WriteDatabaseRecord(IDatabaseMgr *pDatabase, HRECORD hRecord) override;
    void WriteTimer(HENGINETIMER hTimer) override;
    void WriteCompLTPolarCoord(const LTPolarCoord &vVec) override;

private:
    void WriteBit(uint32 nPos, uint32 nValue);

    std::vector<uint8> m_Data;
    uint32 m_nBitPos;
};

extern CFearMessageWrite g_FearMessageWrite;

#endif
