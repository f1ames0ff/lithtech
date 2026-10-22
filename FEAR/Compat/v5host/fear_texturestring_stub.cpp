#include "fear_texturestring_stub.h"
#include "fear_host_api.h"
#include "fear_render_api.h"
#include "iltdrawprim.h"

#include <windows.h>
#include <cstdint>
#include <new>
#include <string>

struct FearTextureStringInst
{
    std::wstring m_sString;
    CFontInfo m_Font;
    LTRect2n m_Extents;
    uint32 m_nRefCount = 1;
    void *m_hTexture = NULL;
    int32 m_nTextureWidth = 0;
    int32 m_nTextureHeight = 0;
    FearTextureStringInst *m_pNext = NULL;
};

namespace
{
    const size_t kFearTextureStringMaxChars = 4096;
    const int32 kFearTextureStringDefaultHeight = 16;
    const int32 kFearTextureStringPadding = 2;

    FearTextureStringInst *g_pFearTextureStrings = NULL;

    const FearRenderApi *FearTextureRender(void)
    {
        const FearEngineApi *pApi = FearHost_GetEngineApi();
        return pApi ? pApi->m_pRenderApi : NULL;
    }

    void FearCopyWideString(std::wstring &sDest, const wchar_t *pszString)
    {
        sDest.clear();
        if (!pszString)
            return;

        size_t nLength = 0;
        while (nLength < kFearTextureStringMaxChars && pszString[nLength] != L'\0')
            ++nLength;

        sDest.assign(pszString, nLength);
    }

    bool FearIsTextureStringValid(const FearTextureStringInst *pInst)
    {
        if (!pInst)
            return false;

        for (const FearTextureStringInst *pCur = g_pFearTextureStrings; pCur; pCur = pCur->m_pNext)
        {
            if (pCur == pInst)
                return true;
        }

        return false;
    }

    FearTextureStringInst *FearGetTextureStringInst(HTEXTURESTRING hString)
    {
        FearTextureStringInst *pInst = reinterpret_cast<FearTextureStringInst *>(hString);
        return FearIsTextureStringValid(pInst) ? pInst : NULL;
    }

    void FearLinkTextureString(FearTextureStringInst *pInst)
    {
        pInst->m_pNext = g_pFearTextureStrings;
        g_pFearTextureStrings = pInst;
    }

    void FearUnlinkTextureString(const FearTextureStringInst *pInst)
    {
        FearTextureStringInst **ppLink = &g_pFearTextureStrings;
        while (*ppLink)
        {
            if (*ppLink == pInst)
            {
                *ppLink = pInst->m_pNext;
                return;
            }

            ppLink = &(*ppLink)->m_pNext;
        }
    }

    uint32 FearMaxChannel(uint32 r, uint32 g, uint32 b)
    {
        uint32 nMax = r;
        if (g > nMax)
            nMax = g;
        if (b > nMax)
            nMax = b;
        return nMax;
    }

    void FearReleaseTextTexture(FearTextureStringInst *pInst)
    {
        if (!pInst || !pInst->m_hTexture)
            return;

        const FearRenderApi *pRender = FearTextureRender();
        if (pRender && pRender->ReleaseTexture)
            pRender->ReleaseTexture(pInst->m_hTexture);

        pInst->m_hTexture = NULL;
        pInst->m_nTextureWidth = 0;
        pInst->m_nTextureHeight = 0;
    }

    void FearBuildTextTexture(FearTextureStringInst *pInst)
    {
        if (!pInst)
            return;

        FearReleaseTextTexture(pInst);

        if (pInst->m_sString.empty())
            return;

        const FearRenderApi *pRender = FearTextureRender();
        if (!pRender || !pRender->CreateTextureFromRGBA)
            return;

        HDC hDC = CreateCompatibleDC(NULL);
        if (!hDC)
            return;

        int32 nFontHeight = (int32)pInst->m_Font.m_nHeight;
        if (nFontHeight <= 0)
            nFontHeight = kFearTextureStringDefaultHeight;

        LOGFONTW lf;
        memset(&lf, 0, sizeof(lf));
        lf.lfHeight = -nFontHeight;
        lf.lfWeight = (pInst->m_Font.m_nStyle & CFontInfo::kStyle_Bold) ? FW_BOLD : FW_NORMAL;
        lf.lfItalic = (pInst->m_Font.m_nStyle & CFontInfo::kStyle_Italic) ? TRUE : FALSE;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfQuality = ANTIALIASED_QUALITY;
        lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        if (pInst->m_Font.m_szTypeface[0])
            MultiByteToWideChar(CP_ACP, 0, pInst->m_Font.m_szTypeface, -1, lf.lfFaceName, LF_FACESIZE);

        HFONT hFont = CreateFontIndirectW(&lf);
        if (!hFont)
        {
            DeleteDC(hDC);
            return;
        }

        HGDIOBJ hOldFont = SelectObject(hDC, hFont);

        SIZE textSize;
        memset(&textSize, 0, sizeof(textSize));
        GetTextExtentPoint32W(hDC, pInst->m_sString.c_str(), (int)pInst->m_sString.size(), &textSize);

        int32 nWidth = (int32)textSize.cx + kFearTextureStringPadding * 2;
        int32 nHeight = (int32)textSize.cy + kFearTextureStringPadding * 2;
        if (nWidth < 1)
            nWidth = 1;
        if (nHeight < 1)
            nHeight = 1;

        BITMAPINFO bmi;
        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = nWidth;
        bmi.bmiHeader.biHeight = -nHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void *pBits = NULL;
        HBITMAP hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
        if (hBitmap && pBits)
        {
            HGDIOBJ hOldBitmap = SelectObject(hDC, hBitmap);
            memset(pBits, 0, (size_t)nWidth * (size_t)nHeight * 4);

            SetBkMode(hDC, TRANSPARENT);
            SetTextColor(hDC, RGB(255, 255, 255));
            SetTextAlign(hDC, TA_LEFT | TA_TOP);

            RECT rc;
            rc.left = kFearTextureStringPadding;
            rc.top = kFearTextureStringPadding;
            rc.right = rc.left + textSize.cx;
            rc.bottom = rc.top + textSize.cy;
            DrawTextW(hDC, pInst->m_sString.c_str(), (int)pInst->m_sString.size(), &rc,
                DT_NOPREFIX | DT_SINGLELINE | DT_LEFT | DT_TOP);

            uint32 *pPixels = reinterpret_cast<uint32 *>(pBits);
            const size_t nPixels = (size_t)nWidth * (size_t)nHeight;
            for (size_t i = 0; i < nPixels; ++i)
            {
                uint32 nColor = pPixels[i];
                uint32 nR = (nColor >> 16) & 0xFF;
                uint32 nG = (nColor >> 8) & 0xFF;
                uint32 nB = nColor & 0xFF;
                uint32 nA = FearMaxChannel(nR, nG, nB);
                pPixels[i] = (nA << 24) | 0x00FFFFFF;
            }

            pInst->m_hTexture = pRender->CreateTextureFromRGBA(pPixels, nWidth, nHeight);
            if (pInst->m_hTexture)
            {
                pInst->m_nTextureWidth = nWidth;
                pInst->m_nTextureHeight = nHeight;
            }

            SelectObject(hDC, hOldBitmap);
        }

        if (hBitmap)
            DeleteObject(hBitmap);

        SelectObject(hDC, hOldFont);
        DeleteObject(hFont);
        DeleteDC(hDC);
    }

    void FearUpdateTextureStringExtents(FearTextureStringInst *pInst)
    {
        if (!pInst)
            return;

        if (pInst->m_hTexture && pInst->m_nTextureWidth > 0 && pInst->m_nTextureHeight > 0)
        {
            pInst->m_Extents.Init(0, 0, pInst->m_nTextureWidth, pInst->m_nTextureHeight);
            return;
        }

        uint32 nHeight = pInst->m_Font.m_nHeight;
        if (nHeight == 0)
            nHeight = (uint32)kFearTextureStringDefaultHeight;

        pInst->m_Extents.Init(0, 0, 0, (int32)nHeight);
    }

    FearTextureStringInst *FearCreateTextureStringInst(const wchar_t *pszString, const CFontInfo &Font)
    {
        if (!pszString)
            return NULL;

        FearTextureStringInst *pInst = new (std::nothrow) FearTextureStringInst();
        if (!pInst)
            return NULL;

        pInst->m_Font = Font;
        FearCopyWideString(pInst->m_sString, pszString);
        FearBuildTextTexture(pInst);
        FearUpdateTextureStringExtents(pInst);
        FearLinkTextureString(pInst);
        return pInst;
    }
}

CFearTextureString g_FearTextureString;

HCUSTOMFONTFILE CFearTextureString::RegisterCustomFontFile(const char *pszRelResource)
{
    (void)pszRelResource;
    return reinterpret_cast<HCUSTOMFONTFILE>(static_cast<std::uintptr_t>(1));
}

LTRESULT CFearTextureString::UnregisterCustomFontFile(HCUSTOMFONTFILE hCustomFontFile)
{
    if (!hCustomFontFile)
        return LT_INVALIDPARAMS;

    return LT_OK;
}

HTEXTURESTRING CFearTextureString::CreateTextureString(const wchar_t *pszString, const CFontInfo &Font)
{
    return reinterpret_cast<HTEXTURESTRING>(FearCreateTextureStringInst(pszString, Font));
}

HTEXTURESTRING CFearTextureString::CreateTextureSubstring(const wchar_t *pszString, HTEXTURESTRING hSrcString)
{
    if (!pszString)
        return NULL;

    CFontInfo Font;
    const FearTextureStringInst *pSrc = FearGetTextureStringInst(hSrcString);
    if (pSrc)
        Font = pSrc->m_Font;

    return reinterpret_cast<HTEXTURESTRING>(FearCreateTextureStringInst(pszString, Font));
}

LTRESULT CFearTextureString::WordWrapString(HTEXTURESTRING hString, uint32 nWidth)
{
    FearTextureStringInst *pInst = FearGetTextureStringInst(hString);
    if (!pInst)
        return LT_INVALIDPARAMS;

    if (nWidth == 0 || pInst->m_sString.empty())
        return LT_OK;

    const size_t nApproxCharWidth = 8;
    const size_t nMaxChars = nWidth / nApproxCharWidth;
    if (nMaxChars == 0)
        return LT_OK;

    std::wstring sWrapped;
    size_t nLineChars = 0;
    size_t nLastSpace = std::wstring::npos;

    for (size_t i = 0; i < pInst->m_sString.size(); ++i)
    {
        wchar_t c = pInst->m_sString[i];
        if (c == L'\n')
        {
            sWrapped.push_back(c);
            nLineChars = 0;
            nLastSpace = std::wstring::npos;
            continue;
        }

        if (c == L' ')
            nLastSpace = sWrapped.size();

        sWrapped.push_back(c);
        ++nLineChars;

        if (nLineChars >= nMaxChars)
        {
            if (nLastSpace != std::wstring::npos)
            {
                sWrapped[nLastSpace] = L'\n';
                nLineChars = sWrapped.size() - nLastSpace - 1;
                nLastSpace = std::wstring::npos;
            }
            else
            {
                sWrapped.push_back(L'\n');
                nLineChars = 0;
            }
        }
    }

    pInst->m_sString.swap(sWrapped);
    FearBuildTextTexture(pInst);
    FearUpdateTextureStringExtents(pInst);
    return LT_OK;
}

LTRESULT CFearTextureString::RecreateTextureString(HTEXTURESTRING hString, const wchar_t *pszString, const CFontInfo &Font)
{
    FearTextureStringInst *pInst = FearGetTextureStringInst(hString);
    if (!pInst || !pszString)
        return LT_INVALIDPARAMS;

    pInst->m_Font = Font;
    FearCopyWideString(pInst->m_sString, pszString);
    FearBuildTextTexture(pInst);
    FearUpdateTextureStringExtents(pInst);
    return LT_OK;
}

LTRESULT CFearTextureString::RecreateTextureSubstring(HTEXTURESTRING hString, const wchar_t *pszString, HTEXTURESTRING hSrcTexture)
{
    FearTextureStringInst *pInst = FearGetTextureStringInst(hString);
    if (!pInst || !pszString)
        return LT_INVALIDPARAMS;

    const FearTextureStringInst *pSrc = FearGetTextureStringInst(hSrcTexture);
    if (pSrc)
        pInst->m_Font = pSrc->m_Font;

    FearCopyWideString(pInst->m_sString, pszString);
    FearBuildTextTexture(pInst);
    FearUpdateTextureStringExtents(pInst);
    return LT_OK;
}

LTRESULT CFearTextureString::GetStringExtents(HTEXTURESTRING hString, LTRect2n &rExtents)
{
    rExtents.Init();

    const FearTextureStringInst *pInst = FearGetTextureStringInst(hString);
    if (!pInst)
        return LT_INVALIDPARAMS;

    rExtents = pInst->m_Extents;
    return LT_OK;
}

LTRESULT CFearTextureString::GetStringLength(HTEXTURESTRING hString, uint32 *pnLength)
{
    if (!pnLength)
        return LT_INVALIDPARAMS;

    *pnLength = 0;

    const FearTextureStringInst *pInst = FearGetTextureStringInst(hString);
    if (!pInst)
        return LT_INVALIDPARAMS;

    *pnLength = (uint32)pInst->m_sString.size();
    return LT_OK;
}

LTRESULT CFearTextureString::GetString(HTEXTURESTRING hString, wchar_t *pszBuffer, uint32 nBufferLen)
{
    if (!pszBuffer || nBufferLen == 0)
        return LT_INVALIDPARAMS;

    pszBuffer[0] = L'\0';

    const FearTextureStringInst *pInst = FearGetTextureStringInst(hString);
    if (!pInst)
        return LT_INVALIDPARAMS;

    uint32 nCopyLen = (uint32)pInst->m_sString.size();
    if (nCopyLen >= nBufferLen)
        nCopyLen = nBufferLen - 1;

    if (nCopyLen > 0)
        pInst->m_sString.copy(pszBuffer, nCopyLen);

    pszBuffer[nCopyLen] = L'\0';
    return LT_OK;
}

LTRESULT CFearTextureString::GetFont(HTEXTURESTRING hString, CFontInfo &Font)
{
    Font = CFontInfo();

    const FearTextureStringInst *pInst = FearGetTextureStringInst(hString);
    if (!pInst)
        return LT_INVALIDPARAMS;

    Font = pInst->m_Font;
    return LT_OK;
}

LTRESULT CFearTextureString::GetTextureImage(HTEXTURESTRING hString, HDYNAMICTEXTURE &hTexture)
{
    hTexture = NULL;

    if (!FearGetTextureStringInst(hString))
        return LT_INVALIDPARAMS;

    return LT_UNSUPPORTED;
}

LTRESULT CFearTextureString::GetCharRect(HTEXTURESTRING hString, uint32 nCharIndex, LTRect2n &rPlacementRect, LTRect2n &rBlackBox, LTVector2 &vUVPos, LTVector2 &vUVDims)
{
    rPlacementRect.Init();
    rBlackBox.Init();
    vUVPos.Init();
    vUVDims.Init();

    const FearTextureStringInst *pInst = FearGetTextureStringInst(hString);
    if (!pInst || nCharIndex >= (uint32)pInst->m_sString.size())
        return LT_INVALIDPARAMS;

    const uint32 nLen = (uint32)pInst->m_sString.size();
    const int32 nWidth = pInst->m_nTextureWidth > 0 ? pInst->m_nTextureWidth : (int32)(nLen * 8);
    const int32 nHeight = pInst->m_nTextureHeight > 0 ? pInst->m_nTextureHeight : (int32)kFearTextureStringDefaultHeight;
    const int32 nCharWidth = nWidth / (int32)nLen;

    const int32 nLeft = (int32)nCharIndex * nCharWidth;
    const int32 nRight = (nCharIndex + 1 == nLen) ? nWidth : nLeft + nCharWidth;

    rPlacementRect.Init(nLeft, 0, nRight, nHeight);
    rBlackBox = rPlacementRect;
    vUVPos.Init((float)nLeft / (float)nWidth, 0.0f);
    vUVDims.Init((float)(nRight - nLeft) / (float)nWidth, 1.0f);
    return LT_OK;
}

void CFearTextureString::ReleaseTextureString(HTEXTURESTRING hString)
{
    FearTextureStringInst *pInst = FearGetTextureStringInst(hString);
    if (!pInst)
        return;

    if (pInst->m_nRefCount > 1)
    {
        --pInst->m_nRefCount;
        return;
    }

    FearReleaseTextTexture(pInst);
    FearUnlinkTextureString(pInst);
    delete pInst;
}

void CFearTextureString::AddRefTextureString(HTEXTURESTRING hString)
{
    FearTextureStringInst *pInst = FearGetTextureStringInst(hString);
    if (!pInst)
        return;

    ++pInst->m_nRefCount;
}

LTRESULT CFearTextureString::SetupTextRendering(ILTDrawPrim *pDrawPrim)
{
    (void)pDrawPrim;
    return LT_OK;
}

static void FearDrawTextureStringQuad(ILTDrawPrim *pDrawPrim, void *hTexture, int32 nWidth, int32 nHeight,
    const LTVector2 &vAnchor, uint32 nColor, const LTVector2 &vAnchorScale)
{
    if (!pDrawPrim || !hTexture)
        return;

    const float fWidth = (float)nWidth;
    const float fHeight = (float)nHeight;
    const float fLeft = vAnchor.x - fWidth * vAnchorScale.x;
    const float fTop = vAnchor.y - fHeight * vAnchorScale.y;

    LT_POLYGT4 quad;
    const float fX[4] = { fLeft, fLeft + fWidth, fLeft + fWidth, fLeft };
    const float fY[4] = { fTop, fTop, fTop + fHeight, fTop + fHeight };
    const float fU[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
    const float fV[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

    for (int i = 0; i < 4; ++i)
    {
        quad.verts[i].pos.Init(fX[i], fY[i], 0.0f);
        quad.verts[i].uv.Init(fU[i], fV[i]);
        quad.verts[i].rgba.m_nColor = nColor;
    }

    pDrawPrim->SetTexture((HTEXTURE)hTexture);
    pDrawPrim->SetRenderMode(eLTDrawPrimRenderMode_Modulate_Translucent);
    pDrawPrim->DrawPrim(&quad, 1);
}

LTRESULT CFearTextureString::RenderString(HTEXTURESTRING hString, ILTDrawPrim *pDrawPrim, const LTVector2 &vAnchor, uint32 nColor, const LTVector2 &vAnchorScale, const LTVector2 &vGround, const LTVector2 &vDown, const LTVector2 &vStretch, bool bSnapAnchor)
{
    (void)vGround;
    (void)vDown;
    (void)vStretch;
    (void)bSnapAnchor;

    const FearTextureStringInst *pInst = FearGetTextureStringInst(hString);
    if (!pInst || !pInst->m_hTexture)
        return LT_OK;

    FearDrawTextureStringQuad(pDrawPrim, pInst->m_hTexture, pInst->m_nTextureWidth, pInst->m_nTextureHeight, vAnchor, nColor, vAnchorScale);
    return LT_OK;
}

LTRESULT CFearTextureString::RenderStringClipped(HTEXTURESTRING hString, ILTDrawPrim *pDrawPrim, const LTRect2n &rClipRect, const LTVector2 &vAnchor, uint32 nColor, const LTVector2 &vAnchorScale, bool bSnapAnchor)
{
    (void)rClipRect;
    return RenderString(hString, pDrawPrim, vAnchor, nColor, vAnchorScale, LTVector2(1.0f, 0.0f), LTVector2(0.0f, 1.0f), LTVector2(1.0f, 1.0f), bSnapAnchor);
}

LTRESULT CFearTextureString::RenderSubString(HTEXTURESTRING hString, const wchar_t *pszString, ILTDrawPrim *pDrawPrim, const LTVector2 &vAnchor, uint32 nColor, const LTVector2 &vGround, const LTVector2 &vDown, const LTVector2 &vStretch)
{
    (void)hString;
    (void)pszString;
    (void)pDrawPrim;
    (void)vAnchor;
    (void)nColor;
    (void)vGround;
    (void)vDown;
    (void)vStretch;
    return LT_OK;
}

LTRESULT CFearTextureString::RenderSubStringClipped(HTEXTURESTRING hString, const wchar_t *pszString, ILTDrawPrim *pDrawPrim, const LTRect2n &rClipRect, const LTVector2 &vAnchor, uint32 nColor)
{
    (void)hString;
    (void)pszString;
    (void)pDrawPrim;
    (void)rClipRect;
    (void)vAnchor;
    (void)nColor;
    return LT_OK;
}
