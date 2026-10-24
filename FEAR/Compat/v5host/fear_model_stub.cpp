#include "fear_model_stub.h"
#include "fear_host_api.h"
#include "fear_object_map.h"
#include "fear_server_host.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
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
    HMODELANIM m_TrackerAnimation[256];
    uint32 m_TrackerTime[256];
    uint32 m_TrackerLastTime[256];
    uint32 m_TrackerNextKey[256];
    uint32 m_TrackerLength[256];
    float m_TrackerRate[256];
    bool m_TrackerLooping[256];
    bool m_TrackerPlaying[256];
};

struct FearModelNode
{
    std::string m_Name;
    float m_Position[3];
    float m_Rotation[4];
};

struct FearModelSocket
{
    std::string m_Name;
    HMODELNODE m_ParentNode;
    float m_Position[3];
    float m_Rotation[4];
};

struct FearModelObject
{
    std::string m_Filename;
    std::vector<FearModelNode> m_Nodes;
    std::vector<FearModelSocket> m_Sockets;
    std::vector<std::string> m_AnimationNames;
};

struct FearModelKeyFrame
{
    uint32 m_Time;
    std::string m_Key;
};

struct FearModelAnimationKeys
{
    bool m_bLoaded;
    std::vector<FearModelKeyFrame> m_Frames;
};

std::map<HOBJECT, FearModelWeights> g_ModelWeights;
std::map<HOBJECT, FearModelObject> g_ModelObjects;
std::map<HOBJECT, std::map<uint32, FearModelAnimationKeys> > g_ModelAnimationKeys;
std::vector<std::string> g_WeightSetNames;
TFearModelKeySink g_pfnModelKeySink = NULL;

unsigned int FearReadU32(const unsigned char *pData, size_t nOffset)
{
    return (unsigned int)pData[nOffset] |
        ((unsigned int)pData[nOffset + 1] << 8) |
        ((unsigned int)pData[nOffset + 2] << 16) |
        ((unsigned int)pData[nOffset + 3] << 24);
}

float FearReadF32(const unsigned char *pData, size_t nOffset)
{
    unsigned int nBits = FearReadU32(pData, nOffset);
    float fValue;
    memcpy(&fValue, &nBits, sizeof(fValue));
    return fValue;
}

bool FearReadModelFile(const char *pszFilename, std::vector<unsigned char> &data)
{
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (!pApi || !pApi->FileOpen || !pszFilename || !pszFilename[0])
        return false;

    void *hFile = pApi->FileOpen(pszFilename);
    if (!hFile)
        return false;

    unsigned long long nSize = pApi->FileSize(hFile);
    if (nSize < 0x60 || nSize > 64ull * 1024ull * 1024ull)
    {
        pApi->FileClose(hFile);
        return false;
    }

    data.resize((size_t)nSize);
    bool bRead = pApi->FileRead(hFile, data.data(), (unsigned int)nSize) != 0;
    pApi->FileClose(hFile);
    if (!bRead)
        data.clear();
    return bRead;
}

bool FearQuaternionIsValid(const float *pRotation)
{
    float fLengthSqr = 0.0f;
    for (int i = 0; i < 4; ++i)
        fLengthSqr += pRotation[i] * pRotation[i];
    return fLengthSqr > 0.95f && fLengthSqr < 1.05f;
}

bool FearModelParseNodes(const std::vector<unsigned char> &data,
    std::vector<FearModelNode> &nodes, std::vector<FearModelSocket> &sockets,
    std::vector<std::string> &animationNames)
{
    if (data.size() < 0x60 || memcmp(data.data(), "MODL", 4) != 0)
        return false;

    unsigned int nNodeCount = FearReadU32(data.data(), 0x10);
    unsigned int nNamePoolLength = FearReadU32(data.data(), 0x2C);
    size_t nPoolStart = 0x5C;
    size_t nPoolEnd = nPoolStart + nNamePoolLength;
    size_t nNodesEnd = nPoolEnd + (size_t)nNodeCount * 39;
    if (nNodeCount == 0 || nNodeCount > 4096 ||
        nPoolEnd > data.size() || nNodesEnd > data.size())
        return false;

    std::map<unsigned int, std::string> names;
    for (size_t nOffset = nPoolStart; nOffset < nPoolEnd;)
    {
        size_t nEnd = nOffset;
        while (nEnd < nPoolEnd && data[nEnd] != 0)
            ++nEnd;
        if (nEnd > nOffset)
            names[(unsigned int)(nOffset - nPoolStart)] =
                std::string((const char *)data.data() + nOffset, nEnd - nOffset);
        nOffset = nEnd + 1;
        while (nOffset < nPoolEnd && (nOffset & 3))
            ++nOffset;
    }

    nodes.clear();
    sockets.clear();
    animationNames.clear();
    nodes.reserve(nNodeCount);
    for (unsigned int i = 0; i < nNodeCount; ++i)
    {
        const unsigned char *pNode = data.data() + nPoolEnd + (size_t)i * 39;
        unsigned int nNameOffset = FearReadU32(pNode, 0);
        FearModelNode node;
        std::map<unsigned int, std::string>::iterator it = names.find(nNameOffset);
        node.m_Name = it != names.end() ? it->second : std::string();
        for (int k = 0; k < 3; ++k)
            node.m_Position[k] = FearReadF32(pNode, 7 + k * 4);
        for (int k = 0; k < 4; ++k)
            node.m_Rotation[k] = FearReadF32(pNode, 19 + k * 4);
        nodes.push_back(node);
    }

    const size_t nSocketScanEnd = LTMIN(data.size(), nNodesEnd + 1024u * 1024u);
    for (size_t nOffset = nNodesEnd; nOffset + 40 <= nSocketScanEnd; ++nOffset)
    {
        unsigned int nParent = FearReadU32(data.data(), nOffset);
        unsigned int nNameOffset = FearReadU32(data.data(), nOffset + 4);
        if (nParent >= nNodeCount)
            continue;

        std::map<unsigned int, std::string>::iterator it = names.find(nNameOffset);
        if (it == names.end() || it->second.empty())
            continue;

        FearModelSocket socket;
        socket.m_Name = it->second;
        socket.m_ParentNode = (HMODELNODE)nParent;
        for (int k = 0; k < 4; ++k)
            socket.m_Rotation[k] = FearReadF32(data.data(), nOffset + 8 + k * 4);
        for (int k = 0; k < 3; ++k)
            socket.m_Position[k] = FearReadF32(data.data(), nOffset + 24 + k * 4);
        float fScale = FearReadF32(data.data(), nOffset + 36);
        if (!FearQuaternionIsValid(socket.m_Rotation) || fScale < 0.5f || fScale > 2.0f)
            continue;

        sockets.push_back(socket);
        nOffset += 39;
    }

    for (std::map<unsigned int, std::string>::iterator it = names.begin();
         it != names.end(); ++it)
    {
        bool bNodeName = false;
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (it->second == nodes[i].m_Name)
            {
                bNodeName = true;
                break;
            }
        }
        if (!bNodeName && !it->second.empty())
            animationNames.push_back(it->second);
    }
    return true;
}

std::string FearModelWithPackedExtension(const char *pszFilename)
{
    if (!pszFilename || !pszFilename[0])
        return std::string();

    std::string filename(pszFilename);
    size_t nDot = filename.find_last_of('.');
    size_t nSlash = filename.find_last_of("\\/");
    if (nDot != std::string::npos && (nSlash == std::string::npos || nDot > nSlash))
        filename.resize(nDot);
    filename += ".Model00p";
    return filename;
}

void FearModelLoadObject(FearModelObject &object)
{
    object.m_Nodes.clear();
    object.m_Sockets.clear();
    object.m_AnimationNames.clear();
    std::vector<unsigned char> data;
    if (!FearReadModelFile(object.m_Filename.c_str(), data))
    {
        const std::string packed = FearModelWithPackedExtension(object.m_Filename.c_str());
        if (packed != object.m_Filename)
            FearReadModelFile(packed.c_str(), data);
    }

    if (!data.empty() && FearModelParseNodes(
        data, object.m_Nodes, object.m_Sockets, object.m_AnimationNames))
    {
        printf("FEARMODEL: loaded object model='%s' nodes=%u sockets=%u animations=%u\n",
            object.m_Filename.c_str(), (unsigned)object.m_Nodes.size(),
            (unsigned)object.m_Sockets.size(), (unsigned)object.m_AnimationNames.size());
    }
    else
    {
        printf("FEARMODEL: node parse failed model='%s'\n", object.m_Filename.c_str());
    }
    fflush(stdout);
}

const FearModelNode *FearFindNode(const FearModelObject &object, HMODELNODE hNode)
{
    return hNode < object.m_Nodes.size() ? &object.m_Nodes[hNode] : NULL;
}

std::string FearNormalizeName(const char *pszName)
{
    std::string normalized;
    for (const char *p = pszName; p && *p; ++p)
    {
        unsigned char c = (unsigned char)*p;
        if (isalnum(c))
            normalized.push_back((char)tolower(c));
    }
    return normalized;
}

bool FearNamesMatch(const std::string &name, const char *pszName)
{
    if (stricmp(name.c_str(), pszName) == 0)
        return true;
    return FearNormalizeName(name.c_str()) == FearNormalizeName(pszName);
}

HMODELANIM FearFindOrAddAnimation(FearModelObject &object, const char *pszName)
{
    const std::string normalized = FearNormalizeName(pszName);
    if (normalized.empty())
        return INVALID_MODEL_ANIM;

    for (size_t i = 0; i < object.m_AnimationNames.size(); ++i)
    {
        if (FearNormalizeName(object.m_AnimationNames[i].c_str()) == normalized)
            return (HMODELANIM)i;
    }

    object.m_AnimationNames.push_back(pszName);
    static int s_nSyntheticAnimations = 0;
    if (s_nSyntheticAnimations < 64)
    {
        ++s_nSyntheticAnimations;
        printf("FEARMODEL: synthetic animation '%s' model='%s' index=%u\n",
            pszName, object.m_Filename.c_str(),
            (unsigned)(object.m_AnimationNames.size() - 1));
        fflush(stdout);
    }
    return (HMODELANIM)(object.m_AnimationNames.size() - 1);
}

void FearFillTransformFromParts(const float *pPosition, const float *pRotation,
    HOBJECT hObject, LTTransform &transform, bool bWorldSpace)
{
    LTVector position(pPosition[0], pPosition[1], pPosition[2]);
    LTRotation rotation;
    rotation.m_Quat[0] = pRotation[0];
    rotation.m_Quat[1] = pRotation[1];
    rotation.m_Quat[2] = pRotation[2];
    rotation.m_Quat[3] = pRotation[3];
    transform.Init(position, rotation, 1.0f);

    if (!bWorldSpace)
        return;

    float objectPosition[3] = {0.0f, 0.0f, 0.0f};
    float objectRotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    if (!FearClient_GetObjectTransform(hObject, objectPosition, objectRotation))
        return;

    LTRotation objectQuat;
    objectQuat.m_Quat[0] = objectRotation[0];
    objectQuat.m_Quat[1] = objectRotation[1];
    objectQuat.m_Quat[2] = objectRotation[2];
    objectQuat.m_Quat[3] = objectRotation[3];
    LTRigidTransform objectTransform(
        LTVector(objectPosition[0], objectPosition[1], objectPosition[2]), objectQuat);
    transform = objectTransform * transform;
}

void FearFillNodeTransform(HOBJECT hObject, const FearModelNode &node,
    LTTransform &transform, bool bWorldSpace)
{
    FearFillTransformFromParts(node.m_Position, node.m_Rotation,
        hObject, transform, bWorldSpace);
}

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
            state.m_TrackerAnimation[i] = INVALID_MODEL_ANIM;
            state.m_TrackerTime[i] = 0;
            state.m_TrackerLastTime[i] = 0;
            state.m_TrackerNextKey[i] = 0;
            state.m_TrackerLength[i] = 1000;
            state.m_TrackerRate[i] = 1.0f;
            state.m_TrackerLooping[i] = false;
            state.m_TrackerPlaying[i] = false;
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

void FearModel_SetObjectFilename(HOBJECT hObject, const char *pszFilename)
{
    if (!hObject)
        return;

    FearModelObject &object = g_ModelObjects[hObject];
    const std::string filename = pszFilename ? pszFilename : "";
    if (object.m_Filename == filename)
        return;

    object.m_Filename = filename;
    object.m_Nodes.clear();
    if (!filename.empty())
        FearModelLoadObject(object);
}

const char *FearModel_GetObjectFilename(HOBJECT hObject)
{
    std::map<HOBJECT, FearModelObject>::iterator it = g_ModelObjects.find(hObject);
    return it != g_ModelObjects.end() ? it->second.m_Filename.c_str() : "";
}

void FearModel_RemoveObject(HOBJECT hObject)
{
    g_ModelObjects.erase(hObject);
    g_ModelWeights.erase(hObject);
    g_ModelAnimationKeys.erase(hObject);
}

static FearModelAnimationKeys &FearModelAnimationKeysFor(
    HOBJECT hObject, HMODELANIM hAnimation)
{
    FearModelAnimationKeys &animation =
        g_ModelAnimationKeys[hObject][(uint32)hAnimation];
    if (animation.m_bLoaded)
        return animation;

    uint32 nKeyFrames = 0;
    if (!FearServerHost_GetAnimKeyFrames(
        hObject, (uint32)hAnimation, &nKeyFrames))
        return animation;

    animation.m_Frames.reserve(nKeyFrames);
    for (uint32 i = 0; i < nKeyFrames; ++i)
    {
        uint32 nTime = 0;
        char szKey[1024] = {0};
        if (!FearServerHost_GetAnimKeyFrame(
            hObject, (uint32)hAnimation, i, &nTime, szKey, sizeof(szKey)))
            continue;
        if (!szKey[0])
            continue;

        FearModelKeyFrame frame;
        frame.m_Time = nTime;
        frame.m_Key = szKey;
        animation.m_Frames.push_back(frame);
    }
    static int s_nAnimationKeyLogs = 0;
    if (s_nAnimationKeyLogs < 128 || (uint32)hAnimation == 261)
    {
        ++s_nAnimationKeyLogs;
        printf("FEARMODEL: keyframes obj=%p anim=%u native=%u keys=%u\n",
            (void *)hObject, (unsigned)hAnimation, (unsigned)nKeyFrames,
            (unsigned)animation.m_Frames.size());
        fflush(stdout);
    }
    animation.m_bLoaded = true;
    return animation;
}

void FearModel_SetKeySink(TFearModelKeySink pSink)
{
    g_pfnModelKeySink = pSink;
}

void FearModel_TickAnimationKeys(float fDelta)
{
    if (!g_pfnModelKeySink || fDelta <= 0.0f)
        return;

    struct FearModelKeyEvent
    {
        HOBJECT m_hObject;
        uint32 m_nTracker;
        std::string m_Key;
    };
    std::vector<FearModelKeyEvent> events;

    for (std::map<HOBJECT, FearModelWeights>::iterator it = g_ModelWeights.begin();
         it != g_ModelWeights.end(); ++it)
    {
        HOBJECT hObject = it->first;
        FearModelWeights &state = it->second;
        for (uint32 nTracker = 0; nTracker < 256; ++nTracker)
        {
            if (!state.m_Trackers[nTracker] || !state.m_TrackerPlaying[nTracker] ||
                state.m_TrackerAnimation[nTracker] == INVALID_MODEL_ANIM)
                continue;

            const uint32 nLength =
                state.m_TrackerLength[nTracker] ? state.m_TrackerLength[nTracker] : 1;
            const uint32 nLastTime = state.m_TrackerLastTime[nTracker];
            float fTime = (float)nLastTime +
                fDelta * 1000.0f * state.m_TrackerRate[nTracker];
            bool bLooped = false;
            if (fTime >= (float)nLength)
            {
                if (state.m_TrackerLooping[nTracker])
                {
                    fTime = fmodf(fTime, (float)nLength);
                    bLooped = true;
                }
                else
                {
                    fTime = (float)nLength;
                    state.m_TrackerPlaying[nTracker] = false;
                }
            }
            const uint32 nNewTime = (uint32)fTime;

            FearModelAnimationKeys &animation = FearModelAnimationKeysFor(
                hObject, state.m_TrackerAnimation[nTracker]);
            uint32 nNextKey = state.m_TrackerNextKey[nTracker];
            while (nNextKey < animation.m_Frames.size() &&
                   animation.m_Frames[nNextKey].m_Time < nLastTime)
                ++nNextKey;

            for (; nNextKey < animation.m_Frames.size(); ++nNextKey)
            {
                const FearModelKeyFrame &frame = animation.m_Frames[nNextKey];
                if (frame.m_Time > nNewTime)
                    break;

                FearModelKeyEvent event;
                event.m_hObject = hObject;
                event.m_nTracker = nTracker;
                event.m_Key = frame.m_Key;
                events.push_back(event);
            static int s_nPistolFireKeyLogs = 0;
            if ((uint32)state.m_TrackerAnimation[nTracker] == 261 &&
                s_nPistolFireKeyLogs < 16)
            {
                ++s_nPistolFireKeyLogs;
                printf("FEARMODEL: queued anim261 key obj=%p tracker=%u time=%u key='%s'\n",
                    (void *)hObject, (unsigned)nTracker,
                    (unsigned)frame.m_Time, frame.m_Key.c_str());
                fflush(stdout);
            }
            }

            if (bLooped)
            {
                for (nNextKey = 0;
                     nNextKey < animation.m_Frames.size() &&
                     animation.m_Frames[nNextKey].m_Time <= nNewTime;
                     ++nNextKey)
                {
                    FearModelKeyEvent event;
                    event.m_hObject = hObject;
                    event.m_nTracker = nTracker;
                    event.m_Key = animation.m_Frames[nNextKey].m_Key;
                    events.push_back(event);
                }
            }

            state.m_TrackerNextKey[nTracker] = nNextKey;
            state.m_TrackerLastTime[nTracker] = nNewTime;
            state.m_TrackerTime[nTracker] = nNewTime;
        }
    }

    for (size_t i = 0; i < events.size(); ++i)
    {
        g_pfnModelKeySink(
            events[i].m_hObject, events[i].m_nTracker, events[i].m_Key.c_str());
    }
}

LTRESULT CFearModel::GetSocket(HOBJECT hObj, const char *pSocketName, HMODELSOCKET &hSocket)
{
    if (!hObj || !pSocketName)
        return LT_INVALIDPARAMS;

    unsigned int nServerSocket = 0;
    if (FearServerHost_GetSocket(hObj, pSocketName, &nServerSocket))
    {
        hSocket = (HMODELSOCKET)nServerSocket;
        return LT_OK;
    }

    std::map<HOBJECT, FearModelObject>::iterator it = g_ModelObjects.find(hObj);
    if (it == g_ModelObjects.end())
        return LT_NOTINITIALIZED;

    for (size_t i = 0; i < it->second.m_Sockets.size(); ++i)
    {
        if (FearNamesMatch(it->second.m_Sockets[i].m_Name, pSocketName))
        {
            hSocket = (HMODELSOCKET)i;
            printf("FEARMODEL: GetSocket obj=%p name='%s' socket=%u parent=%u model='%s'\n",
                (void *)hObj, pSocketName, (unsigned)hSocket,
                (unsigned)it->second.m_Sockets[i].m_ParentNode,
                it->second.m_Filename.c_str());
            fflush(stdout);
            return LT_OK;
        }
    }
    return LT_NOTFOUND;
}

LTRESULT CFearModel::GetSocketTransform(HOBJECT hObj, HMODELSOCKET hSocket, LTTransform &transform, bool bWorldSpace)
{
    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    if (FearServerHost_GetSocketTransform(
        hObj, (unsigned int)hSocket, position, rotation, bWorldSpace))
    {
        FearFillTransformFromParts(position, rotation, hObj, transform, false);
        return LT_OK;
    }

    std::map<HOBJECT, FearModelObject>::iterator it = g_ModelObjects.find(hObj);
    if (it == g_ModelObjects.end())
        return LT_NOTINITIALIZED;
    if (hSocket >= it->second.m_Sockets.size())
        return LT_NOTFOUND;

    const FearModelSocket &socket = it->second.m_Sockets[hSocket];
    FearFillTransformFromParts(socket.m_Position, socket.m_Rotation,
        hObj, transform, bWorldSpace);
    return LT_OK;
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
    static int s_nLogs = 0;
    if (s_nLogs++ < 4)
        printf("STUB ILTModel::SetLODDistanceBias\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearModel::GetNode(HOBJECT hObj, const char *pNodeName, HMODELNODE &hNode)
{
    if (!hObj || !pNodeName)
        return LT_INVALIDPARAMS;

    unsigned int nServerNode = 0;
    if (FearServerHost_GetNode(hObj, pNodeName, &nServerNode))
    {
        hNode = (HMODELNODE)nServerNode;
        return LT_OK;
    }

    std::map<HOBJECT, FearModelObject>::iterator it = g_ModelObjects.find(hObj);
    if (it == g_ModelObjects.end() || it->second.m_Nodes.empty())
        return LT_NOTINITIALIZED;

    for (size_t i = 0; i < it->second.m_Nodes.size(); ++i)
    {
        if (FearNamesMatch(it->second.m_Nodes[i].m_Name, pNodeName))
        {
            hNode = (HMODELNODE)i;
            static int s_nLoggedNodes = 0;
            if (s_nLoggedNodes < 32)
            {
                ++s_nLoggedNodes;
                printf("FEARMODEL: GetNode obj=%p name='%s' node=%u model='%s'\n",
                    (void *)hObj, pNodeName, (unsigned)hNode, it->second.m_Filename.c_str());
                fflush(stdout);
            }
            return LT_OK;
        }
    }
    return LT_NOTFOUND;
}

LTRESULT CFearModel::GetNodeName(HOBJECT hObj, HMODELNODE hNode, char *name, uint32 maxlen)
{
    if (!hObj || !name || maxlen == 0)
        return LT_INVALIDPARAMS;

    std::map<HOBJECT, FearModelObject>::iterator it = g_ModelObjects.find(hObj);
    const FearModelNode *pNode =
        it != g_ModelObjects.end() ? FearFindNode(it->second, hNode) : NULL;
    if (!pNode)
        return LT_NOTFOUND;

    strncpy(name, pNode->m_Name.c_str(), maxlen - 1);
    name[maxlen - 1] = '\0';
    return LT_OK;
}

LTRESULT CFearModel::GetNodeTransform(HOBJECT hObj, HMODELNODE hNode, LTTransform &transform, bool bWorldSpace)
{
    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    if (FearServerHost_GetNodeTransform(
        hObj, (unsigned int)hNode, position, rotation, bWorldSpace))
    {
        FearFillTransformFromParts(position, rotation, hObj, transform, false);
        return LT_OK;
    }

    std::map<HOBJECT, FearModelObject>::iterator it = g_ModelObjects.find(hObj);
    const FearModelNode *pNode =
        it != g_ModelObjects.end() ? FearFindNode(it->second, hNode) : NULL;
    if (!pNode)
        return LT_NOTFOUND;

    FearFillNodeTransform(hObj, *pNode, transform, bWorldSpace);
    return LT_OK;
}

LTRESULT CFearModel::GetNextNode(HOBJECT hObject, HMODELNODE hNode, HMODELNODE &pNext)
{
    std::map<HOBJECT, FearModelObject>::iterator it = g_ModelObjects.find(hObject);
    if (it == g_ModelObjects.end() || it->second.m_Nodes.empty())
        return LT_NOTINITIALIZED;

    HMODELNODE nIndex = hNode == INVALID_MODEL_NODE ? 0 : hNode + 1;
    if (nIndex >= it->second.m_Nodes.size())
        return LT_FINISHED;
    pNext = nIndex;
    return LT_OK;
}

LTRESULT CFearModel::GetRootNode(HOBJECT hObj, HMODELNODE &hNode)
{
    std::map<HOBJECT, FearModelObject>::iterator it = g_ModelObjects.find(hObj);
    if (it == g_ModelObjects.end() || it->second.m_Nodes.empty())
        return LT_NOTINITIALIZED;
    hNode = 0;
    return LT_OK;
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
    unsigned int nServerNodes = 0;
    if (FearServerHost_GetNumNodes(hObj, &nServerNodes))
    {
        num_nodes = nServerNodes;
        return LT_OK;
    }

    std::map<HOBJECT, FearModelObject>::iterator it = g_ModelObjects.find(hObj);
    if (it == g_ModelObjects.end())
        return LT_NOTINITIALIZED;
    num_nodes = (uint32)it->second.m_Nodes.size();
    return LT_OK;
}

LTRESULT CFearModel::GetBindPoseNodeTransform(HOBJECT hObj, HMODELNODE node, LTRigidTransform &tBindSpace)
{
    std::map<HOBJECT, FearModelObject>::iterator it = g_ModelObjects.find(hObj);
    const FearModelNode *pNode =
        it != g_ModelObjects.end() ? FearFindNode(it->second, node) : NULL;
    if (!pNode)
        return LT_NOTFOUND;

    tBindSpace.m_vPos.Init(pNode->m_Position[0], pNode->m_Position[1], pNode->m_Position[2]);
    for (int i = 0; i < 4; ++i)
        tBindSpace.m_rRot.m_Quat[i] = pNode->m_Rotation[i];
    return LT_OK;
}

LTRESULT CFearModel::AddNodeControlFn(HOBJECT hObj, HMODELNODE hNode, NodeControlFn pFn, void *pUserData)
{
    return hObj && hNode != INVALID_MODEL_NODE ? LT_OK : LT_INVALIDPARAMS;
}

LTRESULT CFearModel::AddNodeControlFn(HOBJECT hObj, NodeControlFn pFn, void *pUserData)
{
    return hObj ? LT_OK : LT_INVALIDPARAMS;
}

LTRESULT CFearModel::RemoveNodeControlFn(HOBJECT hObj, HMODELNODE hNode, NodeControlFn pFn, void* pUserData)
{
    return hObj && hNode != INVALID_MODEL_NODE ? LT_OK : LT_INVALIDPARAMS;
}

LTRESULT CFearModel::RemoveNodeControlFn(HOBJECT hObj, NodeControlFn pFn, void* pUserData)
{
    return hObj ? LT_OK : LT_INVALIDPARAMS;
}

LTRESULT CFearModel::UpdateMainTracker(HOBJECT hObj, float fUpdateDelta)
{
    if (!hObj)
        return LT_INVALIDPARAMS;

    FearModelWeights &weights = FearModelWeightsFor(hObj);
    for (int i = 0; i < 256; ++i)
    {
        if (!weights.m_Trackers[i] || !weights.m_TrackerPlaying[i] ||
            weights.m_TrackerAnimation[i] == INVALID_MODEL_ANIM)
            continue;

        float fTime = (float)weights.m_TrackerTime[i] +
            fUpdateDelta * 1000.0f * weights.m_TrackerRate[i];
        uint32 nLength = weights.m_TrackerLength[i] ? weights.m_TrackerLength[i] : 1;
        if (fTime >= (float)nLength)
        {
            if (weights.m_TrackerLooping[i])
            {
                fTime = fmodf(fTime, (float)nLength);
            }
            else
            {
                fTime = (float)nLength;
                weights.m_TrackerPlaying[i] = false;
            }
        }
        weights.m_TrackerTime[i] = (uint32)fTime;
    }
    return LT_OK;
}

LTRESULT CFearModel::GetModelAnimUserDims(HOBJECT hModel, HMODELANIM hAnim, LTVector *pDims)
{
    if (!hModel || !pDims)
        return LT_INVALIDPARAMS;

    float dims[3] = {0.0f, 0.0f, 0.0f};
    if (FearServerHost_GetModelAnimUserDims(
        hModel, (unsigned int)hAnim, dims))
    {
        pDims->Init(dims[0], dims[1], dims[2]);
        return LT_OK;
    }

    pDims->Init(16.0f, 16.0f, 46.0f);
    return LT_OK;
}

LTRESULT CFearModel::GetAnimLength(HOBJECT hModel, HMODELANIM hAnim, uint32 &length)
{
    if (!hModel || hAnim == INVALID_MODEL_ANIM)
        return LT_INVALIDPARAMS;

    unsigned int nLength = 0;
    if (FearServerHost_GetAnimLength(hModel, (unsigned int)hAnim, &nLength))
    {
        length = nLength;
        return LT_OK;
    }

    length = 1000;
    return LT_OK;
}

LTRESULT CFearModel::GetAnimKeyFrames(HOBJECT hModel, HMODELANIM hAnim, uint32 &nNumKeyFrames)
{
    if (!hModel || hAnim == INVALID_MODEL_ANIM)
        return LT_INVALIDPARAMS;

    unsigned int nKeyFrames = 0;
    if (FearServerHost_GetAnimKeyFrames(hModel, (unsigned int)hAnim, &nKeyFrames))
    {
        nNumKeyFrames = nKeyFrames;
        return LT_OK;
    }

    nNumKeyFrames = 2;
    return LT_OK;
}

LTRESULT CFearModel::GetAnimNodeTransform(HOBJECT hModel, HMODELANIM hAnim, HMODELNODE hNode, uint32 nKeyFrame, LTRigidTransform& tTransform)
{
    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    if (FearServerHost_GetAnimNodeTransform(
        hModel, (unsigned int)hAnim, (unsigned int)hNode, nKeyFrame,
        position, rotation))
    {
        tTransform.m_vPos.Init(position[0], position[1], position[2]);
        for (int i = 0; i < 4; ++i)
            tTransform.m_rRot.m_Quat[i] = rotation[i];
        return LT_OK;
    }

    std::map<HOBJECT, FearModelObject>::iterator it = g_ModelObjects.find(hModel);
    const FearModelNode *pNode =
        it != g_ModelObjects.end() ? FearFindNode(it->second, hNode) : NULL;
    if (!pNode)
        return LT_NOTFOUND;

    tTransform.m_vPos.Init(pNode->m_Position[0], pNode->m_Position[1], pNode->m_Position[2]);
    for (int i = 0; i < 4; ++i)
        tTransform.m_rRot.m_Quat[i] = pNode->m_Rotation[i];
    return LT_OK;
}

LTRESULT CFearModel::GetPlaybackState(HOBJECT hModel, ANIMTRACKERID TrackerID, uint32 &flags)
{
    if (!hModel)
        return LT_INVALIDPARAMS;
    FearModelWeights &weights = FearModelWeightsFor(hModel);
    flags = weights.m_TrackerPlaying[TrackerID] ? 0 : MS_PLAYDONE;
    return LT_OK;
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
    state.m_TrackerAnimation[TrackerID] = INVALID_MODEL_ANIM;
    state.m_TrackerTime[TrackerID] = 0;
    state.m_TrackerPlaying[TrackerID] = false;
    printf("FEARMODEL: RemoveTracker %u\n", (unsigned)TrackerID);
    return LT_OK;
}

LTRESULT CFearModel::SetAnimBlend(HOBJECT hModel, ANIMTRACKERID TrackerID, float fBlend)
{
    return hModel ? LT_OK : LT_INVALIDPARAMS;
}

LTRESULT CFearModel::CopyAnimation(HOBJECT hModel, ANIMTRACKERID SourceTrackerID, ANIMTRACKERID DestTrackerID)
{
    if (!hModel)
        return LT_INVALIDPARAMS;
    FearModelWeights &state = FearModelWeightsFor(hModel);
    state.m_TrackerAnimation[DestTrackerID] = state.m_TrackerAnimation[SourceTrackerID];
    state.m_TrackerTime[DestTrackerID] = state.m_TrackerTime[SourceTrackerID];
    state.m_TrackerLastTime[DestTrackerID] = state.m_TrackerLastTime[SourceTrackerID];
    state.m_TrackerNextKey[DestTrackerID] = state.m_TrackerNextKey[SourceTrackerID];
    state.m_TrackerLength[DestTrackerID] = state.m_TrackerLength[SourceTrackerID];
    state.m_TrackerRate[DestTrackerID] = state.m_TrackerRate[SourceTrackerID];
    state.m_TrackerLooping[DestTrackerID] = state.m_TrackerLooping[SourceTrackerID];
    state.m_TrackerPlaying[DestTrackerID] = state.m_TrackerPlaying[SourceTrackerID];
    state.m_Trackers[DestTrackerID] = true;
    return LT_OK;
}

LTRESULT CFearModel::GetAnimIndex(HOBJECT hModel, const char *pAnimName, uint32 & anim_index)
{
    if (!hModel || !pAnimName)
        return LT_INVALIDPARAMS;

    unsigned int nServerAnimation = 0;
    if (FearServerHost_GetAnimIndex(hModel, pAnimName, &nServerAnimation))
    {
        std::map<HOBJECT, FearModelObject>::iterator it = g_ModelObjects.find(hModel);
        if (it != g_ModelObjects.end())
        {
            if (it->second.m_AnimationNames.size() <= nServerAnimation)
                it->second.m_AnimationNames.resize(nServerAnimation + 1);
            it->second.m_AnimationNames[nServerAnimation] = pAnimName;
        }
        anim_index = nServerAnimation;
        return LT_OK;
    }

    std::map<HOBJECT, FearModelObject>::iterator it = g_ModelObjects.find(hModel);
    if (it == g_ModelObjects.end())
        return LT_NOTINITIALIZED;

    HMODELANIM nAnimation = FearFindOrAddAnimation(it->second, pAnimName);
    if (nAnimation == INVALID_MODEL_ANIM)
        return LT_NOTFOUND;

    anim_index = nAnimation;
    static int s_nLoggedAnimationLookups = 0;
    if (s_nLoggedAnimationLookups < 48)
    {
        ++s_nLoggedAnimationLookups;
        printf("FEARMODEL: GetAnimIndex obj=%p name='%s' index=%u model='%s'\n",
            (void *)hModel, pAnimName, (unsigned)anim_index, it->second.m_Filename.c_str());
        fflush(stdout);
    }
    return LT_OK;
}

LTRESULT CFearModel::GetAnimName(HOBJECT hModel, uint32 dwAnimIndex, char *pszBuffer, uint32 dwBufferLen)
{
    if (!hModel || !pszBuffer || !dwBufferLen)
        return LT_INVALIDPARAMS;

    if (FearServerHost_GetAnimName(
        hModel, dwAnimIndex, pszBuffer, dwBufferLen))
        return LT_OK;

    std::map<HOBJECT, FearModelObject>::iterator it = g_ModelObjects.find(hModel);
    if (it == g_ModelObjects.end() || dwAnimIndex >= it->second.m_AnimationNames.size())
        return LT_NOTFOUND;

    strncpy(pszBuffer, it->second.m_AnimationNames[dwAnimIndex].c_str(), dwBufferLen - 1);
    pszBuffer[dwBufferLen - 1] = '\0';
    return LT_OK;
}

LTRESULT CFearModel::GetCurAnim(HOBJECT hModel, ANIMTRACKERID TrackerID, HMODELANIM &hAnim)
{
    if (!hModel)
        return LT_INVALIDPARAMS;
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    uint32 nEngineAnim = INVALID_MODEL_ANIM;
    if (pApi && pApi->ModelGetCurAnim &&
        pApi->ModelGetCurAnim(hModel, (uint32)TrackerID, &nEngineAnim))
    {
        FearModelWeightsFor(hModel).m_TrackerAnimation[TrackerID] =
            (HMODELANIM)nEngineAnim;
    }
    hAnim = FearModelWeightsFor(hModel).m_TrackerAnimation[TrackerID];
    return LT_OK;
}

LTRESULT CFearModel::SetCurAnim(HOBJECT hModel, ANIMTRACKERID TrackerID, HMODELANIM hAnim, bool bInterpolate)
{
    if (!hModel)
        return LT_INVALIDPARAMS;
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (pApi && pApi->ModelSetCurAnim &&
        !pApi->ModelSetCurAnim(hModel, (uint32)TrackerID, (uint32)hAnim,
            bInterpolate ? 1 : 0))
    {
        static int s_nEngineSetCurAnimFailures = 0;
        if (s_nEngineSetCurAnimFailures < 16)
        {
            ++s_nEngineSetCurAnimFailures;
            printf("FEARMODEL: engine SetCurAnim failed obj=%p tracker=%u anim=%u\n",
                (void *)hModel, (unsigned)TrackerID, (unsigned)hAnim);
            fflush(stdout);
        }
    }
    FearModelWeights &state = FearModelWeightsFor(hModel);
    state.m_Trackers[TrackerID] = true;
    state.m_TrackerAnimation[TrackerID] = hAnim;
    state.m_TrackerTime[TrackerID] = 0;
    state.m_TrackerLastTime[TrackerID] = 0;
    state.m_TrackerNextKey[TrackerID] = 0;
    state.m_TrackerPlaying[TrackerID] = true;
    if ((uint32)hAnim == 261)
    {
        static int s_nPistolFireSetLogs = 0;
        if (s_nPistolFireSetLogs < 16)
        {
            ++s_nPistolFireSetLogs;
            printf("FEARMODEL: set anim261 obj=%p tracker=%u playing=1\n",
                (void *)hModel, (unsigned)TrackerID);
            fflush(stdout);
        }
    }
    unsigned int nLength = 0;
    if (FearServerHost_GetAnimLength(hModel, (unsigned int)hAnim, &nLength))
        state.m_TrackerLength[TrackerID] = nLength;
    static int s_nSetAnimationLogs = 0;
    if (s_nSetAnimationLogs < 64)
    {
        ++s_nSetAnimationLogs;
        printf("FEARMODEL: SetCurAnim obj=%p tracker=%u anim=%u interp=%d\n",
            (void *)hModel, (unsigned)TrackerID, (unsigned)hAnim, bInterpolate ? 1 : 0);
        fflush(stdout);
    }
    return LT_OK;
}

LTRESULT CFearModel::GetIsInterpolating(HOBJECT hModel, ANIMTRACKERID TrackerID)
{
    return hModel ? LT_NO : LT_INVALIDPARAMS;
}

LTRESULT CFearModel::ResetAnim(HOBJECT hModel, ANIMTRACKERID TrackerID)
{
    if (!hModel)
        return LT_INVALIDPARAMS;
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (pApi && pApi->ModelResetAnim)
        pApi->ModelResetAnim(hModel, (uint32)TrackerID);
    FearModelWeights &state = FearModelWeightsFor(hModel);
    state.m_TrackerTime[TrackerID] = 0;
    state.m_TrackerLastTime[TrackerID] = 0;
    state.m_TrackerNextKey[TrackerID] = 0;
    return LT_OK;
}

LTRESULT CFearModel::GetLooping(HOBJECT hModel, ANIMTRACKERID TrackerID)
{
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (hModel && pApi && pApi->ModelGetLooping)
        return pApi->ModelGetLooping(hModel, (uint32)TrackerID) ? LT_YES : LT_NO;
    return hModel ?
        (FearModelWeightsFor(hModel).m_TrackerLooping[TrackerID] ? LT_YES : LT_NO) :
        LT_INVALIDPARAMS;
}

LTRESULT CFearModel::SetLooping(HOBJECT hModel, ANIMTRACKERID TrackerID, bool bLooping)
{
    if (!hModel)
        return LT_INVALIDPARAMS;
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (pApi && pApi->ModelSetLooping)
        pApi->ModelSetLooping(hModel, (uint32)TrackerID, bLooping ? 1 : 0);
    FearModelWeightsFor(hModel).m_TrackerLooping[TrackerID] = bLooping;
    return LT_OK;
}

LTRESULT CFearModel::GetPlaying(HOBJECT hModel, ANIMTRACKERID TrackerID)
{
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (hModel && pApi && pApi->ModelGetPlaying)
        return pApi->ModelGetPlaying(hModel, (uint32)TrackerID) ? LT_YES : LT_NO;
    return hModel ?
        (FearModelWeightsFor(hModel).m_TrackerPlaying[TrackerID] ? LT_YES : LT_NO) :
        LT_INVALIDPARAMS;
}

LTRESULT CFearModel::SetPlaying(HOBJECT hModel, ANIMTRACKERID TrackerID, bool bPlaying)
{
    if (!hModel)
        return LT_INVALIDPARAMS;
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (pApi && pApi->ModelSetPlaying)
        pApi->ModelSetPlaying(hModel, (uint32)TrackerID, bPlaying ? 1 : 0);
    FearModelWeights &state = FearModelWeightsFor(hModel);
    state.m_Trackers[TrackerID] = true;
    state.m_TrackerPlaying[TrackerID] = bPlaying;
    return LT_OK;
}

LTRESULT CFearModel::GetCurAnimLength(HOBJECT hModel, ANIMTRACKERID TrackerID, uint32 &length)
{
    if (!hModel)
        return LT_INVALIDPARAMS;
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    uint32 nEngineLength = 0;
    if (pApi && pApi->ModelGetCurAnimLength &&
        pApi->ModelGetCurAnimLength(hModel, (uint32)TrackerID, &nEngineLength))
    {
        FearModelWeightsFor(hModel).m_TrackerLength[TrackerID] = nEngineLength;
    }
    length = FearModelWeightsFor(hModel).m_TrackerLength[TrackerID];
    return LT_OK;
}

LTRESULT CFearModel::GetCurAnimTime(HOBJECT hModel, ANIMTRACKERID TrackerID, uint32 &curTime)
{
    if (!hModel)
        return LT_INVALIDPARAMS;
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    uint32 nEngineTime = 0;
    if (pApi && pApi->ModelGetCurAnimTime &&
        pApi->ModelGetCurAnimTime(hModel, (uint32)TrackerID, &nEngineTime))
    {
        FearModelWeightsFor(hModel).m_TrackerTime[TrackerID] = nEngineTime;
    }
    curTime = FearModelWeightsFor(hModel).m_TrackerTime[TrackerID];
    return LT_OK;
}

LTRESULT CFearModel::SetCurAnimTime(HOBJECT hModel, ANIMTRACKERID TrackerID, uint32 curTime)
{
    if (!hModel)
        return LT_INVALIDPARAMS;
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (pApi && pApi->ModelSetCurAnimTime)
        pApi->ModelSetCurAnimTime(hModel, (uint32)TrackerID, curTime);
    FearModelWeights &state = FearModelWeightsFor(hModel);
    state.m_TrackerTime[TrackerID] = curTime;
    state.m_TrackerLastTime[TrackerID] = curTime;
    state.m_TrackerNextKey[TrackerID] = 0;
    return LT_OK;
}

LTRESULT CFearModel::SetAnimRate(HOBJECT hModel, ANIMTRACKERID TrackerID, float fRate)
{
    if (!hModel)
        return LT_INVALIDPARAMS;
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    if (pApi && pApi->ModelSetAnimRate)
        pApi->ModelSetAnimRate(hModel, (uint32)TrackerID, fRate);
    FearModelWeightsFor(hModel).m_TrackerRate[TrackerID] = fRate;
    return LT_OK;
}

LTRESULT CFearModel::GetAnimRate(HOBJECT hModel, ANIMTRACKERID TrackerID, float &fRate)
{
    if (!hModel)
        return LT_INVALIDPARAMS;
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    float fEngineRate = 0.0f;
    if (pApi && pApi->ModelGetAnimRate &&
        pApi->ModelGetAnimRate(hModel, (uint32)TrackerID, &fEngineRate))
    {
        FearModelWeightsFor(hModel).m_TrackerRate[TrackerID] = fEngineRate;
    }
    fRate = FearModelWeightsFor(hModel).m_TrackerRate[TrackerID];
    return LT_OK;
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
    if (!hModel || !pszName)
        return LT_INVALIDPARAMS;
    hSet = (HPHYSICSWEIGHTSET)FearModelWeightSetHandle(pszName);
    return LT_OK;
}

LTRESULT CFearModel::SetPhysicsWeightSet(HOBJECT hModel, HPHYSICSWEIGHTSET hWeightSet)
{
    return hModel ? LT_OK : LT_INVALIDPARAMS;
}

LTRESULT CFearModel::GetPhysicsWeightSet(HOBJECT hModel, HPHYSICSWEIGHTSET& hWeightSet)
{
    if (!hModel)
        return LT_INVALIDPARAMS;
    hWeightSet = (HPHYSICSWEIGHTSET)FearModelWeightSetHandle("RigidBody");
    return LT_OK;
}

LTRESULT CFearModel::GetPhysicsVisNode(HOBJECT hModel, HMODELNODE& hNode)
{
    return GetRootNode(hModel, hNode);
}

LTRESULT CFearModel::DirtyAllNodeTransforms(HOBJECT hModel)
{
    return hModel ? LT_OK : LT_INVALIDPARAMS;
}

LTRESULT CFearModel::DirtyNodeTransform(HOBJECT hModel, HMODELNODE hNode)
{
    return hModel ? LT_OK : LT_INVALIDPARAMS;
}

LTRESULT CFearModel::UpdateModelAttachments(HOBJECT hModel)
{
    return hModel ? LT_OK : LT_INVALIDPARAMS;
}

LTRESULT CFearModel::GetModelFilename(HOBJECT hObj, char *pRetFilename, uint32 nFileBufLen)
{
    if (!hObj || !pRetFilename || nFileBufLen == 0)
        return LT_INVALIDPARAMS;

    const char *pszFilename = FearModel_GetObjectFilename(hObj);
    strncpy(pRetFilename, pszFilename, nFileBufLen - 1);
    pRetFilename[nFileBufLen - 1] = '\0';
    return LT_OK;
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
    if (!hModel)
        return LT_INVALIDPARAMS;
    tTransform.m_vPos.Init();
    tTransform.m_rRot.Identity();
    return LT_OK;
}

LTRESULT CFearModel::ResetMovementEncodingTransform(HOBJECT hModel)
{
    return hModel ? LT_OK : LT_INVALIDPARAMS;
}

LTRESULT CFearModel::SetMovementEncodingTracker(HOBJECT hModel, ANIMTRACKERID TrackerID)
{
    return hModel ? LT_OK : LT_INVALIDPARAMS;
}

LTRESULT CFearModel::DisableMovementEncoding(HOBJECT hModel)
{
    return hModel ? LT_OK : LT_INVALIDPARAMS;
}
