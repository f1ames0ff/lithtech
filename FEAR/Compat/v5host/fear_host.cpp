#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include <string>
#include <vector>
#include "fear_host_api.h"
#include "fear_render_api.h"
#include "ltbasedefs.h"
#include "ltassert.h"
#include "ltmodule.h"
#include "iclientshell.h"
#include "iltclient.h"
#include "iltmessage.h"
#include "fear_client_facade.h"
#include "fear_object_map.h"
#include "fear_server_host.h"
#include "fear_input_stub.h"
#include "fear_resourcemgr_stub.h"
#include "fear_common_stub.h"
#include "fear_physics_stub.h"
#include "fear_model_stub.h"
#include "fear_timer_stub.h"
#include "fear_soundmgr_stub.h"
#include "fear_cursor_stub.h"
#include "fear_filemgr_stub.h"
#include "fear_contenttransfer_stub.h"
#include "fear_renderer_stub.h"
#include "fear_drawprim_stub.h"
#include "fear_texturemgr_stub.h"
#include "fear_texturestring_stub.h"
#include "fear_input_stub.h"
#include "fear_gameutil_stub.h"

namespace
{

enum FearTokenResult
{
    FEAR_TOKEN_NONE,
    FEAR_TOKEN_VALUE,
    FEAR_TOKEN_SEPARATOR
};

bool FearIsCommandSeparator(char c)
{
    return c == ';' || (unsigned char)c <= 31 || c == 127;
}

FearTokenResult FearReadToken(const char *&pCursor, char *&pOutput)
{
    while (*pCursor == ' ')
        ++pCursor;
    if (!*pCursor)
        return FEAR_TOKEN_NONE;

    char *pToken = pOutput;
    int nParenDepth = 0;
    for (;;)
    {
        char c = *pCursor;
        if (!c)
            break;
        if (c == '%')
        {
            ++pCursor;
            c = *pCursor;
            if (!c)
                break;
            goto AddCharacter;
        }
        if (FearIsCommandSeparator(c))
        {
            *pOutput = '\0';
            if (!*pToken)
                ++pCursor;
            return *pToken ? FEAR_TOKEN_VALUE : FEAR_TOKEN_SEPARATOR;
        }
        if (c == '(')
        {
            if (++nParenDepth > 1)
                goto AddCharacter;
        }
        else if (c == ')')
        {
            --nParenDepth;
            if (nParenDepth <= 0)
            {
                ++pCursor;
                break;
            }
            goto AddCharacter;
        }
        else if (c == '"')
        {
            if (nParenDepth)
            {
                ++pCursor;
                break;
            }
            nParenDepth = 1;
        }
        else if (c == ' ' && nParenDepth == 0)
        {
            break;
        }
        else
        {
AddCharacter:
            *pOutput++ = c;
            if (pOutput - pToken >= PARSE_MAXTOKENSIZE)
                --pOutput;
        }
        ++pCursor;
    }

    *pOutput++ = '\0';
    return *pToken ? FEAR_TOKEN_VALUE : FEAR_TOKEN_NONE;
}

bool FearParseCommand(
    const char *pCommand, const char **ppNextCommand, char *pBuffer,
    char **pArguments, int *pArgumentCount)
{
    *pArgumentCount = 0;
    char *pOutput = pBuffer;
    while (true)
    {
        char *pToken = pOutput;
        FearTokenResult result = FearReadToken(pCommand, pOutput);
        if (result == FEAR_TOKEN_NONE)
            return false;
        if (result == FEAR_TOKEN_VALUE)
        {
            pArguments[(*pArgumentCount)++] = pToken;
            if (*pArgumentCount >= PARSE_MAXTOKENS)
                return false;
        }
        else
        {
            *ppNextCommand = pCommand;
            return true;
        }
    }
}

}

bool ConParse::Parse()
{
    if (!m_pCommandPos)
        return false;
    if (!FearParseCommand(
        m_pCommandPos, &m_pCommandPos, m_ArgBuffer, m_Args, &m_nArgs))
        m_pCommandPos = NULL;
    return true;
}

int LTAssert::RunAssert(const char*, const char*, int, const char*, bool, const char*, ...)
{
    return 0;
}

ILTClient *g_pLTClient = NULL;
define_holder(ILTClient, g_pLTClient);
IClientShell *g_pShell = NULL;
define_holder(IClientShell, g_pShell);

static void FearHost_DeliverModelKeyText(
    HOBJECT hObject, uint32 nTracker, const char *pszKey)
{
    if (!g_pShell || !pszKey || !pszKey[0])
        return;

    static int s_nModelKeyLogs = 0;
    if (s_nModelKeyLogs < 64 || strstr(pszKey, "FIRE"))
    {
        if (s_nModelKeyLogs < 64)
            ++s_nModelKeyLogs;
        printf("FEARPORT: source model key obj=%p tracker=%u key='%s'\n",
            hObject, (unsigned)nTracker, pszKey);
        fflush(stdout);
    }

    char szKey[1024];
    strncpy(szKey, pszKey, sizeof(szKey) - 1);
    szKey[sizeof(szKey) - 1] = '\0';
    ConParse parse;
    parse.Init(szKey);
    while (parse.Parse())
    {
        if (parse.m_nArgs == 0)
            continue;

        ArgList arguments;
        arguments.argc = (int)parse.m_nArgs;
        arguments.argv = parse.m_Args;
        g_pShell->OnModelKey((HLOCALOBJ)hObject, nTracker, &arguments);
    }
}

define_interface(CFearClient, ILTClient);
define_interface(CFearResourceMgr, ILTResourceMgr);
define_interface(CFearCommon, ILTCommon);
define_interface(CFearPhysics, ILTPhysics);
define_interface(CFearModel, ILTModel);
define_interface(CFearTimer, ILTTimer);
define_interface(CFearSoundMgr, ILTSoundMgr);
define_interface(CFearCursor, ILTCursor);
define_interface(CFearRenderer, ILTRenderer);
define_interface(CFearDrawPrim, ILTDrawPrim);
define_interface(CFearTextureMgr, ILTTextureMgr);
define_interface(CFearTextureString, ILTTextureString);
define_interface(CFearInput, ILTInput);
define_interface(CFearGameUtil, ILTGameUtil);
define_interface(CFearClientContentTransfer, ILTClientContentTransfer);

static HMODULE g_hGameClient = NULL;
static const FearEngineApi *g_pEngineApi = NULL;
static const unsigned char kFearSfxCharacterId = 11;

typedef void (*TFearClientMouseMove)(int, int);
typedef void (*TFearClientMouseButton)(int, int, int, int);
typedef void (*TFearClientMouseWheel)(int, int, int);
typedef void (*TFearClientDisableWindowHook)();
typedef void (*TFearClientPreLoadWorld)(const char *);
typedef void (*TFearClientOnEnterWorld)();
typedef void (*TFearClientLogState)();
typedef int (*TFearClientIsPostload)();
typedef void (*TFearClientPressKey)(int);
typedef void (*TFearClientReplySaveFailed)();
typedef void (*TFearClientEnterWorldPreview)();
typedef void (*TFearClientRenderWorldPreview)();
typedef int (*TFearClientStartNewGame)();
typedef int (*TFearClientIsPlaying)();
typedef int (*TFearClientGetGameState)();
typedef void (*TFearClientCloseUserMenu)();
typedef int (*TFearClientPreparePortPlayer)(void *);
typedef void *(*TFearClientGetMoveObject)();
typedef int (*TFearClientIsPlayerMovementAllowed)();
typedef int (*TFearClientGetMovementState)(float *, float *, float *);
typedef int (*TFearClientCreateLocalCharacter)(void *);
typedef void (*TFearClientResetObjectFX)(void *);
typedef void (*TFearClientRenderPortCamera)();
typedef int (*TFearClientInstallDefaultBindings)();
typedef void (*TFearClientLogInputState)();
typedef void (__cdecl *TFearClientApplyServerCamera)(
    void *, unsigned int, unsigned int, unsigned int,
    const float *, const float *, const unsigned char *, unsigned int, int);

static TFearClientMouseMove g_pfnClientMouseMove = NULL;
static TFearClientMouseButton g_pfnClientMouseButton = NULL;
static TFearClientMouseWheel g_pfnClientMouseWheel = NULL;
static TFearClientDisableWindowHook g_pfnClientDisableWindowHook = NULL;
static TFearClientPreLoadWorld g_pfnClientPreLoadWorld = NULL;
static TFearClientOnEnterWorld g_pfnClientOnEnterWorld = NULL;
static TFearClientLogState g_pfnClientLogState = NULL;
static TFearClientIsPostload g_pfnClientIsPostload = NULL;
static TFearClientPressKey g_pfnClientPressKey = NULL;
static TFearClientReplySaveFailed g_pfnClientReplySaveFailed = NULL;
static TFearClientEnterWorldPreview g_pfnClientEnterWorldPreview = NULL;
static TFearClientRenderWorldPreview g_pfnClientRenderWorldPreview = NULL;
static TFearClientStartNewGame g_pfnClientStartNewGame = NULL;
static TFearClientIsPlaying g_pfnClientIsPlaying = NULL;
static TFearClientGetGameState g_pfnClientGetGameState = NULL;
static TFearClientCloseUserMenu g_pfnClientCloseUserMenu = NULL;
static TFearClientPreparePortPlayer g_pfnClientPreparePortPlayer = NULL;
static TFearClientGetMoveObject g_pfnClientGetMoveObject = NULL;
static TFearClientIsPlayerMovementAllowed g_pfnClientIsPlayerMovementAllowed = NULL;
static TFearClientGetMovementState g_pfnClientGetMovementState = NULL;
static TFearClientCreateLocalCharacter g_pfnClientCreateLocalCharacter = NULL;
static TFearClientResetObjectFX g_pfnClientResetObjectFX = NULL;
static TFearClientRenderPortCamera g_pfnClientRenderPortCamera = NULL;
static TFearClientInstallDefaultBindings g_pfnClientInstallDefaultBindings = NULL;
static TFearClientLogInputState g_pfnClientLogInputState = NULL;
static TFearClientApplyServerCamera g_pfnClientApplyServerCamera = NULL;

struct FearCameraProxyState
{
    void *m_pServerObject;
    unsigned short m_ClientId;
    unsigned long long m_SFXHash;
    unsigned int m_LastSeen;
    bool m_bNotified;
};

static std::vector<FearCameraProxyState> g_CameraProxies;
static unsigned int g_nCameraVisitGeneration = 0;
static unsigned short g_nNextCameraClientId = 0x4000;

static unsigned long long FearHost_HashBytes(const unsigned char *pData, unsigned int nBits)
{
    unsigned long long nHash = 1469598103934665603ull;
    unsigned int nBytes = (nBits + 7) >> 3;
    for (unsigned int i = 0; i < nBytes; ++i)
    {
        nHash ^= pData ? pData[i] : 0;
        nHash *= 1099511628211ull;
    }
    return nHash;
}

static void __cdecl FearHost_CameraVisitor(const FearServerObjectSnapshot *pSnapshot, void *)
{
    if (!pSnapshot || !pSnapshot->m_pServerObject || !g_pfnClientApplyServerCamera)
        return;

    FearCameraProxyState *pState = NULL;
    for (size_t i = 0; i < g_CameraProxies.size(); ++i)
    {
        if (g_CameraProxies[i].m_pServerObject == pSnapshot->m_pServerObject)
        {
            pState = &g_CameraProxies[i];
            break;
        }
    }

    if (!pState)
    {
        FearCameraProxyState state;
        state.m_pServerObject = pSnapshot->m_pServerObject;
        state.m_ClientId = g_nNextCameraClientId++;
        state.m_SFXHash = 0;
        state.m_LastSeen = 0;
        state.m_bNotified = false;
        g_CameraProxies.push_back(state);
        pState = &g_CameraProxies.back();
    }

    unsigned long long nHash = FearHost_HashBytes(pSnapshot->m_pSFXData, pSnapshot->m_SFXBits);
    bool bNotify = !pState->m_bNotified && pSnapshot->m_SFXBits != 0;
    pState->m_LastSeen = g_nCameraVisitGeneration;
    pState->m_SFXHash = nHash;
    pState->m_bNotified = pState->m_bNotified || bNotify;

    HOBJECT hObject = FearClient_ResolveServerObject(pState->m_ClientId);
    if (!hObject)
        return;

    g_pfnClientApplyServerCamera(
        hObject, pSnapshot->m_Flags, pSnapshot->m_Flags2, pSnapshot->m_UserFlags,
        pSnapshot->m_Position, pSnapshot->m_Rotation, pSnapshot->m_pSFXData, pSnapshot->m_SFXBits,
        bNotify ? 1 : 0);

    if (bNotify)
    {
        printf("FEARPORT: camera proxy id=%u obj=%p name='%s' user=%08x pos=(%.1f %.1f %.1f)\n",
            (unsigned)pState->m_ClientId, pSnapshot->m_pServerObject, pSnapshot->m_Name,
            pSnapshot->m_UserFlags, pSnapshot->m_Position[0], pSnapshot->m_Position[1],
            pSnapshot->m_Position[2]);
        fflush(stdout);
    }
}

static void FearHost_SyncCameras()
{
    if (!g_pfnClientApplyServerCamera)
        return;

    ++g_nCameraVisitGeneration;
    FearServerHost_VisitCameras(FearHost_CameraVisitor, NULL);

    for (size_t i = 0; i < g_CameraProxies.size(); ++i)
    {
        FearCameraProxyState &state = g_CameraProxies[i];
        if (state.m_LastSeen + 8 < g_nCameraVisitGeneration)
        {
            g_pfnClientApplyServerCamera(
                FearClient_ResolveServerObject(state.m_ClientId), 0, 0, 0, NULL, NULL, NULL, 0, 0);
        }
    }
}

struct FearObjectProxyState
{
    void *m_pServerObject;
    unsigned short m_ObjectId;
    bool m_bNotified;
};

static std::vector<FearObjectProxyState> g_ObjectProxies;

struct FearRenderObject
{
    void *m_pServerObject;
    unsigned short m_ObjectId;
    bool m_bSkinned;
    float m_Position[3];
    float m_Rotation[4];
    char m_ModelFilename[260];
    char m_MaterialFilename[260];
    float m_DistanceSq;
};

static std::vector<FearRenderObject> g_RenderObjects;
static unsigned short g_nRenderSkipCharacterId = FEAR_INVALID_OBJECT_ID;

struct FearModelMaterialCache
{
    std::string m_Model;
    std::string m_Material;
};

static std::vector<FearModelMaterialCache> g_ModelMaterialCache;

static void FearHost_GetModelMaterial(
    const FearServerObjectSnapshot *pSnapshot, char *pszMaterial, size_t nLength)
{
    pszMaterial[0] = '\0';
    if (!pSnapshot || !pSnapshot->m_ModelFilename[0])
        return;
    if (pSnapshot->m_ObjectId == g_nRenderSkipCharacterId)
        return;
    for (size_t i = 0; i < g_ModelMaterialCache.size(); ++i)
    {
        if (_stricmp(g_ModelMaterialCache[i].m_Model.c_str(),
            pSnapshot->m_ModelFilename) == 0)
        {
            strncpy(pszMaterial, g_ModelMaterialCache[i].m_Material.c_str(), nLength - 1);
            pszMaterial[nLength - 1] = '\0';
            return;
        }
    }

    FearModelMaterialCache cache;
    cache.m_Model = pSnapshot->m_ModelFilename;
    char filename[260] = {0};
    if (FearServerHost_GetMaterialFilenameRaw(
        pSnapshot->m_pServerObject, 0, filename, sizeof(filename)))
        cache.m_Material = filename;
    g_ModelMaterialCache.push_back(cache);
    strncpy(pszMaterial, cache.m_Material.c_str(), nLength - 1);
    pszMaterial[nLength - 1] = '\0';
}

static void __cdecl FearHost_ObjectVisitor(const FearServerObjectSnapshot *pSnapshot, void *)
{
    if (!pSnapshot || !pSnapshot->m_pServerObject ||
        pSnapshot->m_ObjectId == FEAR_INVALID_OBJECT_ID ||
        !pSnapshot->m_pSFXData || !pSnapshot->m_SFXBits ||
        !g_pfnClientApplyServerCamera)
        return;

    FearObjectProxyState *pState = NULL;
    for (size_t i = 0; i < g_ObjectProxies.size(); ++i)
    {
        if (g_ObjectProxies[i].m_ObjectId == pSnapshot->m_ObjectId)
        {
            pState = &g_ObjectProxies[i];
            break;
        }
    }

    if (!pState)
    {
        FearObjectProxyState state;
        state.m_pServerObject = pSnapshot->m_pServerObject;
        state.m_ObjectId = pSnapshot->m_ObjectId;
        state.m_bNotified = false;
        g_ObjectProxies.push_back(state);
        pState = &g_ObjectProxies.back();
    }
    else if (pState->m_pServerObject != pSnapshot->m_pServerObject)
    {
        pState->m_pServerObject = pSnapshot->m_pServerObject;
        pState->m_bNotified = false;
    }

    HOBJECT hObject = FearClient_ResolveServerObjectWithModel(
        pSnapshot->m_ObjectId, pSnapshot->m_ModelFilename, pSnapshot->m_Name);
    if (!hObject)
        return;

    bool bNotify = !pState->m_bNotified;
    if (bNotify && pSnapshot->m_pSFXData[0] == kFearSfxCharacterId && g_pfnClientResetObjectFX)
        g_pfnClientResetObjectFX(hObject);

    g_pfnClientApplyServerCamera(
        hObject, pSnapshot->m_Flags, pSnapshot->m_Flags2, pSnapshot->m_UserFlags,
        pSnapshot->m_Position, pSnapshot->m_Rotation, pSnapshot->m_pSFXData, pSnapshot->m_SFXBits,
        bNotify ? 1 : 0);

    if (bNotify)
    {
        pState->m_bNotified = true;
        printf("FEARPORT: object proxy id=%u obj=%p class='%s' name='%s' model='%s' sfx=%u bits=%u\n",
            (unsigned)pSnapshot->m_ObjectId, pSnapshot->m_pServerObject,
            pSnapshot->m_ClassName, pSnapshot->m_Name, pSnapshot->m_ModelFilename,
            (unsigned)pSnapshot->m_pSFXData[0], pSnapshot->m_SFXBits);
        fflush(stdout);
    }
}

static void __cdecl FearHost_RenderObjectVisitor(
    const FearServerObjectSnapshot *pSnapshot, void *pContext)
{
    if (!pSnapshot || !pSnapshot->m_ModelFilename[0])
        return;
    if (pSnapshot->m_ObjectId == g_nRenderSkipCharacterId)
        return;

    const float *pPlayerPosition = (const float *)pContext;
    float dx = pSnapshot->m_Position[0] - pPlayerPosition[0];
    float dy = pSnapshot->m_Position[1] - pPlayerPosition[1];
    float dz = pSnapshot->m_Position[2] - pPlayerPosition[2];

    FearRenderObject object;
    object.m_pServerObject = pSnapshot->m_pServerObject;
    object.m_ObjectId = pSnapshot->m_ObjectId;
    object.m_bSkinned = _stricmp(pSnapshot->m_ClassName, "CAI") == 0 ||
        _stricmp(pSnapshot->m_ClassName, "CPlayerObj") == 0;
    memcpy(object.m_Position, pSnapshot->m_Position, sizeof(object.m_Position));
    memcpy(object.m_Rotation, pSnapshot->m_Rotation, sizeof(object.m_Rotation));
    strncpy(object.m_ModelFilename, pSnapshot->m_ModelFilename,
        sizeof(object.m_ModelFilename) - 1);
    object.m_ModelFilename[sizeof(object.m_ModelFilename) - 1] = '\0';
    FearHost_GetModelMaterial(pSnapshot, object.m_MaterialFilename,
        sizeof(object.m_MaterialFilename));
    object.m_DistanceSq = dx * dx + dy * dy + dz * dz;
    g_RenderObjects.push_back(object);
}

static void FearHost_RefreshRenderObjects()
{
    g_RenderObjects.clear();
    g_nRenderSkipCharacterId =
        (g_pfnClientIsPlayerMovementAllowed && g_pfnClientIsPlayerMovementAllowed()) ?
        FearServerHost_GetLocalCharacterObjectId() : FEAR_INVALID_OBJECT_ID;

    float playerPosition[3] = {0.0f, 0.0f, 0.0f};
    if (g_pfnClientGetMovementState)
    {
        float velocity[3] = {0.0f, 0.0f, 0.0f};
        float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        g_pfnClientGetMovementState(playerPosition, velocity, rotation);
    }
    FearServerHost_VisitObjects(
        FearHost_RenderObjectVisitor, playerPosition);
    std::sort(g_RenderObjects.begin(), g_RenderObjects.end(),
        [](const FearRenderObject &a, const FearRenderObject &b)
        {
            return a.m_DistanceSq < b.m_DistanceSq;
        });
}

static int FearHost_SyncObjects()
{
    if (!g_pfnClientApplyServerCamera)
        return 0;

    int nObjects = FearServerHost_VisitObjects(FearHost_ObjectVisitor, NULL);
    FearHost_RefreshRenderObjects();
    return nObjects;
}

struct FearNodeBindCache
{
    std::string m_Filename;
    unsigned int m_nNodeCount;
    bool m_bBound;
    bool m_bLogged;
    unsigned int m_nPoseUpdates;
    std::vector<float> m_Matrices;
};

static std::vector<FearNodeBindCache> g_NodeBindCaches;

static void FearMatrixIdentity(float *pMatrix)
{
    memset(pMatrix, 0, sizeof(float) * 16);
    pMatrix[0] = pMatrix[5] = pMatrix[10] = pMatrix[15] = 1.0f;
}

static void FearMatrixFromTransform(
    const float *pPosition, const float *pRotation, float *pMatrix)
{
    const float x = pRotation[0], y = pRotation[1];
    const float z = pRotation[2], w = pRotation[3];
    const float xx = x * x, yy = y * y, zz = z * z;
    const float xy = x * y, xz = x * z, yz = y * z;
    const float xw = x * w, yw = y * w, zw = z * w;

    pMatrix[0] = 1.0f - 2.0f * (yy + zz);
    pMatrix[1] = 2.0f * (xy + zw);
    pMatrix[2] = 2.0f * (xz - yw);
    pMatrix[3] = 0.0f;
    pMatrix[4] = 2.0f * (xy - zw);
    pMatrix[5] = 1.0f - 2.0f * (xx + zz);
    pMatrix[6] = 2.0f * (yz + xw);
    pMatrix[7] = 0.0f;
    pMatrix[8] = 2.0f * (xz + yw);
    pMatrix[9] = 2.0f * (yz - xw);
    pMatrix[10] = 1.0f - 2.0f * (xx + yy);
    pMatrix[11] = 0.0f;
    pMatrix[12] = pPosition[0];
    pMatrix[13] = pPosition[1];
    pMatrix[14] = pPosition[2];
    pMatrix[15] = 1.0f;
}

static void FearMatrixMultiply(const float *pA, const float *pB, float *pOut)
{
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            pOut[i * 4 + j] =
                pA[i * 4 + 0] * pB[0 * 4 + j] +
                pA[i * 4 + 1] * pB[1 * 4 + j] +
                pA[i * 4 + 2] * pB[2 * 4 + j] +
                pA[i * 4 + 3] * pB[3 * 4 + j];
        }
    }
}

static void FearMatrixInvertRigid(const float *pMatrix, float *pInverse)
{
    pInverse[0] = pMatrix[0];
    pInverse[1] = pMatrix[4];
    pInverse[2] = pMatrix[8];
    pInverse[3] = 0.0f;
    pInverse[4] = pMatrix[1];
    pInverse[5] = pMatrix[5];
    pInverse[6] = pMatrix[9];
    pInverse[7] = 0.0f;
    pInverse[8] = pMatrix[2];
    pInverse[9] = pMatrix[6];
    pInverse[10] = pMatrix[10];
    pInverse[11] = 0.0f;
    pInverse[12] = -(pMatrix[12] * pInverse[0] +
        pMatrix[13] * pInverse[4] + pMatrix[14] * pInverse[8]);
    pInverse[13] = -(pMatrix[12] * pInverse[1] +
        pMatrix[13] * pInverse[5] + pMatrix[14] * pInverse[9]);
    pInverse[14] = -(pMatrix[12] * pInverse[2] +
        pMatrix[13] * pInverse[6] + pMatrix[14] * pInverse[10]);
    pInverse[15] = 1.0f;
}

static FearNodeBindCache *FearHost_GetNodeBindCache(const FearRenderObject &object)
{
    for (size_t i = 0; i < g_NodeBindCaches.size(); ++i)
    {
        if (g_NodeBindCaches[i].m_Filename == object.m_ModelFilename)
            return &g_NodeBindCaches[i];
    }

    FearNodeBindCache cache;
    cache.m_Filename = object.m_ModelFilename;
    cache.m_nNodeCount = 0;
    cache.m_bBound = false;
    cache.m_bLogged = false;
    cache.m_nPoseUpdates = 0;
    if (FearServerHost_GetNumNodesRaw(
        object.m_pServerObject, &cache.m_nNodeCount) &&
        cache.m_nNodeCount > 0 && cache.m_nNodeCount <= 256)
    {
        cache.m_Matrices.assign((size_t)cache.m_nNodeCount * 16, 0.0f);
        cache.m_bBound = true;
        for (unsigned int node = 0; node < cache.m_nNodeCount; ++node)
        {
            float position[3] = {0.0f, 0.0f, 0.0f};
            float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
            float *pMatrix = &cache.m_Matrices[node * 16];
            if (FearServerHost_GetBindPoseNodeTransformRaw(
                object.m_pServerObject, node, position, rotation))
                FearMatrixFromTransform(position, rotation, pMatrix);
            else
                FearMatrixIdentity(pMatrix);
        }
        for (unsigned int material = 0; material < 16; ++material)
        {
            char filename[260] = {0};
            if (!FearServerHost_GetMaterialFilenameRaw(
                object.m_pServerObject, material, filename, sizeof(filename)))
                break;
            printf("FEARMODELPOSE: material model='%s' index=%u file='%s'\n",
                cache.m_Filename.c_str(), material, filename);
        }
        FearServerHost_DumpModelPiecesRaw(object.m_pServerObject, 64);
        fflush(stdout);
    }
    if (!cache.m_bBound)
        return NULL;
    g_NodeBindCaches.push_back(cache);
    return &g_NodeBindCaches.back();
}

static bool FearHost_BuildSkinMatrices(
    const FearRenderObject &object, std::vector<float> &matrices)
{
    FearNodeBindCache *pCache = FearHost_GetNodeBindCache(object);
    if (!pCache || !pCache->m_bBound)
        return false;

    matrices.resize((size_t)pCache->m_nNodeCount * 16);
    for (unsigned int node = 0; node < pCache->m_nNodeCount; ++node)
    {
        float position[3] = {0.0f, 0.0f, 0.0f};
        float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        float current[16];
        float bindInverse[16];
        if (FearServerHost_GetNodeTransformRaw(
            object.m_pServerObject, node, position, rotation, false))
            FearMatrixFromTransform(position, rotation, current);
        else
            FearMatrixIdentity(current);
        FearMatrixInvertRigid(&pCache->m_Matrices[node * 16], bindInverse);
        FearMatrixMultiply(current, bindInverse, &matrices[node * 16]);
    }

    if (!pCache->m_bLogged)
    {
        pCache->m_bLogged = true;
        printf("FEARMODELPOSE: bind model='%s' nodes=%u\n",
            pCache->m_Filename.c_str(), pCache->m_nNodeCount);
        fflush(stdout);
    }
    ++pCache->m_nPoseUpdates;
    if (pCache->m_nPoseUpdates <= 3 ||
        (pCache->m_nPoseUpdates % 120) == 0)
    {
        const float *pNode = &matrices[0];
        const float *pLast = &matrices[(pCache->m_nNodeCount - 1) * 16];
        printf("FEARMODELPOSE: pose model='%s' update=%u node0=%.2f,%.2f,%.2f node%u=%.2f,%.2f,%.2f\n",
            pCache->m_Filename.c_str(), pCache->m_nPoseUpdates,
            pNode[12], pNode[13], pNode[14],
            pCache->m_nNodeCount - 1,
            pLast[12], pLast[13], pLast[14]);
        fflush(stdout);
    }
    return true;
}

static void FearHost_QueueClientModels()
{
    if (!g_pEngineApi || !g_pEngineApi->m_pRenderApi ||
        !g_pEngineApi->m_pRenderApi->QueueModel)
        return;

    static std::vector<std::string> s_LoggedModels;
    int nMapped = FearClient_GetMappedObjectCount();
    int nQueued = 0;
    for (int i = 0; i < nMapped && nQueued < 32; ++i)
    {
        HOBJECT hObject = NULL;
        char filename[260] = {0};
        float position[3] = {0.0f, 0.0f, 0.0f};
        float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        if (!FearClient_GetClientModelInfo(
            i, &hObject, filename, sizeof(filename), position, rotation))
            continue;

        float matrix[16];
        FearMatrixFromTransform(position, rotation, matrix);
        g_pEngineApi->m_pRenderApi->QueueModel(filename, NULL, matrix);
        ++nQueued;

        bool bLogged = false;
        for (size_t j = 0; j < s_LoggedModels.size(); ++j)
        {
            if (_stricmp(s_LoggedModels[j].c_str(), filename) == 0)
            {
                bLogged = true;
                break;
            }
        }
        if (!bLogged)
        {
            s_LoggedModels.push_back(filename);
            printf("FEARMODEL: client object=%p model='%s' pos=%.1f %.1f %.1f\n",
                hObject, filename, position[0], position[1], position[2]);
            fflush(stdout);
        }
    }
}

static void FearHost_QueueRenderObjects()
{
    if (!g_pEngineApi || !g_pEngineApi->m_pRenderApi ||
        !g_pEngineApi->m_pRenderApi->QueueModel)
        return;

    size_t nCount = g_RenderObjects.size();
    if (nCount > 64)
        nCount = 64;

    std::vector<float> skinMatrices;
    for (size_t i = 0; i < nCount; ++i)
    {
        const FearRenderObject &object = g_RenderObjects[i];
        float x = object.m_Rotation[0];
        float y = object.m_Rotation[1];
        float z = object.m_Rotation[2];
        float w = object.m_Rotation[3];
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float xw = x * w, yw = y * w, zw = z * w;

        float matrix[16] = {
            1.0f - 2.0f * (yy + zz), 2.0f * (xy + zw), 2.0f * (xz - yw), 0.0f,
            2.0f * (xy - zw), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + xw), 0.0f,
            2.0f * (xz + yw), 2.0f * (yz - xw), 1.0f - 2.0f * (xx + yy), 0.0f,
            object.m_Position[0], object.m_Position[1], object.m_Position[2], 1.0f
        };
        if (object.m_bSkinned &&
            g_pEngineApi->m_pRenderApi->QueueSkinnedModel &&
            FearHost_BuildSkinMatrices(object, skinMatrices))
        {
            g_pEngineApi->m_pRenderApi->QueueSkinnedModel(
                object.m_ModelFilename, object.m_MaterialFilename, matrix, skinMatrices.data(),
                (unsigned int)(skinMatrices.size() / 16));
        }
        else
        {
            g_pEngineApi->m_pRenderApi->QueueModel(
                object.m_ModelFilename, object.m_MaterialFilename, matrix);
        }
    }
    FearHost_QueueClientModels();
}

static bool g_bWorldPreview = false;
static bool g_bAwaitPostload = false;
static bool g_bPreviewKeys[256] = {false};
static float g_fPreviewPos[3] = { -1100.0f, -1372.0f, -370.0f };
static float g_fPreviewYaw = -0.0784588f;
static float g_fPreviewPitch = 0.0f;
static int g_nPreviewLastMouseX = 320;
static int g_nPreviewLastMouseY = 240;
static bool g_bPreviewMouseInitialized = false;
static DWORD g_nPreviewLastTick = 0;

extern "C" __declspec(dllexport) void FearHost_OnKeyDown(int key, int rep);
extern "C" __declspec(dllexport) void FearHost_OnKeyUp(int key);

struct FearQueuedInput
{
    unsigned int m_nDownFrame;
    unsigned int m_nUpFrame;
    int m_nKey;
    bool m_bDownSent;
    bool m_bUpSent;
};

static std::vector<FearQueuedInput> g_QueuedInputs;
static unsigned int g_nQueuedInputFrame = 0;
static bool g_bQueuedInputsInitialized = false;

static int FearParseVirtualKey(const char *pszKey)
{
    if (!pszKey || !pszKey[0])
        return 0;

    if (!_stricmp(pszKey, "DOWN")) return VK_DOWN;
    if (!_stricmp(pszKey, "UP")) return VK_UP;
    if (!_stricmp(pszKey, "LEFT")) return VK_LEFT;
    if (!_stricmp(pszKey, "RIGHT")) return VK_RIGHT;
    if (!_stricmp(pszKey, "RETURN") || !_stricmp(pszKey, "ENTER")) return VK_RETURN;
    if (!_stricmp(pszKey, "ESCAPE") || !_stricmp(pszKey, "ESC")) return VK_ESCAPE;
    if (!_stricmp(pszKey, "SPACE")) return VK_SPACE;
    if (!_stricmp(pszKey, "TAB")) return VK_TAB;
    if (pszKey[1] == 0)
    {
        char ch = (char)toupper((unsigned char)pszKey[0]);
        if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))
            return ch;
    }

    char *pEnd = NULL;
    long nValue = strtol(pszKey, &pEnd, 0);
    if (pEnd == pszKey || *pEnd || nValue <= 0 || nValue > 255)
        return 0;
    return (int)nValue;
}

static void FearHost_InitializeQueuedInputs()
{
    if (g_bQueuedInputsInitialized)
        return;
    g_bQueuedInputsInitialized = true;

    char szSequence[4096] = {0};
    if (GetEnvironmentVariableA("FEAR_QUEUE_INPUT", szSequence, sizeof(szSequence)) == 0)
        return;

    char *pContext = NULL;
    char *pToken = strtok_s(szSequence, ",;", &pContext);
    while (pToken && g_QueuedInputs.size() < 64)
    {
        char *pFieldContext = NULL;
        char *pszFrame = strtok_s(pToken, ":", &pFieldContext);
        char *pszKey = strtok_s(NULL, ":", &pFieldContext);
        char *pszHold = strtok_s(NULL, ":", &pFieldContext);
        if (pszFrame && pszKey)
        {
            char *pEnd = NULL;
            unsigned long nFrame = strtoul(pszFrame, &pEnd, 0);
            int nKey = FearParseVirtualKey(pszKey);
            int nHold = pszHold ? atoi(pszHold) : 2;
            if (pEnd != pszFrame && !*pEnd && nKey != 0)
            {
                if (nHold < 1) nHold = 1;
                if (nHold > 600) nHold = 600;
                FearQueuedInput input;
                input.m_nDownFrame = (unsigned int)nFrame;
                input.m_nUpFrame = (unsigned int)(nFrame + (unsigned long)nHold);
                input.m_nKey = nKey;
                input.m_bDownSent = false;
                input.m_bUpSent = false;
                g_QueuedInputs.push_back(input);
            }
        }
        pToken = strtok_s(NULL, ",;", &pContext);
    }

    printf("FEARPORT: queued input events=%u\n", (unsigned int)g_QueuedInputs.size());
    fflush(stdout);
}

static void FearHost_ProcessQueuedInputs()
{
    FearHost_InitializeQueuedInputs();
    const unsigned int nFrame = g_nQueuedInputFrame++;
    for (size_t i = 0; i < g_QueuedInputs.size(); ++i)
    {
        FearQueuedInput &input = g_QueuedInputs[i];
        if (!input.m_bDownSent && nFrame >= input.m_nDownFrame)
        {
            printf("FEARPORT: queued key down frame=%u vk=%d\n", nFrame, input.m_nKey);
            fflush(stdout);
            FearHost_OnKeyDown(input.m_nKey, 0);
            input.m_bDownSent = true;
        }
        if (input.m_bDownSent && !input.m_bUpSent && nFrame >= input.m_nUpFrame)
        {
            printf("FEARPORT: queued key up frame=%u vk=%d\n", nFrame, input.m_nKey);
            fflush(stdout);
            FearHost_OnKeyUp(input.m_nKey);
            input.m_bUpSent = true;
        }
    }
}

static void FearHost_UpdatePreviewCamera()
{
    if (g_pEngineApi && g_pEngineApi->m_pRenderApi && g_pEngineApi->m_pRenderApi->SetWorldCamera)
        g_pEngineApi->m_pRenderApi->SetWorldCamera(g_fPreviewPos, g_fPreviewYaw, g_fPreviewPitch);
}

static unsigned int g_nServerMsgCounts[256];
static unsigned int g_nClientMsgCounts[256];

static bool FearHost_ShouldLogMsg(unsigned int *pCounts, unsigned int nId)
{
    unsigned int nCount = ++pCounts[nId];
    if (nId == 231)
        return nCount <= 20 || (nCount % 600) == 0;
    return nCount == 1 || (nCount % 600) == 0;
}

extern "C" void FearHost_DeliverServerMessage(ILTMessage_Read *pMessage)
{
    if (!g_pShell || !pMessage)
        return;

    uint32 nSaved = pMessage->Tell();
    unsigned char nMessageID = (unsigned char)pMessage->ReadBits(8);
    pMessage->SeekTo(nSaved);

    bool bLogMsg = FearHost_ShouldLogMsg(g_nServerMsgCounts, nMessageID);
    unsigned char nMessageState = 0;
    if (bLogMsg)
        printf("FEARPORT: server message id=%u count=%u", (unsigned)nMessageID, g_nServerMsgCounts[nMessageID]);
    if (nMessageID == 187)
    {
        if (bLogMsg)
            printf(" save-data refused\n");
        fflush(stdout);
        if (g_pfnClientReplySaveFailed)
            g_pfnClientReplySaveFailed();
        return;
    }
    if (nMessageID == 229 || nMessageID == 233 || nMessageID == 230 ||
        nMessageID == 138)
    {
        if (bLogMsg)
        {
            printf(" skipped\n");
            fflush(stdout);
        }
        return;
    }
    pMessage->SeekTo(nSaved);
    pMessage->ReadBits(8);
    if (bLogMsg)
    {
        if (nMessageID == 160)
            printf(" pause=%u", (unsigned)pMessage->ReadBits(1));
        else if (nMessageID == 245 || nMessageID == 195 || nMessageID == 105)
        {
            nMessageState = (unsigned char)pMessage->ReadBits(8);
            printf(" state=%u", (unsigned)nMessageState);
        }
        printf("\n");
        fflush(stdout);
    }
    else if (nMessageID == 245 || nMessageID == 195 || nMessageID == 105)
    {
        nMessageState = (unsigned char)pMessage->ReadBits(8);
    }
    pMessage->SeekTo(nSaved);
    if (nMessageID == 245 && nMessageState == 12)
        g_bAwaitPostload = true;

    char szWorld[MAX_PATH] = {0};
    if (nMessageID == 186)
    {
        pMessage->SeekTo(nSaved);
        pMessage->ReadBits(8);
        pMessage->ReadString(szWorld, sizeof(szWorld));
        pMessage->SeekTo(nSaved);
    }

    unsigned short nPlayerObjectId = FearServerHost_GetPlayerObjectId();
    if (nPlayerObjectId != FEAR_INVALID_OBJECT_ID)
        FearClient_SetLocalServerObject(nPlayerObjectId);

    FearClient_BeginServerMessage();
    g_pShell->OnMessage(pMessage);
    FearClient_EndServerMessage();

    static bool s_bWorldLoadTriggered = false;
    if (!s_bWorldLoadTriggered && (nMessageID == 186 || nMessageID == 195) && g_pfnClientPreLoadWorld)
    {
        const char *pszWorld = szWorld[0] ? szWorld : "Worlds\\Release\\Intro.World00p";
        g_pfnClientPreLoadWorld(pszWorld);
        if (g_pfnClientOnEnterWorld)
            g_pfnClientOnEnterWorld();
        s_bWorldLoadTriggered = true;
    }
}

const FearEngineApi *FearHost_GetEngineApi(void)
{
    return g_pEngineApi;
}

extern "C" __declspec(dllexport) int FearHost_MakeScreenshot(const char *pszFilename)
{
    if (!g_pEngineApi || !g_pEngineApi->m_pRenderApi ||
        !g_pEngineApi->m_pRenderApi->MakeScreenShot ||
        !pszFilename || !pszFilename[0])
        return 0;

    return g_pEngineApi->m_pRenderApi->MakeScreenShot(pszFilename);
}

static void FearHost_OpenLogFile(void)
{
    char szPath[1024];
    DWORD nLen = GetEnvironmentVariableA("FEAR_HOST_LOG", szPath, (DWORD)sizeof(szPath));
    if (nLen == 0 || nLen >= (DWORD)sizeof(szPath))
        return;
    if (!freopen(szPath, "w", stdout))
        return;
    freopen(szPath, "a", stderr);
    setvbuf(stdout, NULL, _IONBF, 0);
}

extern "C" __declspec(dllexport) int FearHost_Init(const char *pszGameClientDll, const FearEngineApi *pApi)
{
    FearModel_SetKeySink(FearHost_DeliverModelKeyText);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    FearHost_OpenLogFile();
    g_pEngineApi = pApi;
    CInterfaceDatabase *pMaster = GetMasterDatabase();
    g_hGameClient = LoadLibraryA(pszGameClientDll);
    if (!g_hGameClient)
        return 0;
    typedef void (*TSetMaster)(CInterfaceDatabase*);
    TSetMaster pfnSetMaster = (TSetMaster)GetProcAddress(g_hGameClient, "SetMasterDatabase");
    if (pfnSetMaster)
        pfnSetMaster(pMaster);
    g_pfnClientMouseMove = (TFearClientMouseMove)GetProcAddress(g_hGameClient, "FearClient_MouseMove");
    g_pfnClientMouseButton = (TFearClientMouseButton)GetProcAddress(g_hGameClient, "FearClient_MouseButton");
    g_pfnClientMouseWheel = (TFearClientMouseWheel)GetProcAddress(g_hGameClient, "FearClient_MouseWheel");
    g_pfnClientDisableWindowHook = (TFearClientDisableWindowHook)GetProcAddress(g_hGameClient, "FearClient_DisableWindowHook");
    g_pfnClientPreLoadWorld = (TFearClientPreLoadWorld)GetProcAddress(g_hGameClient, "FearClient_PreLoadWorld");
    g_pfnClientOnEnterWorld = (TFearClientOnEnterWorld)GetProcAddress(g_hGameClient, "FearClient_OnEnterWorld");
    g_pfnClientLogState = (TFearClientLogState)GetProcAddress(g_hGameClient, "FearClient_LogState");
    g_pfnClientInstallDefaultBindings = (TFearClientInstallDefaultBindings)GetProcAddress(g_hGameClient, "FearClient_InstallDefaultBindings");
    g_pfnClientLogInputState = (TFearClientLogInputState)GetProcAddress(g_hGameClient, "FearClient_LogInputState");
    g_pfnClientApplyServerCamera = (TFearClientApplyServerCamera)GetProcAddress(g_hGameClient, "FearClient_ApplyServerCamera");
    g_pfnClientIsPostload = (TFearClientIsPostload)GetProcAddress(g_hGameClient, "FearClient_IsPostload");
    g_pfnClientPressKey = (TFearClientPressKey)GetProcAddress(g_hGameClient, "FearClient_PressKey");
    g_pfnClientReplySaveFailed = (TFearClientReplySaveFailed)GetProcAddress(g_hGameClient, "FearClient_ReplySaveFailed");
    g_pfnClientEnterWorldPreview = (TFearClientEnterWorldPreview)GetProcAddress(g_hGameClient, "FearClient_EnterWorldPreview");
    g_pfnClientRenderWorldPreview = (TFearClientRenderWorldPreview)GetProcAddress(g_hGameClient, "FearClient_RenderWorldPreview");
    g_pfnClientStartNewGame = (TFearClientStartNewGame)GetProcAddress(g_hGameClient, "FearClient_StartNewGame");
    g_pfnClientIsPlaying = (TFearClientIsPlaying)GetProcAddress(g_hGameClient, "FearClient_IsPlaying");
    g_pfnClientGetGameState = (TFearClientGetGameState)GetProcAddress(g_hGameClient, "FearClient_GetGameState");
    g_pfnClientCloseUserMenu = (TFearClientCloseUserMenu)GetProcAddress(g_hGameClient, "FearClient_CloseUserMenu");
    g_pfnClientPreparePortPlayer = (TFearClientPreparePortPlayer)GetProcAddress(g_hGameClient, "FearClient_PreparePortPlayer");
    g_pfnClientGetMoveObject = (TFearClientGetMoveObject)GetProcAddress(g_hGameClient, "FearClient_GetMoveObject");
    g_pfnClientIsPlayerMovementAllowed =
        (TFearClientIsPlayerMovementAllowed)GetProcAddress(
            g_hGameClient, "FearClient_IsPlayerMovementAllowed");
    g_pfnClientGetMovementState =
        (TFearClientGetMovementState)GetProcAddress(
            g_hGameClient, "FearClient_GetMovementState");
    g_pfnClientCreateLocalCharacter = (TFearClientCreateLocalCharacter)GetProcAddress(g_hGameClient, "FearClient_CreateLocalCharacter");
    g_pfnClientResetObjectFX = (TFearClientResetObjectFX)GetProcAddress(g_hGameClient, "FearClient_ResetObjectFX");
    g_pfnClientRenderPortCamera = (TFearClientRenderPortCamera)GetProcAddress(g_hGameClient, "FearClient_RenderPortCamera");
    FearServerHost_SetMessageSink(FearHost_DeliverServerMessage);
    char szRetailAbi[16] = {0};
    DWORD nRetailAbi = GetEnvironmentVariableA("FEAR_RETAIL_ABI", szRetailAbi, sizeof(szRetailAbi));
    char szToolsClient[16] = {0};
    DWORD nToolsClient = GetEnvironmentVariableA("FEAR_TOOLS_CLIENT", szToolsClient, sizeof(szToolsClient));
    char szLegacyListenerShim[8] = {0};
    DWORD nLegacyListenerShim = GetEnvironmentVariableA(
        "FEAR_LEGACY_LISTENER_SHIM", szLegacyListenerShim, sizeof(szLegacyListenerShim));
    if (nRetailAbi > 0 && szRetailAbi[0] != '0')
        FearPatchSoundVtable();
    else if (nToolsClient > 0 && szToolsClient[0] != '0' &&
        nLegacyListenerShim > 0 && szLegacyListenerShim[0] != '0')
        FearPatchSoundListenerOnly();
    return (g_pShell != NULL) ? 1 : 0;
}

extern "C" __declspec(dllexport) int FearHost_OnEngineInitialized(const FearMode *pMode, const unsigned char *pGuid)
{
    if (!g_pShell || !pMode)
        return 0;
    RMode mode;
    memset(&mode, 0, sizeof(mode));
    mode.m_Width = pMode->m_Width;
    mode.m_Height = pMode->m_Height;
    mode.m_BitDepth = pMode->m_BitDepth;
    mode.m_bHWTnL = (pMode->m_bHWTnL != 0);
    strncpy(mode.m_DeviceName, pMode->m_DeviceName, sizeof(mode.m_DeviceName) - 1);
    LTGUID guid;
    memset(&guid, 0, sizeof(guid));
    if (pGuid)
        memcpy(&guid, pGuid, sizeof(guid) < 16 ? sizeof(guid) : 16);
    int nResult = (int)g_pShell->OnEngineInitialized(&guid, &mode);
    if (g_pfnClientDisableWindowHook)
        g_pfnClientDisableWindowHook();
    return nResult;
}

extern "C" __declspec(dllexport) void FearHost_OnEvent(uint32 nEventId, uint32 nParam)
{
    if (!g_pShell)
        return;

    printf("FEARPORT: host event id=%u param=%u\n", (unsigned)nEventId, (unsigned)nParam);
    fflush(stdout);
    g_pShell->OnEvent(nEventId, nParam);
}

extern "C" __declspec(dllexport) void FearHost_Update(void)
{
    static LARGE_INTEGER s_nFrequency = {0};
    static LARGE_INTEGER s_nPreviousCounter = {0};
    LARGE_INTEGER nCounter = {0};
    QueryPerformanceFrequency(&s_nFrequency);
    QueryPerformanceCounter(&nCounter);
    if (s_nPreviousCounter.QuadPart != 0 && s_nFrequency.QuadPart > 0)
    {
        float fDelta = (float)((double)(nCounter.QuadPart - s_nPreviousCounter.QuadPart) /
            (double)s_nFrequency.QuadPart);
        if (fDelta < (1.0f / 1000.0f))
            fDelta = 1.0f / 1000.0f;
        if (fDelta > 0.1f)
            fDelta = 0.1f;
        FearHost_SetFrameDelta(fDelta);
    }
    s_nPreviousCounter = nCounter;

    if (FearServerHost_IsRunning())
        FearServerHost_Update();

    if (!g_pShell)
        return;

    FearHost_ProcessQueuedInputs();

    static bool s_bInitialObjectSyncComplete = false;
    static unsigned int s_nObjectSyncFrames = 0;
    if (g_pfnClientGetGameState && g_pfnClientGetGameState() != 0 &&
        (!s_bInitialObjectSyncComplete || (++s_nObjectSyncFrames % 30) == 0))
    {
        int nObjects = FearHost_SyncObjects();
        if (nObjects > 0 && !s_bInitialObjectSyncComplete)
        {
            s_bInitialObjectSyncComplete = true;
            printf("FEARPORT: initial object replication complete count=%d\n", nObjects);
            fflush(stdout);
        }
    }

    unsigned short nPlayerObjectId = FearServerHost_GetPlayerObjectId();
    if (nPlayerObjectId != FEAR_INVALID_OBJECT_ID)
    {
        float position[3] = {0.0f, 0.0f, 0.0f};
        float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        FearClient_SetLocalServerObject(nPlayerObjectId);
        if (FearServerHost_GetPlayerTransform(position, rotation))
        {
            // While the client owns the local player, its predicted position is
            // authoritative for rendering; the server transform must not snap it back.
            if (!FearClient_IsLocalPlayerScope())
                FearClient_UpdateServerObjectTransform(nPlayerObjectId, position, rotation);
        }
    }

    FearServerHost_DeliverMessages();

    g_pShell->PreUpdate();
    g_pShell->Update();
    g_pShell->PostUpdate();
    FearModel_TickAnimationKeys(FearHost_GetFrameDelta());

    if (g_pfnClientIsPlayerMovementAllowed &&
        g_pfnClientIsPlayerMovementAllowed() &&
        g_pfnClientGetMovementState)
    {
        float vPosition[3] = {0.0f, 0.0f, 0.0f};
        float vVelocity[3] = {0.0f, 0.0f, 0.0f};
        float vRotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        if (g_pfnClientGetMovementState(vPosition, vVelocity, vRotation))
        {
            static unsigned int s_nServerSyncs = 0;
            ++s_nServerSyncs;
            bool bSynced = FearServerHost_SetPlayerTransform(
                vPosition, vVelocity, vRotation);
            if (bSynced && (s_nServerSyncs <= 10 || (s_nServerSyncs % 300) == 0))
            {
                printf("FEARPORT: server player sync n=%u pos=%.2f %.2f %.2f "
                    "vel=%.2f %.2f %.2f\n",
                    s_nServerSyncs,
                    vPosition[0], vPosition[1], vPosition[2],
                    vVelocity[0], vVelocity[1], vVelocity[2]);
                fflush(stdout);
            }
        }
    }

    static int s_bAutoConfirm = -1;
    if (s_bAutoConfirm < 0)
    {
        char szAutoConfirm[8] = {0};
        s_bAutoConfirm = GetEnvironmentVariableA("FEAR_AUTO_CONFIRM", szAutoConfirm, sizeof(szAutoConfirm)) > 0 &&
            szAutoConfirm[0] && szAutoConfirm[0] != '0';
    }
    if (s_bAutoConfirm && g_pfnClientGetGameState && g_pfnClientCloseUserMenu &&
        g_pfnClientGetGameState() == 5)
    {
        g_pfnClientCloseUserMenu();
        printf("FEARPORT: auto-confirm closed user menu\n");
        fflush(stdout);
    }

    static bool s_bPortCameraReady = false;
    static unsigned short s_nPortPlayerId = FEAR_INVALID_OBJECT_ID;
    static unsigned short s_nPortCharacterId = FEAR_INVALID_OBJECT_ID;
    const bool bClientPlaying = g_pfnClientIsPlaying && g_pfnClientIsPlaying();
    unsigned short nObjectId = FearServerHost_GetPlayerObjectId();
    if (nObjectId == FEAR_INVALID_OBJECT_ID)
        nObjectId = 0;
    unsigned short nCharacterObjectId = FearServerHost_GetLocalCharacterObjectId();
    if (nCharacterObjectId == FEAR_INVALID_OBJECT_ID)
        nCharacterObjectId = nObjectId;
    if (nObjectId != s_nPortPlayerId || nCharacterObjectId != s_nPortCharacterId)
    {
        if (s_bPortCameraReady)
        {
            printf("FEARPORT: port player id changed %u:%u -> %u:%u\n",
                (unsigned)s_nPortPlayerId, (unsigned)s_nPortCharacterId,
                (unsigned)nObjectId, (unsigned)nCharacterObjectId);
            fflush(stdout);
        }
        s_nPortPlayerId = nObjectId;
        s_nPortCharacterId = nCharacterObjectId;
        s_bPortCameraReady = false;
    }
    if (!s_bPortCameraReady && bClientPlaying && g_bFearConnected && FearServerHost_GetPlayerObject() &&
        g_pfnClientPreparePortPlayer && g_pfnClientRenderPortCamera)
    {
        HOBJECT hTarget = g_pfnClientGetMoveObject ? (HOBJECT)g_pfnClientGetMoveObject() : NULL;
        if (!hTarget)
            hTarget = FearClient_GetClientObject();
        if (!hTarget)
            hTarget = FearClient_ResolveServerObject(nObjectId);
        HOBJECT hCharacterTarget = FearClient_ResolveServerObject(nCharacterObjectId);
        if (!hCharacterTarget)
            hCharacterTarget = hTarget;

        if (hTarget)
        {
            int nLocalPlayer = 0;
            if (g_pfnClientCreateLocalCharacter)
            {
                printf("FEARPORT: local character target id=%u object=%p mapped=%u\n",
                    (unsigned)nCharacterObjectId, hCharacterTarget,
                    (unsigned)FearClient_GetServerObjectId(hCharacterTarget));
                fflush(stdout);
                FearClient_SetLocalPlayerScope(true);
                nLocalPlayer = g_pfnClientCreateLocalCharacter(hCharacterTarget);
                FearClient_SetLocalPlayerScope(false);
            }

            if (g_pfnClientPreparePortPlayer(hTarget))
            {
                s_bPortCameraReady = true;
                printf("FEARPORT: port camera ready target=%p character=%p id=%u:%u localplayer=%d\n",
                    hTarget, hCharacterTarget, (unsigned)nObjectId,
                    (unsigned)nCharacterObjectId, nLocalPlayer);
                fflush(stdout);
            }
        }
    }

    if (s_bPortCameraReady && bClientPlaying && g_pfnClientRenderPortCamera)
    {
        FearHost_RefreshRenderObjects();
        FearHost_QueueRenderObjects();
        g_pfnClientRenderPortCamera();
    }

    static int s_nCameraSyncFrames = 0;
    if (bClientPlaying && (++s_nCameraSyncFrames % 2) == 0)
        FearHost_SyncCameras();

    if (g_bAwaitPostload && g_pfnClientIsPostload && g_pfnClientIsPostload() && g_pfnClientPressKey)
    {
        g_pfnClientPressKey(VK_SPACE);
        g_bAwaitPostload = false;
    }

    static int s_nMenuFrames = 0;
    static int s_nAutoStartAttempts = 0;
    static int s_bAutoStart = -1;
    static int s_bBindingsInstalled = -1;
    if (s_bAutoStart < 0)
    {
        char szAutoStart[8] = {0};
        DWORD nLen = GetEnvironmentVariableA("FEAR_AUTOSTART", szAutoStart, sizeof(szAutoStart));
        s_bAutoStart = (nLen > 0 && szAutoStart[0] != '0') ? 1 : 0;
    }
    ++s_nMenuFrames;
    if (s_bBindingsInstalled != 1 && g_pfnClientInstallDefaultBindings && (s_nMenuFrames % 30) == 0)
    {
        s_bBindingsInstalled = g_pfnClientInstallDefaultBindings();
        if (s_bBindingsInstalled == 1)
        {
            printf("FEARPORT: default command bindings installed\n");
            fflush(stdout);
        }
    }
    if (g_pfnClientLogState && (s_nMenuFrames % 60) == 0)
    {
        float vSrvPos[3] = {0.0f, 0.0f, 0.0f};
        float vSrvRot[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        int nHasSrv = FearServerHost_GetPlayerTransform(vSrvPos, vSrvRot) ? 1 : 0;
        printf("FEARPORT: serverframe=%d srvplayer=%d pos=%.2f %.2f %.2f inputqueries=%d\n",
            s_nMenuFrames, nHasSrv, vSrvPos[0], vSrvPos[1], vSrvPos[2],
            FearInput_GetQueryCounter() ? *FearInput_GetQueryCounter() : -1);
        fflush(stdout);
        g_pfnClientLogState();
        if (g_pfnClientLogInputState)
            g_pfnClientLogInputState();
    }
    if (s_bAutoStart && !g_bFearConnected && g_pfnClientStartNewGame &&
        s_nMenuFrames > 120 && s_nAutoStartAttempts < 1 && (s_nMenuFrames % 60) == 0)
    {
        ++s_nAutoStartAttempts;
        int nStarted = g_pfnClientStartNewGame();
        printf("FEARPORT: auto-start new game attempt=%d result=%d\n", s_nAutoStartAttempts, nStarted);
        fflush(stdout);
    }

    static int s_nAutoWalkDown = -1;
    static int s_nAutoWalkUp = -1;
    static int s_bAutoWalkPressed = 0;
    static int s_nAutoWalkKey = 0x57;
    if (s_nAutoWalkDown < 0)
    {
        char szWalk[64] = {0};
        if (GetEnvironmentVariableA("FEAR_AUTOWALK", szWalk, sizeof(szWalk)) > 0)
        {
            int nDown = 0, nUp = 0, nKey = 0x57;
            int nFields = sscanf(szWalk, "%d,%d,%d", &nDown, &nUp, &nKey);
            if (nFields >= 2)
            {
                s_nAutoWalkDown = nDown;
                s_nAutoWalkUp = nUp;
                s_nAutoWalkKey = nKey;
                printf("FEARPORT: autowalk key=%d down=%d up=%d\n", nKey, nDown, nUp);
            }
        }
        if (s_nAutoWalkDown < 0)
            s_nAutoWalkDown = 0x7FFFFFFF;
    }

    if (!s_bAutoWalkPressed && s_nMenuFrames >= s_nAutoWalkDown && !g_bWorldPreview)
    {
        s_bAutoWalkPressed = 1;
        FearHost_OnKeyDown(s_nAutoWalkKey, 0);
        printf("FEARPORT: autowalk key down at frame %d\n", s_nMenuFrames);
        fflush(stdout);
    }
    else if (s_bAutoWalkPressed == 1 && s_nMenuFrames >= s_nAutoWalkUp && !g_bWorldPreview)
    {
        s_bAutoWalkPressed = 2;
        FearHost_OnKeyUp(s_nAutoWalkKey);
        printf("FEARPORT: autowalk key up at frame %d\n", s_nMenuFrames);
        fflush(stdout);
    }
}

extern "C" __declspec(dllexport) void FearHost_OnKeyDown(int key, int rep)
{
    FearInput_SetKeyState(key, true);
    if (key == 'W' || key == 'A' || key == 'S' || key == 'D' || key == VK_SPACE)
    {
        printf("FEARPORT: keydown vk=%d rep=%d\n", key, rep);
        fflush(stdout);
    }
    if (g_bWorldPreview)
    {
        if (key >= 0 && key < 256)
            g_bPreviewKeys[key] = true;
        return;
    }
    if (g_pShell)
        g_pShell->OnKeyDown(key, rep);
}

extern "C" __declspec(dllexport) void FearHost_OnKeyUp(int key)
{
    FearInput_SetKeyState(key, false);
    if (g_bWorldPreview)
    {
        if (key >= 0 && key < 256)
            g_bPreviewKeys[key] = false;
        return;
    }
    if (g_pShell)
        g_pShell->OnKeyUp(key);
}

extern "C" __declspec(dllexport) void FearHost_OnMouseMove(int x, int y)
{
    FearInput_SetMousePos(x, y);
    if (g_bWorldPreview)
    {
        if (!g_bPreviewMouseInitialized)
        {
            g_bPreviewMouseInitialized = true;
            g_nPreviewLastMouseX = x;
            g_nPreviewLastMouseY = y;
            return;
        }
        int dx = x - g_nPreviewLastMouseX;
        int dy = y - g_nPreviewLastMouseY;
        g_nPreviewLastMouseX = x;
        g_nPreviewLastMouseY = y;
        g_fPreviewYaw += (float)dx * 0.0025f;
        g_fPreviewPitch -= (float)dy * 0.0025f;
        if (g_fPreviewPitch > 1.35f)
            g_fPreviewPitch = 1.35f;
        if (g_fPreviewPitch < -1.35f)
            g_fPreviewPitch = -1.35f;
        FearHost_UpdatePreviewCamera();
        return;
    }
    if (g_pfnClientMouseMove)
        g_pfnClientMouseMove(x, y);
}

extern "C" __declspec(dllexport) void FearHost_OnMouseButton(int nButton, int nDown, int x, int y)
{
    FearInput_SetMouseButton(nButton, nDown != 0);
    FearInput_SetMousePos(x, y);
    if (g_bWorldPreview)
        return;
    if (g_pfnClientMouseButton)
        g_pfnClientMouseButton(nButton, nDown, x, y);
}

extern "C" __declspec(dllexport) void FearHost_OnModelKey(void *hObj, uint32 hTrackerID, void *pArgs)
{
    static int s_nModelKeyLogs = 0;
    if (s_nModelKeyLogs < 32)
    {
        ++s_nModelKeyLogs;
        printf("FEARPORT: OnModelKey obj=%p tracker=%u args=%p\n",
            hObj, (unsigned)hTrackerID, pArgs);
        fflush(stdout);
    }
    if (g_pShell)
        g_pShell->OnModelKey((HLOCALOBJ)hObj, hTrackerID, (ArgList *)pArgs);
}

extern "C" __declspec(dllexport) void FearHost_OnMouseWheel(int x, int y, int nDelta)
{
    if (g_pfnClientMouseWheel)
        g_pfnClientMouseWheel(x, y, nDelta);
}

extern "C" __declspec(dllexport) void FearHost_OnEngineTerm(void)
{
    printf("FEARPORT: OnEngineTerm\n");
    fflush(stdout);
    FearServerHost_Shutdown();
    if (g_pShell)
        g_pShell->OnEngineTerm();
}

extern "C" __declspec(dllexport) void FearHost_Shutdown(void)
{
    FearServerHost_Shutdown();
    if (g_hGameClient)
    {
        FreeLibrary(g_hGameClient);
        g_hGameClient = NULL;
    }
}
