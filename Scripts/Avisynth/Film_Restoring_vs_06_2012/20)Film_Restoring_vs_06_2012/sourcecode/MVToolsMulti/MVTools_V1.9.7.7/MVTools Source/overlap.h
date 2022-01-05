#ifndef __OVERLAP__
#define __OVERLAP__

#include "math.h"
#define M_PI       3.14159265358979323846f

// top, middle, botom and left, middle, right windows
#define OW_TL 0
#define OW_TM 1
#define OW_TR 2
#define OW_ML 3
#define OW_MM 4
#define OW_MR 5
#define OW_BL 6
#define OW_BM 7
#define OW_BR 8

class OverlapWindows
{
    int nx; // window sizes
    int ny;
    int ox; // overap sizes
    int oy;
    int size; // full window size= nx*ny

    short *Overlap9Windows;
    short *Overlap9Windows_Ptrs[9]; // overlap windows pointers

    float *fWin1UVx;
    float *fWin1UVxfirst;
    float *fWin1UVxlast;
    float *fWin1UVy;
    float *fWin1UVyfirst;
    float *fWin1UVylast;
public :

    OverlapWindows(int _nx, int _ny, int _ox, int _oy);
    ~OverlapWindows();

    inline int Getnx() const { return nx; }
    inline int Getny() const { return ny; }
    inline int GetSize() const { return size; }
    inline short *GetWindow(int i) const { return Overlap9Windows_Ptrs[i]; }
};

typedef void (OverlapsFunction)(unsigned short *pDst, int nDstPitch,
                                const unsigned char *pSrc, int nSrcPitch,
                                short *pWin, int nWinPitch);

template<int const width, int const height>
void Overlaps_C(unsigned short *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch)
{
    int nDstPitch1=nDstPitch-width;
    int nSrcPitch1=nSrcPitch-width;
    int nWinPitch1=nWinPitch-width;

    for (int j=0; j<height; ++j) {
        for (int k=0; k<width; ++k) {
            (*(pDst++)) += (((*(pSrc++))*(*(pWin++))+256)>>6);
        }
        pDst += nDstPitch1;
        pSrc += nSrcPitch1;
        pWin += nWinPitch1;
    }
}

extern "C" void __cdecl  Overlaps32x16_mmx(unsigned short *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch);
extern "C" void __cdecl  Overlaps16x16_mmx(unsigned short *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch);
extern "C" void __cdecl  Overlaps8x16_mmx (unsigned short *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch);
extern "C" void __cdecl  Overlaps8x8_mmx  (unsigned short *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch);
extern "C" void __cdecl  Overlaps4x8_mmx  (unsigned short *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch);
extern "C" void __cdecl  Overlaps4x4_mmx  (unsigned short *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch);
extern "C" void __cdecl  Overlaps2x4_mmx  (unsigned short *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch);
extern "C" void __cdecl  Overlaps2x2_mmx  (unsigned short *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch);
extern "C" void __cdecl  Overlaps8x4_mmx  (unsigned short *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch);
extern "C" void __cdecl  Overlaps4x2_mmx  (unsigned short *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch);
extern "C" void __cdecl  Overlaps16x8_mmx (unsigned short *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch);
extern "C" void __cdecl  Overlaps16x2_mmx (unsigned short *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch);
extern "C" void __cdecl  Overlaps8x2_mmx  (unsigned short *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch);
extern "C" void __cdecl  Overlaps8x1_mmx  (unsigned short *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch);

void Short2Bytes(unsigned char *pDst, int nDstPitch, unsigned short *pDstShort, int dstShortPitch, 
                 int nWidth, int nHeight);

void LimitChanges_c(unsigned char *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch, 
                    int nWidth, int nHeight, int nLimit);

extern "C" void  __cdecl  LimitChanges_mmx(unsigned char *pDst, int nDstPitch, 
                                           const unsigned char *pSrc, int nSrcPitch,
                                           int nWidth, int nHeight, int nLimit);

#endif