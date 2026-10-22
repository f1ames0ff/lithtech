#ifndef __FEAR_MODEL_STUB_H__
#define __FEAR_MODEL_STUB_H__

#include "ltbasedefs.h"
#include "iltmodel.h"

class CFearModel : public ILTModel
{
public:
    declare_interface(CFearModel)
    CFearModel() = default;
    ~CFearModel() {}

    LTRESULT GetSocket(HOBJECT hObj, const char *pSocketName, HMODELSOCKET &hSocket) override;
    LTRESULT GetSocketTransform(HOBJECT hObj, HMODELSOCKET hSocket, LTTransform &transform, bool bWorldSpace) override;
    LTRESULT GetNumPieces(HOBJECT hObj, uint32 & NumPieces) override;
    LTRESULT GetPiece(HOBJECT hObj, const char *pPieceName, HMODELPIECE &hPiece) override;
    LTRESULT GetPiece(HOBJECT hObj, uint32 nIndex, HMODELPIECE &hPiece) override;
    LTRESULT GetPieceName(HOBJECT hObj, HMODELPIECE &hPiece, char *pPieceName, uint32 nPieceNameLen) override;
    LTRESULT GetPieceHideStatus(HOBJECT hObj, HMODELPIECE hPiece, bool &bHidden) override;
    LTRESULT SetPieceHideStatus(HOBJECT hObj, HMODELPIECE hPiece, bool bHidden) override;
    LTRESULT GetLODDistanceBias(HOBJECT hModel, float& fLODDistBias) override;
    LTRESULT SetLODDistanceBias(HOBJECT hModel, float fLODDistBias) override;
    LTRESULT GetNode(HOBJECT hObj, const char *pNodeName, HMODELNODE &hNode) override;
    LTRESULT GetNodeName(HOBJECT hObj, HMODELNODE hNode, char *name, uint32 maxlen) override;
    LTRESULT GetNodeTransform(HOBJECT hObj, HMODELNODE hNode, LTTransform &transform, bool bWorldSpace) override;
    LTRESULT GetNextNode(HOBJECT hObject, HMODELNODE hNode, HMODELNODE &pNext) override;
    LTRESULT GetRootNode(HOBJECT hObj, HMODELNODE &hNode) override;
    LTRESULT GetNumChildren(HOBJECT hObj, HMODELNODE hNode, uint32 &NumChildren) override;
    LTRESULT GetChild(HOBJECT hObj, HMODELNODE parent, uint32 index, HMODELNODE &child) override;
    LTRESULT GetParent(HOBJECT hObj, HMODELNODE node, HMODELNODE &parent) override;
    LTRESULT GetNumNodes(HOBJECT hObj, uint32 &num_nodes) override;
    LTRESULT GetBindPoseNodeTransform(HOBJECT hObj, HMODELNODE node, LTRigidTransform &tBindSpace) override;
    LTRESULT AddNodeControlFn(HOBJECT hObj, HMODELNODE hNode, NodeControlFn pFn, void *pUserData) override;
    LTRESULT AddNodeControlFn(HOBJECT hObj, NodeControlFn pFn, void *pUserData) override;
    LTRESULT RemoveNodeControlFn(HOBJECT hObj, HMODELNODE hNode, NodeControlFn pFn, void* pUserData) override;
    LTRESULT RemoveNodeControlFn(HOBJECT hObj, NodeControlFn pFn, void* pUserData) override;
    LTRESULT UpdateMainTracker(HOBJECT hObj, float fUpdateDelta) override;
    LTRESULT GetModelAnimUserDims(HOBJECT hModel, HMODELANIM hAnim, LTVector *pDims) override;
    LTRESULT GetAnimLength(HOBJECT hModel, HMODELANIM hAnim, uint32 &length) override;
    LTRESULT GetAnimKeyFrames(HOBJECT hModel, HMODELANIM hAnim, uint32 &nNumKeyFrames) override;
    LTRESULT GetAnimNodeTransform(HOBJECT hModel, HMODELANIM hAnim, HMODELNODE hNode, uint32 nKeyFrame, LTRigidTransform& tTransform) override;
    LTRESULT GetPlaybackState(HOBJECT hModel, ANIMTRACKERID TrackerID, uint32 &flags) override;
    LTRESULT AddTracker(HOBJECT hModel, ANIMTRACKERID TrackerID, bool bNetwork) override;
    LTRESULT RemoveTracker(HOBJECT hModel, ANIMTRACKERID TrackerID) override;
    LTRESULT SetAnimBlend(HOBJECT hModel, ANIMTRACKERID TrackerID, float fBlend) override;
    LTRESULT CopyAnimation(HOBJECT hModel, ANIMTRACKERID SourceTrackerID, ANIMTRACKERID DestTrackerID) override;
    LTRESULT GetAnimIndex(HOBJECT hModel, const char *pAnimName, uint32 & anim_index) override;
    LTRESULT GetAnimName(HOBJECT hModel, uint32 dwAnimIndex, char *pszBuffer, uint32 dwBufferLen) override;
    LTRESULT GetCurAnim(HOBJECT hModel, ANIMTRACKERID TrackerID, HMODELANIM &hAnim) override;
    LTRESULT SetCurAnim(HOBJECT hModel, ANIMTRACKERID TrackerID, HMODELANIM hAnim, bool bInterpolate) override;
    LTRESULT GetIsInterpolating(HOBJECT hModel, ANIMTRACKERID TrackerID) override;
    LTRESULT ResetAnim(HOBJECT hModel, ANIMTRACKERID TrackerID) override;
    LTRESULT GetLooping(HOBJECT hModel, ANIMTRACKERID TrackerID) override;
    LTRESULT SetLooping(HOBJECT hModel, ANIMTRACKERID TrackerID, bool bLooping) override;
    LTRESULT GetPlaying(HOBJECT hModel, ANIMTRACKERID TrackerID) override;
    LTRESULT SetPlaying(HOBJECT hModel, ANIMTRACKERID TrackerID, bool bPlaying) override;
    LTRESULT GetCurAnimLength(HOBJECT hModel, ANIMTRACKERID TrackerID, uint32 &length) override;
    LTRESULT GetCurAnimTime(HOBJECT hModel, ANIMTRACKERID TrackerID, uint32 &curTime) override;
    LTRESULT SetCurAnimTime(HOBJECT hModel, ANIMTRACKERID TrackerID, uint32 curTime) override;
    LTRESULT SetAnimRate(HOBJECT hModel, ANIMTRACKERID TrackerID, float fRate) override;
    LTRESULT GetAnimRate(HOBJECT hModel, ANIMTRACKERID TrackerID, float &fRate) override;
    LTRESULT FindWeightSet(HOBJECT hObj, const char *pSetName, HMODELWEIGHTSET &hSet) override;
    LTRESULT GetWeightSet(HOBJECT hModel, ANIMTRACKERID TrackerID, HMODELWEIGHTSET &hSet) override;
    LTRESULT SetWeightSet(HOBJECT hModel, ANIMTRACKERID TrackerID, HMODELWEIGHTSET hSet) override;
    LTRESULT FindPhysicsWeightSet(HOBJECT hModel, const char* pszName, HPHYSICSWEIGHTSET& hSet) override;
    LTRESULT SetPhysicsWeightSet(HOBJECT hModel, HPHYSICSWEIGHTSET hWeightSet) override;
    LTRESULT GetPhysicsWeightSet(HOBJECT hModel, HPHYSICSWEIGHTSET& hWeightSet) override;
    LTRESULT GetPhysicsVisNode(HOBJECT hModel, HMODELNODE& hNode) override;
    LTRESULT DirtyAllNodeTransforms(HOBJECT hModel) override;
    LTRESULT DirtyNodeTransform(HOBJECT hModel, HMODELNODE hNode) override;
    LTRESULT UpdateModelAttachments(HOBJECT hModel) override;
    LTRESULT GetModelFilename(HOBJECT hObj, char *pRetFilename, uint32 nFileBufLen) override;
    LTRESULT GetMaterialFilename(HOBJECT hObj, uint32 nIndex, char *pRetFilename, uint32 nFileBufLen) override;
    LTRESULT SetMaterialFilename(HOBJECT hObj, uint32 nIndex, const char *pFilename) override;
    LTRESULT GetMovementEncodingTransform(HOBJECT hModel, LTRigidTransform& tTransform) override;
    LTRESULT ResetMovementEncodingTransform(HOBJECT hModel) override;
    LTRESULT SetMovementEncodingTracker(HOBJECT hModel, ANIMTRACKERID TrackerID) override;
    LTRESULT DisableMovementEncoding(HOBJECT hModel) override;
};

extern CFearModel g_FearModel;

#endif
