#include "fear_common_stub.h"
#include "fear_messagewrite_stub.h"
#include <stdio.h>

template <typename T> struct SvcDefault { static T Get() { return T(); } };
template <> struct SvcDefault<LTRESULT> { static LTRESULT Get() { return LT_UNSUPPORTED; } };
template <> struct SvcDefault<const char*> { static const char* Get() { return ""; } };

CFearCommon g_FearCommon;

LTRESULT CFearCommon::SetObjectResource(HOBJECT hObj, EObjectResource eType, uint32 nIndex, const char* pszResource)
{
    printf("STUB ILTCommon::SetObjectResource\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::SetObjectFilenames(HOBJECT hObj, ObjectCreateStruct* pStruct)
{
    printf("STUB ILTCommon::SetObjectFilenames\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::GetObjectFlags(const HOBJECT hObj, const ObjFlagType flagType, uint32 &dwFlags)
{
    printf("STUB ILTCommon::GetObjectFlags\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::SetObjectFlags(HOBJECT hObj, const ObjFlagType flagType, uint32 dwFlags, uint32 dwMask)
{
    printf("STUB ILTCommon::SetObjectFlags\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::GetAttachmentObjects(HATTACHMENT hAttachment, HOBJECT &hParent, HOBJECT &hChild)
{
    printf("STUB ILTCommon::GetAttachmentObjects\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::GetAttachments(HLOCALOBJ hObj, HLOCALOBJ *inList, uint32 inListSize, uint32 &outListSize, uint32 &outNumAttachments)
{
    printf("STUB ILTCommon::GetAttachments\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::GetAttachedModelNodeTransform(HATTACHMENT hAttachment, HMODELNODE hNode, LTTransform &transform)
{
    printf("STUB ILTCommon::GetAttachedModelNodeTransform\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::GetAttachmentTransform(HATTACHMENT hAttachment, LTTransform &transform, bool bWorldSpace)
{
    printf("STUB ILTCommon::GetAttachmentTransform\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::GetAttachedModelSocketTransform(HATTACHMENT hAttachment, HMODELSOCKET hSocket, LTTransform &transform)
{
    printf("STUB ILTCommon::GetAttachedModelSocketTransform\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::NumAttachments(HLOCALOBJ hObject, uint32 &dwAttachCount)
{
    printf("STUB ILTCommon::NumAttachments\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::GetObjectType(HOBJECT hObj, uint32 *type)
{
    printf("STUB ILTCommon::GetObjectType\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::Parse(ConParse *pParse)
{
    printf("STUB ILTCommon::Parse\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::Parse(ConParseW *pParse)
{
    printf("STUB ILTCommon::Parse\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::CreateMessage(ILTMessage_Write* &pMsg)
{
    g_FearMessageWrite.Reset();
    pMsg = &g_FearMessageWrite;
    return LT_OK;
}

LTRESULT CFearCommon::GetPointStatus(const LTVector *pPoint)
{
    printf("STUB ILTCommon::GetPointStatus\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::GetPolyTextureFlags(HPOLY hPoly, uint32 *pFlags)
{
    printf("STUB ILTCommon::GetPolyTextureFlags\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::GetPolyPlane(HPOLY hPoly, LTPlane *pPlane)
{
    printf("STUB ILTCommon::GetPolyPlane\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::GetPolyOcclusionIndex(HPOLY hPoly, uint8 *pIndex)
{
    printf("STUB ILTCommon::GetPolyOcclusionIndex\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::CompressVector(const LTVector& vec, CompVector& CompV)
{
    printf("STUB ILTCommon::CompressVector\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::UncompressVector(const CompVector& CompV, LTVector& vec)
{
    printf("STUB ILTCommon::UncompressVector\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::CompressRotation(const LTRotation& rot, CompRot& CompR)
{
    printf("STUB ILTCommon::CompressRotation\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::UncompressRotation(const CompRot& CompR, LTRotation& rot)
{
    printf("STUB ILTCommon::UncompressRotation\n");
    return SvcDefault<LTRESULT>::Get();
}
