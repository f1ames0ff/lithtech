#include "dds_loader.h"

#include <string.h>
#include <new>

namespace
{

const unsigned int FEAR_DDS_HEADER_SIZE = 128;
const unsigned int FEAR_DDS_MAGIC = 0x20534444;
const unsigned int FEAR_DDS_MAX_DIMENSION = 16384;
const size_t FEAR_DDS_MAX_IMAGE_BYTES = 256 * 1024 * 1024;

const unsigned int FEAR_DDS_PIXELFORMAT_ALPHAPIXELS = 0x00000001;
const unsigned int FEAR_DDS_PIXELFORMAT_FOURCC = 0x00000004;
const unsigned int FEAR_DDS_PIXELFORMAT_RGB = 0x00000040;
const unsigned int FEAR_DDS_HEADER_PITCH = 0x00000008;

#pragma pack(push, 1)
struct FearDDSHeader
{
    unsigned int m_Magic;
    unsigned int m_Size;
    unsigned int m_Flags;
    unsigned int m_Height;
    unsigned int m_Width;
    unsigned int m_PitchOrLinearSize;
    unsigned int m_Depth;
    unsigned int m_MipMapCount;
    unsigned int m_Reserved1[11];
    unsigned int m_PixelFormatSize;
    unsigned int m_PixelFormatFlags;
    unsigned int m_FourCC;
    unsigned int m_RGBBitCount;
    unsigned int m_RBitMask;
    unsigned int m_GBitMask;
    unsigned int m_BBitMask;
    unsigned int m_ABitMask;
    unsigned int m_Caps;
    unsigned int m_Caps2;
    unsigned int m_Caps3;
    unsigned int m_Caps4;
    unsigned int m_Reserved2;
};
#pragma pack(pop)

static_assert(sizeof(FearDDSHeader) == FEAR_DDS_HEADER_SIZE, "DDS header layout mismatch");

unsigned int FearDDSTextToFourCC(const char *pText)
{
    return (unsigned int)(unsigned char)pText[0] |
        ((unsigned int)(unsigned char)pText[1] << 8) |
        ((unsigned int)(unsigned char)pText[2] << 16) |
        ((unsigned int)(unsigned char)pText[3] << 24);
}

unsigned char FearDDSExpand5(unsigned int nValue)
{
    return (unsigned char)((nValue << 3) | (nValue >> 2));
}

unsigned char FearDDSExpand6(unsigned int nValue)
{
    return (unsigned char)((nValue << 2) | (nValue >> 4));
}

void FearDDSReadChannel(unsigned int nPixel, unsigned int nMask, unsigned char *pOut)
{
    if (nMask == 0)
    {
        *pOut = 255;
        return;
    }

    unsigned int nShift = 0;
    while (((nMask >> nShift) & 1u) == 0)
        ++nShift;

    unsigned int nBits = 0;
    unsigned int nRemaining = nMask >> nShift;
    while ((nRemaining & 1u) != 0)
    {
        ++nBits;
        nRemaining >>= 1;
    }

    unsigned int nValue = (nPixel & nMask) >> nShift;
    if (nBits >= 32)
    {
        *pOut = (unsigned char)nValue;
        return;
    }

    unsigned long long nMaxValue = (1ull << nBits) - 1ull;
    *pOut = (unsigned char)(((unsigned long long)nValue * 255ull + nMaxValue / 2ull) / nMaxValue);
}

void FearDDSDecodeColorBlock(const unsigned char *pBlock, bool bFourColor,
    unsigned char *pPixels, unsigned int nPitch, unsigned int nWidth, unsigned int nHeight,
    unsigned int nBlockX, unsigned int nBlockY)
{
    unsigned int nColor0 = (unsigned int)pBlock[0] | ((unsigned int)pBlock[1] << 8);
    unsigned int nColor1 = (unsigned int)pBlock[2] | ((unsigned int)pBlock[3] << 8);

    unsigned char nRed[4];
    unsigned char nGreen[4];
    unsigned char nBlue[4];
    unsigned char nAlpha[4];

    nRed[0] = FearDDSExpand5((nColor0 >> 11) & 0x1F);
    nGreen[0] = FearDDSExpand6((nColor0 >> 5) & 0x3F);
    nBlue[0] = FearDDSExpand5(nColor0 & 0x1F);
    nAlpha[0] = 255;

    nRed[1] = FearDDSExpand5((nColor1 >> 11) & 0x1F);
    nGreen[1] = FearDDSExpand6((nColor1 >> 5) & 0x3F);
    nBlue[1] = FearDDSExpand5(nColor1 & 0x1F);
    nAlpha[1] = 255;

    if (bFourColor || nColor0 > nColor1)
    {
        nRed[2] = (unsigned char)((2 * nRed[0] + nRed[1]) / 3);
        nGreen[2] = (unsigned char)((2 * nGreen[0] + nGreen[1]) / 3);
        nBlue[2] = (unsigned char)((2 * nBlue[0] + nBlue[1]) / 3);
        nAlpha[2] = 255;

        nRed[3] = (unsigned char)((nRed[0] + 2 * nRed[1]) / 3);
        nGreen[3] = (unsigned char)((nGreen[0] + 2 * nGreen[1]) / 3);
        nBlue[3] = (unsigned char)((nBlue[0] + 2 * nBlue[1]) / 3);
        nAlpha[3] = 255;
    }
    else
    {
        nRed[2] = (unsigned char)((nRed[0] + nRed[1]) / 2);
        nGreen[2] = (unsigned char)((nGreen[0] + nGreen[1]) / 2);
        nBlue[2] = (unsigned char)((nBlue[0] + nBlue[1]) / 2);
        nAlpha[2] = 255;

        nRed[3] = 0;
        nGreen[3] = 0;
        nBlue[3] = 0;
        nAlpha[3] = 0;
    }

    for (unsigned int nY = 0; nY < 4; ++nY)
    {
        if (nBlockY + nY >= nHeight)
            break;

        unsigned int nIndices = pBlock[4 + nY];
        size_t nRowOffset = (size_t)(nBlockY + nY) * nPitch;

        for (unsigned int nX = 0; nX < 4; ++nX)
        {
            if (nBlockX + nX >= nWidth)
                break;

            unsigned int nIndex = (nIndices >> (nX * 2)) & 3u;
            unsigned char *pDestination = pPixels + nRowOffset + (size_t)(nBlockX + nX) * 4;
            pDestination[0] = nRed[nIndex];
            pDestination[1] = nGreen[nIndex];
            pDestination[2] = nBlue[nIndex];
            pDestination[3] = nAlpha[nIndex];
        }
    }
}

void FearDDSApplyAlphaNibbles(const unsigned char *pBlock, unsigned char *pPixels,
    unsigned int nPitch, unsigned int nWidth, unsigned int nHeight,
    unsigned int nBlockX, unsigned int nBlockY)
{
    for (unsigned int nY = 0; nY < 4; ++nY)
    {
        if (nBlockY + nY >= nHeight)
            break;

        unsigned int nRowAlpha = (unsigned int)pBlock[nY * 2] | ((unsigned int)pBlock[nY * 2 + 1] << 8);
        unsigned char *pRow = pPixels + (size_t)(nBlockY + nY) * nPitch;

        for (unsigned int nX = 0; nX < 4; ++nX)
        {
            if (nBlockX + nX >= nWidth)
                break;

            unsigned int nAlpha = (nRowAlpha >> (nX * 4)) & 0xFu;
            pRow[(size_t)(nBlockX + nX) * 4 + 3] = (unsigned char)(nAlpha * 17);
        }
    }
}

void FearDDSApplyAlphaDXT5(const unsigned char *pBlock, unsigned char *pPixels,
    unsigned int nPitch, unsigned int nWidth, unsigned int nHeight,
    unsigned int nBlockX, unsigned int nBlockY)
{
    unsigned int nAlpha0 = pBlock[0];
    unsigned int nAlpha1 = pBlock[1];

    unsigned char nPalette[8];
    nPalette[0] = (unsigned char)nAlpha0;
    nPalette[1] = (unsigned char)nAlpha1;

    if (nAlpha0 > nAlpha1)
    {
        nPalette[2] = (unsigned char)((6 * nAlpha0 + nAlpha1) / 7);
        nPalette[3] = (unsigned char)((5 * nAlpha0 + 2 * nAlpha1) / 7);
        nPalette[4] = (unsigned char)((4 * nAlpha0 + 3 * nAlpha1) / 7);
        nPalette[5] = (unsigned char)((3 * nAlpha0 + 4 * nAlpha1) / 7);
        nPalette[6] = (unsigned char)((2 * nAlpha0 + 5 * nAlpha1) / 7);
        nPalette[7] = (unsigned char)((nAlpha0 + 6 * nAlpha1) / 7);
    }
    else
    {
        nPalette[2] = (unsigned char)((4 * nAlpha0 + nAlpha1) / 5);
        nPalette[3] = (unsigned char)((3 * nAlpha0 + 2 * nAlpha1) / 5);
        nPalette[4] = (unsigned char)((2 * nAlpha0 + 3 * nAlpha1) / 5);
        nPalette[5] = (unsigned char)((nAlpha0 + 4 * nAlpha1) / 5);
        nPalette[6] = 0;
        nPalette[7] = 255;
    }

    unsigned long long nIndices = 0;
    for (unsigned int i = 0; i < 6; ++i)
        nIndices |= (unsigned long long)pBlock[2 + i] << (i * 8);

    for (unsigned int nY = 0; nY < 4; ++nY)
    {
        if (nBlockY + nY >= nHeight)
            break;

        unsigned char *pRow = pPixels + (size_t)(nBlockY + nY) * nPitch;

        for (unsigned int nX = 0; nX < 4; ++nX)
        {
            if (nBlockX + nX >= nWidth)
                break;

            unsigned int nPixelIndex = nY * 4 + nX;
            unsigned int nIndex = (unsigned int)((nIndices >> (nPixelIndex * 3)) & 7ull);
            pRow[(size_t)(nBlockX + nX) * 4 + 3] = nPalette[nIndex];
        }
    }
}

void FearDDSDecodeDXT(const unsigned char *pPayload, unsigned int nWidth, unsigned int nHeight,
    unsigned int nFormat, unsigned int nBlockBytes, unsigned char *pPixels)
{
    unsigned int nPitch = nWidth * 4;
    size_t nBlocksX = ((size_t)nWidth + 3) / 4;

    for (unsigned int nBlockY = 0; nBlockY < nHeight; nBlockY += 4)
    {
        for (unsigned int nBlockX = 0; nBlockX < nWidth; nBlockX += 4)
        {
            size_t nBlockIndex = ((size_t)nBlockY / 4) * nBlocksX + ((size_t)nBlockX / 4);
            const unsigned char *pBlock = pPayload + nBlockIndex * nBlockBytes;

            if (nFormat == 1)
            {
                FearDDSDecodeColorBlock(pBlock, false, pPixels, nPitch, nWidth, nHeight, nBlockX, nBlockY);
            }
            else if (nFormat == 3)
            {
                FearDDSDecodeColorBlock(pBlock + 8, true, pPixels, nPitch, nWidth, nHeight, nBlockX, nBlockY);
                FearDDSApplyAlphaNibbles(pBlock, pPixels, nPitch, nWidth, nHeight, nBlockX, nBlockY);
            }
            else
            {
                FearDDSDecodeColorBlock(pBlock + 8, true, pPixels, nPitch, nWidth, nHeight, nBlockX, nBlockY);
                FearDDSApplyAlphaDXT5(pBlock, pPixels, nPitch, nWidth, nHeight, nBlockX, nBlockY);
            }
        }
    }
}

bool FearDDSDecodeRaw(const FearDDSHeader &header, const unsigned char *pPayload,
    unsigned int nPayloadSize, unsigned char *pPixels)
{
    if (header.m_RGBBitCount == 0 || (header.m_RGBBitCount % 8) != 0)
        return false;

    unsigned int nBytesPerPixel = header.m_RGBBitCount / 8;
    if (nBytesPerPixel < 2 || nBytesPerPixel > 4)
        return false;

    if (header.m_RBitMask == 0 || header.m_GBitMask == 0 || header.m_BBitMask == 0)
        return false;

    unsigned int nRowBytes = header.m_Width * nBytesPerPixel;
    unsigned int nPitch = nRowBytes;

    if ((header.m_Flags & FEAR_DDS_HEADER_PITCH) != 0 &&
        header.m_PitchOrLinearSize >= nRowBytes &&
        header.m_PitchOrLinearSize <= 0x40000000)
    {
        unsigned long long nPitchedRequired = (unsigned long long)(header.m_Height - 1) *
            header.m_PitchOrLinearSize + nRowBytes;
        if (nPitchedRequired <= (unsigned long long)nPayloadSize)
            nPitch = header.m_PitchOrLinearSize;
    }

    unsigned long long nRequired = (unsigned long long)(header.m_Height - 1) * nPitch + nRowBytes;
    if (nRequired > (unsigned long long)nPayloadSize)
        return false;

    unsigned int nPitchOut = header.m_Width * 4;
    bool bHasAlpha = (header.m_PixelFormatFlags & FEAR_DDS_PIXELFORMAT_ALPHAPIXELS) != 0 &&
        header.m_ABitMask != 0;

    for (unsigned int nY = 0; nY < header.m_Height; ++nY)
    {
        const unsigned char *pSourceRow = pPayload + (size_t)nY * nPitch;
        unsigned char *pDestinationRow = pPixels + (size_t)nY * nPitchOut;

        for (unsigned int nX = 0; nX < header.m_Width; ++nX)
        {
            const unsigned char *pSource = pSourceRow + (size_t)nX * nBytesPerPixel;
            unsigned int nPixel = (unsigned int)pSource[0] | ((unsigned int)pSource[1] << 8);
            if (nBytesPerPixel > 2)
                nPixel |= (unsigned int)pSource[2] << 16;
            if (nBytesPerPixel > 3)
                nPixel |= (unsigned int)pSource[3] << 24;

            unsigned char *pDestination = pDestinationRow + (size_t)nX * 4;
            FearDDSReadChannel(nPixel, header.m_RBitMask, &pDestination[0]);
            FearDDSReadChannel(nPixel, header.m_GBitMask, &pDestination[1]);
            FearDDSReadChannel(nPixel, header.m_BBitMask, &pDestination[2]);

            if (bHasAlpha)
                FearDDSReadChannel(nPixel, header.m_ABitMask, &pDestination[3]);
            else
                pDestination[3] = 255;
        }
    }

    return true;
}

bool FearDDSDecode(const unsigned char *pData, unsigned int nDataSize, FearDDSImage &image)
{
    if (nDataSize < FEAR_DDS_HEADER_SIZE)
        return false;

    FearDDSHeader header;
    memcpy(&header, pData, sizeof(header));

    if (header.m_Magic != FEAR_DDS_MAGIC || header.m_Size != 124 || header.m_PixelFormatSize != 32)
        return false;

    if (header.m_Width == 0 || header.m_Height == 0)
        return false;

    if (header.m_Width > FEAR_DDS_MAX_DIMENSION || header.m_Height > FEAR_DDS_MAX_DIMENSION)
        return false;

    size_t nPixels = (size_t)header.m_Width * (size_t)header.m_Height;
    if (nPixels > FEAR_DDS_MAX_IMAGE_BYTES / 4)
        return false;

    unsigned char *pPixels = new (std::nothrow) unsigned char[nPixels * 4];
    if (!pPixels)
        return false;

    const unsigned char *pPayload = pData + FEAR_DDS_HEADER_SIZE;
    unsigned int nPayloadSize = nDataSize - FEAR_DDS_HEADER_SIZE;

    bool bDecoded = false;

    if ((header.m_PixelFormatFlags & FEAR_DDS_PIXELFORMAT_FOURCC) != 0)
    {
        unsigned int nFormat = 0;
        unsigned int nBlockBytes = 0;

        if (header.m_FourCC == FearDDSTextToFourCC("DXT1"))
        {
            nFormat = 1;
            nBlockBytes = 8;
        }
        else if (header.m_FourCC == FearDDSTextToFourCC("DXT3"))
        {
            nFormat = 3;
            nBlockBytes = 16;
        }
        else if (header.m_FourCC == FearDDSTextToFourCC("DXT5"))
        {
            nFormat = 5;
            nBlockBytes = 16;
        }

        if (nBlockBytes != 0)
        {
            unsigned long long nBlocksX = ((unsigned long long)header.m_Width + 3) / 4;
            unsigned long long nBlocksY = ((unsigned long long)header.m_Height + 3) / 4;
            unsigned long long nRequired = nBlocksX * nBlocksY * nBlockBytes;

            if (nRequired <= (unsigned long long)nPayloadSize)
            {
                FearDDSDecodeDXT(pPayload, header.m_Width, header.m_Height, nFormat, nBlockBytes, pPixels);
                bDecoded = true;
            }
        }
    }
    else if ((header.m_PixelFormatFlags & FEAR_DDS_PIXELFORMAT_RGB) != 0)
    {
        bDecoded = FearDDSDecodeRaw(header, pPayload, nPayloadSize, pPixels);
    }

    if (!bDecoded)
    {
        delete [] pPixels;
        return false;
    }

    image.m_Width = header.m_Width;
    image.m_Height = header.m_Height;
    image.m_pPixels = pPixels;
    return true;
}

}

bool FearDDS_Load(const unsigned char *pData, unsigned int nDataSize, FearDDSImage &image)
{
    if (!pData)
        return false;

    FearDDSImage decoded;
    decoded.m_Width = 0;
    decoded.m_Height = 0;
    decoded.m_pPixels = NULL;

    if (!FearDDSDecode(pData, nDataSize, decoded))
        return false;

    FearDDS_Free(image);
    image = decoded;
    return true;
}

void FearDDS_Free(FearDDSImage &image)
{
    if (image.m_pPixels)
        delete [] image.m_pPixels;

    image.m_Width = 0;
    image.m_Height = 0;
    image.m_pPixels = NULL;
}
