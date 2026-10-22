#ifndef __FEAR_COMMON_STUB_H__
#define __FEAR_COMMON_STUB_H__

#include "ltbasedefs.h"
#include "iltcommon.h"

class CFearCommon : public ILTCommon
{
public:
    declare_interface(CFearCommon)
    CFearCommon() = default;
    ~CFearCommon() {}

    LTRESULT SetObjectResource(HOBJECT hObj, EObjectResource eType, uint32 nIndex, const char* pszResource) override;
    LTRESULT SetObjectFilenames(HOBJECT hObj, ObjectCreateStruct* pStruct) override;
    LTRESULT GetObjectFlags(const HOBJECT hObj, const ObjFlagType flagType, uint32 &dwFlags) override;
    LTRESULT SetObjectFlags(HOBJECT hObj, const ObjFlagType flagType, uint32 dwFlags, uint32 dwMask) override;
    LTRESULT GetAttachmentObjects(HATTACHMENT hAttachment, HOBJECT &hParent, HOBJECT &hChild) override;
    LTRESULT GetAttachments(HLOCALOBJ hObj, HLOCALOBJ *inList, uint32 inListSize, uint32 &outListSize, uint32 &outNumAttachments) override;
    LTRESULT GetAttachedModelNodeTransform(HATTACHMENT hAttachment, HMODELNODE hNode, LTTransform &transform) override;
    LTRESULT GetAttachmentTransform(HATTACHMENT hAttachment, LTTransform &transform, bool bWorldSpace) override;
    LTRESULT GetAttachedModelSocketTransform(HATTACHMENT hAttachment, HMODELSOCKET hSocket, LTTransform &transform) override;
    LTRESULT NumAttachments(HLOCALOBJ hObject, uint32 &dwAttachCount) override;
    LTRESULT GetObjectType(HOBJECT hObj, uint32 *type) override;
    LTRESULT Parse(ConParse *pParse) override;
    LTRESULT Parse(ConParseW *pParse) override;
    LTRESULT CreateMessage(ILTMessage_Write* &pMsg) override;
    LTRESULT GetPointStatus(const LTVector *pPoint) override;
    LTRESULT GetPolyTextureFlags(HPOLY hPoly, uint32 *pFlags) override;
    LTRESULT GetPolyPlane(HPOLY hPoly, LTPlane *pPlane) override;
    LTRESULT GetPolyOcclusionIndex(HPOLY hPoly, uint8 *pIndex) override;
    LTRESULT CompressVector(const LTVector& vec, CompVector& CompV) override;
    LTRESULT UncompressVector(const CompVector& CompV, LTVector& vec) override;
    LTRESULT CompressRotation(const LTRotation& rot, CompRot& CompR) override;
    LTRESULT UncompressRotation(const CompRot& CompR, LTRotation& rot) override;
};

extern CFearCommon g_FearCommon;

#endif
