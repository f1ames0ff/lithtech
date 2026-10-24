#pragma once

#define FEAR_INVALID_OBJECT_ID 0x7FFF

struct FearServerObjectSnapshot
{
    unsigned short m_ObjectId;
    unsigned short m_Reserved;
    void *m_pServerObject;
    unsigned int m_Flags;
    unsigned int m_Flags2;
    unsigned int m_UserFlags;
    float m_Position[3];
    float m_Rotation[4];
    const unsigned char *m_pSFXData;
    unsigned int m_SFXBits;
    char m_Name[64];
    char m_ClassName[64];
    char m_ModelFilename[260];
};

typedef void (__cdecl *TFearServerObjectVisitor)(const FearServerObjectSnapshot *, void *);
typedef int (__cdecl *TFearGameServerVisitCameras)(TFearServerObjectVisitor, void *);
typedef int (__cdecl *TFearGameServerVisitObjects)(TFearServerObjectVisitor, void *);
typedef void *(__cdecl *TFearGameServerResolveObject)(unsigned short);
typedef unsigned short (__cdecl *TFearGameServerObjectWireId)(void *);
typedef unsigned short (__cdecl *TFearGameServerGetLocalCharacterWireId)(void);
typedef int (__cdecl *TFearGameServerSetPlayerTransform)(
    const float *, const float *, const float *);
typedef int (__cdecl *TFearGameServerGetAnimLength)(
    void *, unsigned int, unsigned int *);
typedef int (__cdecl *TFearGameServerGetAnimKeyFrames)(
    void *, unsigned int, unsigned int *);
typedef int (__cdecl *TFearGameServerGetAnimNodeTransform)(
    void *, unsigned int, unsigned int, unsigned int, float *, float *);
typedef int (__cdecl *TFearGameServerGetNodeTransform)(
    void *, unsigned int, float *, float *, int);
typedef int (__cdecl *TFearGameServerGetBindPoseNodeTransform)(
    void *, unsigned int, float *, float *);
typedef int (__cdecl *TFearGameServerGetAnimIndex)(
    void *, const char *, unsigned int *);
typedef int (__cdecl *TFearGameServerGetAnimName)(
    void *, unsigned int, char *, unsigned int);
typedef int (__cdecl *TFearGameServerGetModelAnimUserDims)(
    void *, unsigned int, float *);
typedef int (__cdecl *TFearGameServerGetSocket)(
    void *, const char *, unsigned int *);
typedef int (__cdecl *TFearGameServerGetSocketTransform)(
    void *, unsigned int, float *, float *, int);
typedef int (__cdecl *TFearGameServerGetNode)(
    void *, const char *, unsigned int *);
typedef int (__cdecl *TFearGameServerGetNumNodes)(
    void *, unsigned int *);
typedef int (__cdecl *TFearGameServerDumpModelPieces)(
    void *, unsigned int);
