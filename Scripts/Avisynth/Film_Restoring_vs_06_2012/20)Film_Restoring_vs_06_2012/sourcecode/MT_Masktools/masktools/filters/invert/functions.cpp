#include "filter.h"

using namespace Filtering;

void Filtering::MaskTools::Filters::Invert::invert_c(Byte *pDst, int nDstPitch, int nWidth, int nHeight)
{
   for ( int j = 0; j < nHeight; j++ )
   {
      for ( int i = 0; i < nWidth; i++ )
         pDst[i] = 255 - pDst[i];
      pDst += nDstPitch;
   }
}