#ifndef __Mt_MaskFunctions_H__
#define __Mt_MaskFunctions_H__

#include "../../common/common.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask {

typedef Byte (Operator)(Byte, Byte, Byte, Byte, Byte, Byte, Byte, Byte, Byte, const Word matrix[10], int nLowThreshold, int nHighThreshold);

template<Operator op, class T>
void generic_c(Byte *pDst, int nDstPitch, const Byte *pSrc, int nSrcPitch, 
               T &thresholds, const Word matrix[10], int nWidth, int nHeight)
{
   const Byte *pSrcp = pSrc - nSrcPitch;
   const Byte *pSrcn = pSrc + nSrcPitch;

   /* top-left */
   pDst[0] = op(pSrc[0], pSrc[0], pSrc[1], pSrc[0], pSrc[0], pSrc[1], pSrcn[0], pSrcn[0], pSrcn[1], matrix, thresholds.min(0), thresholds.max(0));

   /* top */
   for ( int x = 1; x < nWidth - 1; x++ )
      pDst[x] = op(pSrc[x-1], pSrc[x], pSrc[x+1], pSrc[x-1], pSrc[x], pSrc[x+1], pSrcn[x-1], pSrcn[x], pSrcn[x+1], matrix, thresholds.min(x), thresholds.max(x));

   /* top-right */
   pDst[nWidth-1] = op(pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], pSrcn[nWidth-2], pSrcn[nWidth-1], pSrcn[nWidth-1], matrix, thresholds.min(nWidth-1), thresholds.max(nWidth-1));

   pDst  += nDstPitch;
   pSrcp += nSrcPitch;
   pSrc  += nSrcPitch;
   pSrcn += nSrcPitch;

   thresholds.nextRow();

   for ( int y = 1; y < nHeight - 1; y++ )
   {
      /* left */
      pDst[0] = op(pSrcp[0], pSrcp[0], pSrcp[1], pSrc[0], pSrc[0], pSrc[1], pSrcn[0], pSrcn[0], pSrcn[1], matrix, thresholds.min(0), thresholds.max(0));

      /* center */
      for ( int x = 1; x < nWidth - 1; x++ )
         pDst[x] = op(pSrcp[x-1], pSrcp[x], pSrcp[x+1], pSrc[x-1], pSrc[x], pSrc[x+1], pSrcn[x-1], pSrcn[x], pSrcn[x+1], matrix, thresholds.min(x), thresholds.max(x));

      /* right */
      pDst[nWidth-1] = op(pSrcp[nWidth-2], pSrcp[nWidth-1], pSrcp[nWidth-1], pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], pSrcn[nWidth-2], pSrcn[nWidth-1], pSrcn[nWidth-1], matrix, thresholds.min(nWidth-1), thresholds.max(nWidth-1));

      pDst  += nDstPitch;
      pSrcp += nSrcPitch;
      pSrc  += nSrcPitch;
      pSrcn += nSrcPitch;

      thresholds.nextRow();
   }

   /* bottom-left */
   pDst[0] = op(pSrcp[0], pSrcp[0], pSrcp[1], pSrc[0], pSrc[0], pSrc[1], pSrc[0], pSrc[0], pSrc[1], matrix, thresholds.min(0), thresholds.max(0));

   /* bottom */
   for ( int x = 1; x < nWidth - 1; x++ )
      pDst[x] = op(pSrcp[x-1], pSrcp[x], pSrcp[x+1], pSrc[x-1], pSrc[x], pSrc[x+1], pSrc[x-1], pSrc[x], pSrc[x+1], matrix, thresholds.min(x), thresholds.max(x));

   /* bottom-right */
   pDst[nWidth-1] = op(pSrcp[nWidth-2], pSrcp[nWidth-1], pSrcp[nWidth-1], pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], pSrc[nWidth-2], pSrc[nWidth-1], pSrc[nWidth-1], matrix, thresholds.min(nWidth-1), thresholds.max(nWidth-1));
}

} } } }

#endif