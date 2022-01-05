#include "filter.h"
#include "../functions.h"

using namespace Filtering;

template<class T, class U>
static void custom_c(Byte *pDst, int nDstPitch, const Byte *pSrc1, int nSrc1Pitch, const Byte *pSrc2, int nSrc2Pitch, const Byte *pLut, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight)
{
   T new_value( nCoordinates );
   U new_value2( nCoordinates );
   for ( int j = 0; j < nHeight; j++ )
   {
      for ( int i = 0; i < nWidth; i++ )
      {
         new_value.reset();
         new_value2.reset();
         for ( int k = 0; k < nCoordinates; k+=2 )
         {
            int x = pCoordinates[k] + i;
            int y = pCoordinates[k+1] + j;

            if ( x < 0 ) x = 0;
            if ( x >= nWidth ) x = nWidth - 1;
            if ( y < 0 ) y = 0;
            if ( y >= nHeight ) y = nHeight - 1;

            new_value.add( pSrc1[x + ( y - j ) * nSrc1Pitch] );
            new_value2.add( pSrc2[x + ( y - j ) * nSrc2Pitch] );
         }
         pDst[i] = pLut[ (new_value2.finalize() << 16) + (pDst[ i ] << 8) + new_value.finalize() ];
      }
      pSrc1 += nSrc1Pitch;
      pSrc2 += nSrc2Pitch;
      pDst += nDstPitch;
   }
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut { namespace SpatialExtended {

Processor *processors_array[NUM_MODES][NUM_MODES] = 
{
   MPROCESSOR_DUAL( custom_c, Nonizer ),
   MPROCESSOR_DUAL( custom_c, Averager<int>  ),
   MPROCESSOR_DUAL( custom_c, Minimizer ),
   MPROCESSOR_DUAL( custom_c, Maximizer ),
   MPROCESSOR_DUAL( custom_c, Deviater<int> ),
   MPROCESSOR_DUAL( custom_c, Rangizer ),
   MPROCESSOR_DUAL( custom_c, Medianizer ),
   MPROCESSOR_DUAL( custom_c, MedianizerBetter<4> ),
   MPROCESSOR_DUAL( custom_c, MedianizerBetter<6> ),
   MPROCESSOR_DUAL( custom_c, MedianizerBetter<2> ),
};

} } } } }

