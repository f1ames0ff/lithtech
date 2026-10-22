#include "fear_world.h"
#include "cshell_filemgr.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <string>
#include <new>

struct FearWorldVertex
{
    float x, y, z;
    DWORD color;
};

static std::vector<FearWorldVertex> g_WorldVertices;
static IDirect3DVertexBuffer9 *g_pWorldVB = NULL;
static UINT g_nWorldVBVertices = 0;
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

    unsigned char colorR = (unsigned char)(64 + ((nMaterial * 37) % 160));
    unsigned char colorG = (unsigned char)(64 + ((nMaterial * 73) % 160));
    unsigned char colorB = (unsigned char)(64 + ((nMaterial * 151) % 160));
    DWORD color = 0xff000000 | ((DWORD)colorR << 16) | ((DWORD)colorG << 8) | colorB;

    for (int i = 0; i < 3; ++i)
    {
        size_t nOffset = (size_t)nVertexStart * nVertexSize + (size_t)nIds[i] * nVertexSize;
        if (nOffset + 12 > nVertexBlockSize)
            return;
        FearWorldVertex v;
        v.x = FearReadF32(pVertexBlock + nOffset);
        v.y = FearReadF32(pVertexBlock + nOffset + 4);
        v.z = FearReadF32(pVertexBlock + nOffset + 8);
        v.color = color;
        g_WorldVertices.push_back(v);
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

    if (g_WorldVertices.empty())
    {
        printf("FEARWORLD: no triangles parsed\n");
        return 0;
    }

    g_WorldMin[0] = g_WorldMin[1] = g_WorldMin[2] = 1.0e30f;
    g_WorldMax[0] = g_WorldMax[1] = g_WorldMax[2] = -1.0e30f;
    for (size_t i = 0; i < g_WorldVertices.size(); ++i)
    {
        const FearWorldVertex &v = g_WorldVertices[i];
        if (v.x < g_WorldMin[0]) g_WorldMin[0] = v.x;
        if (v.y < g_WorldMin[1]) g_WorldMin[1] = v.y;
        if (v.z < g_WorldMin[2]) g_WorldMin[2] = v.z;
        if (v.x > g_WorldMax[0]) g_WorldMax[0] = v.x;
        if (v.y > g_WorldMax[1]) g_WorldMax[1] = v.y;
        if (v.z > g_WorldMax[2]) g_WorldMax[2] = v.z;
    }

    printf("FEARWORLD: surfaces=%u visible_surfaces=%u skipped_shadow_surfaces=%u materials=%u vertices=%u triangles=%u bounds=(%.0f,%.0f,%.0f)-(%.0f,%.0f,%.0f)\n",
        nSurfaceCount, nVisibleSurfaces, nSkippedSurfaces, nMaterialCount,
        (unsigned)g_WorldVertices.size(), (unsigned)(g_WorldVertices.size() / 3),
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

void FearWorld_Render(IDirect3DDevice9 *pDevice)
{
    if (!pDevice)
        return;
    if (!g_bWorldLoaded)
        FearWorld_Load("Worlds\\Release\\Intro.World00p");
    if (g_WorldVertices.empty())
        return;

    if (!g_pWorldVB)
    {
        if (FAILED(pDevice->CreateVertexBuffer((UINT)(g_WorldVertices.size() * sizeof(FearWorldVertex)),
            D3DUSAGE_WRITEONLY, D3DFVF_XYZ | D3DFVF_DIFFUSE, D3DPOOL_MANAGED, &g_pWorldVB, NULL)))
        {
            printf("FEARWORLD: vertex buffer creation failed\n");
            return;
        }
        void *pData = NULL;
        if (FAILED(g_pWorldVB->Lock(0, 0, &pData, 0)))
            return;
        memcpy(pData, g_WorldVertices.data(), g_WorldVertices.size() * sizeof(FearWorldVertex));
        g_pWorldVB->Unlock();
        g_nWorldVBVertices = (UINT)g_WorldVertices.size();
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
    pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    pDevice->SetTexture(0, NULL);

    FearMatrix view = FearLookAt(eye, target, up);
    FearMatrix proj = FearPerspective(1.2f, 640.0f / 480.0f, 1.0f, 200000.0f);
    pDevice->SetTransform(D3DTS_VIEW, (const D3DMATRIX *)&view);
    pDevice->SetTransform(D3DTS_PROJECTION, (const D3DMATRIX *)&proj);

    pDevice->SetStreamSource(0, g_pWorldVB, 0, sizeof(FearWorldVertex));
    pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, g_nWorldVBVertices / 3);
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
    if (g_pWorldVB)
    {
        g_pWorldVB->Release();
        g_pWorldVB = NULL;
    }
    g_WorldVertices.clear();
    g_bWorldLoaded = false;
}
