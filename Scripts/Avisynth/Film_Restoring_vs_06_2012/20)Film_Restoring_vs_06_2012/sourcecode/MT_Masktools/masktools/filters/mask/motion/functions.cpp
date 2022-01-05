#include "filter.h"
#include "../../../../common/functions/functions.h"

using namespace Filtering;

typedef unsigned int (Sad)(Byte *pDst, int nDstPitch, const Byte *pSrc, int nSrcPitch, int nWidth, int nHeight);
typedef void (Mask)(Byte *pDst, int nDstPitch, const Byte *pSrc, int nSrcPitch, int nLowThreshold, int nHighThreshold, int nWidth, int nHeight);

static unsigned int sad_c(Byte *pDst, int nDstPitch, const Byte *pSrc, int nSrcPitch, int nWidth, int nHeight)
{
   unsigned int nSad = 0;
	for ( int y = 0; y < nHeight; y++ ) 
	{
		for (int x = 0; x < nWidth; x++ )
         nSad += abs(pDst[x] - pSrc[x]);
      pDst += nDstPitch;
      pSrc += nSrcPitch;
	}
   return nSad;
}

static void mask_c(Byte *pDst, int nDstPitch, const Byte *pSrc, int nSrcPitch, int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
{
	for ( int y = 0; y < nHeight; y++ ) 
	{
		for (int x = 0; x < nWidth; x++ )
         pDst[x] = threshold<Byte, int>( abs<int>( pDst[x] - pSrc[x] ), nLowThreshold, nHighThreshold );
      pDst += nDstPitch;
      pSrc += nSrcPitch;
   }
}

template <Sad sad, Mask mask, Functions::Memset memsetPlane>
bool mask_t(Byte *pDst, int nDstPitch, const Byte *pSrc, int nSrcPitch, int nLowThreshold, int nHighThreshold, int nMotionThreshold, int nSceneChange, int nSceneChangeValue, int nWidth, int nHeight)
{
   bool scenechange = nSceneChange >= 2 ? nSceneChange == 3 : sad(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight) > (unsigned int)(nMotionThreshold * nWidth * nHeight);

   if ( scenechange )
      memsetPlane(pDst, nDstPitch, nWidth, nHeight, nSceneChangeValue);
   else
      mask(pDst, nDstPitch, pSrc, nSrcPitch, nLowThreshold, nHighThreshold, nWidth, nHeight);

   return scenechange;
}

extern "C" Sad Motion_sad8_isse;
extern "C" Sad Motion_sad8_3dnow;

extern "C" Mask Motion_motion8_mmx;
extern "C" Mask Motion_motion8_isse;
extern "C" Mask Motion_motion8_3dnow;

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

Processor *mask_c       = &mask_t<::sad_c          , ::mask_c            , Functions::memset_c>;
Processor *mask8_mmx    = &mask_t<::sad_c          , Motion_motion8_mmx  , memset8_mmx>;
Processor *mask8_isse   = &mask_t<Motion_sad8_isse , Motion_motion8_isse , memset8_isse>;
Processor *mask8_3dnow  = &mask_t<Motion_sad8_3dnow, Motion_motion8_3dnow, memset8_3dnow>;

} } } } }