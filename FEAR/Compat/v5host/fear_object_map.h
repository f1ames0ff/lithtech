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
unsigned short FearClient_GetServerObjectId(HOBJECT hObject);
void FearClient_RegisterObject(HOBJECT hObject);
void FearClient_UpdateServerObjectTransform(unsigned short nServerId, const float *pPos, const float *pRotation);
bool FearClient_GetObjectTransform(HOBJECT hObject, float *pPos, float *pRotation);
void FearClient_RemoveObject(HOBJECT hObject);

#ifdef __cplusplus
}
#endif

#endif
