#include "fear_world.h"
#include "cshell_filemgr.h"
#include "fear_host_api.h"
#include "fear_render_api.h"
#include "dds_loader.h"
#include "fear_model.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <string>
#include <new>

struct FearWorldVertex
{
    float x, y, z;
    float nx, ny, nz;
    DWORD diffuse;
    float u, v;
};

struct FearWorldVertexLayout
{
    unsigned int m_nStride;
    int m_nUVOffset;
    int m_nNormalOffset;
    int m_nColorOffset;
};

struct FearWorldBatch
{
    std::string m_Name;
    std::vector<FearWorldVertex> m_Vertices;
    IDirect3DVertexBuffer9 *m_pVertexBuffer;
    unsigned int m_nVertexCount;
    DWORD m_Color;
    IDirect3DTexture9 *m_pTexture;
    IDirect3DTexture9 *m_pNormalTexture;
    IDirect3DTexture9 *m_pSpecularTexture;
    IDirect3DTexture9 *m_pEmissiveTexture;
    bool m_bTextureLoaded;
    float m_Center[3];
    float m_Radius;
};

#define D3DFVF_FEARWORLD (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1)

static std::vector<FearWorldBatch> g_WorldBatches;
static bool g_bWorldLoaded = false;
static float g_WorldMin[3] = {0, 0, 0};
static float g_WorldMax[3] = {0, 0, 0};
static float g_CameraPos[3] = { -1100.0f, -1372.0f, -370.0f };
static float g_CameraYaw = -0.0784588f;
static float g_CameraPitch = 0.0f;
static float g_CameraFovX = 0.0f;
static float g_CameraFovY = 0.0f;

static DWORD FearWorldCullMode()
{
    static DWORD s_nMode = 0xFFFFFFFF;
    if (s_nMode == 0xFFFFFFFF)
    {
        char szValue[32] = {0};
        DWORD nLength = GetEnvironmentVariableA("FEAR_CULL", szValue, sizeof(szValue));
        s_nMode = D3DCULL_CCW;
        if (nLength > 0 && nLength < sizeof(szValue))
        {
            if (lstrcmpiA(szValue, "cw") == 0)
                s_nMode = D3DCULL_CW;
            else if (lstrcmpiA(szValue, "none") == 0)
                s_nMode = D3DCULL_NONE;
        }
        printf("FEARWORLD: cull mode %s (%lu)\n",
            s_nMode == D3DCULL_CW ? "cw" : (s_nMode == D3DCULL_CCW ? "ccw" : "none"),
            (unsigned long)s_nMode);
        fflush(stdout);
    }
    return s_nMode;
}

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

static bool FearMaterialDumpEnabled()
{
    static int s_nEnabled = -1;
    if (s_nEnabled < 0)
    {
        char szValue[8] = {0};
        s_nEnabled = GetEnvironmentVariableA("FEAR_MATDUMP", szValue, sizeof(szValue)) > 0 ? 1 : 0;
        if (s_nEnabled)
            printf("FEARWORLD: material dump enabled\n");
    }
    return s_nEnabled != 0;
}

struct FearMaterialMaps
{
    std::string m_Diffuse;
    std::string m_Normal;
    std::string m_Specular;
    std::string m_Emissive;
    std::string m_Shader;
};

static bool FearReadMaterialTexture(const char *pszMaterial, FearMaterialMaps &maps)
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
    static unsigned int s_nDumped = 0;
    bool bDump = FearMaterialDumpEnabled() && s_nDumped < 12;
    if (bDump || nShaderLength > 0)
        maps.m_Shader.assign((const char *)data.data() + o, nShaderLength);
    o += nShaderLength;

    if (o + 4 > data.size())
        return false;
    unsigned int nParams = FearReadU32(data.data() + o);
    o += 4;
    if (nParams > 1024)
        return false;

    if (bDump)
    {
        ++s_nDumped;
        printf("FEARWORLD: material %s shader=%s params=%u\n", pszMaterial, maps.m_Shader.c_str(), nParams);
        fflush(stdout);
    }

    std::string firstName;
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
            if (bDump)
            {
                printf("FEARWORLD:   %s = %s\n", name.c_str(), value.c_str());
                fflush(stdout);
            }
            if (FearHasDDSExtension(value.c_str()))
            {
                if (maps.m_Diffuse.empty())
                    maps.m_Diffuse = value;
                if (_stricmp(name.c_str(), "tDiffuseMap") == 0)
                    maps.m_Diffuse = value;
                else if (_stricmp(name.c_str(), "tNormalMap") == 0)
                    maps.m_Normal = value;
                else if (_stricmp(name.c_str(), "tSpecularMap") == 0)
                    maps.m_Specular = value;
                else if (_stricmp(name.c_str(), "tEmissiveMap") == 0)
                    maps.m_Emissive = value;
            }
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

    if (maps.m_Diffuse.empty())
        maps.m_Diffuse = firstName;
    return !maps.m_Diffuse.empty();
}

static bool FearReadVertexUV(const unsigned char *pVertex, const FearWorldVertexLayout &layout, float &u, float &v)
{
    if (layout.m_nUVOffset < 0)
    {
        u = 0.0f;
        v = 0.0f;
        return false;
    }

    unsigned int nOffset = (unsigned int)layout.m_nUVOffset;
    if (nOffset + 8 > layout.m_nStride)
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

struct FearWorldLight
{
    float m_Pos[3];
    float m_Color[3];
    float m_Dir[3];
    float m_Radius;
    float m_Intensity;
    float m_FovX;
    float m_FovY;
    int m_nClass;
    bool m_bHasDir;
};

static std::vector<FearWorldLight> g_WorldLights;
static bool g_bLightBake = false;
static float g_WorldAmbient[3] = { 25.0f, 25.0f, 25.0f };
static float g_WorldStartPos[3] = { 0.0f, 0.0f, 0.0f };
static bool g_bWorldHasStart = false;
static bool g_bWorldFogEnabled = false;
static float g_WorldFogColor[3] = { 127.0f, 127.0f, 127.0f };
static float g_WorldFogNear = 1.0f;
static float g_WorldFogFar = 5000.0f;

static void FearQuatDirection(const float *pQuat, float *pOut)
{
    float x = pQuat[0], y = pQuat[1], z = pQuat[2], w = pQuat[3];
    pOut[0] = 2.0f * (x * z + w * y);
    pOut[1] = 2.0f * (y * z - w * x);
    pOut[2] = 1.0f - 2.0f * (x * x + y * y);
}

static bool FearBlobFloats(const unsigned char *pBlob, unsigned int nBlobSize, int nOffset, float *pOut, int nCount)
{
    if (nOffset < 0 || (unsigned int)nOffset + (unsigned int)nCount * 4u > nBlobSize)
        return false;
    for (int i = 0; i < nCount; ++i)
        pOut[i] = FearReadF32(pBlob + nOffset + i * 4);
    return true;
}

static const char *FearBlobString(const unsigned char *pBlob, unsigned int nBlobSize, int nOffset)
{
    if (nOffset < 0 || (unsigned int)nOffset >= nBlobSize)
        return "";
    return (const char *)pBlob + nOffset;
}

static int FearLightClassFromName(const char *pszClass)
{
    if (strcmp(pszClass, "LightPoint") == 0) return 1;
    if (strcmp(pszClass, "LightSpot") == 0) return 2;
    if (strcmp(pszClass, "LightDirectional") == 0) return 3;
    if (strcmp(pszClass, "LightCube") == 0) return 4;
    return 0;
}

static bool FearLightDumpEnabled()
{
    static int s_nEnabled = -1;
    if (s_nEnabled < 0)
    {
        char szValue[8] = {0};
        s_nEnabled = GetEnvironmentVariableA("FEAR_LIGHTDUMP", szValue, sizeof(szValue)) > 0 ? 1 : 0;
    }
    return s_nEnabled != 0;
}

static void FearDumpProperty(const char *pszClass, const char *pszName, int nType,
    const unsigned char *pBlob, unsigned int nBlobLength, int nValueOffset, unsigned int nInline)
{
    if (nType == 0 || nType == 7 || nType == 8)
    {
        printf("LIGHTDUMP: %s.%s type=%d str=%s\n", pszClass, pszName, nType,
            FearBlobString(pBlob, nBlobLength, nValueOffset));
    }
    else if (nType == 1 || nType == 2 || nType == 6)
    {
        float f[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        FearBlobFloats(pBlob, nBlobLength, nValueOffset, f, nType == 1 ? 3 : 4);
        printf("LIGHTDUMP: %s.%s type=%d vec=(%.4f,%.4f,%.4f,%.4f)\n", pszClass, pszName, nType,
            f[0], f[1], f[2], f[3]);
    }
    else
    {
        printf("LIGHTDUMP: %s.%s type=%d raw=%u float=%.4f\n", pszClass, pszName, nType,
            nInline, *(const float *)&nInline);
    }
    fflush(stdout);
}

static void FearWorldParseObjects(const unsigned char *pData, size_t nSize, size_t nObjectOffset)
{
    if (nObjectOffset + 4 > nSize)
        return;

    unsigned int nCount = FearReadU32(pData + nObjectOffset);
    size_t o = nObjectOffset + 4;
    unsigned int nParsed = 0;
    unsigned int nLightCounts[5] = { 0, 0, 0, 0, 0 };
    unsigned int nLightDumps[5] = { 0, 0, 0, 0, 0 };

    for (unsigned int i = 0; i < nCount; ++i)
    {
        if (o + 2 > nSize)
            break;

        unsigned int nNameLength = FearReadU16(pData + o);
        o += 2;
        if (o + nNameLength > nSize)
            break;
        char szClass[128] = {0};
        unsigned int nCopy = nNameLength < sizeof(szClass) - 1 ? nNameLength : sizeof(szClass) - 1;
        memcpy(szClass, pData + o, nCopy);
        o += nNameLength;

        if (o + 8 > nSize)
            break;
        unsigned int nPropertyCount = FearReadU32(pData + o);
        o += 4;
        unsigned int nBlobLength = FearReadU32(pData + o);
        o += 4;
        if (o + nBlobLength > nSize || nPropertyCount > 4096)
            break;

        const unsigned char *pBlob = pData + o;
        o += nBlobLength;

        int nLightClass = FearLightClassFromName(szClass);
        FearWorldLight light;
        memset(&light, 0, sizeof(light));
        light.m_nClass = nLightClass;
        bool bHasPos = false;
        bool bHasColor = false;
        bool bHasRadius = false;
        float pos[3] = { 0.0f, 0.0f, 0.0f };
        bool bDumpLight = nLightClass != 0 && FearLightDumpEnabled() && nLightDumps[nLightClass] < 4;
        if (bDumpLight)
        {
            ++nLightDumps[nLightClass];
            printf("LIGHTDUMP: --- %s\n", szClass);
        }

        for (unsigned int j = 0; j < nPropertyCount; ++j)
        {
            if (o + 8 > nSize)
                break;
            int nNameOffset = (int)FearReadU32(pData + o);
            o += 4;
            int nType = (int)FearReadU32(pData + o);
            o += 4;
            if (o + 4 > nSize)
                break;
            int nValueOffset = (int)FearReadU32(pData + o);
            o += 4;

            const char *pszProperty = FearBlobString(pBlob, nBlobLength, nNameOffset);

            if (bDumpLight)
                FearDumpProperty(szClass, pszProperty, nType, pBlob, nBlobLength, nValueOffset,
                    FearReadU32(pData + o - 4));

            if (strcmp(pszProperty, "Pos") == 0 && nType == 1)
            {
                if (FearBlobFloats(pBlob, nBlobLength, nValueOffset, pos, 3))
                    bHasPos = true;
            }
            else if (strcmp(pszProperty, "LightColor") == 0 && nType == 2)
            {
                if (FearBlobFloats(pBlob, nBlobLength, nValueOffset, light.m_Color, 3))
                    bHasColor = true;
            }
            else if (strcmp(pszProperty, "LightRadius") == 0 && nType == 3)
            {
                light.m_Radius = FearReadF32(pData + o - 4);
                bHasRadius = true;
            }
            else if (strcmp(pszProperty, "IntensityScale") == 0 && nType == 3)
            {
                light.m_Intensity = FearReadF32(pData + o - 4);
            }
            else if (strcmp(pszProperty, "Rotation") == 0 && nType == 6)
            {
                float quat[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                if (FearBlobFloats(pBlob, nBlobLength, nValueOffset, quat, 4))
                {
                    FearQuatDirection(quat, light.m_Dir);
                    light.m_bHasDir = true;
                }
            }
            else if (strcmp(pszProperty, "FovX") == 0 && nType == 3)
            {
                light.m_FovX = FearReadF32(pData + o - 4);
            }
            else if (strcmp(pszProperty, "FovY") == 0 && nType == 3)
            {
                light.m_FovY = FearReadF32(pData + o - 4);
            }
            else if (strcmp(pszProperty, "AmbientLight") == 0 && nType == 2)
            {
                FearBlobFloats(pBlob, nBlobLength, nValueOffset, g_WorldAmbient, 3);
            }
            else if (strcmp(pszProperty, "FogEnable") == 0 && nType == 4)
            {
                g_bWorldFogEnabled = FearReadU32(pData + o - 4) != 0;
            }
            else if (strcmp(pszProperty, "FogColor") == 0 && nType == 2)
            {
                FearBlobFloats(pBlob, nBlobLength, nValueOffset, g_WorldFogColor, 3);
            }
            else if (strcmp(pszProperty, "FogNearZ") == 0 && nType == 3)
            {
                g_WorldFogNear = FearReadF32(pData + o - 4);
            }
            else if (strcmp(pszProperty, "FogFarZ") == 0 && nType == 3)
            {
                g_WorldFogFar = FearReadF32(pData + o - 4);
            }
        }

        if (nLightClass != 0)
        {
            ++nLightCounts[nLightClass];
            if (bHasPos)
            {
                light.m_Pos[0] = pos[0];
                light.m_Pos[1] = pos[1];
                light.m_Pos[2] = pos[2];
                if (!bHasColor)
                {
                    light.m_Color[0] = 255.0f;
                    light.m_Color[1] = 255.0f;
                    light.m_Color[2] = 255.0f;
                }
                if (!bHasRadius || light.m_Radius <= 0.0f)
                    light.m_Radius = 300.0f;
                g_WorldLights.push_back(light);
            }
        }
        else if (strcmp(szClass, "GameStartPoint") == 0 && bHasPos && !g_bWorldHasStart)
        {
            g_WorldStartPos[0] = pos[0];
            g_WorldStartPos[1] = pos[1];
            g_WorldStartPos[2] = pos[2];
            g_bWorldHasStart = true;
        }

        ++nParsed;
    }

    float fMaxRadius = 0.0f;
    for (size_t i = 0; i < g_WorldLights.size(); ++i)
    {
        if (g_WorldLights[i].m_Radius > fMaxRadius)
            fMaxRadius = g_WorldLights[i].m_Radius;
    }

    printf("FEARWORLD: objects=%u/%u lights: point=%u spot=%u directional=%u cube=%u max_radius=%.0f\n",
        nParsed, nCount, nLightCounts[1], nLightCounts[2], nLightCounts[3], nLightCounts[4], fMaxRadius);
    printf("FEARWORLD: ambient=(%.0f,%.0f,%.0f) fog=%d color=(%.0f,%.0f,%.0f) near=%.0f far=%.0f start=(%.0f,%.0f,%.0f)\n",
        g_WorldAmbient[0], g_WorldAmbient[1], g_WorldAmbient[2],
        g_bWorldFogEnabled ? 1 : 0,
        g_WorldFogColor[0], g_WorldFogColor[1], g_WorldFogColor[2],
        g_WorldFogNear, g_WorldFogFar,
        g_bWorldHasStart ? g_WorldStartPos[0] : 0.0f,
        g_bWorldHasStart ? g_WorldStartPos[1] : 0.0f,
        g_bWorldHasStart ? g_WorldStartPos[2] : 0.0f);
    fflush(stdout);
}

static bool FearLightBakeEnabled()
{
    static int s_nEnabled = -1;
    if (s_nEnabled < 0)
    {
        char szValue[8] = {0};
        s_nEnabled = GetEnvironmentVariableA("FEAR_LIGHTS", szValue, sizeof(szValue)) > 0 ? (atoi(szValue) != 0 ? 1 : 0) : 0;
    }
    return s_nEnabled != 0;
}

static DWORD FearShadeVertex(const FearWorldVertex &v)
{
    float fR = g_WorldAmbient[0];
    float fG = g_WorldAmbient[1];
    float fB = g_WorldAmbient[2];
    for (size_t i = 0; i < g_WorldLights.size(); ++i)
    {
        const FearWorldLight &light = g_WorldLights[i];
        if (light.m_Radius <= 0.0f)
            continue;
        float dx = light.m_Pos[0] - v.x;
        float dy = light.m_Pos[1] - v.y;
        float dz = light.m_Pos[2] - v.z;
        float fDist2 = dx * dx + dy * dy + dz * dz;
        if (fDist2 >= light.m_Radius * light.m_Radius)
            continue;
        float fDist = sqrtf(fDist2);
        float fInv = 1.0f / (fDist > 0.001f ? fDist : 0.001f);
        float fNdl = (v.nx * dx + v.ny * dy + v.nz * dz) * fInv;
        if (fNdl <= 0.0f)
            continue;
        float fAtten = 1.0f - fDist / light.m_Radius;
        fAtten *= fAtten;
        float fScale = fNdl * fAtten;
        fR += light.m_Color[0] * fScale;
        fG += light.m_Color[1] * fScale;
        fB += light.m_Color[2] * fScale;
    }
    if (fR > 255.0f) fR = 255.0f;
    if (fG > 255.0f) fG = 255.0f;
    if (fB > 255.0f) fB = 255.0f;
    if (fR < 0.0f) fR = 0.0f;
    if (fG < 0.0f) fG = 0.0f;
    if (fB < 0.0f) fB = 0.0f;
    return 0xff000000u | ((DWORD)fR << 16) | ((DWORD)fG << 8) | (DWORD)fB;
}

static void FearAddTriangle(const unsigned char *pVertexBlock, unsigned int nVertexBlockSize,
    const FearWorldVertexLayout &layout, unsigned int nVertexStart, unsigned int nVertices,
    unsigned int nVertexBase, unsigned int nMaterial,
    unsigned int nTri0, unsigned int nTri1, unsigned int nTri2)
{
    unsigned int nVertexSize = layout.m_nStride;
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
        if (nOffset + nVertexSize > nVertexBlockSize)
            return;
        const unsigned char *pVertex = pVertexBlock + nOffset;
        FearWorldVertex v;
        v.x = FearReadF32(pVertex);
        v.y = FearReadF32(pVertex + 4);
        v.z = FearReadF32(pVertex + 8);

        v.nx = 0.0f;
        v.ny = 1.0f;
        v.nz = 0.0f;
        if (layout.m_nNormalOffset >= 0 && (unsigned int)layout.m_nNormalOffset + 12 <= nVertexSize)
        {
            const unsigned char *pNormal = pVertex + layout.m_nNormalOffset;
            v.nx = FearReadF32(pNormal);
            v.ny = FearReadF32(pNormal + 4);
            v.nz = FearReadF32(pNormal + 8);
        }

        v.diffuse = 0xffffffffu;
        if (layout.m_nColorOffset >= 0 && (unsigned int)layout.m_nColorOffset + 4 <= nVertexSize)
        {
            const unsigned char *pColor = pVertex + layout.m_nColorOffset;
            v.diffuse = 0xff000000u | ((DWORD)pColor[0] << 16) | ((DWORD)pColor[1] << 8) | (DWORD)pColor[2];
        }
        else if (g_bLightBake)
        {
            v.diffuse = FearShadeVertex(v);
        }

        v.u = 0.0f;
        v.v = 0.0f;
        FearReadVertexUV(pVertex, layout, v.u, v.v);
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

    FearWorldParseObjects(data.data(), data.size(), (size_t)FearReadU32(data.data() + 12));
    g_bLightBake = FearLightBakeEnabled() && !g_WorldLights.empty();
    printf("FEARWORLD: light bake %s (%u lights)\n", g_bLightBake ? "enabled" : "disabled", (unsigned int)g_WorldLights.size());

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

    std::vector<FearWorldVertexLayout> vertexLayouts;
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

        FearWorldVertexLayout layout;
        layout.m_nStride = 0;
        layout.m_nUVOffset = -1;
        layout.m_nNormalOffset = -1;
        layout.m_nColorOffset = -1;

        unsigned int nProperties = nSize / 8;
        for (unsigned int j = 0; j < nProperties; ++j)
        {
            const unsigned char *pProperty = data.data() + o + (size_t)j * 8;
            unsigned char nFormat = pProperty[4];
            unsigned char nUsage = pProperty[6];
            unsigned int nBytes = 0;
            switch (nFormat)
            {
            case 1: nBytes = 8; break;
            case 2: nBytes = 12; break;
            case 3: nBytes = 16; break;
            case 4: nBytes = 4; break;
            case 5: nBytes = 4; break;
            default: nBytes = 0; break;
            }

            if (nUsage == 5 && nBytes == 8 && layout.m_nUVOffset < 0)
                layout.m_nUVOffset = (int)layout.m_nStride;
            else if (nUsage == 3 && nBytes == 12 && layout.m_nNormalOffset < 0)
                layout.m_nNormalOffset = (int)layout.m_nStride;
            else if (nUsage == 10 && nBytes == 4 && layout.m_nColorOffset < 0)
                layout.m_nColorOffset = (int)layout.m_nStride;

            layout.m_nStride += nBytes;
        }

        vertexLayouts.push_back(layout);
        o += nSize;
    }

    printf("FEARWORLD: vertex types=%u\n", (unsigned int)vertexLayouts.size());
    for (size_t i = 0; i < vertexLayouts.size(); ++i)
    {
        const FearWorldVertexLayout &layout = vertexLayouts[i];
        printf("FEARWORLD:   vtype %u stride=%u uv=%d normal=%d color=%d\n", (unsigned int)i,
            layout.m_nStride, layout.m_nUVOffset, layout.m_nNormalOffset, layout.m_nColorOffset);
    }
    fflush(stdout);

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
        batch.m_pNormalTexture = NULL;
        batch.m_pSpecularTexture = NULL;
        batch.m_pEmissiveTexture = NULL;
        batch.m_bTextureLoaded = false;
        batch.m_Center[0] = batch.m_Center[1] = batch.m_Center[2] = 0.0f;
        batch.m_Radius = 0.0f;
    }

    unsigned int nVisibleSurfaces = 0;
    unsigned int nSkippedSurfaces = 0;
    unsigned int nUnknownVertexTypes = 0;
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
        unsigned int nVertexType = FearReadU32(pSurface + 32);

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

        FearWorldVertexLayout layout;
        layout.m_nStride = nVertexSize;
        layout.m_nUVOffset = -1;
        layout.m_nNormalOffset = -1;
        layout.m_nColorOffset = -1;
        if (nVertexType < vertexLayouts.size() && vertexLayouts[nVertexType].m_nStride == nVertexSize)
        {
            layout = vertexLayouts[nVertexType];
        }
        else
        {
            ++nUnknownVertexTypes;
        }
        for (unsigned int nIndex = 0; nIndex + 2 < nIndexCount; nIndex += 3)
        {
            size_t nTriangleOffset = nTriangleBase + (size_t)(nIndexStart + nIndex) * 2;
            FearAddTriangle(pVertexBlock, nVertexBlockSize, layout, nVertexStart, nVertices,
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
        float fMin[3] = { 1.0e30f, 1.0e30f, 1.0e30f };
        float fMax[3] = { -1.0e30f, -1.0e30f, -1.0e30f };
        for (size_t j = 0; j < batch.m_Vertices.size(); ++j)
        {
            const FearWorldVertex &v = batch.m_Vertices[j];
            if (v.x < g_WorldMin[0]) g_WorldMin[0] = v.x;
            if (v.y < g_WorldMin[1]) g_WorldMin[1] = v.y;
            if (v.z < g_WorldMin[2]) g_WorldMin[2] = v.z;
            if (v.x > g_WorldMax[0]) g_WorldMax[0] = v.x;
            if (v.y > g_WorldMax[1]) g_WorldMax[1] = v.y;
            if (v.z > g_WorldMax[2]) g_WorldMax[2] = v.z;
            if (v.x < fMin[0]) fMin[0] = v.x;
            if (v.y < fMin[1]) fMin[1] = v.y;
            if (v.z < fMin[2]) fMin[2] = v.z;
            if (v.x > fMax[0]) fMax[0] = v.x;
            if (v.y > fMax[1]) fMax[1] = v.y;
            if (v.z > fMax[2]) fMax[2] = v.z;
        }
        batch.m_Center[0] = (fMin[0] + fMax[0]) * 0.5f;
        batch.m_Center[1] = (fMin[1] + fMax[1]) * 0.5f;
        batch.m_Center[2] = (fMin[2] + fMax[2]) * 0.5f;
        float fSpanX = fMax[0] - fMin[0];
        float fSpanY = fMax[1] - fMin[1];
        float fSpanZ = fMax[2] - fMin[2];
        batch.m_Radius = 0.5f * sqrtf(fSpanX * fSpanX + fSpanY * fSpanY + fSpanZ * fSpanZ);
    }

    if (nVertexCount == 0)
    {
        printf("FEARWORLD: no triangles parsed\n");
        return 0;
    }

    printf("FEARWORLD: surfaces=%u visible_surfaces=%u skipped_shadow_surfaces=%u unknown_vertex_types=%u materials=%u used_materials=%u vertices=%u triangles=%u bounds=(%.0f,%.0f,%.0f)-(%.0f,%.0f,%.0f)\n",
        nSurfaceCount, nVisibleSurfaces, nSkippedSurfaces, nUnknownVertexTypes, nMaterialCount, nUsedMaterials,
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

static FearMatrix FearMatrixMultiply(const FearMatrix &a, const FearMatrix &b)
{
    FearMatrix r;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            r.m[i * 4 + j] = a.m[i * 4 + 0] * b.m[0 * 4 + j] +
                             a.m[i * 4 + 1] * b.m[1 * 4 + j] +
                             a.m[i * 4 + 2] * b.m[2 * 4 + j] +
                             a.m[i * 4 + 3] * b.m[3 * 4 + j];
        }
    }
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

    static int s_nNmDumpLeft = -1;
    if (s_nNmDumpLeft < 0)
        s_nNmDumpLeft = GetEnvironmentVariableA("FEAR_NMDUMP", NULL, 0) > 0 ? 10 : 0;
    if (s_nNmDumpLeft > 0)
    {
        --s_nNmDumpLeft;
        double sum[3] = {0.0, 0.0, 0.0};
        double sumA = 0.0;
        double sumSq[4] = {0.0, 0.0, 0.0, 0.0};
        size_t count = (size_t)image.m_Width * image.m_Height;
        for (size_t i = 0; i < count; ++i)
        {
            for (int c = 0; c < 3; ++c)
            {
                double v = image.m_pPixels[i * 4 + c];
                sum[c] += v;
                sumSq[c] += v * v;
            }
            sumA += image.m_pPixels[i * 4 + 3];
            sumSq[3] += (double)image.m_pPixels[i * 4 + 3] * image.m_pPixels[i * 4 + 3];
        }
        char szFourCC[5] = {0};
        if (data.size() > 88)
            memcpy(szFourCC, data.data() + 84, 4);
        double meanA = sumA / count;
        printf("FEARWORLD: tex %s %ux%u fourcc=%s mean=(%.1f %.1f %.1f a=%.1f) sd=(%.1f %.1f %.1f a=%.1f)\n",
            pszFilename, image.m_Width, image.m_Height, szFourCC,
            sum[0] / count, sum[1] / count, sum[2] / count, meanA,
            sqrt(sumSq[0] / count - (sum[0] / count) * (sum[0] / count)),
            sqrt(sumSq[1] / count - (sum[1] / count) * (sum[1] / count)),
            sqrt(sumSq[2] / count - (sum[2] / count) * (sum[2] / count)),
            sqrt(sumSq[3] / count - meanA * meanA));
        fflush(stdout);
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
    unsigned int nNormal = 0;
    unsigned int nSpecular = 0;
    unsigned int nEmissive = 0;
    for (size_t i = 0; i < g_WorldBatches.size(); ++i)
    {
        FearWorldBatch &batch = g_WorldBatches[i];
        if (batch.m_Vertices.empty() || batch.m_bTextureLoaded)
            continue;

        batch.m_bTextureLoaded = true;
        FearMaterialMaps maps;
        if (!FearReadMaterialTexture(batch.m_Name.c_str(), maps))
            continue;

        batch.m_pTexture = FearWorldLoadDDS(pDevice, maps.m_Diffuse.c_str());
        if (batch.m_pTexture)
            ++nLoaded;
        batch.m_pNormalTexture = FearWorldLoadDDS(pDevice, maps.m_Normal.c_str());
        if (batch.m_pNormalTexture)
            ++nNormal;
        batch.m_pSpecularTexture = FearWorldLoadDDS(pDevice, maps.m_Specular.c_str());
        if (batch.m_pSpecularTexture)
            ++nSpecular;
        batch.m_pEmissiveTexture = FearWorldLoadDDS(pDevice, maps.m_Emissive.c_str());
        if (batch.m_pEmissiveTexture)
            ++nEmissive;
    }

    printf("FEARWORLD: loaded %u material textures (normal=%u specular=%u emissive=%u)\n",
        nLoaded, nNormal, nSpecular, nEmissive);
    fflush(stdout);
}

static bool FearWorldCreateBatchBuffer(IDirect3DDevice9 *pDevice, FearWorldBatch &batch)
{
    if (batch.m_Vertices.empty())
        return false;
    if (batch.m_pVertexBuffer)
        return true;

    if (FAILED(pDevice->CreateVertexBuffer((UINT)(batch.m_Vertices.size() * sizeof(FearWorldVertex)),
        D3DUSAGE_WRITEONLY, D3DFVF_FEARWORLD, D3DPOOL_MANAGED, &batch.m_pVertexBuffer, NULL)))
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

static const char *g_FearShaderSource =
"struct VS_INPUT\n"
"{\n"
"    float3 Pos    : POSITION;\n"
"    float3 Normal : NORMAL;\n"
"    float4 Color  : COLOR0;\n"
"    float2 UV     : TEXCOORD0;\n"
"};\n"
"\n"
"struct VS_OUTPUT\n"
"{\n"
"    float4 Pos    : POSITION;\n"
"    float3 World  : TEXCOORD0;\n"
"    float3 Normal : TEXCOORD1;\n"
"    float4 Color  : COLOR0;\n"
"    float2 UV     : TEXCOORD2;\n"
"};\n"
"\n"
"row_major float4x4 g_WorldViewProj : register(c0);\n"
"\n"
"VS_OUTPUT FearVS(VS_INPUT i)\n"
"{\n"
"    VS_OUTPUT o;\n"
"    o.Pos = mul(float4(i.Pos, 1.0), g_WorldViewProj);\n"
"    o.World = i.Pos;\n"
"    o.Normal = i.Normal;\n"
"    o.Color = i.Color;\n"
"    o.UV = i.UV;\n"
"    return o;\n"
"}\n"
"\n"
"sampler2D g_Diffuse  : register(s0);\n"
"sampler2D g_Normal   : register(s1);\n"
"sampler2D g_Specular : register(s2);\n"
"sampler2D g_Emissive : register(s3);\n"
"\n"
"float4 g_Ambient       : register(c0);\n"
"float4 g_Camera        : register(c1);\n"
"float4 g_Params        : register(c2);\n"
"float4 g_Tint          : register(c3);\n"
"float4 g_LightPos[8]   : register(c4);\n"
"float4 g_LightColor[8] : register(c12);\n"
"float4 g_LightDir[8]   : register(c20);\n"
"float4 g_LightKind[8]  : register(c28);\n"
"float4 g_Tone          : register(c36);\n"
"\n"
"float3 FearTonemap(float3 c)\n"
"{\n"
"    if (g_Tone.x < 0.5)\n"
"        return saturate(c);\n"
"    c = max(c, 0.0);\n"
"    float3 num = c * (2.51 * c + 0.03);\n"
"    float3 den = c * (2.43 * c + 0.59) + 0.14;\n"
"    return saturate(num / den);\n"
"}\n"
"\n"
"float4 FearPS(VS_OUTPUT i) : COLOR0\n"
"{\n"
"    float2 uv = i.UV;\n"
"    float3 albedo = tex2D(g_Diffuse, uv).rgb * lerp(1.0, i.Color.rgb, g_Tint.a);\n"
"    float3 N = normalize(i.Normal);\n"
"\n"
"    if (g_Params.z > 0.5)\n"
"    {\n"
"        float3 nrm = tex2D(g_Normal, uv).xyz * 2.0 - 1.0;\n"
"        float3 dp1 = ddx(i.World);\n"
"        float3 dp2 = ddy(i.World);\n"
"        float2 duv1 = ddx(uv);\n"
"        float2 duv2 = ddy(uv);\n"
"        float3 dp2perp = cross(dp2, N);\n"
"        float3 dp1perp = cross(N, dp1);\n"
"        float3 T = dp2perp * duv1.x + dp1perp * duv2.x;\n"
"        float3 B = dp2perp * duv1.y + dp1perp * duv2.y;\n"
"        float denom = max(max(dot(T, T), dot(B, B)), 1e-8);\n"
"        float invmax = rsqrt(denom);\n"
"        float3x3 basis = float3x3(T * invmax, B * invmax, N);\n"
"        N = normalize(mul(nrm, basis));\n"
"    }\n"
"\n"
"    float3 V = normalize(g_Camera.xyz - i.World);\n"
"    float3 light = g_Ambient.rgb;\n"
"    float3 spec = 0.0;\n"
"    int count = (int)g_Params.x;\n"
"\n"
"    [unroll]\n"
"    for (int k = 0; k < 8; ++k)\n"
"    {\n"
"        float active = (k < count) ? 1.0 : 0.0;\n"
"        float kind = g_LightKind[k].x;\n"
"        float radius = g_LightColor[k].a;\n"
"        float3 L = float3(0.0, 1.0, 0.0);\n"
"        float atten = 0.0;\n"
"        if (kind < 1.5)\n"
"        {\n"
"            if (kind < 0.5)\n"
"            {\n"
"                float3 toLight = g_LightPos[k].xyz - i.World;\n"
"                float d = length(toLight);\n"
"                L = toLight / max(d, 1e-4);\n"
"                float f = saturate(1.0 - d / max(radius, 1.0));\n"
"                atten = f * f;\n"
"            }\n"
"            else\n"
"            {\n"
"                L = -g_LightDir[k].xyz;\n"
"                atten = 1.0;\n"
"            }\n"
"        }\n"
"        else\n"
"        {\n"
"            float3 toLight = g_LightPos[k].xyz - i.World;\n"
"            float d = length(toLight);\n"
"            L = toLight / max(d, 1e-4);\n"
"            float f = saturate(1.0 - d / max(radius, 1.0));\n"
"            float cone = saturate((dot(-L, g_LightDir[k].xyz) - g_LightDir[k].w) * 8.0);\n"
"            atten = f * f * cone;\n"
"        }\n"
"        atten *= active;\n"
"        float ndl = saturate(dot(N, L));\n"
"        float3 lc = g_LightColor[k].rgb * (ndl * atten);\n"
"        light += lc;\n"
"        float3 H = normalize(L + V);\n"
"        float sp = pow(saturate(dot(N, H)), 24.0);\n"
"        spec += lc * sp * g_Params.w;\n"
"    }\n"
"\n"
"    float3 specMap = tex2D(g_Specular, uv).rgb;\n"
"    float3 baked = lerp(light, float3(1.0, 1.0, 1.0), g_Tint.a);\n"
"    float3 colour = albedo * baked * g_Tint.rgb;\n"
"    colour += spec * specMap;\n"
"    colour += tex2D(g_Emissive, uv).rgb * g_Tint.rgb;\n"
"    colour = FearTonemap(colour * g_Params.y);\n"
"    return float4(colour, 1.0);\n"
"}\n";

struct FearShaderBlob
{
    virtual HRESULT __stdcall QueryInterface(const void *, void **) = 0;
    virtual ULONG __stdcall AddRef() = 0;
    virtual ULONG __stdcall Release() = 0;
    virtual void *__stdcall GetBufferPointer() = 0;
    virtual size_t __stdcall GetBufferSize() = 0;
};

struct FearShaderMacroEntry
{
    const char *m_Name;
    const char *m_Definition;
};

typedef HRESULT (WINAPI *FearD3DCompileFn)(const void *pSrcData, size_t SrcDataSize, const char *pFileName,
    const FearShaderMacroEntry *pDefines, void *pInclude, const char *pEntrypoint, const char *pTarget,
    unsigned int Flags1, unsigned int Flags2, FearShaderBlob **ppCode, FearShaderBlob **ppErrorMsgs);

struct FearWorldShaderState
{
    IDirect3DDevice9 *m_pDevice;
    IDirect3DVertexShader9 *m_pVS;
    IDirect3DPixelShader9 *m_pPS;
    IDirect3DVertexDeclaration9 *m_pDecl;
    IDirect3DTexture9 *m_pBlack;
    IDirect3DTexture9 *m_pWhite;
    float m_AmbientScale;
    float m_Exposure;
    float m_SpecularScale;
    float m_VCol;
    int m_ToneMode;
    bool m_bNormalMaps;
    bool m_bUsable;
};

static FearWorldShaderState g_FearShaders = {0};
static bool g_bShadersTried = false;

static float FearEnvFloat(const char *pszName, float fDefault)
{
    char szValue[32] = {0};
    if (GetEnvironmentVariableA(pszName, szValue, sizeof(szValue)) == 0)
        return fDefault;
    if (!szValue[0])
        return fDefault;
    return (float)atof(szValue);
}

static void FearWorldDestroyShaders()
{
    if (g_FearShaders.m_pDecl)
        g_FearShaders.m_pDecl->Release();
    if (g_FearShaders.m_pVS)
        g_FearShaders.m_pVS->Release();
    if (g_FearShaders.m_pPS)
        g_FearShaders.m_pPS->Release();
    if (g_FearShaders.m_pBlack)
        g_FearShaders.m_pBlack->Release();
    if (g_FearShaders.m_pWhite)
        g_FearShaders.m_pWhite->Release();
    memset(&g_FearShaders, 0, sizeof(g_FearShaders));
}

static bool FearWorldCompileShader(FearD3DCompileFn pCompile, const char *pszEntry, const char *pszTarget,
    FearShaderBlob **ppBlob)
{
    const char *pszSource = g_FearShaderSource;
    FearShaderBlob *pErrors = NULL;
    HRESULT hr = pCompile(pszSource, strlen(pszSource), "fear_world.fx", NULL, NULL,
        pszEntry, pszTarget, 4, 0, ppBlob, &pErrors);
    if (FAILED(hr))
    {
        if (pErrors)
        {
            printf("FEARWORLD: %s compile failed: %s\n", pszTarget, (const char *)pErrors->GetBufferPointer());
            pErrors->Release();
        }
        else
        {
            printf("FEARWORLD: %s compile failed hr=0x%08x\n", pszTarget, (unsigned int)hr);
        }
        fflush(stdout);
        return false;
    }
    if (pErrors)
        pErrors->Release();
    return *ppBlob != NULL;
}

static bool FearWorldInitShaders(IDirect3DDevice9 *pDevice)
{
    if (g_bShadersTried && g_FearShaders.m_pDevice == pDevice)
        return g_FearShaders.m_bUsable;
    if (g_bShadersTried)
        FearWorldDestroyShaders();

    g_bShadersTried = true;
    g_FearShaders.m_pDevice = pDevice;

    char szMode[16] = {0};
    if (GetEnvironmentVariableA("FEAR_SHADER", szMode, sizeof(szMode)) > 0 && atoi(szMode) == 0)
    {
        printf("FEARWORLD: shader path disabled by FEAR_SHADER\n");
        fflush(stdout);
        return false;
    }

    HMODULE hCompiler = LoadLibraryA("d3dcompiler_47.dll");
    if (!hCompiler)
        hCompiler = LoadLibraryA("d3dcompiler_43.dll");
    if (!hCompiler)
    {
        printf("FEARWORLD: no d3dcompiler available, falling back to fixed function\n");
        fflush(stdout);
        return false;
    }

    FearD3DCompileFn pCompile = (FearD3DCompileFn)GetProcAddress(hCompiler, "D3DCompile");
    if (!pCompile)
    {
        printf("FEARWORLD: D3DCompile missing in d3dcompiler\n");
        fflush(stdout);
        return false;
    }

    FearShaderBlob *pVSBlob = NULL;
    FearShaderBlob *pPSBlob = NULL;
    if (!FearWorldCompileShader(pCompile, "FearVS", "vs_3_0", &pVSBlob) ||
        !FearWorldCompileShader(pCompile, "FearPS", "ps_3_0", &pPSBlob))
    {
        if (pVSBlob)
            pVSBlob->Release();
        if (pPSBlob)
            pPSBlob->Release();
        return false;
    }

    HRESULT hr = pDevice->CreateVertexShader((const DWORD *)pVSBlob->GetBufferPointer(), &g_FearShaders.m_pVS);
    if (SUCCEEDED(hr))
        hr = pDevice->CreatePixelShader((const DWORD *)pPSBlob->GetBufferPointer(), &g_FearShaders.m_pPS);
    pVSBlob->Release();
    pPSBlob->Release();
    if (FAILED(hr))
    {
        printf("FEARWORLD: shader creation failed hr=0x%08x\n", (unsigned int)hr);
        fflush(stdout);
        FearWorldDestroyShaders();
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
    if (FAILED(pDevice->CreateVertexDeclaration(decl, &g_FearShaders.m_pDecl)))
    {
        printf("FEARWORLD: vertex declaration creation failed\n");
        fflush(stdout);
        FearWorldDestroyShaders();
        return false;
    }

    if (FAILED(pDevice->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &g_FearShaders.m_pBlack, NULL)))
    {
        FearWorldDestroyShaders();
        return false;
    }
    D3DLOCKED_RECT locked;
    memset(&locked, 0, sizeof(locked));
    if (SUCCEEDED(g_FearShaders.m_pBlack->LockRect(0, &locked, NULL, 0)))
    {
        *(DWORD *)locked.pBits = 0xff000000u;
        g_FearShaders.m_pBlack->UnlockRect(0);
    }

    if (FAILED(pDevice->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &g_FearShaders.m_pWhite, NULL)))
    {
        FearWorldDestroyShaders();
        return false;
    }
    memset(&locked, 0, sizeof(locked));
    if (SUCCEEDED(g_FearShaders.m_pWhite->LockRect(0, &locked, NULL, 0)))
    {
        *(DWORD *)locked.pBits = 0xffffffffu;
        g_FearShaders.m_pWhite->UnlockRect(0);
    }

    g_FearShaders.m_AmbientScale = FearEnvFloat("FEAR_AMBIENT", 1.0f);
    g_FearShaders.m_Exposure = FearEnvFloat("FEAR_EXPOSURE", 0.72f);
    g_FearShaders.m_SpecularScale = FearEnvFloat("FEAR_SPECULAR", 1.0f);
    g_FearShaders.m_bNormalMaps = FearEnvFloat("FEAR_NORMALMAP", 1.0f) > 0.5f;
    g_FearShaders.m_VCol = FearEnvFloat("FEAR_VCOL", 1.0f);
    g_FearShaders.m_ToneMode = FearEnvFloat("FEAR_TONE", 1.0f) > 0.5f ? 1 : 0;
    g_FearShaders.m_bUsable = true;

    printf("FEARWORLD: shader path ready ambient=%.2f exposure=%.2f specular=%.2f normalmaps=%d vcol=%.2f tone=%d\n",
        g_FearShaders.m_AmbientScale, g_FearShaders.m_Exposure, g_FearShaders.m_SpecularScale,
        g_FearShaders.m_bNormalMaps ? 1 : 0, g_FearShaders.m_VCol, g_FearShaders.m_ToneMode);
    fflush(stdout);
    return true;
}

static void FearWorldSelectLights(const FearWorldBatch &batch, float fRadiusScale, unsigned int *pIndex, unsigned int &nCount)
{
    nCount = 0;
    float fBest[8];
    unsigned int nBest[8];
    for (size_t i = 0; i < g_WorldLights.size(); ++i)
    {
        const FearWorldLight &light = g_WorldLights[i];
        float fReach = light.m_Radius * fRadiusScale + batch.m_Radius;
        float dx = light.m_Pos[0] - batch.m_Center[0];
        float dy = light.m_Pos[1] - batch.m_Center[1];
        float dz = light.m_Pos[2] - batch.m_Center[2];
        float fDist2 = dx * dx + dy * dy + dz * dz;
        if (light.m_nClass == 3)
            fDist2 = 0.0f;
        else if (fDist2 > fReach * fReach)
            continue;

        float fScore = (light.m_Color[0] + light.m_Color[1] + light.m_Color[2]) * light.m_Radius;
        fScore /= (1.0f + sqrtf(fDist2));

        unsigned int nSlot = nCount;
        if (nCount < 8)
        {
            ++nCount;
        }
        else
        {
            nSlot = 0;
            for (unsigned int k = 1; k < 8; ++k)
                if (fBest[k] < fBest[nSlot])
                    nSlot = k;
            if (fScore <= fBest[nSlot])
                continue;
        }
        fBest[nSlot] = fScore;
        nBest[nSlot] = (unsigned int)i;
    }
    for (unsigned int i = 0; i < nCount; ++i)
        pIndex[i] = nBest[i];
}

static void FearWorldQuatDirection(const float *pQuat, float *pOut)
{
    FearQuatDirection(pQuat, pOut);
}

static FearModel *g_pDebugModel = NULL;
static bool g_bDebugModelInit = false;

static float FearWorldEnvFloat(const char *pszName, float fDefault)
{
    char szValue[64] = {0};
    DWORD nLen = GetEnvironmentVariableA(pszName, szValue, sizeof(szValue));
    if (nLen == 0 || nLen >= sizeof(szValue) || !szValue[0])
        return fDefault;
    return (float)atof(szValue);
}

static void FearWorldQueueModels()
{
    if (!g_bDebugModelInit)
    {
        g_bDebugModelInit = true;
        char szName[128] = {0};
        DWORD nLen = GetEnvironmentVariableA("FEAR_DEBUG_MODEL", szName, sizeof(szName));
        if (nLen > 0 && nLen < sizeof(szName) && szName[0])
        {
            g_pDebugModel = FearModel_LoadByName(szName);
            if (g_pDebugModel)
            {
                printf("FEARWORLD: debug model %s loaded verts=%d tris=%d\n",
                    szName, FearModel_GetVertexCount(g_pDebugModel),
                    FearModel_GetTriangleCount(g_pDebugModel));
            }
            else
            {
                printf("FEARWORLD: debug model %s unavailable\n", szName);
            }
            fflush(stdout);
        }
    }

    if (!g_pDebugModel)
        return;

    int nVerts = FearModel_GetVertexCount(g_pDebugModel);
    if (nVerts <= 0)
        return;

    float fSize = FearWorldEnvFloat("FEAR_MODEL_SIZE", 120.0f);
    float fDist = FearWorldEnvFloat("FEAR_MODEL_DIST", 170.0f);
    float fYawDeg = FearWorldEnvFloat("FEAR_MODEL_YAW", 90.0f);

    float lo[3] = {0, 0, 0};
    float hi[3] = {0, 0, 0};
    FearModel_GetBounds(g_pDebugModel, lo, hi);
    float fExtent = hi[0] - lo[0];
    if (hi[1] - lo[1] > fExtent)
        fExtent = hi[1] - lo[1];
    if (hi[2] - lo[2] > fExtent)
        fExtent = hi[2] - lo[2];
    if (fExtent < 1.0e-3f)
        fExtent = 1.0f;
    float fScale = fSize / fExtent;
    float ctr[3] = { (lo[0] + hi[0]) * 0.5f, (lo[1] + hi[1]) * 0.5f, (lo[2] + hi[2]) * 0.5f };

    float cosPitch = cosf(g_CameraPitch);
    float fwd[3] = { sinf(g_CameraYaw) * cosPitch, sinf(g_CameraPitch), cosf(g_CameraYaw) * cosPitch };
    float right[3] = { cosf(g_CameraYaw), 0.0f, -sinf(g_CameraYaw) };
    float up[3] = {
        right[1] * fwd[2] - right[2] * fwd[1],
        right[2] * fwd[0] - right[0] * fwd[2],
        right[0] * fwd[1] - right[1] * fwd[0] };

    float origin[3] = {
        g_CameraPos[0] + fwd[0] * fDist,
        g_CameraPos[1] + fwd[1] * fDist,
        g_CameraPos[2] + fwd[2] * fDist };

    float fYaw = fYawDeg * 3.14159265f / 180.0f;
    float cy = cosf(fYaw), sy = sinf(fYaw);

    float row0[3], row1[3], row2[3];
    for (int k = 0; k < 3; ++k)
    {
        row0[k] = (cy * right[k] + sy * fwd[k]) * fScale;
        row1[k] = up[k] * fScale;
        row2[k] = (sy * right[k] - cy * fwd[k]) * fScale;
    }

    float m[16];
    memset(m, 0, sizeof(m));
    m[0] = row0[0]; m[1] = row0[1]; m[2] = row0[2]; m[3] = 0.0f;
    m[4] = row1[0]; m[5] = row1[1]; m[6] = row1[2]; m[7] = 0.0f;
    m[8] = row2[0]; m[9] = row2[1]; m[10] = row2[2]; m[11] = 0.0f;
    m[12] = origin[0] - (row0[0] * ctr[0] + row1[0] * ctr[1] + row2[0] * ctr[2]);
    m[13] = origin[1] - (row0[1] * ctr[0] + row1[1] * ctr[1] + row2[1] * ctr[2]);
    m[14] = origin[2] - (row0[2] * ctr[0] + row1[2] * ctr[1] + row2[2] * ctr[2]);
    m[15] = 1.0f;

    FearModel_Queue(g_pDebugModel, m);
}

void FearWorld_Render(IDirect3DDevice9 *pDevice)
{
    static int s_nRenderCalls = 0;
    ++s_nRenderCalls;

    if (!pDevice)
        return;
    if (!g_bWorldLoaded)
        FearWorld_Load("Worlds\\Release\\Intro.World00p");
    if (g_WorldBatches.empty())
    {
        if (s_nRenderCalls == 1 || (s_nRenderCalls % 300) == 0)
        {
            printf("FEARWORLD: render #%d device=%p no batches (loaded=%d)\n",
                s_nRenderCalls, (void *)pDevice, g_bWorldLoaded ? 1 : 0);
            fflush(stdout);
        }
        return;
    }

    if (s_nRenderCalls == 1 || (s_nRenderCalls % 300) == 0)
    {
        printf("FEARWORLD: render #%d device=%p batches=%u batches_ready=%u\n",
            s_nRenderCalls, (void *)pDevice, (unsigned int)g_WorldBatches.size(),
            (unsigned int)g_WorldBatches.size());
        fflush(stdout);
    }

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
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_CULLMODE, FearWorldCullMode());

    FearMatrix view = FearLookAt(eye, target, up);
    float fFovY = g_CameraFovY;
    float fAspect = 640.0f / 480.0f;
    if (fFovY > 0.01f && g_CameraFovX > 0.01f)
    {
        float fDerived = tanf(g_CameraFovX * 0.5f) / tanf(fFovY * 0.5f);
        if (fDerived > 0.2f && fDerived < 5.0f)
            fAspect = fDerived;
    }
    if (fFovY <= 0.01f)
        fFovY = 1.2f;
    FearMatrix proj = FearPerspective(fFovY, fAspect, 1.0f, 200000.0f);
    pDevice->SetTransform(D3DTS_VIEW, (const D3DMATRIX *)&view);
    pDevice->SetTransform(D3DTS_PROJECTION, (const D3DMATRIX *)&proj);

    FearWorldQueueModels();

    bool bShaderDraw = FearWorldInitShaders(pDevice);
    static bool s_bShaderDebug = GetEnvironmentVariableA("FEAR_SHADERDBG", NULL, 0) > 0;
    static bool s_bDebugReported = false;
    FearMatrix wvp = FearIdentity();
    if (bShaderDraw)
    {
        wvp = FearMatrixMultiply(view, proj);
        pDevice->SetFVF(D3DFVF_FEARWORLD);
        pDevice->SetVertexShader(g_FearShaders.m_pVS);
        pDevice->SetPixelShader(g_FearShaders.m_pPS);
        HRESULT hrConst = pDevice->SetVertexShaderConstantF(0, wvp.m, 4);
        if (s_bShaderDebug && !s_bDebugReported)
        {
            s_bDebugReported = true;
            float fReadBack[16] = {0};
            pDevice->GetVertexShaderConstantF(0, fReadBack, 4);
            IDirect3DVertexShader9 *pVSNow = NULL;
            IDirect3DPixelShader9 *pPSNow = NULL;
            pDevice->GetVertexShader(&pVSNow);
            pDevice->GetPixelShader(&pPSNow);
            printf("FEARWORLD: shaderdbg setconst=0x%08x vs=%p/%p ps=%p/%p rb0=(%.4f %.4f %.4f %.4f)\n",
                (unsigned int)hrConst, (void *)pVSNow, (void *)g_FearShaders.m_pVS,
                (void *)pPSNow, (void *)g_FearShaders.m_pPS,
                fReadBack[0], fReadBack[1], fReadBack[2], fReadBack[3]);
            fflush(stdout);
            if (pVSNow) pVSNow->Release();
            if (pPSNow) pPSNow->Release();
        }

        float fAmbient[4] = {
            g_WorldAmbient[0] / 255.0f * g_FearShaders.m_AmbientScale,
            g_WorldAmbient[1] / 255.0f * g_FearShaders.m_AmbientScale,
            g_WorldAmbient[2] / 255.0f * g_FearShaders.m_AmbientScale,
            1.0f };
        pDevice->SetPixelShaderConstantF(0, fAmbient, 1);
        float fCamera[4] = { eye[0], eye[1], eye[2], 1.0f };
        pDevice->SetPixelShaderConstantF(1, fCamera, 1);
        float fTone[4] = { (float)g_FearShaders.m_ToneMode, 0.0f, 0.0f, 0.0f };
        pDevice->SetPixelShaderConstantF(36, fTone, 1);

        for (unsigned int nStage = 0; nStage < 4; ++nStage)
        {
            pDevice->SetSamplerState(nStage, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            pDevice->SetSamplerState(nStage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            pDevice->SetSamplerState(nStage, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        }
        pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        for (unsigned int nStage = 1; nStage < 4; ++nStage)
            pDevice->SetTextureStageState(nStage, D3DTSS_COLOROP, D3DTOP_DISABLE);
    }
    else
    {
        pDevice->SetVertexShader(NULL);
        pDevice->SetPixelShader(NULL);
        pDevice->SetFVF(D3DFVF_FEARWORLD);
        pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    }

    for (size_t i = 0; i < g_WorldBatches.size(); ++i)
    {
        FearWorldBatch &batch = g_WorldBatches[i];
        if (!FearWorldCreateBatchBuffer(pDevice, batch))
            continue;

        if (bShaderDraw)
        {
            pDevice->SetTexture(0, batch.m_pTexture ? batch.m_pTexture : g_FearShaders.m_pWhite);
            pDevice->SetTexture(1, batch.m_pNormalTexture);
            pDevice->SetTexture(2, batch.m_pSpecularTexture);
            pDevice->SetTexture(3, batch.m_pEmissiveTexture);

            float fTint[4] = { 1.0f, 1.0f, 1.0f, g_FearShaders.m_VCol };
            if (!batch.m_pTexture)
            {
                fTint[0] = ((batch.m_Color >> 16) & 0xff) / 255.0f;
                fTint[1] = ((batch.m_Color >> 8) & 0xff) / 255.0f;
                fTint[2] = (batch.m_Color & 0xff) / 255.0f;
            }
            pDevice->SetPixelShaderConstantF(3, fTint, 1);
            unsigned int nLightIndex[8] = {0};
            unsigned int nLightCount = 0;
            FearWorldSelectLights(batch, 1.0f, nLightIndex, nLightCount);

            float fLightPos[8][4] = {{0}};
            float fLightColor[8][4] = {{0}};
            float fLightDir[8][4] = {{0}};
            float fLightKind[8][4] = {{0}};
            for (unsigned int k = 0; k < nLightCount && k < 8; ++k)
            {
                const FearWorldLight &light = g_WorldLights[nLightIndex[k]];
                fLightPos[k][0] = light.m_Pos[0];
                fLightPos[k][1] = light.m_Pos[1];
                fLightPos[k][2] = light.m_Pos[2];
                fLightPos[k][3] = 1.0f;
                fLightColor[k][0] = light.m_Color[0] / 255.0f;
                fLightColor[k][1] = light.m_Color[1] / 255.0f;
                fLightColor[k][2] = light.m_Color[2] / 255.0f;
                fLightColor[k][3] = light.m_Radius;
                fLightDir[k][0] = light.m_Dir[0];
                fLightDir[k][1] = light.m_Dir[1];
                fLightDir[k][2] = light.m_Dir[2];
                float fCos = 0.0f;
                if (light.m_FovX > 1.0f && light.m_FovX < 180.0f)
                    fCos = cosf(light.m_FovX * 0.5f * 3.14159265f / 180.0f);
                fLightDir[k][3] = fCos;
                float fKind = 0.0f;
                if (light.m_nClass == 3)
                    fKind = 1.0f;
                else if (light.m_nClass == 2)
                    fKind = 2.0f;
                fLightKind[k][0] = fKind;
                fLightKind[k][1] = light.m_Intensity;
            }
            pDevice->SetPixelShaderConstantF(4, &fLightPos[0][0], 8);
            pDevice->SetPixelShaderConstantF(12, &fLightColor[0][0], 8);
            pDevice->SetPixelShaderConstantF(20, &fLightDir[0][0], 8);
            pDevice->SetPixelShaderConstantF(28, &fLightKind[0][0], 8);

            bool bNormalMap = g_FearShaders.m_bNormalMaps && batch.m_pNormalTexture != NULL;
            float fParams[4] = {
                (float)nLightCount,
                g_FearShaders.m_Exposure,
                bNormalMap ? 1.0f : 0.0f,
                g_FearShaders.m_SpecularScale };
            pDevice->SetPixelShaderConstantF(2, fParams, 1);

            pDevice->SetStreamSource(0, batch.m_pVertexBuffer, 0, sizeof(FearWorldVertex));
            HRESULT hrDraw = pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, batch.m_nVertexCount / 3);
            if (s_bShaderDebug && !s_bDebugReported)
            {
                s_bDebugReported = true;
                const FearWorldVertex &v0 = batch.m_Vertices[0];
                float in[4] = { v0.x, v0.y, v0.z, 1.0f };
                float out[4];
                for (int c = 0; c < 4; ++c)
                    out[c] = in[0] * wvp.m[0 * 4 + c] + in[1] * wvp.m[1 * 4 + c] +
                             in[2] * wvp.m[2 * 4 + c] + in[3] * wvp.m[3 * 4 + c];
                printf("FEARWORLD: shaderdbg batch=%s verts=%u draw=0x%08x hr=0x%08x nlights=%u ndc=(%.3f,%.3f,%.3f,w=%.1f)\n",
                    batch.m_Name.c_str(), batch.m_nVertexCount, (unsigned int)hrDraw, (unsigned int)hrDraw,
                    nLightCount, out[0] / out[3], out[1] / out[3], out[2] / out[3], out[3]);
                fflush(stdout);
            }
            continue;
        }

        pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, batch.m_pTexture ? 0xffffffff : batch.m_Color);
        pDevice->SetTexture(0, batch.m_pTexture);
        if (batch.m_pTexture)
        {
            pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        }
        else
        {
            pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
            pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        }
        pDevice->SetStreamSource(0, batch.m_pVertexBuffer, 0, sizeof(FearWorldVertex));
        pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, batch.m_nVertexCount / 3);
    }

    FearModel_RenderQueued(pDevice, view.m, proj.m, eye, fFovY, fAspect);
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

void FearWorld_SetFov(float fFovX, float fFovY)
{
    static bool s_bLogged = false;
    if (fFovX > 0.01f && fFovX < 3.2f)
        g_CameraFovX = fFovX;
    if (fFovY > 0.01f && fFovY < 3.2f)
        g_CameraFovY = fFovY;
    if (!s_bLogged)
    {
        s_bLogged = true;
        printf("FEARWORLD: camera fov x=%.4f rad (%.1f deg) y=%.4f rad (%.1f deg)\n",
            g_CameraFovX, g_CameraFovX * 57.29578f, g_CameraFovY, g_CameraFovY * 57.29578f);
        fflush(stdout);
    }
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
        if (batch.m_pNormalTexture)
        {
            batch.m_pNormalTexture->Release();
            batch.m_pNormalTexture = NULL;
        }
        if (batch.m_pSpecularTexture)
        {
            batch.m_pSpecularTexture->Release();
            batch.m_pSpecularTexture = NULL;
        }
        if (batch.m_pEmissiveTexture)
        {
            batch.m_pEmissiveTexture->Release();
            batch.m_pEmissiveTexture = NULL;
        }
        batch.m_Vertices.clear();
    }
    FearWorldDestroyShaders();
    g_bShadersTried = false;
    g_WorldBatches.clear();
    g_bWorldLoaded = false;
}
