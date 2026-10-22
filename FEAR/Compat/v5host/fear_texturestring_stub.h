#ifndef __FEAR_TEXTURESTRING_STUB_H__
#define __FEAR_TEXTURESTRING_STUB_H__

#include "ltbasedefs.h"
#include "ilttexturestring.h"

class CFearTextureString : public ILTTextureString
{
public:
    declare_interface(CFearTextureString)
    CFearTextureString() = default;
    ~CFearTextureString() {}

    HCUSTOMFONTFILE RegisterCustomFontFile(const char* pszRelResource) override;
    LTRESULT UnregisterCustomFontFile(HCUSTOMFONTFILE hCustomFontFile) override;
    HTEXTURESTRING CreateTextureString(const wchar_t* pszString, const CFontInfo& Font) override;
    HTEXTURESTRING CreateTextureSubstring(const wchar_t* pszString, HTEXTURESTRING hSrcString) override;
    LTRESULT WordWrapString(HTEXTURESTRING hString, uint32 nWidth) override;
    LTRESULT RecreateTextureString(HTEXTURESTRING hString, const wchar_t* pszString, const CFontInfo& Font) override;
    LTRESULT RecreateTextureSubstring(HTEXTURESTRING hString, const wchar_t* pszString, HTEXTURESTRING hSrcTexture) override;
    LTRESULT GetStringExtents(HTEXTURESTRING hString, LTRect2n& rExtents) override;
    LTRESULT GetStringLength(HTEXTURESTRING hString, uint32* pnLength) override;
    LTRESULT GetString(HTEXTURESTRING hString, wchar_t* pszBuffer, uint32 nBufferLen) override;
    LTRESULT GetFont(HTEXTURESTRING hString, CFontInfo& Font) override;
    LTRESULT GetTextureImage(HTEXTURESTRING hString, HDYNAMICTEXTURE& hTexture) override;
    LTRESULT GetCharRect(HTEXTURESTRING hString, uint32 nCharIndex, LTRect2n& rPlacementRect, LTRect2n& rBlackBox, LTVector2& vUVPos, LTVector2& vUVDims) override;
    void ReleaseTextureString(HTEXTURESTRING hString) override;
    void AddRefTextureString(HTEXTURESTRING hString) override;
    LTRESULT SetupTextRendering(ILTDrawPrim* pDrawPrim) override;
    LTRESULT RenderString(HTEXTURESTRING hString, ILTDrawPrim* pDrawPrim, const LTVector2& vAnchor, uint32 nColor, const LTVector2& vAnchorScale, const LTVector2& vGround, const LTVector2& vDown, const LTVector2& vStretch, bool bSnapAnchor) override;
    LTRESULT RenderStringClipped(HTEXTURESTRING hString, ILTDrawPrim* pDrawPrim, const LTRect2n& rClipRect, const LTVector2& vAnchor, uint32 nColor, const LTVector2& vAnchorScale, bool bSnapAnchor) override;
    LTRESULT RenderSubString(HTEXTURESTRING hString, const wchar_t* pszString, ILTDrawPrim* pDrawPrim, const LTVector2& vAnchor, uint32 nColor, const LTVector2& vGround, const LTVector2& vDown, const LTVector2& vStretch) override;
    LTRESULT RenderSubStringClipped(HTEXTURESTRING hString, const wchar_t* pszString, ILTDrawPrim* pDrawPrim, const LTRect2n& rClipRect, const LTVector2& vAnchor, uint32 nColor) override;
};

extern CFearTextureString g_FearTextureString;

#endif
