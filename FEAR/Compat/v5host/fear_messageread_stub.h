#ifndef __FEAR_MESSAGEREAD_STUB_H__
#define __FEAR_MESSAGEREAD_STUB_H__

#include "ltbasedefs.h"
#include "iltmessage.h"
#include <vector>

class CFearMessageRead : public ILTMessage_Read
{
public:
    declare_interface(CFearMessageRead)
    CFearMessageRead() = default;
    ~CFearMessageRead() {}
    void AddRef() override;
    void Release() override;
    void StaticRelease() override;
    void Free() override;
    void SetData(const uint8 *pData, uint32 nBits);

    ILTMessage_Read * Clone() const override;
    ILTMessage_Read * SubMsg(uint32 nPos) const override;
    ILTMessage_Read * SubMsg(uint32 nPos, uint32 nLength) const override;
    uint32 Size() const override;
    void Seek(int32 nOffset) override;
    void SeekTo(uint32 nPos) override;
    uint32 Tell() const override;
    uint32 TellEnd() const override;
    bool EOM() const override;
    uint32 ReadBits(uint32 nBits) override;
    uint64 ReadBits64(uint32 nBits) override;
    void ReadData(void *pData, uint32 nBits) override;
    ILTMessage_Read * ReadMessage() override;
    uint32 ReadString(char *pDest, uint32 nMaxLen) override;
    uint32 ReadWString(wchar_t *pwDest, uint32 nMaxLen) override;
    LTVector ReadCompLTVector() override;
    LTVector ReadCompPos() override;
    LTRotation ReadCompLTRotation() override;
    HOBJECT ReadObject() override;
    LTRotation ReadYRotation() override;
    HRECORD ReadDatabaseRecord(IDatabaseMgr *pDatabase, HCATEGORY hCategory) override;
    LTPolarCoord ReadCompLTPolarCoord() override;
    HENGINETIMER ReadTimer() override;
    uint32 PeekBits(uint32 nBits) const override;
    uint64 PeekBits64(uint32 nBits) const override;
    void PeekData(void *pData, uint32 nBits) const override;
    ILTMessage_Read * PeekMessage() const override;
    uint32 PeekString(char *pDest, uint32 nMaxLen) const override;
    uint32 PeekWString(wchar_t *pwDest, uint32 nMaxLen) const override;
    LTVector PeekCompLTVector() const override;
    LTVector PeekCompPos() const override;
    LTRotation PeekCompLTRotation() const override;
    HOBJECT PeekObject() const override;
    LTRotation PeekYRotation() const override;
    HRECORD PeekDatabaseRecord(IDatabaseMgr *pDatabase, HCATEGORY hCategory) override;
private:
    CFearMessageRead *CreateFromBits(const uint8 *pData, uint32 nStart, uint32 nBits) const;
    const uint8 *m_pData = nullptr;
    uint32 m_nDataBits = 0;
    uint32 m_nReadPos = 0;
    std::vector<uint8> m_Storage;
    int m_nRefs = 0;
};



#endif
