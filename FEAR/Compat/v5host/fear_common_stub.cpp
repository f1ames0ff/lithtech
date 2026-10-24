#include "fear_common_stub.h"
#include "fear_object_map.h"
#include "fear_messagewrite_stub.h"
#include "fear_model_stub.h"
#include "ltobjectcreate.h"
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
    if (!hObj || !pStruct)
        return LT_INVALIDPARAMS;

    FearModel_SetObjectFilename(hObj, pStruct->m_Filename);
    FearClient_SetObjectType(hObj, OT_MODEL);
    printf("FEARPORT: SetObjectFilenames obj=%p model='%s'\n",
        (void *)hObj, pStruct->m_Filename);
    fflush(stdout);
    return LT_OK;
}

LTRESULT CFearCommon::GetObjectFlags(const HOBJECT hObj, const ObjFlagType flagType, uint32 &dwFlags)
{
    uint32 nFlags = 0;
    if (!FearClient_GetObjectFlags(hObj, (unsigned int)flagType, &nFlags))
        return LT_NOTFOUND;

    dwFlags = nFlags;
    return LT_OK;
}

LTRESULT CFearCommon::SetObjectFlags(HOBJECT hObj, const ObjFlagType flagType, uint32 dwFlags, uint32 dwMask)
{
    if (!FearClient_SetObjectFlags(hObj, (unsigned int)flagType, dwFlags, dwMask))
        return LT_NOTFOUND;

    return LT_OK;
}

LTRESULT CFearCommon::GetAttachmentObjects(HATTACHMENT hAttachment, HOBJECT &hParent, HOBJECT &hChild)
{
    printf("STUB ILTCommon::GetAttachmentObjects\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearCommon::GetAttachments(HLOCALOBJ hObj, HLOCALOBJ *inList, uint32 inListSize, uint32 &outListSize, uint32 &outNumAttachments)
{
    if (!hObj)
        return LT_INVALIDPARAMS;
    outListSize = 0;
    outNumAttachments = 0;
    return LT_OK;
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
    if (!hObject)
        return LT_INVALIDPARAMS;
    dwAttachCount = 0;
    return LT_OK;
}

LTRESULT CFearCommon::GetObjectType(HOBJECT hObj, uint32 *type)
{
    if (!hObj || !type)
        return LT_INVALIDPARAMS;

    *type = FearClient_GetObjectType(hObj);
    return LT_OK;
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
    pMsg = new CFearMessageWrite;
    static int s_nCreateLogs = 0;
    if (++s_nCreateLogs <= 4)
    {
        void **pVtable = *reinterpret_cast<void ***>(pMsg);
        printf("FEARMSGW: create msg=%p vtable=%p slot9=%p slot10=%p\n",
            (void *)pMsg, (void *)pVtable, pVtable[9], pVtable[10]);
        fflush(stdout);
    }
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
