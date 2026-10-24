#include "Stdafx.h"
#include "CommandMgr.h"
#include "MsgIDs.h"
#include "ClientServerShared.h"
#include "ServerUtilities.h"
#include "iltmodel.h"
#include "PlayerObj.h"
#include "ServerMissionMgr.h"
#include "../v5host/fear_server_object_snapshot.h"

#include <stdlib.h>
#include <string.h>

extern ILTServer *g_pLTServer;

static unsigned short FearServerObjectId(HOBJECT hObject)
{
    unsigned short nObjectId = FEAR_INVALID_OBJECT_ID;
    CAutoMessage cId;
    if (!cId.IsValid())
        return nObjectId;

    cId.WriteObject(hObject);
    CLTMsgRef_Read cIdRead = cId.Read();
    if (cIdRead && cIdRead->Size() >= 15)
    {
        cIdRead->SeekTo(0);
        nObjectId = (unsigned short)cIdRead->ReadBits(15);
    }
    return nObjectId;
}

static void FearServerCaptureCamera(HOBJECT hObject, TFearServerObjectVisitor pVisitor, void *pContext)
{
    if (!hObject || !pVisitor)
        return;

    FearServerObjectSnapshot snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.m_pServerObject = hObject;
    snapshot.m_Rotation[3] = 1.0f;

    snapshot.m_ObjectId = FearServerObjectId(hObject);

    g_pCommonLT->GetObjectFlags(hObject, OFT_Flags, snapshot.m_Flags);
    g_pCommonLT->GetObjectFlags(hObject, OFT_Flags2, snapshot.m_Flags2);
    g_pCommonLT->GetObjectFlags(hObject, OFT_User, snapshot.m_UserFlags);
    g_pLTServer->GetObjectPos(hObject, (LTVector *)snapshot.m_Position);
    g_pLTServer->GetObjectRotation(hObject, (LTRotation *)snapshot.m_Rotation);
    g_pLTServer->GetObjectName(hObject, snapshot.m_Name, sizeof(snapshot.m_Name));

    HCLASS hClass = g_pLTServer->GetObjectClass(hObject);
    if (hClass)
        g_pLTServer->GetClassName(hClass, snapshot.m_ClassName, sizeof(snapshot.m_ClassName));

    ILTModel *pModelLT = g_pLTServer->GetModelLT();
    if (pModelLT)
    {
        LTRESULT nModelResult =
            pModelLT->GetModelFilename(hObject, snapshot.m_ModelFilename, sizeof(snapshot.m_ModelFilename));
        if (nModelResult != LT_OK)
            snapshot.m_ModelFilename[0] = '\0';
    }

    CAutoMessage cSFX;
    if (cSFX.IsValid() && (g_pLTServer->GetObjectSFXMessage(hObject, cSFX) == LT_OK))
    {
        CLTMsgRef_Read cRead = cSFX.Read();
        if (cRead && cRead->Size())
        {
            uint32 nBits = cRead->Size();
            uint32 nBytes = (nBits + 7) >> 3;
            unsigned char *pData = (unsigned char *)malloc(nBytes);
            if (pData)
            {
                cRead->SeekTo(0);
                cRead->ReadData(pData, nBits);
                snapshot.m_pSFXData = pData;
                snapshot.m_SFXBits = nBits;
            }
        }
    }

    pVisitor(&snapshot, pContext);
    free((void *)snapshot.m_pSFXData);
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_VisitCameras(
    TFearServerObjectVisitor pVisitor, void *pContext)
{
    if (!g_pLTServer || !pVisitor)
        return 0;

    int nVisited = 0;
    for (HOBJECT hObject = g_pLTServer->GetNextObject(NULL);
         hObject;
         hObject = g_pLTServer->GetNextObject(hObject))
    {
        if (!IsKindOf(hObject, "Camera"))
            continue;

        FearServerCaptureCamera(hObject, pVisitor, pContext);
        ++nVisited;
    }
    return nVisited;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_VisitObjects(
    TFearServerObjectVisitor pVisitor, void *pContext)
{
    if (!g_pLTServer || !pVisitor)
        return 0;

    int nVisited = 0;
    for (HOBJECT hObject = g_pLTServer->GetNextObject(NULL);
         hObject;
         hObject = g_pLTServer->GetNextObject(hObject))
    {
        if (IsKindOf(hObject, "Camera") || FearServerObjectId(hObject) == FEAR_INVALID_OBJECT_ID)
            continue;

        CAutoMessage cSFX;
        if (!cSFX.IsValid() || g_pLTServer->GetObjectSFXMessage(hObject, cSFX) != LT_OK)
            continue;

        CLTMsgRef_Read cRead = cSFX.Read();
        if (!cRead || !cRead->Size())
            continue;

        FearServerCaptureCamera(hObject, pVisitor, pContext);
        ++nVisited;
    }
    return nVisited;
}

static void __cdecl FearServerDumpCameraVisitor(const FearServerObjectSnapshot *pSnapshot, void *pContext)
{
    int *pCount = (int *)pContext;
    if (!pSnapshot || !pCount)
        return;

    unsigned int nSFXId = 0xFFFFFFFFu;
    if (pSnapshot->m_pSFXData && pSnapshot->m_SFXBits >= 8)
        nSFXId = pSnapshot->m_pSFXData[0];

    printf("FEARCAM: #%d obj=%p id=%u name='%s' class='%s' pos=(%.2f %.2f %.2f) flags=%08x flags2=%08x user=%08x sfx=%u/%u first=%u\n",
        *pCount, pSnapshot->m_pServerObject, (unsigned)pSnapshot->m_ObjectId, pSnapshot->m_Name, pSnapshot->m_ClassName,
        pSnapshot->m_Position[0], pSnapshot->m_Position[1], pSnapshot->m_Position[2],
        pSnapshot->m_Flags, pSnapshot->m_Flags2, pSnapshot->m_UserFlags,
        pSnapshot->m_SFXBits, (pSnapshot->m_SFXBits + 7) >> 3, nSFXId);
    fflush(stdout);
    ++*pCount;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_DumpCameras(void)
{
    int nCount = 0;
    int nVisited = FearGameServer_VisitCameras(FearServerDumpCameraVisitor, &nCount);
    printf("FEARCAM: cameras=%d visited=%d\n", nVisited, nCount);
    fflush(stdout);
    return nVisited;
}

extern "C" __declspec(dllexport) void __cdecl FearGameServer_QueueCommand(const char *pszCommand)
{
    if (g_pCmdMgr && pszCommand && pszCommand[0])
        g_pCmdMgr->QueueCommand(pszCommand, (ILTBaseClass *)NULL, (ILTBaseClass *)NULL);
}

extern "C" __declspec(dllexport) void *__cdecl FearGameServer_ResolveObject(unsigned short nObjectId)
{
    if (!g_pLTServer || nObjectId == FEAR_INVALID_OBJECT_ID)
        return NULL;

    for (HOBJECT hObject = g_pLTServer->GetNextObject(NULL);
         hObject;
         hObject = g_pLTServer->GetNextObject(hObject))
    {
        if (FearServerObjectId(hObject) == nObjectId)
            return hObject;
    }
    return NULL;
}

extern "C" __declspec(dllexport) unsigned short __cdecl FearGameServer_GetObjectWireId(void *pObject)
{
    return pObject ? FearServerObjectId((HOBJECT)pObject) : FEAR_INVALID_OBJECT_ID;
}

extern "C" __declspec(dllexport) unsigned short __cdecl FearGameServer_GetLocalCharacterWireId(void)
{
    HOBJECT hPlayer = NULL;
    LTRESULT nResult = g_pLTServer ? FindNamedObject("player", hPlayer) : LT_NOTFOUND;
    unsigned short nObjectId = nResult == LT_OK ? FearServerObjectId(hPlayer) : FEAR_INVALID_OBJECT_ID;
    if (nObjectId == FEAR_INVALID_OBJECT_ID && g_pLTServer)
    {
        for (HOBJECT hObject = g_pLTServer->GetNextObject(NULL);
             hObject;
             hObject = g_pLTServer->GetNextObject(hObject))
        {
            if (!IsPlayer(hObject) || !IsCharacter(hObject))
                continue;

            unsigned short nCandidateId = FearServerObjectId(hObject);
            if (nCandidateId == FEAR_INVALID_OBJECT_ID)
                continue;

            static HOBJECT s_hLoggedFallback = NULL;
            if (s_hLoggedFallback != hObject)
            {
                s_hLoggedFallback = hObject;
                char szName[128] = {0};
                g_pLTServer->GetObjectName(hObject, szName, sizeof(szName));
                printf("FEARSERVER: local character fallback object=%p id=%u name='%s'\n",
                    (void *)hObject, (unsigned)nCandidateId, szName);
                fflush(stdout);
            }

            hPlayer = hObject;
            nObjectId = nCandidateId;
            break;
        }
    }

    static LTRESULT s_nLastResult = LT_ERROR;
    static HOBJECT s_hLastObject = (HOBJECT)0x1;
    static unsigned short s_nLastObjectId = 0xFFFE;
    if (s_nLastResult != nResult || s_hLastObject != hPlayer || s_nLastObjectId != nObjectId)
    {
        s_nLastResult = nResult;
        s_hLastObject = hPlayer;
        s_nLastObjectId = nObjectId;
        printf("FEARSERVER: local character lookup result=%d object=%p id=%u\n",
            (int)nResult, (void *)hPlayer, (unsigned)nObjectId);
        fflush(stdout);
    }

    if (hPlayer && g_pServerMissionMgr &&
        g_pServerMissionMgr->GetServerGameState() == EServerGameState_Loading)
    {
        CPlayerObj *pPlayer = dynamic_cast<CPlayerObj *>(g_pLTServer->HandleToObject(hPlayer));
        if (pPlayer)
        {
            printf("FEARSERVER: local player entered world state=Loading\n");
            fflush(stdout);
            g_pServerMissionMgr->OnPlayerInWorld(*pPlayer);
        }
    }

    return nObjectId;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_SetPlayerTransform(
    const float *pPosition, const float *pVelocity, const float *pRotation)
{
    if (!g_pLTServer || !pPosition)
        return 0;

    HOBJECT hPlayer = NULL;
    if (FindNamedObject("player", hPlayer) != LT_OK || !hPlayer)
    {
        for (HOBJECT hObject = g_pLTServer->GetNextObject(NULL);
             hObject;
             hObject = g_pLTServer->GetNextObject(hObject))
        {
            if (IsPlayer(hObject) && IsCharacter(hObject))
            {
                hPlayer = hObject;
                break;
            }
        }
    }

    if (!hPlayer)
        return 0;

    LTVector vPosition(pPosition[0], pPosition[1], pPosition[2]);
    g_pLTServer->SetObjectPos(hPlayer, vPosition);

    if (pVelocity)
    {
        LTVector vVelocity(pVelocity[0], pVelocity[1], pVelocity[2]);
        ILTPhysics *pPhysics = g_pLTServer->Physics();
        if (pPhysics)
            pPhysics->SetVelocity(hPlayer, vVelocity);
    }

    if (pRotation)
    {
        LTRotation rRotation;
        for (int i = 0; i < 4; ++i)
            rRotation.m_Quat[i] = pRotation[i];
        g_pLTServer->SetObjectRotation(hPlayer, rRotation);
    }

    return 1;
}

static ILTModel *FearServerModelInterface(void)
{
    return g_pLTServer ? g_pLTServer->GetModelLT() : NULL;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_GetAnimLength(
    void *pObject, unsigned int nAnimation, unsigned int *pLength)
{
    ILTModel *pModelLT = FearServerModelInterface();
    if (!pObject || !pLength || !pModelLT)
        return 0;

    uint32 nLength = 0;
    if (pModelLT->GetAnimLength((HOBJECT)pObject, (HMODELANIM)nAnimation, nLength) != LT_OK)
        return 0;

    *pLength = nLength;
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_GetAnimKeyFrames(
    void *pObject, unsigned int nAnimation, unsigned int *pKeyFrames)
{
    ILTModel *pModelLT = FearServerModelInterface();
    if (!pObject || !pKeyFrames || !pModelLT)
        return 0;

    uint32 nKeyFrames = 0;
    if (pModelLT->GetAnimKeyFrames((HOBJECT)pObject, (HMODELANIM)nAnimation, nKeyFrames) != LT_OK)
        return 0;

    *pKeyFrames = nKeyFrames;
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_GetAnimNodeTransform(
    void *pObject, unsigned int nAnimation, unsigned int nNode, unsigned int nKeyFrame,
    float *pPosition, float *pRotation)
{
    ILTModel *pModelLT = FearServerModelInterface();
    if (!pObject || !pPosition || !pRotation || !pModelLT)
        return 0;

    LTRigidTransform transform;
    if (pModelLT->GetAnimNodeTransform(
        (HOBJECT)pObject, (HMODELANIM)nAnimation, (HMODELNODE)nNode,
        nKeyFrame, transform) != LT_OK)
        return 0;

    pPosition[0] = transform.m_vPos.x;
    pPosition[1] = transform.m_vPos.y;
    pPosition[2] = transform.m_vPos.z;
    for (int i = 0; i < 4; ++i)
        pRotation[i] = transform.m_rRot.m_Quat[i];
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_GetNodeTransform(
    void *pObject, unsigned int nNode, float *pPosition, float *pRotation, int bWorldSpace)
{
    ILTModel *pModelLT = FearServerModelInterface();
    if (!pObject || !pPosition || !pRotation || !pModelLT)
        return 0;

    LTTransform transform;
    if (pModelLT->GetNodeTransform(
        (HOBJECT)pObject, (HMODELNODE)nNode, transform, bWorldSpace != 0) != LT_OK)
        return 0;

    pPosition[0] = transform.m_vPos.x;
    pPosition[1] = transform.m_vPos.y;
    pPosition[2] = transform.m_vPos.z;
    for (int i = 0; i < 4; ++i)
        pRotation[i] = transform.m_rRot.m_Quat[i];
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_GetBindPoseNodeTransform(
    void *pObject, unsigned int nNode, float *pPosition, float *pRotation)
{
    ILTModel *pModelLT = FearServerModelInterface();
    if (!pObject || !pPosition || !pRotation || !pModelLT)
        return 0;

    LTRigidTransform transform;
    if (pModelLT->GetBindPoseNodeTransform(
        (HOBJECT)pObject, (HMODELNODE)nNode, transform) != LT_OK)
        return 0;

    pPosition[0] = transform.m_vPos.x;
    pPosition[1] = transform.m_vPos.y;
    pPosition[2] = transform.m_vPos.z;
    for (int i = 0; i < 4; ++i)
        pRotation[i] = transform.m_rRot.m_Quat[i];
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_GetAnimIndex(
    void *pObject, const char *pszName, unsigned int *pAnimation)
{
    ILTModel *pModelLT = FearServerModelInterface();
    if (!pObject || !pszName || !pAnimation || !pModelLT)
        return 0;

    uint32 nAnimation = 0;
    if (pModelLT->GetAnimIndex((HOBJECT)pObject, pszName, nAnimation) != LT_OK)
        return 0;

    *pAnimation = nAnimation;
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_GetAnimName(
    void *pObject, unsigned int nAnimation, char *pszName, unsigned int nNameLength)
{
    ILTModel *pModelLT = FearServerModelInterface();
    if (!pObject || !pszName || !nNameLength || !pModelLT)
        return 0;

    return pModelLT->GetAnimName(
        (HOBJECT)pObject, nAnimation, pszName, nNameLength) == LT_OK;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_GetModelAnimUserDims(
    void *pObject, unsigned int nAnimation, float *pDims)
{
    ILTModel *pModelLT = FearServerModelInterface();
    if (!pObject || !pDims || !pModelLT)
        return 0;

    LTVector dims;
    if (pModelLT->GetModelAnimUserDims(
        (HOBJECT)pObject, (HMODELANIM)nAnimation, &dims) != LT_OK)
        return 0;

    pDims[0] = dims.x;
    pDims[1] = dims.y;
    pDims[2] = dims.z;
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_GetSocket(
    void *pObject, const char *pszName, unsigned int *pSocket)
{
    ILTModel *pModelLT = FearServerModelInterface();
    if (!pObject || !pszName || !pSocket || !pModelLT)
        return 0;

    HMODELSOCKET hSocket = INVALID_MODEL_SOCKET;
    if (pModelLT->GetSocket((HOBJECT)pObject, pszName, hSocket) != LT_OK)
        return 0;

    *pSocket = (unsigned int)hSocket;
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_GetSocketTransform(
    void *pObject, unsigned int nSocket, float *pPosition, float *pRotation, int bWorldSpace)
{
    ILTModel *pModelLT = FearServerModelInterface();
    if (!pObject || !pPosition || !pRotation || !pModelLT)
        return 0;

    LTTransform transform;
    if (pModelLT->GetSocketTransform(
        (HOBJECT)pObject, (HMODELSOCKET)nSocket, transform, bWorldSpace != 0) != LT_OK)
        return 0;

    pPosition[0] = transform.m_vPos.x;
    pPosition[1] = transform.m_vPos.y;
    pPosition[2] = transform.m_vPos.z;
    for (int i = 0; i < 4; ++i)
        pRotation[i] = transform.m_rRot.m_Quat[i];
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_GetNode(
    void *pObject, const char *pszName, unsigned int *pNode)
{
    ILTModel *pModelLT = FearServerModelInterface();
    if (!pObject || !pszName || !pNode || !pModelLT)
        return 0;

    HMODELNODE hNode = INVALID_MODEL_NODE;
    if (pModelLT->GetNode((HOBJECT)pObject, pszName, hNode) != LT_OK)
        return 0;

    *pNode = (unsigned int)hNode;
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_GetNumNodes(
    void *pObject, unsigned int *pNumNodes)
{
    ILTModel *pModelLT = FearServerModelInterface();
    if (!pObject || !pNumNodes || !pModelLT)
        return 0;

    uint32 nNumNodes = 0;
    if (pModelLT->GetNumNodes((HOBJECT)pObject, nNumNodes) != LT_OK)
        return 0;

    *pNumNodes = nNumNodes;
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_GetMaterialFilename(
    void *pObject, unsigned int nIndex, char *pszName, unsigned int nNameLength)
{
    ILTModel *pModelLT = FearServerModelInterface();
    if (!pObject || !pszName || !nNameLength || !pModelLT)
        return 0;

    return pModelLT->GetMaterialFilename(
        (HOBJECT)pObject, nIndex, pszName, nNameLength) == LT_OK;
}

static bool FearServerReadable(const void *pAddress, unsigned int nBytes)
{
    if (!pAddress || !nBytes)
        return false;

    unsigned int nAddress = (unsigned int)(uintptr_t)pAddress;
    if (nAddress + nBytes < nAddress)
        return false;

    MEMORY_BASIC_INFORMATION region;
    if (!VirtualQuery(pAddress, &region, sizeof(region)))
        return false;
    if (region.State != MEM_COMMIT ||
        (region.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
        return false;

    unsigned int nRegionEnd = (unsigned int)(uintptr_t)region.BaseAddress +
        (unsigned int)region.RegionSize;
    return nAddress + nBytes <= nRegionEnd;
}

extern "C" __declspec(dllexport) int __cdecl FearGameServer_DumpModelPieces(
    void *pObject, unsigned int nMaxPieces)
{
    if (!FearServerReadable(pObject, 0x114))
        return 0;

    unsigned int nModel = *(unsigned int *)((unsigned char *)pObject + 0x110);
    if (!FearServerReadable((void *)nModel, 0x3C))
        return 0;

    unsigned int nPieces = *(unsigned int *)(nModel + 0x38);
    unsigned int nPieceArray = *(unsigned int *)(nModel + 0x34);
    if (nPieces > nMaxPieces)
        nPieces = nMaxPieces;

    printf("FEARMODELPIECES: object=%p model=%p count=%u array=%p\n",
        pObject, (void *)nModel, nPieces, (void *)nPieceArray);
    fflush(stdout);

    for (unsigned int i = 0; i < nPieces; ++i)
    {
        unsigned int nRecord = nPieceArray + i * 12;
        if (!FearServerReadable((void *)nRecord, 12))
            break;

        const char *pszName = *(const char **)nRecord;
        unsigned int nValue = *(unsigned int *)(nRecord + 4);
        unsigned int nPointer = *(unsigned int *)(nRecord + 8);
        char szName[96] = {0};
        for (unsigned int c = 0; c < sizeof(szName) - 1; ++c)
        {
            if (!FearServerReadable(pszName + c, 1))
                break;
            szName[c] = pszName[c];
            if (!szName[c])
                break;
        }
        printf("FEARMODELPIECES: [%u] name='%s' value=%08x pointer=%p\n",
            i, szName, nValue, (void *)nPointer);

        unsigned int nLodArray = nValue;
        unsigned int nLods = nPointer;
        printf("FEARMODELPIECES:     lods=%u array=%p\n",
            nLods, (void *)nLodArray);
        if (nLods > 8)
            nLods = 8;

        for (unsigned int lod = 0; lod < nLods; ++lod)
        {
            unsigned int nRenderObjectAddress = nLodArray + lod * 4;
            if (!FearServerReadable((void *)nRenderObjectAddress, 4))
                break;
            unsigned int nRenderObject = *(unsigned int *)nRenderObjectAddress;
            if (!FearServerReadable((void *)nRenderObject, 64))
            {
                printf("FEARMODELPIECES:     lod%u object=%p unreadable\n",
                    lod, (void *)nRenderObject);
                continue;
            }

            const unsigned int *pWords = (const unsigned int *)nRenderObject;
            printf("FEARMODELPIECES:     lod%u object=%p "
                "w00-07=%08x %08x %08x %08x %08x %08x %08x %08x\n",
                lod, (void *)nRenderObject,
                pWords[0], pWords[1], pWords[2], pWords[3],
                pWords[4], pWords[5], pWords[6], pWords[7]);
            printf("FEARMODELPIECES:     lod%u "
                "w08-15=%08x %08x %08x %08x %08x %08x %08x %08x\n",
                lod,
                pWords[8], pWords[9], pWords[10], pWords[11],
                pWords[12], pWords[13], pWords[14], pWords[15]);
        }
        fflush(stdout);
    }
    return (int)nPieces;
}
