#ifndef __COPYCODE_H__
#define __COPYCODE_H__

#include <memory.h>
#include "MVInterface.h"

void PlaneCopy(unsigned char* dstp, int dst_pitch, const unsigned char* srcp, int src_pitch, int row_size, int height, bool isse);
void MemZoneSet(unsigned char *ptr, unsigned char value, int width,
                int height, int offsetX, int offsetY, int pitch);

// asm routines
void asm_PlaneCopy_ISSE(unsigned char* dstp, int dst_pitch, const unsigned char* srcp, int src_pitch, int row_size, int height);
void memcpy_amd(void *dest, const void *src, size_t n);

typedef void (COPYFunction)(unsigned char *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch, bool isse);

template<int nBlkWidth, int nBlkHeight>
void Copy_C(BYTE *pDst, int nDstPitch, const BYTE *pSrc, int nSrcPitch, bool isse)
{
    PlaneCopy(pDst, nDstPitch, pSrc, nSrcPitch, nBlkWidth, nBlkHeight, isse);
/*
    for ( int j = 0; j < nBlkHeight; ++j ) {
        memcpy(pDst, pSrc, nBlkWidth);
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
*/
}

#endif
