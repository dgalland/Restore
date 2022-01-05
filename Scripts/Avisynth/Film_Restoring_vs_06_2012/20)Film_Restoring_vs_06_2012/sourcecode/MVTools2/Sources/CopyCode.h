#ifndef __COPYCODE_H__
#define __COPYCODE_H__

#include <memory.h>
#include "MVInterface.h"

void BitBlt(unsigned char* dstp, int dst_pitch, const unsigned char* srcp, int src_pitch, int row_size, int height, bool isse);
void asm_BitBlt_ISSE(unsigned char* dstp, int dst_pitch, const unsigned char* srcp, int src_pitch, int row_size, int height);
void memcpy_amd(void *dest, const void *src, size_t n);
void MemZoneSet(unsigned char *ptr, unsigned char value, int width,
				int height, int offsetX, int offsetY, int pitch);

typedef void (COPYFunction)(unsigned char *pDst, int nDstPitch,
                            const unsigned char *pSrc, int nSrcPitch);

template<int nBlkWidth, int nBlkHeight>
void Copy_C(BYTE *pDst, int nDstPitch, const BYTE *pSrc, int nSrcPitch)
{
   for ( int j = 0; j < nBlkHeight; j++ )
   {
//      for ( int i = 0; i < nBlkWidth; i++ )  //  waste cycles removed by Fizick in v1.2
         memcpy(pDst, pSrc, nBlkWidth);
      pDst += nDstPitch;
      pSrc += nSrcPitch;
   }
}

template<int nBlkSize>
void Copy_C(BYTE *pDst, int nDstPitch, const BYTE *pSrc, int nSrcPitch)
{
   Copy_C<nBlkSize, nBlkSize>(pDst, nDstPitch, pSrc, nSrcPitch);
}
// even sizes,
template<int nBlkWidth, int nBlkHeight>
void Copy_mmx(BYTE *pDst, int nDstPitch, const BYTE *pSrc, int nSrcPitch)
{
   int j = 0;
   for ( ; j < nBlkHeight-15; j += 16 )
   {
      int i = 0;
      for ( ; i < nBlkWidth-15; i += 16 )
         Copy16x16_mmx(pDst + i + j * nDstPitch, nDstPitch, pSrc + i + j * nSrcPitch, nSrcPitch);
      for ( ; i < nBlkWidth-7; i += 8 )
      {
         Copy8x16_mmx(pDst + i + j * nDstPitch, nDstPitch, pSrc + i + j * nSrcPitch, nSrcPitch);
      }
      for ( ; i < nBlkWidth-3; i += 4 )
      {
         Copy4x8_mmx(pDst + i + j * nDstPitch, nDstPitch, pSrc + i + j * nSrcPitch, nSrcPitch);
         Copy4x8_mmx(pDst + i + (j + 8) * nDstPitch, nDstPitch, pSrc + i + (j + 8) * nSrcPitch, nSrcPitch);
      }
      for ( ; i < nBlkWidth-1; i += 2 )
      {
         Copy2x4_mmx(pDst + i + j * nDstPitch, nDstPitch, pSrc + i + j * nSrcPitch, nSrcPitch);
         Copy2x4_mmx(pDst + i + (j + 4) * nDstPitch, nDstPitch, pSrc + i + (j + 4) * nSrcPitch, nSrcPitch);
         Copy2x4_mmx(pDst + i + (j + 8) * nDstPitch, nDstPitch, pSrc + i + (j + 8) * nSrcPitch, nSrcPitch);
         Copy2x4_mmx(pDst + i + (j + 12) * nDstPitch, nDstPitch, pSrc + i + (j + 12) * nSrcPitch, nSrcPitch);
      }
   }
   for ( ; j < nBlkHeight-7; j += 8 )
   {
      int i = 0;
      for ( ; i < nBlkWidth-7; i += 8 )
         Copy8x8_mmx(pDst + i + j * nDstPitch, nDstPitch, pSrc + i + j * nSrcPitch, nSrcPitch);
      for ( ; i < nBlkWidth-3; i += 4 )
      {
         Copy4x8_mmx(pDst + i + j * nDstPitch, nDstPitch, pSrc + i + j * nSrcPitch, nSrcPitch);
      }
      for ( ; i < nBlkWidth-1; i += 2 )
      {
         Copy2x4_mmx(pDst + i + j * nDstPitch, nDstPitch, pSrc + i + j * nSrcPitch, nSrcPitch);
         Copy2x4_mmx(pDst + i + (4 + j) * nDstPitch, nDstPitch, pSrc + i + 4 + j * nSrcPitch, nSrcPitch);
      }
   }
   for ( ; j < nBlkHeight-3; j += 4 )
   {
      int i = 0;
      for ( ; i < nBlkWidth-7; i += 8 )
         Copy8x4_mmx(pDst + i + j * nDstPitch, nDstPitch, pSrc + i + j * nSrcPitch, nSrcPitch);
      for ( ; i < nBlkWidth-3; i += 4 )
         Copy4x4_mmx(pDst + i + j * nDstPitch, nDstPitch, pSrc + i + j * nSrcPitch, nSrcPitch);
      for ( ; i < nBlkWidth-1; i += 2 )
      {
         Copy2x4_mmx(pDst + i + j * nDstPitch, nDstPitch, pSrc + i + j * nSrcPitch, nSrcPitch);
      }
   }
   for ( ; j < nBlkHeight-1; j += 2 )
   {
      int i = 0;
      for ( ; i < nBlkWidth-7; i += 8 )
         Copy8x2_mmx(pDst + i + j * nDstPitch, nDstPitch, pSrc + i + j * nSrcPitch, nSrcPitch);
      for ( ; i < nBlkWidth-3; i += 4 )
         Copy4x2_mmx(pDst + i + j * nDstPitch, nDstPitch, pSrc + i + j * nSrcPitch, nSrcPitch);
      for ( ; i < nBlkWidth-1; i += 2 )
         Copy2x2_mmx(pDst + i + j * nDstPitch, nDstPitch, pSrc + i + j * nSrcPitch, nSrcPitch);
   }
   for ( ; j < nBlkHeight; j += 1 )
   {
      int i = 0;
      for ( ; i < nBlkWidth-7; i += 8 )
         Copy8x1_mmx(pDst + i + j * nDstPitch, nDstPitch, pSrc + i + j * nSrcPitch, nSrcPitch);
      for ( ; i < nBlkWidth; i += 1 )
         *(pDst + i + j * nDstPitch) = *(pSrc + i + j * nSrcPitch);
   }
}
/*
extern "C" void __cdecl Copy32x16_mmx(BYTE *pDst, int nDstPitch,
                                   const BYTE *pSrc, int nSrcPitch);

extern "C" void __cdecl Copy16x16_mmx(BYTE *pDst, int nDstPitch,
                                   const BYTE *pSrc, int nSrcPitch);

extern "C" void __cdecl Copy8x8_mmx(BYTE *pDst, int nDstPitch,
                                  const BYTE *pSrc, int nSrcPitch);

extern "C" void __cdecl Copy4x4_mmx(BYTE *pDst, int nDstPitch,
                                  const BYTE *pSrc, int nSrcPitch);

extern "C" void __cdecl Copy2x2_mmx(BYTE *pDst, int nDstPitch,
                                  const BYTE *pSrc, int nSrcPitch);

extern "C" void __cdecl Copy4x8_mmx(BYTE *pDst, int nDstPitch,
                                  const BYTE *pSrc, int nSrcPitch);

extern "C" void __cdecl Copy8x16_mmx(BYTE *pDst, int nDstPitch,
                                  const BYTE *pSrc, int nSrcPitch);

extern "C" void __cdecl Copy8x4_mmx(BYTE *pDst, int nDstPitch,
                                  const BYTE *pSrc, int nSrcPitch);
extern "C" void __cdecl Copy4x2_mmx(BYTE *pDst, int nDstPitch,
                                  const BYTE *pSrc, int nSrcPitch);
extern "C" void __cdecl Copy16x8_mmx(BYTE *pDst, int nDstPitch,
                                  const BYTE *pSrc, int nSrcPitch);
extern "C" void __cdecl Copy2x4_mmx(BYTE *pDst, int nDstPitch,
                                  const BYTE *pSrc, int nSrcPitch);

extern "C" void __cdecl Copy16x2_mmx(BYTE *pDst, int nDstPitch,
                                  const BYTE *pSrc, int nSrcPitch);

extern "C" void __cdecl Copy8x2_mmx(BYTE *pDst, int nDstPitch,
                                  const BYTE *pSrc, int nSrcPitch);

extern "C" void __cdecl Copy8x1_mmx(BYTE *pDst, int nDstPitch,
                                  const BYTE *pSrc, int nSrcPitch);
*/
//extern "C" void __cdecl Copy16x16_mmx(BYTE *pDst, int nDstPitch, const BYTE *pSrc, int nSrcPitch);
#define MK_CFUNC(functionname) extern "C" void __cdecl functionname (BYTE *pDst, int nDstPitch, const BYTE *pSrc, int nSrcPitch)
//default functions
MK_CFUNC(Copy32x16_mmx);
MK_CFUNC(Copy16x16_mmx);
MK_CFUNC(Copy16x8_mmx);
MK_CFUNC(Copy16x2_mmx);
MK_CFUNC(Copy8x16_mmx);
MK_CFUNC(Copy8x8_mmx);
MK_CFUNC(Copy8x4_mmx);
MK_CFUNC(Copy8x2_mmx);
MK_CFUNC(Copy8x1_mmx);
MK_CFUNC(Copy4x8_mmx);
MK_CFUNC(Copy4x4_mmx);
MK_CFUNC(Copy4x2_mmx);
MK_CFUNC(Copy2x4_mmx);
MK_CFUNC(Copy2x2_mmx);
MK_CFUNC(Copy2x1_mmx);

/*
//new functions derived from x264
// unfortunately slower than default
MK_CFUNC(copy_mc_16x16_mmx);
MK_CFUNC(copy_mc_16x8_mmx);
MK_CFUNC(copy_mc_8x16_mmx);
MK_CFUNC(copy_mc_8x8_mmx);
MK_CFUNC(copy_mc_8x4_mmx);
MK_CFUNC(copy_mc_8x2_mmx);
MK_CFUNC(copy_mc_4x8_mmx);
MK_CFUNC(copy_mc_4x4_mmx);
MK_CFUNC(copy_mc_4x2_mmx);

//uses 128 bit blocks, destination MUST be aligned!
MK_CFUNC(copy_mc_16x16_sse2);
MK_CFUNC(copy_mc_16x8_sse2);
MK_CFUNC(copy_mc_16x16_sse3);
MK_CFUNC(copy_mc_16x8_sse3);
MK_CFUNC(copy_mc_16x16_aligned_sse2);//this means, that both in&output must be aligned
MK_CFUNC(copy_mc_16x8_aligned_sse2);
*/
#undef MK_CFUNC

#endif
