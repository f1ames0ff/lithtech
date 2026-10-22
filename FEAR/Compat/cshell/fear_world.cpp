#include "fear_world.h"
#include "cshell_filemgr.h"
#include "fear_host_api.h"
#include "fear_render_api.h"
#include "dds_loader.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <string>
#include <new>

struct FearWorldVertex
{
    float x, y, z;
    float u, v;
};

struct FearWorldBatch
{
    std::string m_Name;
    std::vector<FearWorldVertex> m_Vertices;
    IDirect3DVertexBuffer9 *m_pVertexBuffer;
    unsigned int m_nVertexCount;
    DWORD m_Color;
    IDirect3DTexture9 *m_pTexture;
    bool m_bTextureLoaded;
};

static std::vector<FearWorldBatch> g_WorldBatches;
static bool g_bWorldLoaded = false;
static float g_WorldMin[3] = {0, 0, 0};
static float g_WorldMax[3] = {0, 0, 0};
static float g_CameraPos[3] = { -1100.0f, -1372.0f, -370.0f };
static float g_CameraYaw = -0.0784588f;
static float g_CameraPitch = 0.0f;

static unsigned int FearReadU32(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) | ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

static unsigned short FearReadU16(const unsigned char *p)
{
    return (unsigned short)((unsigned int)p[0] | ((unsigned int)p[1] << 8));
}

static float FearReadF32(const unsigned char *p)
{
    unsigned int n = FearReadU32(p);
    float f;
    memcpy(&f, &n, sizeof(f));
    return f;
}

static bool FearIsShadowVolumeMaterial(const std::string &name)
{
    std::string normalized;
    normalized.reserve(name.size());
    for (size_t i = 0; i < name.size(); ++i)
    {
        char c = name[i];
        if (c == '/')
            c = '\\';
        if (c >= 'A' && c <= 'Z')
            c = (char)(c - 'A' + 'a');
        normalized.push_back(c);
    }
    return normalized == "engine\\shadowvolume.mat00";
}

static DWORD FearMaterialColor(unsigned int nMaterial)
{
    unsigned char colorR = (unsigned char)(64 + ((nMaterial * 37) % 160));
    unsigned char colorG = (unsigned char)(64 + ((nMaterial * 73) % 160));
    unsigned char colorB = (unsigned char)(64 + ((nMaterial * 151) % 160));
    return 0xff000000 | ((DWORD)colorR << 16) | ((DWORD)colorG << 8) | colorB;
}

static bool FearReadFile(const char *pszFilename, std::vector<unsigned char> &data)
{
    void *hFile = FearFile_Open(pszFilename);
    if (!hFile)
        return false;

    unsigned long long nSize = FearFile_Size(hFile);
    if (nSize == 0 || nSize > 64ull * 1024ull * 1024ull)
    {
        FearFile_Close(hFile);
        return false;
    }

    data.resize((size_t)nSize);
    if (!FearFile_Read(hFile, data.data(), (unsigned int)nSize))
    {
        FearFile_Close(hFile);
        data.clear();
        return false;
    }
    FearFile_Close(hFile);
    return true;
}

static bool FearHasDDSExtension(const char *pszFilename)
{
    if (!pszFilename)
        return false;
    size_t nLength = strlen(pszFilename);
    if (nLength < 4)
        return false;
    const char *p = pszFilename + nLength - 4;
    return p[0] == '.' &&
        (p[1] == 'd' || p[1] == 'D') &&
        (p[2] == 'd' || p[2] == 'D') &&
        (p[3] == 's' || p[3] == 'S');
}

static bool FearReadMaterialTexture(const char *pszMaterial, std::string &textureName)
{
    std::vector<unsigned char> data;
    if (!FearReadFile(pszMaterial, data))
        return false;
    if (data.size() < 32 || memcmp(data.data(), "LTMI", 4) != 0)
        return false;

    size_t o = 8;
    if (o + 2 > data.size())
        return false;
    unsigned int nShaderLength = FearReadU16(data.data() + o);
    o += 2;
    if (o + nShaderLength > data.size())
        return false;
    o += nShaderLength;

    if (o + 4 > data.size())
        return false;
    unsigned int nParams = FearReadU32(data.data() + o);
    o += 4;
    if (nParams > 1024)
        return false;

    for (unsigned int i = 0; i < nParams; ++i)
    {
        if (o + 4 > data.size())
            return false;
        unsigned int nType = FearReadU32(data.data() + o);
        o += 4;
        if (o + 2 > data.size())
            return false;
        unsigned int nNameLength = FearReadU16(data.data() + o);
        o += 2;
        if (o + nNameLength > data.size())
            return false;
        std::string name((const char *)data.data() + o, nNameLength);
        o += nNameLength;

        if (nType == 1)
        {
            if (o + 2 > data.size())
                return false;
            unsigned int nValueLength = FearReadU16(data.data() + o);
            o += 2;
            if (o + nValueLength > data.size())
                return false;
            std::string value((const char *)data.data() + o, nValueLength);
            o += nValueLength;
            if (textureName.empty() && FearHasDDSExtension(value.c_str()))
                textureName = value;
        }
        else if (nType == 2)
            o += 12;
        else if (nType == 3)
            o += 16;
        else if (nType == 4 || nType == 5)
            o += 4;
        else
            return false;

        if (o > data.size())
            return false;
    }

    return !textureName.empty();
}

static bool FearReadVertexUV(const unsigned char *pVertex, unsigned int nVertexSize, float &u, float &v)
{
    unsigned int nOffset = 0;
    switch (nVertexSize)
    {
    case 24:
    case 48:
    case 84:
        nOffset = 16;
        break;
    case 32:
    case 36:
    case 40:
    case 56:
        nOffset = 24;
        break;
    case 60:
        nOffset = 28;
        break;
    default:
        return false;
    }

    if (nOffset + 8 > nVertexSize)
        return false;

    u = FearReadF32(pVertex + nOffset);
    v = FearReadF32(pVertex + nOffset + 4);
    return true;
}

static bool FearReadWorldFile(const char *pszFilename, std::vector<unsigned char> &data)
{
    void *hFile = FearFile_Open(pszFilename);
    if (!hFile)
        return false;

    unsigned long long nSize = FearFile_Size(hFile);
    if (nSize == 0 || nSize > 256ull * 1024ull * 1024ull)
    {
        FearFile_Close(hFile);
        return false;
    }

    data.resize((size_t)nSize);
    if (!FearFile_Read(hFile, data.data(), (unsigned int)nSize))
    {
        FearFile_Close(hFile);
        data.clear();
        return false;
    }
    FearFile_Close(hFile);
    return true;
}

static void FearAddTriangle(const unsigned char *pVertexBlock, unsigned int nVertexBlockSize,
    unsigned int nVertexStart, unsigned int nVertexSize, unsigned int nVertices,
    unsigned int nVertexBase, unsigned int nMaterial,
    unsigned int nTri0, unsigned int nTri1, unsigned int nTri2)
{
    if (nVertexSize < 12 || nVertexStart > nVertexBlockSize || nVertexSize > nVertexBlockSize)
        return;

    unsigned int nIds[3];
    nIds[0] = nTri0 + nVertexBase;
    nIds[1] = nTri1 + nVertexBase;
    nIds[2] = nTri2 + nVertexBase;
    for (int i = 0; i < 3; ++i)
    {
        unsigned int nGlobal = nIds[i];
        if (nGlobal < nVertexStart)
            return;
        unsigned int nLocal = nGlobal - nVertexStart;
        if (nLocal >= nVertices)
            return;
        nIds[i] = nLocal;
    }

    if (nMaterial >= g_WorldBatches.size())
        nMaterial = 0;

    FearWorldBatch &batch = g_WorldBatches[nMaterial];
    for (int i = 0; i < 3; ++i)
    {
        size_t nOffset = (size_t)nVertexStart * nVertexSize + (size_t)nIds[i] * nVertexSize;
        if (nOffset + 12 > nVertexBlockSize)
            return;
        FearWorldVertex v;
        v.x = FearReadF32(pVertexBlock + nOffset);
        v.y = FearReadF32(pVertexBlock + nOffset + 4);
        v.z = FearReadF32(pVertexBlock + nOffset + 8);
        v.u = 0.0f;
        v.v = 0.0f;
        FearReadVertexUV(pVertexBlock + nOffset, nVertexSize, v.u, v.v);
        batch.m_Vertices.push_back(v);
    }
}

int FearWorld_Load(const char *pszFilename)
{
    if (g_bWorldLoaded)
        return 1;
    g_bWorldLoaded = true;

    std::vector<unsigned char> data;
    if (!FearReadWorldFile(pszFilename, data))
    {
        printf("FEARWORLD: unable to read %s\n", pszFilename);
        return 0;
    }
    if (data.size() < 56 || FearReadU32(data.data()) != 113)
    {
        printf("FEARWORLD: unsupported world version\n");
        return 0;
    }

    unsigned int nRenderOffset = FearReadU32(data.data() + 4);
    if (nRenderOffset + 40 > data.size())
    {
        printf("FEARWORLD: invalid render offset\n");
        return 0;
    }

    size_t o = (size_t)nRenderOffset;
    o += 24;
    o += 16;
    if (o + 20 > data.size())
        return 0;

    unsigned int nUnknownId = FearReadU32(data.data() + o);
    unsigned int nSurfaceCount = FearReadU32(data.data() + o + 4);
    unsigned int nMaterialCount = FearReadU32(data.data() + o + 8);
    unsigned int nVertexBlockSize = FearReadU32(data.data() + o + 12);
    unsigned int nTriangleBlockSize = FearReadU32(data.data() + o + 16);
    o += 20;
    if (nUnknownId != 0 || nSurfaceCount == 0 || nSurfaceCount > 100000)
        return 0;
    if (o + nVertexBlockSize + nTriangleBlockSize > data.size())
        return 0;

    const unsigned char *pVertexBlock = data.data() + o;
    o += nVertexBlockSize;
    const unsigned char *pTriangleBlock = data.data() + o;
    size_t nTriangleBase = o;
    o += nTriangleBlockSize;

    unsigned int nVertexDefCount = FearReadU32(data.data() + o);
    o += 4;
    if (nVertexDefCount > 256)
        return 0;
    for (unsigned int i = 0; i < nVertexDefCount; ++i)
    {
        if (o + 4 > data.size())
            return 0;
        unsigned int nSize = FearReadU32(data.data() + o);
        o += 4;
        if (nSize > 4096 || o + nSize > data.size())
            return 0;
        o += nSize;
    }

    if (o + 4 > data.size())
        return 0;
    unsigned int nSurfaces = FearReadU32(data.data() + o);
    o += 4;
    if (nSurfaces != nSurfaceCount || o + (size_t)nSurfaces * 36 > data.size())
        return 0;

    std::vector<std::string> materialNames;
    materialNames.reserve(nMaterialCount);
    size_t nMaterialOffset = o + (size_t)nSurfaces * 36;
    for (unsigned int i = 0; i < nMaterialCount; ++i)
    {
        if (nMaterialOffset + 2 > data.size())
            return 0;
        unsigned int nLength = FearReadU16(data.data() + nMaterialOffset);
        nMaterialOffset += 2;
        if (nMaterialOffset + nLength > data.size())
            return 0;
        materialNames.push_back(std::string((const char *)data.data() + nMaterialOffset, nLength));
        nMaterialOffset += nLength;
    }

    g_WorldBatches.clear();
    g_WorldBatches.resize(materialNames.size());
    for (size_t i = 0; i < g_WorldBatches.size(); ++i)
    {
        FearWorldBatch &batch = g_WorldBatches[i];
        batch.m_Name = materialNames[i];
        batch.m_pVertexBuffer = NULL;
        batch.m_nVertexCount = 0;
        batch.m_Color = FearMaterialColor((unsigned int)i);
        batch.m_pTexture = NULL;
        batch.m_bTextureLoaded = false;
    }

    unsigned int nVisibleSurfaces = 0;
    unsigned int nSkippedSurfaces = 0;
    for (unsigned int i = 0; i < nSurfaces; ++i)
    {
        const unsigned char *pSurface = data.data() + o + (size_t)i * 36;
        unsigned int nVertexStart = FearReadU32(pSurface);
        unsigned int nVertices = FearReadU32(pSurface + 4);
        unsigned int nVertexSize = FearReadU32(pSurface + 8);
        unsigned int nIndexStart = FearReadU32(pSurface + 12);
        unsigned int nIndexBase = FearReadU32(pSurface + 16);
        unsigned int nIndexCount = FearReadU32(pSurface + 20);
        unsigned int nMaterial = FearReadU32(pSurface + 24);

        if (nMaterial < materialNames.size() && FearIsShadowVolumeMaterial(materialNames[nMaterial]))
        {
            ++nSkippedSurfaces;
            continue;
        }
        ++nVisibleSurfaces;

        if (nVertexSize < 12 || nVertices == 0)
            continue;
        if ((size_t)nVertexStart * nVertexSize + (size_t)nVertices * nVertexSize > nVertexBlockSize)
            continue;
        if ((size_t)(nIndexStart + nIndexCount) * 2 > nTriangleBlockSize)
            continue;

        for (unsigned int nIndex = 0; nIndex + 2 < nIndexCount; nIndex += 3)
        {
            size_t nTriangleOffset = nTriangleBase + (size_t)(nIndexStart + nIndex) * 2;
            FearAddTriangle(pVertexBlock, nVertexBlockSize, nVertexStart, nVertexSize, nVertices,
                nIndexBase, nMaterial,
                FearReadU16(data.data() + nTriangleOffset),
                FearReadU16(data.data() + nTriangleOffset + 2),
                FearReadU16(data.data() + nTriangleOffset + 4));
        }
    }

    unsigned int nVertexCount = 0;
    unsigned int nUsedMaterials = 0;
    g_WorldMin[0] = g_WorldMin[1] = g_WorldMin[2] = 1.0e30f;
    g_WorldMax[0] = g_WorldMax[1] = g_WorldMax[2] = -1.0e30f;
    for (size_t i = 0; i < g_WorldBatches.size(); ++i)
    {
        FearWorldBatch &batch = g_WorldBatches[i];
        if (batch.m_Vertices.empty())
            continue;
        ++nUsedMaterials;
        nVertexCount += (unsigned int)batch.m_Vertices.size();
        for (size_t j = 0; j < batch.m_Vertices.size(); ++j)
        {
            const FearWorldVertex &v = batch.m_Vertices[j];
            if (v.x < g_WorldMin[0]) g_WorldMin[0] = v.x;
            if (v.y < g_WorldMin[1]) g_WorldMin[1] = v.y;
            if (v.z < g_WorldMin[2]) g_WorldMin[2] = v.z;
            if (v.x > g_WorldMax[0]) g_WorldMax[0] = v.x;
            if (v.y > g_WorldMax[1]) g_WorldMax[1] = v.y;
            if (v.z > g_WorldMax[2]) g_WorldMax[2] = v.z;
        }
    }

    if (nVertexCount == 0)
    {
        printf("FEARWORLD: no triangles parsed\n");
        return 0;
    }

    printf("FEARWORLD: surfaces=%u visible_surfaces=%u skipped_shadow_surfaces=%u materials=%u used_materials=%u vertices=%u triangles=%u bounds=(%.0f,%.0f,%.0f)-(%.0f,%.0f,%.0f)\n",
        nSurfaceCount, nVisibleSurfaces, nSkippedSurfaces, nMaterialCount, nUsedMaterials,
        nVertexCount, nVertexCount / 3,
        g_WorldMin[0], g_WorldMin[1], g_WorldMin[2], g_WorldMax[0], g_WorldMax[1], g_WorldMax[2]);
    fflush(stdout);
    return 1;
}

struct FearMatrix
{
    float m[16];
};

static FearMatrix FearIdentity()
{
    FearMatrix r;
    memset(&r, 0, sizeof(r));
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

static FearMatrix FearLookAt(const float eye[3], const float target[3], const float up[3])
{
    float zx = target[0] - eye[0], zy = target[1] - eye[1], zz = target[2] - eye[2];
    float zl = sqrtf(zx * zx + zy * zy + zz * zz);
    if (zl < 1.0e-6f) zl = 1.0f;
    zx /= zl; zy /= zl; zz /= zl;
    float xx = up[1] * zz - up[2] * zy;
    float xy = up[2] * zx - up[0] * zz;
    float xz = up[0] * zy - up[1] * zx;
    float xl = sqrtf(xx * xx + xy * xy + xz * xz);
    if (xl < 1.0e-6f) xl = 1.0f;
    xx /= xl; xy /= xl; xz /= xl;
    float yx = zy * xz - zz * xy;
    float yy = zz * xx - zx * xz;
    float yz = zx * xy - zy * xx;
    FearMatrix r;
    r.m[0] = xx; r.m[1] = yx; r.m[2] = zx; r.m[3] = 0;
    r.m[4] = xy; r.m[5] = yy; r.m[6] = zy; r.m[7] = 0;
    r.m[8] = xz; r.m[9] = yz; r.m[10] = zz; r.m[11] = 0;
    r.m[12] = -(xx * eye[0] + xy * eye[1] + xz * eye[2]);
    r.m[13] = -(yx * eye[0] + yy * eye[1] + yz * eye[2]);
    r.m[14] = -(zx * eye[0] + zy * eye[1] + zz * eye[2]);
    r.m[15] = 1;
    return r;
}

static FearMatrix FearPerspective(float fFovY, float fAspect, float fNear, float fFar)
{
    float f = 1.0f / tanf(fFovY * 0.5f);
    FearMatrix r;
    memset(&r, 0, sizeof(r));
    r.m[0] = f / fAspect;
    r.m[5] = f;
    r.m[10] = fFar / (fFar - fNear);
    r.m[11] = 1.0f;
    r.m[14] = (-fNear * fFar) / (fFar - fNear);
    return r;
}

static IDirect3DTexture9 *FearWorldLoadDDS(IDirect3DDevice9 *pDevice, const char *pszFilename)
{
    if (!pDevice || !pszFilename || !pszFilename[0])
        return NULL;

    std::vector<unsigned char> data;
    if (!FearReadFile(pszFilename, data))
        return NULL;

    FearDDSImage image = {0, 0, NULL};
    if (!FearDDS_Load(data.data(), (unsigned int)data.size(), image))
        return NULL;
    if (image.m_Width == 0 || image.m_Height == 0)
    {
        FearDDS_Free(image);
        return NULL;
    }

    IDirect3DTexture9 *pTexture = NULL;
    if (FAILED(pDevice->CreateTexture(image.m_Width, image.m_Height, 1, 0,
        D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, NULL)))
    {
        FearDDS_Free(image);
        return NULL;
    }

    D3DLOCKED_RECT locked;
    memset(&locked, 0, sizeof(locked));
    if (FAILED(pTexture->LockRect(0, &locked, NULL, 0)))
    {
        pTexture->Release();
        FearDDS_Free(image);
        return NULL;
    }

    for (unsigned int y = 0; y < image.m_Height; ++y)
    {
        unsigned char *pDest = (unsigned char *)locked.pBits + (size_t)y * locked.Pitch;
        const unsigned char *pSource = image.m_pPixels + (size_t)y * image.m_Width * 4;
        for (unsigned int x = 0; x < image.m_Width; ++x)
        {
            pDest[x * 4 + 0] = pSource[x * 4 + 2];
            pDest[x * 4 + 1] = pSource[x * 4 + 1];
            pDest[x * 4 + 2] = pSource[x * 4 + 0];
            pDest[x * 4 + 3] = pSource[x * 4 + 3];
        }
    }

    pTexture->UnlockRect(0);
    FearDDS_Free(image);
    return pTexture;
}

static void FearWorldLoadTextures(IDirect3DDevice9 *pDevice)
{
    unsigned int nLoaded = 0;
    for (size_t i = 0; i < g_WorldBatches.size(); ++i)
    {
        FearWorldBatch &batch = g_WorldBatches[i];
        if (batch.m_Vertices.empty() || batch.m_bTextureLoaded)
            continue;

        batch.m_bTextureLoaded = true;
        std::string textureName;
        if (!FearReadMaterialTexture(batch.m_Name.c_str(), textureName))
            continue;

        batch.m_pTexture = FearWorldLoadDDS(pDevice, textureName.c_str());
        if (batch.m_pTexture)
            ++nLoaded;
    }

    printf("FEARWORLD: loaded %u material textures\n", nLoaded);
    fflush(stdout);
}

static bool FearWorldCreateBatchBuffer(IDirect3DDevice9 *pDevice, FearWorldBatch &batch)
{
    if (batch.m_Vertices.empty())
        return false;
    if (batch.m_pVertexBuffer)
        return true;

    if (FAILED(pDevice->CreateVertexBuffer((UINT)(batch.m_Vertices.size() * sizeof(FearWorldVertex)),
        D3DUSAGE_WRITEONLY, D3DFVF_XYZ | D3DFVF_TEX1, D3DPOOL_MANAGED, &batch.m_pVertexBuffer, NULL)))
    {
        return false;
    }

    void *pData = NULL;
    if (FAILED(batch.m_pVertexBuffer->Lock(0, 0, &pData, 0)))
        return false;
    memcpy(pData, batch.m_Vertices.data(), batch.m_Vertices.size() * sizeof(FearWorldVertex));
    batch.m_pVertexBuffer->Unlock();
    batch.m_nVertexCount = (unsigned int)batch.m_Vertices.size();
    return true;
}

void FearWorld_Render(IDirect3DDevice9 *pDevice)
{
    if (!pDevice)
        return;
    if (!g_bWorldLoaded)
        FearWorld_Load("Worlds\\Release\\Intro.World00p");
    if (g_WorldBatches.empty())
        return;

    static bool s_bTexturesLoaded = false;
    if (!s_bTexturesLoaded)
    {
        s_bTexturesLoaded = true;
        FearWorldLoadTextures(pDevice);
    }

    float eye[3] = { g_CameraPos[0], g_CameraPos[1], g_CameraPos[2] };
    float cosPitch = cosf(g_CameraPitch);
    float target[3] = {
        eye[0] + sinf(g_CameraYaw) * cosPitch * 2000.0f,
        eye[1] + sinf(g_CameraPitch) * 2000.0f,
        eye[2] + cosf(g_CameraYaw) * cosPitch * 2000.0f
    };
    float up[3] = { 0.0f, 1.0f, 0.0f };

    pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff101018, 1.0f, 0);
    pDevice->SetVertexShader(NULL);
    pDevice->SetPixelShader(NULL);
    pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

    FearMatrix view = FearLookAt(eye, target, up);
    FearMatrix proj = FearPerspective(1.2f, 640.0f / 480.0f, 1.0f, 200000.0f);
    pDevice->SetTransform(D3DTS_VIEW, (const D3DMATRIX *)&view);
    pDevice->SetTransform(D3DTS_PROJECTION, (const D3DMATRIX *)&proj);

    for (size_t i = 0; i < g_WorldBatches.size(); ++i)
    {
        FearWorldBatch &batch = g_WorldBatches[i];
        if (!FearWorldCreateBatchBuffer(pDevice, batch))
            continue;

        pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, batch.m_pTexture ? 0xffffffff : batch.m_Color);
        pDevice->SetTexture(0, batch.m_pTexture);
        pDevice->SetStreamSource(0, batch.m_pVertexBuffer, 0, sizeof(FearWorldVertex));
        pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, batch.m_nVertexCount / 3);
    }
}

void FearWorld_SetCamera(const float *pPos, float fYaw, float fPitch)
{
    if (pPos)
    {
        g_CameraPos[0] = pPos[0];
        g_CameraPos[1] = pPos[1];
        g_CameraPos[2] = pPos[2];
    }
    g_CameraYaw = fYaw;
    if (fPitch > 1.35f)
        fPitch = 1.35f;
    if (fPitch < -1.35f)
        fPitch = -1.35f;
    g_CameraPitch = fPitch;
}

void FearWorld_Shutdown()
{
    for (size_t i = 0; i < g_WorldBatches.size(); ++i)
    {
        FearWorldBatch &batch = g_WorldBatches[i];
        if (batch.m_pVertexBuffer)
        {
            batch.m_pVertexBuffer->Release();
            batch.m_pVertexBuffer = NULL;
        }
        if (batch.m_pTexture)
        {
            batch.m_pTexture->Release();
            batch.m_pTexture = NULL;
        }
        batch.m_Vertices.clear();
    }
    g_WorldBatches.clear();
    g_bWorldLoaded = false;
}
