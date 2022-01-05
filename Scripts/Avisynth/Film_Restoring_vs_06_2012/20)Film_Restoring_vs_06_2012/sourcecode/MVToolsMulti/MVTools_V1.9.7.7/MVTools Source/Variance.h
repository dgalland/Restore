#ifndef __VARIANCE_H__
#define __VARIANCE_H__

typedef unsigned int (VARFunction)(const unsigned char *pSrc, int nSrcPitch, int *pLuma);

template<int nBlkWidth, int nBlkHeight>
unsigned int Var_C(const unsigned char *pSrc, int nSrcPitch, int *pLuma)
{
    const unsigned char *s = pSrc;
    int meanLuma = 0;
    int meanVariance = 0;
    int nSrcPitch1=nSrcPitch-nBlkWidth;
    for ( int j = 0; j < nBlkHeight; ++j ) {
        for ( int i = 0; i < nBlkWidth; ++i)
            meanLuma += *(s++);
        s += nSrcPitch1;
    }
    *pLuma = meanLuma;
    meanLuma = (meanLuma + ((nBlkWidth * nBlkHeight) >> 1)) / (nBlkWidth * nBlkHeight);
    s = pSrc;
    for ( int j = 0; j < nBlkHeight; ++j ) {
        for ( int i = 0; i < nBlkWidth; ++i ) {
            int temp=*(s++) - meanLuma;
            meanVariance+=abs(temp);
        }
        s += nSrcPitch1;
    }
    return meanVariance;
}

extern "C" unsigned int __cdecl Var32x16_iSSE(const unsigned char *pSrc, int nSrcPitch, int *pLuma);
extern "C" unsigned int __cdecl Var16x16_iSSE(const unsigned char *pSrc, int nSrcPitch, int *pLuma);
extern "C" unsigned int __cdecl Var8x8_iSSE  (const unsigned char *pSrc, int nSrcPitch, int *pLuma);
extern "C" unsigned int __cdecl Var4x4_iSSE  (const unsigned char *pSrc, int nSrcPitch, int *pLuma);
extern "C" unsigned int __cdecl Var8x4_iSSE  (const unsigned char *pSrc, int nSrcPitch, int *pLuma);
extern "C" unsigned int __cdecl Var16x8_iSSE (const unsigned char *pSrc, int nSrcPitch, int *pLuma);
extern "C" unsigned int __cdecl Var16x2_iSSE (const unsigned char *pSrc, int nSrcPitch, int *pLuma);

typedef unsigned int (LUMAFunction)(const unsigned char *pSrc, int nSrcPitch);

template<int nBlkWidth, int nBlkHeight>
unsigned int Luma_C(const unsigned char *pSrc, int nSrcPitch)
{
    const unsigned char *s = pSrc;
    int meanLuma = 0;
    int nSrcPitch1=nSrcPitch-nBlkWidth;
    for ( int j = 0; j < nBlkHeight; ++j ) {
        for ( int i = 0; i < nBlkWidth; ++i )
            meanLuma += *(s++);
        s += nSrcPitch1;
    }
    return meanLuma;
}

extern "C" unsigned int __cdecl Luma32x16_iSSE(const unsigned char *pSrc, int nSrcPitch);
extern "C" unsigned int __cdecl Luma16x16_iSSE(const unsigned char *pSrc, int nSrcPitch);
extern "C" unsigned int __cdecl Luma8x8_iSSE  (const unsigned char *pSrc, int nSrcPitch);
extern "C" unsigned int __cdecl Luma4x4_iSSE  (const unsigned char *pSrc, int nSrcPitch);
extern "C" unsigned int __cdecl Luma8x4_iSSE  (const unsigned char *pSrc, int nSrcPitch);
extern "C" unsigned int __cdecl Luma16x8_iSSE (const unsigned char *pSrc, int nSrcPitch);
extern "C" unsigned int __cdecl Luma16x2_iSSE (const unsigned char *pSrc, int nSrcPitch);

#endif
