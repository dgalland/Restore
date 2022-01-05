#include "filter.h"

using namespace Filtering;

void Filtering::MaskTools::Filters::Binarize::lower_c(Byte *pDst, int nDstPitch, int nThreshold, int nWidth, int nHeight)
{
   for ( int j = 0; j < nHeight; j++ )
   {
      for ( int i = 0; i < nWidth; i++ )
         pDst[i] = pDst[i] > nThreshold ? 255 : 0;
      pDst += nDstPitch;
   }
}

void Filtering::MaskTools::Filters::Binarize::upper_c(Byte *pDst, int nDstPitch, int nThreshold, int nWidth, int nHeight)
{
   for ( int j = 0; j < nHeight; j++ )
   {
      for ( int i = 0; i < nWidth; i++ )
         pDst[i] = pDst[i] > nThreshold ? 0 : 255;
      pDst += nDstPitch;
   }
}