#include "filter.h"

using namespace Filtering;

void Filtering::MaskTools::Filters::Lut::Single::lut_c(Byte *pDst, int nDstPitch, int nWidth, int nHeight, const Byte lut[256])
{
   for ( int y = 0; y < nHeight; y++ )
   {
      for ( int x = 0; x < nWidth; x++ )
         pDst[x] = lut[pDst[x]];
      pDst += nDstPitch;
   }
}