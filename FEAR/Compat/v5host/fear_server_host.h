#ifndef FEAR_SERVER_HOST_H
#define FEAR_SERVER_HOST_H

#include "fear_server_object_snapshot.h"

class ILTMessage_Read;

#ifdef __cplusplus
extern "C" {
#endif

bool FearServerHost_Start(const char *pszWorldName);
void FearServerHost_Update(void);
void FearServerHost_SubmitClientMessage(ILTMessage_Read *pMessage);
void FearServerHost_DeliverMessages(void);
void FearServerHost_Shutdown(void);
bool FearServerHost_IsRunning(void);
void *FearServerHost_GetClientHandle(void);
void *FearServerHost_GetPlayerObject(void);
unsigned short FearServerHost_GetPlayerObjectId(void);
unsigned short FearServerHost_GetLocalCharacterObjectId(void);
bool FearServerHost_GetPlayerTransform(float *pPos, float *pRotation);
bool FearServerHost_SetPlayerTransform(
    const float *pPos, const float *pVelocity, const float *pRotation);

typedef void (*TFearServerMessageSink)(ILTMessage_Read *pMessage);
void FearServerHost_SetMessageSink(TFearServerMessageSink pSink);
int FearServerHost_VisitCameras(TFearServerObjectVisitor pVisitor, void *pContext);
int FearServerHost_VisitObjects(TFearServerObjectVisitor pVisitor, void *pContext);
void *FearServerHost_ResolveObject(unsigned short nObjectId);
bool FearServerHost_GetAnimLength(void *pClientObject, unsigned int nAnimation, unsigned int *pLength);
bool FearServerHost_GetAnimKeyFrames(void *pClientObject, unsigned int nAnimation, unsigned int *pKeyFrames);
bool FearServerHost_GetAnimKeyFrame(
    void *pClientObject, unsigned int nAnimation, unsigned int nKeyFrame,
    unsigned int *pTime, char *pszKey, unsigned int nKeyLength);
bool FearServerHost_GetAnimNodeTransform(
    void *pClientObject, unsigned int nAnimation, unsigned int nNode, unsigned int nKeyFrame,
    float *pPosition, float *pRotation);
bool FearServerHost_GetNodeTransform(
    void *pClientObject, unsigned int nNode, float *pPosition, float *pRotation, bool bWorldSpace);
bool FearServerHost_GetBindPoseNodeTransform(
    void *pClientObject, unsigned int nNode, float *pPosition, float *pRotation);
bool FearServerHost_GetNodeTransformRaw(
    void *pServerObject, unsigned int nNode, float *pPosition, float *pRotation, bool bWorldSpace);
bool FearServerHost_GetBindPoseNodeTransformRaw(
    void *pServerObject, unsigned int nNode, float *pPosition, float *pRotation);
bool FearServerHost_GetNumNodesRaw(void *pServerObject, unsigned int *pNumNodes);
bool FearServerHost_GetMaterialFilenameRaw(
    void *pServerObject, unsigned int nIndex, char *pszName, unsigned int nNameLength);
bool FearServerHost_DumpModelPiecesRaw(
    void *pServerObject, unsigned int nMaxPieces);
bool FearServerHost_GetAnimIndex(void *pClientObject, const char *pszName, unsigned int *pAnimation);
bool FearServerHost_GetAnimName(
    void *pClientObject, unsigned int nAnimation, char *pszName, unsigned int nNameLength);
bool FearServerHost_GetModelAnimUserDims(
    void *pClientObject, unsigned int nAnimation, float *pDims);
bool FearServerHost_GetSocket(
    void *pClientObject, const char *pszName, unsigned int *pSocket);
bool FearServerHost_GetSocketTransform(
    void *pClientObject, unsigned int nSocket, float *pPosition, float *pRotation, bool bWorldSpace);
bool FearServerHost_GetNode(void *pClientObject, const char *pszName, unsigned int *pNode);
bool FearServerHost_GetNumNodes(void *pClientObject, unsigned int *pNumNodes);

#ifdef __cplusplus
}
#endif

#endif
