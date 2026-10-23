#ifndef __FEAR_OBJECT_MAP_H__
#define __FEAR_OBJECT_MAP_H__

#include "ltbasedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

HOBJECT FearClient_ResolveServerObject(unsigned short nServerId);
void FearClient_SetLocalServerObject(unsigned short nServerId);
void FearClient_BeginServerMessage(void);
void FearClient_EndServerMessage(void);
bool FearClient_IsServerMessage(void);
HOBJECT FearClient_GetClientObject(void);
HOBJECT FearClient_GetGroundObject(void);
unsigned short FearClient_GetServerObjectId(HOBJECT hObject);
void FearClient_RegisterObject(HOBJECT hObject);
void FearClient_UpdateServerObjectTransform(unsigned short nServerId, const float *pPos, const float *pRotation);
bool FearClient_GetObjectTransform(HOBJECT hObject, float *pPos, float *pRotation);
bool FearClient_SetObjectPosition(HOBJECT hObject, const float *pPos);
void FearClient_RemoveObject(HOBJECT hObject);
bool FearClient_SetObjectDims(HOBJECT hObject, const float *pDims);
bool FearClient_GetObjectDims(HOBJECT hObject, float *pDims);
bool FearClient_SetObjectFlags(HOBJECT hObject, unsigned int nFlagType, unsigned int nFlags, unsigned int nMask);
bool FearClient_GetObjectFlags(HOBJECT hObject, unsigned int nFlagType, unsigned int *pFlags);
void FearClient_SetObjectType(HOBJECT hObject, unsigned int nObjectType);
unsigned int FearClient_GetObjectType(HOBJECT hObject);
void FearClient_SetLocalPlayerScope(bool bInScope);
bool FearClient_IsLocalPlayerScope(void);

#ifdef __cplusplus
}
#endif

#endif
