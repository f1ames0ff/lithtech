// F.E.A.R. (Jupiter EX) *.Model00p loader and D3D9 draw pass.
//
// Container layout, decoded from retail archives:
//   0x00  char[4]  "MODL"
//   0x04  char[4]  version marker
//   0x08  u32[21]  header dwords; [2] is the node count, [9] the part-name pool size
//   0x5C  char[]   part-name pool (NUL separated), offsets are relative to 0x5C
//   ...           39-byte node records
//   ...           geometry: u32 vertexBytes, u32 faceBytes, vertices, u16 x 3 faces
//
// The sections between the node records and the geometry are not fully decoded,
// so the geometry block is located by validating candidate (vertexBytes, faceBytes)
// pairs against the face index range.  The pair is not guaranteed to be DWORD-aligned,
// and character files continue with animation data after the geometry.  Two vertex
// profiles are in use:
//   stride 32 - position, unit normal, u32 flag, u32 file id
//   stride 48/64 - position followed by further vec3 rows; offset 12 is not a
//                  normal there (the row is not unit length), so those meshes
//                  take their normals from the faces instead
// The choice is made per model by measuring the mean squared length of the
// stored normals, so a profile mismatch degrades into recomputed normals rather
// than a corrupted mesh.  There are no texture coordinates in the container.

#include "fear_model.h"
#include "cshell_filemgr.h"
#include "fear_world.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <vector>
#include <string>

#define FEAR_MODEL_MAX_INSTANCES 64

struct FearModelVertex
{
    float x, y, z;
    float nx, ny, nz;
    DWORD color;
    float u, v;
};

struct FearModelInstance
{
    FearModel *m_pModel;
    std::string m_Material;
    float m_Matrix[16];
    std::vector<float> m_NodeMatrices;
    IDirect3DVertexBuffer9 *m_pSkinnedVertexBuffer;
    unsigned int m_nSkinnedVertexCapacity;
};

struct FearModel
{
    std::string m_Path;
    std::vector<FearModelVertex> m_Vertices;
    std::vector<unsigned short> m_Indices;
    std::vector<std::string> m_NodeNames;
    std::vector<unsigned char> m_SkinWeights;
    std::vector<unsigned char> m_SkinBones;
    IDirect3DVertexBuffer9 *m_pVertexBuffer;
    IDirect3DIndexBuffer9 *m_pIndexBuffer;
    unsigned int m_nStride;
    unsigned int m_nNodeCount;
    bool m_bDrawLogged;
    bool m_bSkinLogged;
    float m_BoundsMin[3];
    float m_BoundsMax[3];
};

static FearModelInstance g_ModelInstances[FEAR_MODEL_MAX_INSTANCES];
static int g_nModelInstances = 0;
static std::vector<FearModel *> g_ModelCache;
static std::vector<std::string> g_ModelFailures;

struct FearModelMaterialTexture
{
    std::string m_Material;
    IDirect3DTexture9 *m_pTexture;
};

static std::vector<FearModelMaterialTexture> g_MaterialTextures;

static unsigned int FearModelU32(const unsigned char *p, size_t nOffset)
{
    return (unsigned int)p[nOffset] | ((unsigned int)p[nOffset + 1] << 8) |
        ((unsigned int)p[nOffset + 2] << 16) | ((unsigned int)p[nOffset + 3] << 24);
}

static float FearModelF32(const unsigned char *p, size_t nOffset)
{
    unsigned int nBits = FearModelU32(p, nOffset);
    float fValue;
    memcpy(&fValue, &nBits, sizeof(fValue));
    return fValue;
}

static bool FearModelIsPlainFloat(float fValue)
{
    if (!(fValue == fValue))
        return false;
    if (fValue > 1.0e6f || fValue < -1.0e6f)
        return false;
    if (fValue != 0.0f && fabsf(fValue) < 1.0e-9f)
        return false;
    return true;
}

static bool FearModelReadFile(const char *pszPath, std::vector<unsigned char> &data)
{
    void *hFile = FearFile_Open(pszPath);
    if (!hFile)
        return false;
    unsigned long long nSize = FearFile_Size(hFile);
    if (nSize < 64 || nSize > 64ull * 1024ull * 1024ull)
    {
        FearFile_Close(hFile);
        return false;
    }
    data.resize((size_t)nSize);
    bool bRead = FearFile_Read(hFile, data.data(), (unsigned int)nSize) != 0;
    FearFile_Close(hFile);
    return bRead;
}

static std::string FearModelDir()
{
    char szValue[512] = {0};
    DWORD nLen = GetEnvironmentVariableA("FEAR_MODEL_DIR", szValue, sizeof(szValue));
    if (nLen > 0 && nLen < sizeof(szValue))
        return std::string(szValue);
    return std::string("fear-models");
}

static std::string FearModelBaseName(const char *pszName)
{
    std::string name(pszName ? pszName : "");
    for (size_t i = 0; i < name.size(); ++i)
        if (name[i] == '/' || name[i] == '\\')
            name = name.substr(i + 1);
    size_t nDot = name.find_last_of('.');
    if (nDot != std::string::npos)
        name = name.substr(0, nDot);
    return name;
}

static bool FearModelFileExists(const std::string &path)
{
    void *hFile = FearFile_Open(path.c_str());
    if (!hFile)
        return false;
    FearFile_Close(hFile);
    return true;
}

FearModel *FearModel_LoadFile(const char *pszPath);

static std::string FearModelResolvePath(const char *pszName)
{
    std::string dir = FearModelDir();
    std::string base = FearModelBaseName(pszName);
    std::string candidates[6];
    candidates[0] = std::string(pszName ? pszName : "");
    candidates[1] = dir + "\\" + (pszName ? pszName : "");
    candidates[2] = dir + "\\" + base + ".Model00p";
    candidates[3] = dir + "\\" + base + ".model00p";
    candidates[4] = (pszName ? pszName : "") + std::string(".Model00p");
    candidates[5] = "models\\" + base + ".Model00p";
    for (int i = 0; i < 6; ++i)
    {
        if (candidates[i].empty())
            continue;
        if (FearModelFileExists(candidates[i]))
            return candidates[i];
    }
    return std::string();
}

struct FearModelGeometryHit
{
    size_t m_nOffset;
    unsigned int m_nVertexBytes;
    unsigned int m_nFaceBytes;
    unsigned int m_nStride;
    unsigned int m_nVertices;
    unsigned int m_nTriangles;
    unsigned int m_nTail;
};

static bool FearModelFindGeometry(const std::vector<unsigned char> &data, size_t nStart, FearModelGeometryHit &hit)
{
    size_t nSize = data.size();
    const unsigned char *p = &data[0];
    static const unsigned int s_Strides[] = { 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 80, 96 };

    bool bFound = false;
    for (size_t o = nStart; o + 16 <= nSize; ++o)
    {
        unsigned int nVertexBytes = FearModelU32(p, o);
        unsigned int nFaceBytes = FearModelU32(p, o + 4);
        if (nVertexBytes < 64 || nFaceBytes < 60)
            continue;
        if (nFaceBytes % 6)
            continue;
        unsigned int nTriangles = nFaceBytes / 6;
        if (nTriangles > 400000)
            continue;
        size_t nEnd = o + 8 + (size_t)nVertexBytes + (size_t)nFaceBytes;
        if (nEnd > nSize)
            continue;

        unsigned int nCandidateStride = 0;
        for (unsigned int s = 0; s < sizeof(s_Strides) / sizeof(s_Strides[0]); ++s)
        {
            if (nVertexBytes % s_Strides[s] == 0)
            {
                nCandidateStride = s_Strides[s];
                break;
            }
        }
        if (!nCandidateStride)
            continue;

        const unsigned char *pFaces = p + o + 8 + nVertexBytes;
        unsigned int nMaxIndex = 0;
        for (unsigned int i = 0; i < nTriangles * 3; ++i)
        {
            unsigned int nIndex = (unsigned int)pFaces[i * 2] | ((unsigned int)pFaces[i * 2 + 1] << 8);
            if (nIndex > nMaxIndex)
                nMaxIndex = nIndex;
        }
        if (nMaxIndex < 3)
            continue;

        for (unsigned int s = 0; s < sizeof(s_Strides) / sizeof(s_Strides[0]); ++s)
        {
            unsigned int nStride = s_Strides[s];
            if (nVertexBytes % nStride)
                continue;
            unsigned int nVertices = nVertexBytes / nStride;
            if (nVertices < 4 || nMaxIndex >= nVertices)
                continue;
            unsigned int nSlack = nVertices - 1 - nMaxIndex;
            if (nSlack > 8 && nSlack > nVertices / 32)
                continue;
            if (bFound && nSlack >= hit.m_nTail)
                continue;
            hit.m_nOffset = o;
            hit.m_nVertexBytes = nVertexBytes;
            hit.m_nFaceBytes = nFaceBytes;
            hit.m_nStride = nStride;
            hit.m_nVertices = nVertices;
            hit.m_nTriangles = nTriangles;
            hit.m_nTail = nSlack;
            bFound = true;
            break;
        }
    }
    return bFound;
}

static bool FearModelBuildGeometry(FearModel *pModel, const std::vector<unsigned char> &data,
    const FearModelGeometryHit &hit)
{
    const unsigned char *p = &data[0];
    size_t nVertexStart = hit.m_nOffset + 8;
    size_t nFaceStart = nVertexStart + hit.m_nVertexBytes;

    pModel->m_Vertices.resize(hit.m_nVertices);
    pModel->m_SkinWeights.assign((size_t)hit.m_nVertices * 4, 0);
    pModel->m_SkinBones.assign((size_t)hit.m_nVertices * 4, 0);
    float fSumNormal = 0.0f;
    for (unsigned int i = 0; i < hit.m_nVertices; ++i)
    {
        const unsigned char *pRecord = p + nVertexStart + (size_t)i * hit.m_nStride;
        FearModelVertex &v = pModel->m_Vertices[i];
        v.x = FearModelF32(pRecord, 0);
        v.y = FearModelF32(pRecord, 4);
        v.z = FearModelF32(pRecord, 8);
        if (hit.m_nStride >= 32)
        {
            v.nx = FearModelF32(pRecord, 12);
            v.ny = FearModelF32(pRecord, 16);
            v.nz = FearModelF32(pRecord, 20);
        }
        else
        {
            v.nx = v.ny = v.nz = 0.0f;
        }
        v.u = hit.m_nStride >= 64 ? FearModelF32(pRecord, 24) : 0.0f;
        v.v = hit.m_nStride >= 64 ? FearModelF32(pRecord, 28) : 0.0f;
        if (!FearModelIsPlainFloat(v.u) || !FearModelIsPlainFloat(v.v))
            v.u = v.v = 0.0f;
        if (hit.m_nStride >= 64)
        {
            int nInfluences = 0;
            int slots[4] = {0, 0, 0, 0};
            for (int k = 0; k < 4; ++k)
            {
                if (pRecord[56 + k] != 0)
                    slots[nInfluences++] = k;
            }
            for (int k = 0; k < nInfluences; ++k)
            {
                unsigned int nBone = pRecord[60 + nInfluences - 1 - k];
                if (nBone >= pModel->m_nNodeCount)
                    continue;
                pModel->m_SkinWeights[i * 4 + k] = pRecord[56 + slots[k]];
                pModel->m_SkinBones[i * 4 + k] = (unsigned char)nBone;
            }
        }
        if (!FearModelIsPlainFloat(v.x) || !FearModelIsPlainFloat(v.y) || !FearModelIsPlainFloat(v.z))
            return false;
        fSumNormal += v.nx * v.nx + v.ny * v.ny + v.nz * v.nz;
        v.color = 0xffffffffu;
    }

    pModel->m_Indices.resize((size_t)hit.m_nTriangles * 3);
    for (unsigned int i = 0; i < hit.m_nTriangles * 3; ++i)
    {
        const unsigned char *pFace = p + nFaceStart + (size_t)i * 2;
        pModel->m_Indices[i] = (unsigned short)((unsigned int)pFace[0] | ((unsigned int)pFace[1] << 8));
    }

    size_t nKeep = 0;
    for (unsigned int t = 0; t < hit.m_nTriangles; ++t)
    {
        const unsigned int i0 = pModel->m_Indices[t * 3 + 0];
        const unsigned int i1 = pModel->m_Indices[t * 3 + 1];
        const unsigned int i2 = pModel->m_Indices[t * 3 + 2];
        if (i0 >= hit.m_nVertices || i1 >= hit.m_nVertices || i2 >= hit.m_nVertices)
            continue;
        const FearModelVertex &a = pModel->m_Vertices[i0];
        const FearModelVertex &b = pModel->m_Vertices[i1];
        const FearModelVertex &c = pModel->m_Vertices[i2];
        const float e1[3] = { b.x - a.x, b.y - a.y, b.z - a.z };
        const float e2[3] = { c.x - a.x, c.y - a.y, c.z - a.z };
        const float cx = e1[1] * e2[2] - e1[2] * e2[1];
        const float cy = e1[2] * e2[0] - e1[0] * e2[2];
        const float cz = e1[0] * e2[1] - e1[1] * e2[0];
        if (cx * cx + cy * cy + cz * cz < 1.0e-10f)
            continue;
        pModel->m_Indices[nKeep * 3 + 0] = (unsigned short)i0;
        pModel->m_Indices[nKeep * 3 + 1] = (unsigned short)i1;
        pModel->m_Indices[nKeep * 3 + 2] = (unsigned short)i2;
        ++nKeep;
    }
    if (nKeep != hit.m_nTriangles)
        pModel->m_Indices.resize(nKeep * 3);
    const unsigned int nTriangles = (unsigned int)nKeep;

    float fMeanNormal = hit.m_nVertices ? fSumNormal / (float)hit.m_nVertices : 0.0f;
    bool bNormalsOk = fMeanNormal > 0.9f && fMeanNormal < 1.1f;

    std::vector<float> normals((size_t)hit.m_nVertices * 3, 0.0f);
    if (!bNormalsOk)
    {
        for (unsigned int t = 0; t < nTriangles; ++t)
        {
            unsigned int i0 = pModel->m_Indices[t * 3];
            unsigned int i1 = pModel->m_Indices[t * 3 + 1];
            unsigned int i2 = pModel->m_Indices[t * 3 + 2];
            if (i0 >= hit.m_nVertices || i1 >= hit.m_nVertices || i2 >= hit.m_nVertices)
                continue;
            const FearModelVertex &a = pModel->m_Vertices[i0];
            const FearModelVertex &b = pModel->m_Vertices[i1];
            const FearModelVertex &c = pModel->m_Vertices[i2];
            float e1[3] = { b.x - a.x, b.y - a.y, b.z - a.z };
            float e2[3] = { c.x - a.x, c.y - a.y, c.z - a.z };
            float n[3] = {
                e1[1] * e2[2] - e1[2] * e2[1],
                e1[2] * e2[0] - e1[0] * e2[2],
                e1[0] * e2[1] - e1[1] * e2[0] };
            float fLen = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
            if (fLen < 1.0e-6f)
                continue;
            for (int k = 0; k < 3; ++k)
            {
                unsigned int id = pModel->m_Indices[t * 3 + k];
                normals[id * 3 + 0] += n[0] / fLen;
                normals[id * 3 + 1] += n[1] / fLen;
                normals[id * 3 + 2] += n[2] / fLen;
            }
        }
        for (unsigned int i = 0; i < hit.m_nVertices; ++i)
        {
            float nx = normals[i * 3 + 0], ny = normals[i * 3 + 1], nz = normals[i * 3 + 2];
            float fLen = sqrtf(nx * nx + ny * ny + nz * nz);
            if (fLen < 1.0e-6f)
            {
                nx = 0.0f; ny = 1.0f; nz = 0.0f;
            }
            else
            {
                nx /= fLen; ny /= fLen; nz /= fLen;
            }
            FearModelVertex &v = pModel->m_Vertices[i];
            v.nx = nx;
            v.ny = ny;
            v.nz = nz;
        }
    }

    static const float kLight[3] = { 0.35f, 0.82f, 0.45f };
    float fLightLen = sqrtf(kLight[0] * kLight[0] + kLight[1] * kLight[1] + kLight[2] * kLight[2]);
    float lx = kLight[0] / fLightLen, ly = kLight[1] / fLightLen, lz = kLight[2] / fLightLen;
    pModel->m_BoundsMin[0] = pModel->m_BoundsMin[1] = pModel->m_BoundsMin[2] = 1.0e30f;
    pModel->m_BoundsMax[0] = pModel->m_BoundsMax[1] = pModel->m_BoundsMax[2] = -1.0e30f;
    for (unsigned int i = 0; i < hit.m_nVertices; ++i)
    {
        FearModelVertex &v = pModel->m_Vertices[i];
        float fNdl = v.nx * lx + v.ny * ly + v.nz * lz;
        fNdl = fabsf(fNdl);
        float fShade = 0.34f + 0.66f * fNdl;
        if (fShade > 1.0f) fShade = 1.0f;
        unsigned int nLevel = (unsigned int)(fShade * 255.0f);
        v.color = 0xff000000u | (nLevel << 16) | (nLevel << 8) | nLevel;

        float fBounds[3] = { v.x, v.y, v.z };
        for (int k = 0; k < 3; ++k)
        {
            if (fBounds[k] < pModel->m_BoundsMin[k]) pModel->m_BoundsMin[k] = fBounds[k];
            if (fBounds[k] > pModel->m_BoundsMax[k]) pModel->m_BoundsMax[k] = fBounds[k];
        }
    }
    pModel->m_nStride = sizeof(FearModelVertex);
    return true;
}

static bool FearModelParse(FearModel *pModel, const std::vector<unsigned char> &data)
{
    if (data.size() < 0x60)
        return false;
    if (memcmp(&data[0], "MODL", 4) != 0)
        return false;

    const unsigned char *p = &data[0];
    unsigned int nNodeCount = FearModelU32(p, 8 + 2 * 4);
    unsigned int nPoolLength = FearModelU32(p, 8 + 9 * 4);

    size_t nPoolStart = 0x5C;
    size_t nNodesStart = nPoolStart + nPoolLength;
    if (nPoolLength > data.size() || nNodesStart > data.size())
        return false;

    size_t nNodesEnd = nNodesStart + (size_t)nNodeCount * 39;
    if (nNodeCount > 4000 || nNodesEnd > data.size())
        nNodesEnd = nPoolStart;

    for (size_t o = nPoolStart; o < nNodesStart && o < data.size(); )
    {
        size_t nEnd = o;
        while (nEnd < nNodesStart && data[nEnd] != 0)
            ++nEnd;
        if (nEnd > o)
            pModel->m_NodeNames.push_back(std::string((const char *)&data[o], nEnd - o));
        o = nEnd + 1;
    }

    pModel->m_nNodeCount = nNodeCount;
    FearModelGeometryHit hit;
    if (!FearModelFindGeometry(data, nNodesEnd, hit))
        return false;
    if (!FearModelBuildGeometry(pModel, data, hit))
        return false;

    printf("FEARMODEL: %s nodes=%u verts=%u tris=%u stride=%u geom=0x%x bbox=(%.1f %.1f %.1f)-(%.1f %.1f %.1f)\n",
        pModel->m_Path.c_str(), nNodeCount, hit.m_nVertices, hit.m_nTriangles, hit.m_nStride,
        (unsigned int)hit.m_nOffset,
        pModel->m_BoundsMin[0], pModel->m_BoundsMin[1], pModel->m_BoundsMin[2],
        pModel->m_BoundsMax[0], pModel->m_BoundsMax[1], pModel->m_BoundsMax[2]);
    fflush(stdout);
    return true;
}

FearModel *FearModel_LoadFile(const char *pszPath)
{
    if (!pszPath || !pszPath[0])
        return NULL;
    for (size_t i = 0; i < g_ModelCache.size(); ++i)
        if (g_ModelCache[i] && g_ModelCache[i]->m_Path == pszPath)
            return g_ModelCache[i];
    for (size_t i = 0; i < g_ModelFailures.size(); ++i)
        if (g_ModelFailures[i] == pszPath)
            return NULL;

    std::vector<unsigned char> data;
    if (!FearModelReadFile(pszPath, data))
    {
        printf("FEARMODEL: cannot read %s\n", pszPath);
        fflush(stdout);
        return NULL;
    }

    FearModel *pModel = new FearModel();
    pModel->m_Path = pszPath;
    pModel->m_pVertexBuffer = NULL;
    pModel->m_pIndexBuffer = NULL;
    pModel->m_nStride = 0;
    pModel->m_nNodeCount = 0;
    pModel->m_bDrawLogged = false;
    pModel->m_bSkinLogged = false;

    if (!FearModelParse(pModel, data))
    {
        printf("FEARMODEL: parse failed %s\n", pszPath);
        fflush(stdout);
        delete pModel;
        g_ModelFailures.push_back(pszPath);
        return NULL;
    }
    g_ModelCache.push_back(pModel);
    return pModel;
}

FearModel *FearModel_LoadByName(const char *pszName)
{
    std::string path = FearModelResolvePath(pszName);
    if (path.empty())
    {
        printf("FEARMODEL: %s not found (dir=%s)\n", pszName ? pszName : "(null)", FearModelDir().c_str());
        fflush(stdout);
        return NULL;
    }
    return FearModel_LoadFile(path.c_str());
}

void FearModel_Release(FearModel *pModel)
{
    if (!pModel)
        return;
    if (pModel->m_pVertexBuffer)
        pModel->m_pVertexBuffer->Release();
    if (pModel->m_pIndexBuffer)
        pModel->m_pIndexBuffer->Release();
    for (size_t i = 0; i < g_ModelCache.size(); ++i)
        if (g_ModelCache[i] == pModel)
            g_ModelCache[i] = NULL;
    delete pModel;
}

int FearModel_GetVertexCount(const FearModel *pModel)
{
    return pModel ? (int)pModel->m_Vertices.size() : 0;
}

int FearModel_GetTriangleCount(const FearModel *pModel)
{
    return pModel ? (int)(pModel->m_Indices.size() / 3) : 0;
}

const char *FearModel_GetPath(const FearModel *pModel)
{
    return pModel ? pModel->m_Path.c_str() : "";
}

void FearModel_GetBounds(const FearModel *pModel, float *pMin, float *pMax)
{
    for (int i = 0; i < 3; ++i)
    {
        float fMin = pModel ? pModel->m_BoundsMin[i] : 0.0f;
        float fMax = pModel ? pModel->m_BoundsMax[i] : 0.0f;
        if (pMin)
            pMin[i] = fMin;
        if (pMax)
            pMax[i] = fMax;
    }
}

void FearModel_SetLights(const FearModelLight *pLights, int nCount, const float *pAmbient)
{
    (void)pLights;
    (void)nCount;
    (void)pAmbient;
}

void FearModel_Queue(FearModel *pModel, const char *pszMaterial, const float *pMatrix)
{
    if (!pModel || g_nModelInstances >= FEAR_MODEL_MAX_INSTANCES)
        return;
    FearModelInstance &inst = g_ModelInstances[g_nModelInstances++];
    const std::string material = pszMaterial ? pszMaterial : "";
    inst.m_Material = material;
    inst.m_pModel = pModel;
    inst.m_NodeMatrices.clear();
    if (pMatrix)
        memcpy(inst.m_Matrix, pMatrix, sizeof(inst.m_Matrix));
    else
    {
        memset(inst.m_Matrix, 0, sizeof(inst.m_Matrix));
        inst.m_Matrix[0] = inst.m_Matrix[5] = inst.m_Matrix[10] = inst.m_Matrix[15] = 1.0f;
    }
}

void FearModel_QueueSkinned(FearModel *pModel, const char *pszMaterial, const float *pMatrix,
    const float *pNodeMatrices, unsigned int nNodeCount)
{
    if (!pModel || !pNodeMatrices || nNodeCount == 0 ||
        g_nModelInstances >= FEAR_MODEL_MAX_INSTANCES)
        return;

    if (nNodeCount > 256)
        nNodeCount = 256;
    FearModel_Queue(pModel, pszMaterial, pMatrix);
    FearModelInstance &inst = g_ModelInstances[g_nModelInstances - 1];
    inst.m_NodeMatrices.assign(
        pNodeMatrices, pNodeMatrices + (size_t)nNodeCount * 16);
}

void FearModel_ClearQueue()
{
    g_nModelInstances = 0;
}

void FearModel_Shutdown()
{
    for (int i = 0; i < FEAR_MODEL_MAX_INSTANCES; ++i)
    {
        if (g_ModelInstances[i].m_pSkinnedVertexBuffer)
        {
            g_ModelInstances[i].m_pSkinnedVertexBuffer->Release();
            g_ModelInstances[i].m_pSkinnedVertexBuffer = NULL;
            g_ModelInstances[i].m_nSkinnedVertexCapacity = 0;
        }
    }
    for (size_t i = 0; i < g_MaterialTextures.size(); ++i)
        if (g_MaterialTextures[i].m_pTexture)
            g_MaterialTextures[i].m_pTexture->Release();
    g_MaterialTextures.clear();
    for (size_t i = 0; i < g_ModelCache.size(); ++i)
        FearModel_Release(g_ModelCache[i]);
    g_ModelCache.clear();
    g_ModelFailures.clear();
    g_nModelInstances = 0;
}

static void FearModelMatrixMultiply(const float *a, const float *b, float *out)
{
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            out[i * 4 + j] = a[i * 4 + 0] * b[0 * 4 + j] +
                a[i * 4 + 1] * b[1 * 4 + j] +
                a[i * 4 + 2] * b[2 * 4 + j] +
                a[i * 4 + 3] * b[3 * 4 + j];
        }
    }
}

struct FearModelShaderBlob
{
    virtual HRESULT __stdcall QueryInterface(const void *, void **) = 0;
    virtual ULONG __stdcall AddRef() = 0;
    virtual ULONG __stdcall Release() = 0;
    virtual void *__stdcall GetBufferPointer() = 0;
    virtual size_t __stdcall GetBufferSize() = 0;
};

typedef HRESULT (WINAPI *FearModelCompileFn)(const void *pSrcData, size_t nSrcDataSize,
    const char *pszFileName, const void *pDefines, void *pInclude, const char *pszEntrypoint,
    const char *pszTarget, unsigned int nFlags1, unsigned int nFlags2,
    FearModelShaderBlob **ppCode, FearModelShaderBlob **ppErrors);

static const char *g_FearModelShaderSource =
    "row_major float4x4 g_WVP : register(c240);\n"
    "float4 g_Light : register(c244);\n"
    "float4 g_Tint : register(c245);\n"
    "sampler2D g_Diffuse : register(s0);\n"
    "struct VS_IN { float3 pos : POSITION; float3 nrm : NORMAL; float4 col : COLOR0; float2 uv : TEXCOORD0; };\n"
    "struct VS_OUT { float4 pos : POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };\n"
    "VS_OUT FearModelVS(VS_IN i)\n"
    "{\n"
    "    VS_OUT o;\n"
    "    o.pos = mul(float4(i.pos, 1.0), g_WVP);\n"
    "    o.uv = i.uv;\n"
    "    float3 n = normalize(i.nrm);\n"
    "    if (g_Light.w > 0.5)\n"
    "    {\n"
    "        o.col = float4(n * 0.5 + 0.5, 1.0);\n"
    "    }\n"
    "    else\n"
    "    {\n"
    "        float ndl = saturate((dot(n, g_Light.xyz) + 0.25) / 1.25);\n"
    "        o.col = float4(g_Tint.rgb * (0.22 + 0.78 * ndl), 1.0);\n"
    "    }\n"
    "    return o;\n"
    "}\n"
    "float4 FearModelPS(VS_OUT i) : COLOR\n"
    "{\n"
    "    return tex2D(g_Diffuse, i.uv) * i.col;\n"
    "}\n";

struct FearModelShaderState
{
    IDirect3DDevice9 *m_pDevice;
    IDirect3DVertexShader9 *m_pVS;
    IDirect3DPixelShader9 *m_pPS;
    IDirect3DVertexDeclaration9 *m_pDecl;
    IDirect3DTexture9 *m_pWhite;
    bool m_bUsable;
    bool m_bTried;
};

static FearModelShaderState g_ModelShaders = {0};

static void FearModelDestroyShaders()
{
    if (g_ModelShaders.m_pDecl)
        g_ModelShaders.m_pDecl->Release();
    if (g_ModelShaders.m_pVS)
        g_ModelShaders.m_pVS->Release();
    if (g_ModelShaders.m_pPS)
        g_ModelShaders.m_pPS->Release();
    if (g_ModelShaders.m_pWhite)
        g_ModelShaders.m_pWhite->Release();
    memset(&g_ModelShaders, 0, sizeof(g_ModelShaders));
}

static bool FearModelCompile(FearModelCompileFn pCompile, const char *pszEntry,
    const char *pszTarget, FearModelShaderBlob **ppBlob)
{
    FearModelShaderBlob *pErrors = NULL;
    HRESULT hr = pCompile(g_FearModelShaderSource, strlen(g_FearModelShaderSource),
        "fear_model.fx", NULL, NULL, pszEntry, pszTarget, 4, 0, ppBlob, &pErrors);
    if (FAILED(hr))
    {
        printf("FEARMODEL: %s compile failed: %s\n", pszTarget,
            pErrors ? (const char *)pErrors->GetBufferPointer() : "no blob");
        fflush(stdout);
        if (pErrors)
            pErrors->Release();
        return false;
    }
    if (pErrors)
        pErrors->Release();
    return *ppBlob != NULL;
}

static bool FearModelInitShaders(IDirect3DDevice9 *pDevice)
{
    if (g_ModelShaders.m_bTried && g_ModelShaders.m_pDevice == pDevice)
        return g_ModelShaders.m_bUsable;
    if (g_ModelShaders.m_bTried)
        FearModelDestroyShaders();

    g_ModelShaders.m_bTried = true;
    g_ModelShaders.m_pDevice = pDevice;

    char szFF[16] = {0};
    if (GetEnvironmentVariableA("FEAR_MODEL_FF", szFF, sizeof(szFF)) > 0 && atoi(szFF) != 0)
    {
        printf("FEARMODEL: fixed function path forced by FEAR_MODEL_FF\n");
        fflush(stdout);
        return false;
    }

    HMODULE hCompiler = LoadLibraryA("d3dcompiler_47.dll");
    if (!hCompiler)
        hCompiler = LoadLibraryA("d3dcompiler_43.dll");
    if (!hCompiler)
    {
        printf("FEARMODEL: no d3dcompiler available\n");
        fflush(stdout);
        return false;
    }
    FearModelCompileFn pCompile = (FearModelCompileFn)GetProcAddress(hCompiler, "D3DCompile");
    if (!pCompile)
    {
        printf("FEARMODEL: D3DCompile missing\n");
        fflush(stdout);
        return false;
    }

    FearModelShaderBlob *pVSBlob = NULL;
    FearModelShaderBlob *pPSBlob = NULL;
    if (!FearModelCompile(pCompile, "FearModelVS", "vs_3_0", &pVSBlob) ||
        !FearModelCompile(pCompile, "FearModelPS", "ps_3_0", &pPSBlob))
    {
        if (pVSBlob) pVSBlob->Release();
        if (pPSBlob) pPSBlob->Release();
        return false;
    }

    HRESULT hr = pDevice->CreateVertexShader((const DWORD *)pVSBlob->GetBufferPointer(),
        &g_ModelShaders.m_pVS);
    if (SUCCEEDED(hr))
        hr = pDevice->CreatePixelShader((const DWORD *)pPSBlob->GetBufferPointer(),
            &g_ModelShaders.m_pPS);
    pVSBlob->Release();
    pPSBlob->Release();
    if (SUCCEEDED(hr))
        hr = pDevice->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
            &g_ModelShaders.m_pWhite, NULL);
    if (SUCCEEDED(hr))
    {
        D3DLOCKED_RECT whiteRect;
        memset(&whiteRect, 0, sizeof(whiteRect));
        hr = g_ModelShaders.m_pWhite->LockRect(0, &whiteRect, NULL, 0);
        if (SUCCEEDED(hr))
        {
            *(DWORD *)whiteRect.pBits = 0xffffffffu;
            g_ModelShaders.m_pWhite->UnlockRect(0);
        }
    }
    if (FAILED(hr))
    {
        printf("FEARMODEL: shader creation failed hr=0x%08x\n", (unsigned int)hr);
        fflush(stdout);
        FearModelDestroyShaders();
        return false;
    }

    static const D3DVERTEXELEMENT9 decl[] =
    {
        { 0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0 },
        { 0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
        { 0, 28, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };
    hr = pDevice->CreateVertexDeclaration(decl, &g_ModelShaders.m_pDecl);
    if (FAILED(hr))
    {
        printf("FEARMODEL: declaration creation failed hr=0x%08x\n", (unsigned int)hr);
        fflush(stdout);
        FearModelDestroyShaders();
        return false;
    }

    g_ModelShaders.m_bUsable = true;
    printf("FEARMODEL: model shaders ready (stride=%u)\n", (unsigned int)sizeof(FearModelVertex));
    fflush(stdout);
    return true;
}

static bool FearModelEnsureBuffers(IDirect3DDevice9 *pDevice, FearModel *pModel)
{
    if (pModel->m_pVertexBuffer && pModel->m_pIndexBuffer)
        return true;

    UINT nVertexBytes = (UINT)(pModel->m_Vertices.size() * sizeof(FearModelVertex));
    UINT nIndexBytes = (UINT)(pModel->m_Indices.size() * sizeof(unsigned short));
    if (!nVertexBytes || !nIndexBytes)
        return false;

    if (!pModel->m_pVertexBuffer)
    {
        HRESULT hr = pDevice->CreateVertexBuffer(nVertexBytes, 0,
            D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE, D3DPOOL_MANAGED,
            &pModel->m_pVertexBuffer, NULL);
        if (FAILED(hr))
        {
            printf("FEARMODEL: CreateVertexBuffer(%u) failed hr=0x%08x\n",
                (unsigned int)nVertexBytes, (unsigned int)hr);
            fflush(stdout);
            return false;
        }
        void *pDst = NULL;
        hr = pModel->m_pVertexBuffer->Lock(0, 0, &pDst, 0);
        if (FAILED(hr))
            return false;
        memcpy(pDst, &pModel->m_Vertices[0], nVertexBytes);
        if (FAILED(pModel->m_pVertexBuffer->Unlock()))
            return false;
    }
    if (!pModel->m_pIndexBuffer)
    {
        HRESULT hr = pDevice->CreateIndexBuffer(nIndexBytes, 0, D3DFMT_INDEX16,
            D3DPOOL_MANAGED, &pModel->m_pIndexBuffer, NULL);
        if (FAILED(hr))
        {
            printf("FEARMODEL: CreateIndexBuffer(%u) failed hr=0x%08x\n",
                (unsigned int)nIndexBytes, (unsigned int)hr);
            fflush(stdout);
            return false;
        }
        void *pDst = NULL;
        hr = pModel->m_pIndexBuffer->Lock(0, 0, &pDst, 0);
        if (FAILED(hr))
            return false;
        memcpy(pDst, &pModel->m_Indices[0], nIndexBytes);
        if (FAILED(pModel->m_pIndexBuffer->Unlock()))
            return false;
    }
    return true;
}

static bool FearModelEnsureSkinnedBuffer(
    IDirect3DDevice9 *pDevice, FearModelInstance &inst, unsigned int nVertexCount)
{
    if (inst.m_pSkinnedVertexBuffer &&
        inst.m_nSkinnedVertexCapacity >= nVertexCount)
        return true;

    if (inst.m_pSkinnedVertexBuffer)
    {
        inst.m_pSkinnedVertexBuffer->Release();
        inst.m_pSkinnedVertexBuffer = NULL;
        inst.m_nSkinnedVertexCapacity = 0;
    }

    HRESULT hr = pDevice->CreateVertexBuffer(
        nVertexCount * sizeof(FearModelVertex),
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE,
        D3DPOOL_DEFAULT, &inst.m_pSkinnedVertexBuffer, NULL);
    if (FAILED(hr))
    {
        printf("FEARMODEL: CreateSkinnedVertexBuffer(%u) failed hr=0x%08x\n",
            nVertexCount, (unsigned int)hr);
        fflush(stdout);
        return false;
    }

    inst.m_nSkinnedVertexCapacity = nVertexCount;
    return true;
}

static bool FearModelUpdateSkin(FearModelInstance &inst, FearModel *pModel)
{
    const unsigned int nVertexCount = (unsigned int)pModel->m_Vertices.size();
    const unsigned int nNodeCount =
        (unsigned int)(inst.m_NodeMatrices.size() / 16);
    if (!nNodeCount || pModel->m_SkinWeights.size() != (size_t)nVertexCount * 4)
        return false;

    FearModelVertex *pDst = NULL;
    if (FAILED(inst.m_pSkinnedVertexBuffer->Lock(
        0, 0, (void **)&pDst, D3DLOCK_DISCARD)))
        return false;

    float boundsMin[3] = {1.0e30f, 1.0e30f, 1.0e30f};
    float boundsMax[3] = {-1.0e30f, -1.0e30f, -1.0e30f};
    for (unsigned int i = 0; i < nVertexCount; ++i)
    {
        const FearModelVertex &source = pModel->m_Vertices[i];
        FearModelVertex &target = pDst[i];
        float px = 0.0f, py = 0.0f, pz = 0.0f;
        float nx = 0.0f, ny = 0.0f, nz = 0.0f;
        float fTotalWeight = 0.0f;
        for (int k = 0; k < 4; ++k)
        {
            const float fWeight = pModel->m_SkinWeights[i * 4 + k] / 255.0f;
            const unsigned int nNode = pModel->m_SkinBones[i * 4 + k];
            if (fWeight <= 0.0f || nNode >= nNodeCount)
                continue;
            const float *m = &inst.m_NodeMatrices[nNode * 16];
            px += fWeight * (source.x * m[0] + source.y * m[4] + source.z * m[8] + m[12]);
            py += fWeight * (source.x * m[1] + source.y * m[5] + source.z * m[9] + m[13]);
            pz += fWeight * (source.x * m[2] + source.y * m[6] + source.z * m[10] + m[14]);
            nx += fWeight * (source.nx * m[0] + source.ny * m[4] + source.nz * m[8]);
            ny += fWeight * (source.nx * m[1] + source.ny * m[5] + source.nz * m[9]);
            nz += fWeight * (source.nx * m[2] + source.ny * m[6] + source.nz * m[10]);
            fTotalWeight += fWeight;
        }

        if (fTotalWeight > 0.0f)
        {
            target.x = px / fTotalWeight;
            target.y = py / fTotalWeight;
            target.z = pz / fTotalWeight;
            target.nx = nx / fTotalWeight;
            target.ny = ny / fTotalWeight;
            target.nz = nz / fTotalWeight;
        }
        else
        {
            target = source;
        }
        target.color = source.color;
        const float position[3] = {target.x, target.y, target.z};
        for (int k = 0; k < 3; ++k)
        {
            if (position[k] < boundsMin[k]) boundsMin[k] = position[k];
            if (position[k] > boundsMax[k]) boundsMax[k] = position[k];
        }
    }

    if (FAILED(inst.m_pSkinnedVertexBuffer->Unlock()))
        return false;
    if (!pModel->m_bSkinLogged)
    {
        pModel->m_bSkinLogged = true;
        printf("FEARMODEL: skin %s bbox=(%.1f %.1f %.1f)-(%.1f %.1f %.1f)\n",
            pModel->m_Path.c_str(),
            boundsMin[0], boundsMin[1], boundsMin[2],
            boundsMax[0], boundsMax[1], boundsMax[2]);
        fflush(stdout);
    }
    return true;
}

static IDirect3DTexture9 *FearModelGetMaterialTexture(
    IDirect3DDevice9 *pDevice, const std::string &material)
{
    if (material.empty())
        return NULL;
    for (size_t i = 0; i < g_MaterialTextures.size(); ++i)
    {
        if (_stricmp(g_MaterialTextures[i].m_Material.c_str(), material.c_str()) == 0)
            return g_MaterialTextures[i].m_pTexture;
    }

    FearModelMaterialTexture entry;
    entry.m_Material = material;
    entry.m_pTexture = FearWorld_LoadMaterialTexture(pDevice, material.c_str());
    g_MaterialTextures.push_back(entry);
    printf("FEARMODEL: texture material='%s' loaded=%d\n",
        material.c_str(), entry.m_pTexture ? 1 : 0);
    fflush(stdout);
    return entry.m_pTexture;
}

void FearModel_RenderQueued(IDirect3DDevice9 *pDevice, const float *pView, const float *pProj,
    const float *pEye, float fFovY, float fAspect)
{
    (void)pEye;
    (void)fFovY;
    (void)fAspect;
    if (!pDevice || !pView || !pProj || g_nModelInstances <= 0)
        return;

    float viewProj[16];
    FearModelMatrixMultiply(pView, pProj, viewProj);

    struct FearModelState
    {
        DWORD m_States[8];
        DWORD m_Stage[4][4];
        DWORD m_MinFilter, m_MagFilter, m_MipFilter;
        D3DMATRIX m_World, m_View, m_Proj;
        DWORD m_Fvf;
        IDirect3DVertexShader9 *m_pVS;
        IDirect3DPixelShader9 *m_pPS;
        IDirect3DVertexDeclaration9 *m_pDecl;
        IDirect3DBaseTexture9 *m_pTexture0;
    } saved;

    static const D3DRENDERSTATETYPE kStates[8] = {
        D3DRS_LIGHTING, D3DRS_ZENABLE, D3DRS_ZWRITEENABLE, D3DRS_CULLMODE,
        D3DRS_ALPHABLENDENABLE, D3DRS_ALPHATESTENABLE, D3DRS_DITHERENABLE,
        D3DRS_STENCILENABLE };
    static const D3DTEXTURESTAGESTATETYPE kStage[4] = {
        D3DTSS_COLOROP, D3DTSS_COLORARG1, D3DTSS_COLORARG2, D3DTSS_ALPHAOP };

    for (int i = 0; i < 8; ++i)
        pDevice->GetRenderState(kStates[i], &saved.m_States[i]);
    for (int s = 0; s < 4; ++s)
        for (int i = 0; i < 4; ++i)
            pDevice->GetTextureStageState(s, kStage[i], &saved.m_Stage[s][i]);
    pDevice->GetSamplerState(0, D3DSAMP_MINFILTER, &saved.m_MinFilter);
    pDevice->GetSamplerState(0, D3DSAMP_MAGFILTER, &saved.m_MagFilter);
    pDevice->GetSamplerState(0, D3DSAMP_MIPFILTER, &saved.m_MipFilter);
    pDevice->GetTransform(D3DTS_WORLD, &saved.m_World);
    pDevice->GetTransform(D3DTS_VIEW, &saved.m_View);
    pDevice->GetTransform(D3DTS_PROJECTION, &saved.m_Proj);
    pDevice->GetFVF(&saved.m_Fvf);
    pDevice->GetVertexShader(&saved.m_pVS);
    pDevice->GetPixelShader(&saved.m_pPS);
    pDevice->GetVertexDeclaration(&saved.m_pDecl);
    pDevice->GetTexture(0, &saved.m_pTexture0);

    bool bShader = FearModelInitShaders(pDevice);
    pDevice->SetVertexShader(NULL);
    pDevice->SetPixelShader(NULL);
    pDevice->SetTransform(D3DTS_VIEW, (const D3DMATRIX *)pView);
    pDevice->SetTransform(D3DTS_PROJECTION, (const D3DMATRIX *)pProj);
    if (bShader)
    {
        pDevice->SetVertexDeclaration(g_ModelShaders.m_pDecl);
        pDevice->SetVertexShader(g_ModelShaders.m_pVS);
        pDevice->SetPixelShader(g_ModelShaders.m_pPS);
        float light[4] = {0.35f, 0.72f, -0.60f, 0.0f};
        char szNrm[16] = {0};
        if (GetEnvironmentVariableA("FEAR_MODEL_DEBUGN", szNrm, sizeof(szNrm)) > 0 && atoi(szNrm) != 0)
            light[3] = 1.0f;
        float nLen = sqrtf(light[0] * light[0] + light[1] * light[1] + light[2] * light[2]);
        light[0] /= nLen;
        light[1] /= nLen;
        light[2] /= nLen;
        pDevice->SetVertexShaderConstantF(244, light, 1);
        float tint[4] = {0.72f, 0.72f, 0.72f, 1.0f};
        pDevice->SetVertexShaderConstantF(245, tint, 1);
        static bool s_bLightLogged = false;
        if (!s_bLightLogged) { s_bLightLogged = true; printf("FEARMODEL: light=(%.2f,%.2f,%.2f) debugnorm=%.1f\n", light[0], light[1], light[2], light[3]); fflush(stdout); }
    }
    else
    {
        pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE);
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    }
    pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    char szZ[16] = {0};
    if (GetEnvironmentVariableA("FEAR_MODEL_ZTEST", szZ, sizeof(szZ)) > 0 && atoi(szZ) == 0)
    {
        pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    }
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    pDevice->SetTexture(0, NULL);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    for (unsigned int nStage = 1; nStage < 4; ++nStage)
        pDevice->SetTextureStageState(nStage, D3DTSS_COLOROP, D3DTOP_DISABLE);
    pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

    for (int i = 0; i < g_nModelInstances; ++i)
    {
        FearModelInstance &inst = g_ModelInstances[i];
        FearModel *pModel = inst.m_pModel;
        if (!pModel || !FearModelEnsureBuffers(pDevice, pModel))
            continue;

        IDirect3DVertexBuffer9 *pVertexBuffer = pModel->m_pVertexBuffer;
        bool bSkinned = inst.m_NodeMatrices.size() >= 16 &&
            pModel->m_SkinWeights.size() == pModel->m_Vertices.size() * 4;
        if (bSkinned &&
            FearModelEnsureSkinnedBuffer(
                pDevice, inst, (unsigned int)pModel->m_Vertices.size()) &&
            FearModelUpdateSkin(inst, pModel))
        {
            pVertexBuffer = inst.m_pSkinnedVertexBuffer;
        }
        else
        {
            bSkinned = false;
        }

        IDirect3DTexture9 *pMaterialTexture =
            FearModelGetMaterialTexture(pDevice, inst.m_Material);
        pDevice->SetTexture(0, pMaterialTexture ?
            pMaterialTexture : g_ModelShaders.m_pWhite);

        float wvp[16];
        FearModelMatrixMultiply(inst.m_Matrix, viewProj, wvp);
        pDevice->SetTransform(D3DTS_WORLD, (const D3DMATRIX *)inst.m_Matrix);
        if (bShader)
            pDevice->SetVertexShaderConstantF(240, wvp, 4);
        pDevice->SetStreamSource(0, pVertexBuffer, 0, sizeof(FearModelVertex));
        pDevice->SetIndices(pModel->m_pIndexBuffer);
        HRESULT hrDraw;
        unsigned int nTris = (unsigned int)(pModel->m_Indices.size() / 3);
        static unsigned int s_nTriLimit = 0;
        static int s_nPointMode = -1;
        if (s_nPointMode < 0)
        {
            char szTmp[32] = {0};
            DWORD nLen = GetEnvironmentVariableA("FEAR_MODEL_TRIS", szTmp, sizeof(szTmp));
            s_nTriLimit = (nLen > 0) ? (unsigned int)atoi(szTmp) : 0;
            char szPts[32] = {0};
            nLen = GetEnvironmentVariableA("FEAR_MODEL_POINTS", szPts, sizeof(szPts));
            s_nPointMode = (nLen > 0 && atoi(szPts) != 0) ? 1 : 0;
        }
        if (s_nPointMode)
        {
            hrDraw = pDevice->DrawPrimitive(D3DPT_POINTLIST, 0, (UINT)pModel->m_Vertices.size());
        }
        else
        {
            if (s_nTriLimit && s_nTriLimit < nTris)
                nTris = s_nTriLimit;
            static int s_nNoDraw = -1;
            if (s_nNoDraw < 0)
            {
                char szND[16] = {0};
                s_nNoDraw = (GetEnvironmentVariableA("FEAR_MODEL_NODRAW", szND, sizeof(szND)) > 0 &&
                    atoi(szND) != 0) ? 1 : 0;
            }
            hrDraw = s_nNoDraw ? 0 : pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0,
                (UINT)pModel->m_Vertices.size(), 0, nTris);
        }
        if (!pModel->m_bDrawLogged)
        {
            pModel->m_bDrawLogged = true;
            const FearModelVertex &v0 = pModel->m_Vertices[0];
            float out[4];
            for (int c = 0; c < 4; ++c)
                out[c] = v0.x * wvp[0 * 4 + c] + v0.y * wvp[1 * 4 + c] +
                         v0.z * wvp[2 * 4 + c] + wvp[3 * 4 + c];
            float lo[3] = {0}, hi[3] = {0};
            FearModel_GetBounds(pModel, lo, hi);
            float ndcMin[2] = {1.0e9f, 1.0e9f}, ndcMax[2] = {-1.0e9f, -1.0e9f};
            float wMin = 1.0e9f, wMax = -1.0e9f;
            for (int corner = 0; corner < 8; ++corner)
            {
                float p[3] = {
                    (corner & 1) ? hi[0] : lo[0],
                    (corner & 2) ? hi[1] : lo[1],
                    (corner & 4) ? hi[2] : lo[2] };
                float o[4];
                for (int c = 0; c < 4; ++c)
                    o[c] = p[0] * wvp[0 * 4 + c] + p[1] * wvp[1 * 4 + c] +
                           p[2] * wvp[2 * 4 + c] + wvp[3 * 4 + c];
                float nx = o[0] / o[3], ny = o[1] / o[3];
                if (nx < ndcMin[0]) ndcMin[0] = nx;
                if (ny < ndcMin[1]) ndcMin[1] = ny;
                if (nx > ndcMax[0]) ndcMax[0] = nx;
                if (ny > ndcMax[1]) ndcMax[1] = ny;
                if (o[3] < wMin) wMin = o[3];
                if (o[3] > wMax) wMax = o[3];
            }
            printf("FEARMODEL: draw %s skinned=%u hr=0x%08x verts=%u tris=%u ndc0=(%.3f,%.3f,%.3f,w=%.2f) "
                "ndcbox=(%.3f,%.3f)-(%.3f,%.3f) w=[%.2f..%.2f] color=0x%08x\n",
                pModel->m_Path.c_str(), bSkinned ? 1u : 0u,
                (unsigned int)hrDraw, (unsigned int)pModel->m_Vertices.size(),
                (unsigned int)(pModel->m_Indices.size() / 3),
                out[0] / out[3], out[1] / out[3], out[2] / out[3], out[3],
                ndcMin[0], ndcMin[1], ndcMax[0], ndcMax[1], wMin, wMax, (unsigned int)v0.color);
            fflush(stdout);
        }
    }

    FearModel_ClearQueue();

    pDevice->SetVertexShader(saved.m_pVS);
    pDevice->SetPixelShader(saved.m_pPS);
    pDevice->SetVertexDeclaration(saved.m_pDecl);
    if (saved.m_pVS) saved.m_pVS->Release();
    if (saved.m_pPS) saved.m_pPS->Release();
    if (saved.m_pDecl) saved.m_pDecl->Release();
    pDevice->SetFVF(saved.m_Fvf);
    pDevice->SetTransform(D3DTS_WORLD, &saved.m_World);
    pDevice->SetTransform(D3DTS_VIEW, &saved.m_View);
    pDevice->SetTransform(D3DTS_PROJECTION, &saved.m_Proj);
    for (int i = 0; i < 8; ++i)
        pDevice->SetRenderState(kStates[i], saved.m_States[i]);
    for (int s = 0; s < 4; ++s)
        for (int i = 0; i < 4; ++i)
            pDevice->SetTextureStageState(s, kStage[i], saved.m_Stage[s][i]);
    pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, saved.m_MinFilter);
    pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, saved.m_MagFilter);
    pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, saved.m_MipFilter);
    pDevice->SetTexture(0, saved.m_pTexture0);
    if (saved.m_pTexture0) saved.m_pTexture0->Release();
}
