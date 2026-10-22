#include "cshell_object.h"
#include <windows.h>
#include <string.h>
#include "ltbasedefs.h"
#include "ltmodule.h"
#include "ltobjectcreate.h"
#include "iltclient.h"

static ILTClient *g_pClient = NULL;
define_holder(ILTClient, g_pClient);

static void FearCopyString(char *pDest, uint32 nDestSize, const char *pSrc, uint32 nSrcSize)
{
    if (!pDest || (nDestSize == 0))
        return;

    pDest[0] = '\0';
    if (!pSrc)
        return;

    uint32 nLen = 0;
    while ((nLen < nSrcSize) && (pSrc[nLen] != '\0'))
        nLen++;

    if (nLen >= nDestSize)
        nLen = nDestSize - 1;

    if (nLen > 0)
        memcpy(pDest, pSrc, nLen);

    pDest[nLen] = '\0';
}

void *FearObject_Create(const FearObjectCreate *pStruct)
{
    if (!pStruct || !g_pClient || !g_pClient->CreateObject)
        return NULL;

    ObjectCreateStruct ocs;

    ocs.m_ObjectType = pStruct->m_ObjectType;
    ocs.m_ContainerCode = pStruct->m_ContainerCode;
    ocs.m_Flags = pStruct->m_Flags;
    ocs.m_Flags2 = pStruct->m_Flags2;
    ocs.m_UserData = pStruct->m_UserData;

    ocs.m_Pos.Init(pStruct->m_Pos[0], pStruct->m_Pos[1], pStruct->m_Pos[2]);
    ocs.m_Scale.Init(pStruct->m_Scale, pStruct->m_Scale, pStruct->m_Scale);

    ocs.m_Rotation.m_Quat[0] = pStruct->m_Rotation[0];
    ocs.m_Rotation.m_Quat[1] = pStruct->m_Rotation[1];
    ocs.m_Rotation.m_Quat[2] = pStruct->m_Rotation[2];
    ocs.m_Rotation.m_Quat[3] = pStruct->m_Rotation[3];

    float fQuatMagSqr = (ocs.m_Rotation.m_Quat[0] * ocs.m_Rotation.m_Quat[0]) +
                        (ocs.m_Rotation.m_Quat[1] * ocs.m_Rotation.m_Quat[1]) +
                        (ocs.m_Rotation.m_Quat[2] * ocs.m_Rotation.m_Quat[2]) +
                        (ocs.m_Rotation.m_Quat[3] * ocs.m_Rotation.m_Quat[3]);
    if (fQuatMagSqr < 0.000001f)
        ocs.m_Rotation.Init();

    FearCopyString(ocs.m_Filename, (uint32)sizeof(ocs.m_Filename), pStruct->m_Filename, (uint32)sizeof(pStruct->m_Filename));
    FearCopyString(ocs.m_Name, (uint32)sizeof(ocs.m_Name), pStruct->m_Name, (uint32)sizeof(pStruct->m_Name));

    return (void *)g_pClient->CreateObject(&ocs);
}
