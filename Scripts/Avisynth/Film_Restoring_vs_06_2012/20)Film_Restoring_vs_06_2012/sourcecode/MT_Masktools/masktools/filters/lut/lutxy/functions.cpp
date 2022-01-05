#include "filter.h"

using namespace Filtering;

void Filtering::MaskTools::Filters::Lut::Dual::lut_c(Byte *pDst, int nDstPitch, const Byte *pSrc, int nSrcPitch, int nWidth, int nHeight, const Byte lut[65536])
{
   for ( int y = 0; y < nHeight; y++ )
   {
      for ( int x = 0; x < nWidth; x++ )
         pDst[x] = lut[(pDst[x]<<8) + pSrc[x]];
      pDst += nDstPitch;
      pSrc += nSrcPitch;
   }
}
