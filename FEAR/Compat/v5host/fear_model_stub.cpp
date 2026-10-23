#include "fear_model_stub.h"
#include <stdio.h>
#include <map>
#include <string>
#include <vector>

template <typename T> struct SvcDefault { static T Get() { return T(); } };
template <> struct SvcDefault<LTRESULT> { static LTRESULT Get() { return LT_UNSUPPORTED; } };
template <> struct SvcDefault<const char*> { static const char* Get() { return ""; } };

CFearModel g_FearModel;

namespace
{

struct FearModelWeights
{
    bool m_bInit;
    bool m_Trackers[256];
    HMODELWEIGHTSET m_TrackerWeight[256];
};

std::map<HOBJECT, FearModelWeights> g_ModelWeights;
std::vector<std::string> g_WeightSetNames;

FearModelWeights &FearModelWeightsFor(HOBJECT hObj)
{
    FearModelWeights &state = g_ModelWeights[hObj];
    if (!state.m_bInit)
    {
        state.m_bInit = true;
        for (int i = 0; i < 256; ++i)
        {
            state.m_Trackers[i] = false;
            state.m_TrackerWeight[i] = INVALID_MODEL_WEIGHTSET;
        }
    }
    return state;
}

HMODELWEIGHTSET FearModelWeightSetHandle(const char *pszName)
{
    std::string name = pszName ? pszName : "";
    for (size_t i = 0; i < g_WeightSetNames.size(); ++i)
    {
        if (g_WeightSetNames[i] == name)
            return (HMODELWEIGHTSET)(i + 1);
    }
    g_WeightSetNames.push_back(name);
    return (HMODELWEIGHTSET)g_WeightSetNames.size();
}

}

LTRESULT CFearModel::GetSocket(HOBJECT hObj, const char *pSocketName, HMODELSOCKET &hSocket)
{
    printf("STUB ILTModel::GetSocket\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetSocketTransform(HOBJECT hObj, HMODELSOCKET hSocket, LTTransform &transform, bool bWorldSpace)
{
    printf("STUB ILTModel::GetSocketTransform\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetNumPieces(HOBJECT hObj, uint32 & NumPieces)
{
    printf("STUB ILTModel::GetNumPieces\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetPiece(HOBJECT hObj, const char *pPieceName, HMODELPIECE &hPiece)
{
    printf("STUB ILTModel::GetPiece\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetPiece(HOBJECT hObj, uint32 nIndex, HMODELPIECE &hPiece)
{
    printf("STUB ILTModel::GetPiece\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetPieceName(HOBJECT hObj, HMODELPIECE &hPiece, char *pPieceName, uint32 nPieceNameLen)
{
    printf("STUB ILTModel::GetPieceName\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetPieceHideStatus(HOBJECT hObj, HMODELPIECE hPiece, bool &bHidden)
{
    printf("STUB ILTModel::GetPieceHideStatus\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::SetPieceHideStatus(HOBJECT hObj, HMODELPIECE hPiece, bool bHidden)
{
    printf("STUB ILTModel::SetPieceHideStatus\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetLODDistanceBias(HOBJECT hModel, float& fLODDistBias)
{
    printf("STUB ILTModel::GetLODDistanceBias\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::SetLODDistanceBias(HOBJECT hModel, float fLODDistBias)
{
    printf("STUB ILTModel::SetLODDistanceBias\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetNode(HOBJECT hObj, const char *pNodeName, HMODELNODE &hNode)
{
    printf("STUB ILTModel::GetNode\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetNodeName(HOBJECT hObj, HMODELNODE hNode, char *name, uint32 maxlen)
{
    printf("STUB ILTModel::GetNodeName\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetNodeTransform(HOBJECT hObj, HMODELNODE hNode, LTTransform &transform, bool bWorldSpace)
{
    printf("STUB ILTModel::GetNodeTransform\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetNextNode(HOBJECT hObject, HMODELNODE hNode, HMODELNODE &pNext)
{
    printf("STUB ILTModel::GetNextNode\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetRootNode(HOBJECT hObj, HMODELNODE &hNode)
{
    printf("STUB ILTModel::GetRootNode\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetNumChildren(HOBJECT hObj, HMODELNODE hNode, uint32 &NumChildren)
{
    printf("STUB ILTModel::GetNumChildren\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetChild(HOBJECT hObj, HMODELNODE parent, uint32 index, HMODELNODE &child)
{
    printf("STUB ILTModel::GetChild\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetParent(HOBJECT hObj, HMODELNODE node, HMODELNODE &parent)
{
    printf("STUB ILTModel::GetParent\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetNumNodes(HOBJECT hObj, uint32 &num_nodes)
{
    printf("STUB ILTModel::GetNumNodes\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetBindPoseNodeTransform(HOBJECT hObj, HMODELNODE node, LTRigidTransform &tBindSpace)
{
    printf("STUB ILTModel::GetBindPoseNodeTransform\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::AddNodeControlFn(HOBJECT hObj, HMODELNODE hNode, NodeControlFn pFn, void *pUserData)
{
    printf("STUB ILTModel::AddNodeControlFn\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::AddNodeControlFn(HOBJECT hObj, NodeControlFn pFn, void *pUserData)
{
    printf("STUB ILTModel::AddNodeControlFn\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::RemoveNodeControlFn(HOBJECT hObj, HMODELNODE hNode, NodeControlFn pFn, void* pUserData)
{
    printf("STUB ILTModel::RemoveNodeControlFn\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::RemoveNodeControlFn(HOBJECT hObj, NodeControlFn pFn, void* pUserData)
{
    printf("STUB ILTModel::RemoveNodeControlFn\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::UpdateMainTracker(HOBJECT hObj, float fUpdateDelta)
{
    printf("STUB ILTModel::UpdateMainTracker\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetModelAnimUserDims(HOBJECT hModel, HMODELANIM hAnim, LTVector *pDims)
{
    printf("STUB ILTModel::GetModelAnimUserDims\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetAnimLength(HOBJECT hModel, HMODELANIM hAnim, uint32 &length)
{
    printf("STUB ILTModel::GetAnimLength\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetAnimKeyFrames(HOBJECT hModel, HMODELANIM hAnim, uint32 &nNumKeyFrames)
{
    printf("STUB ILTModel::GetAnimKeyFrames\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetAnimNodeTransform(HOBJECT hModel, HMODELANIM hAnim, HMODELNODE hNode, uint32 nKeyFrame, LTRigidTransform& tTransform)
{
    printf("STUB ILTModel::GetAnimNodeTransform\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetPlaybackState(HOBJECT hModel, ANIMTRACKERID TrackerID, uint32 &flags)
{
    printf("STUB ILTModel::GetPlaybackState\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::AddTracker(HOBJECT hModel, ANIMTRACKERID TrackerID, bool bNetwork)
{
    if (!hModel)
        return LT_INVALIDPARAMS;

    FearModelWeights &state = FearModelWeightsFor(hModel);
    state.m_Trackers[TrackerID] = true;
    printf("FEARMODEL: AddTracker %u network=%d\n", (unsigned)TrackerID, bNetwork ? 1 : 0);
    return LT_OK;
}

LTRESULT CFearModel::RemoveTracker(HOBJECT hModel, ANIMTRACKERID TrackerID)
{
    if (!hModel)
        return LT_INVALIDPARAMS;

    FearModelWeights &state = FearModelWeightsFor(hModel);
    state.m_Trackers[TrackerID] = false;
    state.m_TrackerWeight[TrackerID] = INVALID_MODEL_WEIGHTSET;
    printf("FEARMODEL: RemoveTracker %u\n", (unsigned)TrackerID);
    return LT_OK;
}

LTRESULT CFearModel::SetAnimBlend(HOBJECT hModel, ANIMTRACKERID TrackerID, float fBlend)
{
    printf("STUB ILTModel::SetAnimBlend\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::CopyAnimation(HOBJECT hModel, ANIMTRACKERID SourceTrackerID, ANIMTRACKERID DestTrackerID)
{
    printf("STUB ILTModel::CopyAnimation\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetAnimIndex(HOBJECT hModel, const char *pAnimName, uint32 & anim_index)
{
    printf("STUB ILTModel::GetAnimIndex\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetAnimName(HOBJECT hModel, uint32 dwAnimIndex, char *pszBuffer, uint32 dwBufferLen)
{
    printf("STUB ILTModel::GetAnimName\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetCurAnim(HOBJECT hModel, ANIMTRACKERID TrackerID, HMODELANIM &hAnim)
{
    printf("STUB ILTModel::GetCurAnim\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::SetCurAnim(HOBJECT hModel, ANIMTRACKERID TrackerID, HMODELANIM hAnim, bool bInterpolate)
{
    printf("STUB ILTModel::SetCurAnim\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetIsInterpolating(HOBJECT hModel, ANIMTRACKERID TrackerID)
{
    printf("STUB ILTModel::GetIsInterpolating\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::ResetAnim(HOBJECT hModel, ANIMTRACKERID TrackerID)
{
    printf("STUB ILTModel::ResetAnim\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetLooping(HOBJECT hModel, ANIMTRACKERID TrackerID)
{
    printf("STUB ILTModel::GetLooping\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::SetLooping(HOBJECT hModel, ANIMTRACKERID TrackerID, bool bLooping)
{
    printf("STUB ILTModel::SetLooping\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetPlaying(HOBJECT hModel, ANIMTRACKERID TrackerID)
{
    printf("STUB ILTModel::GetPlaying\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::SetPlaying(HOBJECT hModel, ANIMTRACKERID TrackerID, bool bPlaying)
{
    printf("STUB ILTModel::SetPlaying\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetCurAnimLength(HOBJECT hModel, ANIMTRACKERID TrackerID, uint32 &length)
{
    printf("STUB ILTModel::GetCurAnimLength\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetCurAnimTime(HOBJECT hModel, ANIMTRACKERID TrackerID, uint32 &curTime)
{
    printf("STUB ILTModel::GetCurAnimTime\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::SetCurAnimTime(HOBJECT hModel, ANIMTRACKERID TrackerID, uint32 curTime)
{
    printf("STUB ILTModel::SetCurAnimTime\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::SetAnimRate(HOBJECT hModel, ANIMTRACKERID TrackerID, float fRate)
{
    printf("STUB ILTModel::SetAnimRate\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetAnimRate(HOBJECT hModel, ANIMTRACKERID TrackerID, float &fRate)
{
    printf("STUB ILTModel::GetAnimRate\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::FindWeightSet(HOBJECT hObj, const char *pSetName, HMODELWEIGHTSET &hSet)
{
    if (!hObj || !pSetName)
        return LT_INVALIDPARAMS;

    FearModelWeightsFor(hObj);
    hSet = FearModelWeightSetHandle(pSetName);
    printf("FEARMODEL: FindWeightSet '%s' -> %u\n", pSetName, (unsigned)hSet);
    return LT_OK;
}

LTRESULT CFearModel::GetWeightSet(HOBJECT hModel, ANIMTRACKERID TrackerID, HMODELWEIGHTSET &hSet)
{
    if (!hModel)
        return LT_INVALIDPARAMS;

    FearModelWeights &state = FearModelWeightsFor(hModel);
    hSet = state.m_TrackerWeight[TrackerID];
    if (hSet == INVALID_MODEL_WEIGHTSET)
        hSet = FearModelWeightSetHandle("null");
    return LT_OK;
}

LTRESULT CFearModel::SetWeightSet(HOBJECT hModel, ANIMTRACKERID TrackerID, HMODELWEIGHTSET hSet)
{
    if (!hModel)
        return LT_INVALIDPARAMS;

    FearModelWeights &state = FearModelWeightsFor(hModel);
    state.m_TrackerWeight[TrackerID] = hSet;
    state.m_Trackers[TrackerID] = true;
    return LT_OK;
}

LTRESULT CFearModel::FindPhysicsWeightSet(HOBJECT hModel, const char* pszName, HPHYSICSWEIGHTSET& hSet)
{
    printf("STUB ILTModel::FindPhysicsWeightSet\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::SetPhysicsWeightSet(HOBJECT hModel, HPHYSICSWEIGHTSET hWeightSet)
{
    printf("STUB ILTModel::SetPhysicsWeightSet\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetPhysicsWeightSet(HOBJECT hModel, HPHYSICSWEIGHTSET& hWeightSet)
{
    printf("STUB ILTModel::GetPhysicsWeightSet\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetPhysicsVisNode(HOBJECT hModel, HMODELNODE& hNode)
{
    printf("STUB ILTModel::GetPhysicsVisNode\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::DirtyAllNodeTransforms(HOBJECT hModel)
{
    printf("STUB ILTModel::DirtyAllNodeTransforms\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::DirtyNodeTransform(HOBJECT hModel, HMODELNODE hNode)
{
    printf("STUB ILTModel::DirtyNodeTransform\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::UpdateModelAttachments(HOBJECT hModel)
{
    printf("STUB ILTModel::UpdateModelAttachments\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetModelFilename(HOBJECT hObj, char *pRetFilename, uint32 nFileBufLen)
{
    printf("STUB ILTModel::GetModelFilename\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetMaterialFilename(HOBJECT hObj, uint32 nIndex, char *pRetFilename, uint32 nFileBufLen)
{
    printf("STUB ILTModel::GetMaterialFilename\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::SetMaterialFilename(HOBJECT hObj, uint32 nIndex, const char *pFilename)
{
    printf("STUB ILTModel::SetMaterialFilename\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetMovementEncodingTransform(HOBJECT hModel, LTRigidTransform& tTransform)
{
    printf("STUB ILTModel::GetMovementEncodingTransform\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::ResetMovementEncodingTransform(HOBJECT hModel)
{
    printf("STUB ILTModel::ResetMovementEncodingTransform\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::SetMovementEncodingTracker(HOBJECT hModel, ANIMTRACKERID TrackerID)
{
    printf("STUB ILTModel::SetMovementEncodingTracker\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::DisableMovementEncoding(HOBJECT hModel)
{
    printf("STUB ILTModel::DisableMovementEncoding\n");
    return SvcDefault<LTRESULT>::Get();
}
