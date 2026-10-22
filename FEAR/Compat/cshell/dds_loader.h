#ifndef __FEAR_DDS_LOADER_H__
#define __FEAR_DDS_LOADER_H__

struct FearDDSImage
{
    unsigned int m_Width;
    unsigned int m_Height;
    unsigned char *m_pPixels;
};

bool FearDDS_Load(const unsigned char *pData, unsigned int nDataSize, FearDDSImage &image);
void FearDDS_Free(FearDDSImage &image);

#endif
