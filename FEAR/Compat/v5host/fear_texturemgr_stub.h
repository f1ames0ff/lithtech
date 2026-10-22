#ifndef __FEAR_TEXTUREMGR_STUB_H__
#define __FEAR_TEXTUREMGR_STUB_H__

#include "ltbasedefs.h"
#include "ilttexturemgr.h"

class CFearTextureMgr : public ILTTextureMgr
{
public:
    declare_interface(CFearTextureMgr)
    CFearTextureMgr() = default;
    ~CFearTextureMgr() {}

    LTRESULT CreateTextureFromFile(HTEXTURE &hTexture, const char *pFilename) override;
    LTRESULT GetTextureDims(HTEXTURE hTexture, uint32 &nWidth, uint32 &nHeight) override;
    LTRESULT GetTextureDims(HDYNAMICTEXTURE hTexture, uint32 &nWidth, uint32 &nHeight) override;
    LTRESULT ReleaseTexture(HTEXTURE hTexture) override;
    LTRESULT AddTextureRef(HTEXTURE hTexture) override;
    LTRESULT CreateDynamicTexture(HDYNAMICTEXTURE &hTexture, const CTextureData& TextureData) override;
    LTRESULT CreateDynamicTexture(HDYNAMICTEXTURE &hTexture, ILTInStream* pInFile) override;
    LTRESULT ReleaseDynamicTexture(HDYNAMICTEXTURE hTexture) override;
    LTRESULT AddDynamicTextureRef(HDYNAMICTEXTURE hTexture) override;
};

extern CFearTextureMgr g_FearTextureMgr;

#endif
