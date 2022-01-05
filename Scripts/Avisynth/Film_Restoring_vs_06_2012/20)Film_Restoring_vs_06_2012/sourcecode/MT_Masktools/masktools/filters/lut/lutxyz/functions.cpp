#include "filter.h"

using namespace Filtering;

void Filtering::MaskTools::Filters::Lut::Trial::lut_c(Byte *pDst, int nDstPitch, const Byte *pSrc1, int nSrc1Pitch, const Byte *pSrc2, int nSrc2Pitch, int nWidth, int nHeight, const Byte *lut)
{
   for ( int y = 0; y < nHeight; y++ )
   {
      for ( int x = 0; x < nWidth; x++ )
         pDst[x] = lut[(pDst[x]<<16) + (pSrc1[x]<<8) + (pSrc2[x])];
      pDst += nDstPitch;
      pSrc1 += nSrc1Pitch;
      pSrc2 += nSrc2Pitch;
   }
}
