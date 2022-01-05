#include "filter.h"
#include "../../../filters/mask/functions.h"

using namespace Filtering;

namespace Filtering { namespace MaskTools { namespace Filters { namespace Blur { namespace Mapped {

static inline Byte convolution_all_c(Byte a11, Byte a21, Byte a31, Byte a12, Byte a22, Byte a32, Byte a13, Byte a23, Byte a33, const Word matrix[10], int nLowThreshold, int nHighThreshold)
{
   int nSum = a22 * matrix[4];
   if ( abs(a11 - a22) < nLowThreshold ) nSum += a11 * matrix[0]; else nSum += a22 * matrix[0];
   if ( abs(a21 - a22) < nLowThreshold ) nSum += a21 * matrix[1]; else nSum += a22 * matrix[1];
   if ( abs(a31 - a22) < nLowThreshold ) nSum += a31 * matrix[2]; else nSum += a22 * matrix[2];
   if ( abs(a12 - a22) < nLowThreshold ) nSum += a12 * matrix[3]; else nSum += a22 * matrix[3];
   if ( abs(a32 - a22) < nLowThreshold ) nSum += a32 * matrix[5]; else nSum += a22 * matrix[5];
   if ( abs(a13 - a22) < nLowThreshold ) nSum += a13 * matrix[6]; else nSum += a22 * matrix[6];
   if ( abs(a23 - a22) < nLowThreshold ) nSum += a23 * matrix[7]; else nSum += a22 * matrix[7];
   if ( abs(a33 - a22) < nLowThreshold ) nSum += a33 * matrix[8]; else nSum += a22 * matrix[8];

   return clip<Byte, int>((nSum + matrix[9] / 2)/ matrix[9], 0, 255);
}

static inline Byte convolution_below_c(Byte a11, Byte a21, Byte a31, Byte a12, Byte a22, Byte a32, Byte a13, Byte a23, Byte a33, const Word matrix[10], int nLowThreshold, int nHighThreshold)
{
   int nSum = a22 * matrix[4];
   int nSumCoeff = matrix[4];
   if ( abs(a11 - a22) < nLowThreshold ) { nSum += a11 * matrix[0]; nSumCoeff += matrix[0]; }
   if ( abs(a21 - a22) < nLowThreshold ) { nSum += a21 * matrix[1]; nSumCoeff += matrix[1]; }
   if ( abs(a31 - a22) < nLowThreshold ) { nSum += a31 * matrix[2]; nSumCoeff += matrix[2]; }
   if ( abs(a12 - a22) < nLowThreshold ) { nSum += a12 * matrix[3]; nSumCoeff += matrix[3]; }
   if ( abs(a32 - a22) < nLowThreshold ) { nSum += a32 * matrix[5]; nSumCoeff += matrix[5]; }
   if ( abs(a13 - a22) < nLowThreshold ) { nSum += a13 * matrix[6]; nSumCoeff += matrix[6]; }
   if ( abs(a23 - a22) < nLowThreshold ) { nSum += a23 * matrix[7]; nSumCoeff += matrix[7]; }
   if ( abs(a33 - a22) < nLowThreshold ) { nSum += a33 * matrix[8]; nSumCoeff += matrix[8]; }

   if ( nSumCoeff ) return clip<Byte, int>((nSum + nSumCoeff / 2) / nSumCoeff, 0, 255);
   else return a22;
}

class Thresholds {
   const Byte *pThreshold;
   int nPitch;
public:
   Thresholds(const Byte *pThreshold, int nPitch) :
   pThreshold(pThreshold), nPitch(nPitch)
   {
   }

   int minpitch() const { return nPitch; }
   int maxpitch() const { return nPitch; }
   void nextRow() { pThreshold += nPitch; }
   Byte min(int x) const { return pThreshold[x]; }
   Byte max(int x) const { return pThreshold[x]; }
};

template<Filters::Mask::Operator op>
void mask_t(Byte *pDst, int nDstPitch, const Byte *pSrc, int nSrcPitch, const Byte *pMask, int nMaskPitch, const Word matrix[10], int nWidth, int nHeight)
{
   Filters::Mask::generic_c<op, Thresholds>(pDst, nDstPitch, pSrc, nSrcPitch, Thresholds(pMask, nMaskPitch), matrix, nWidth, nHeight);
}

Processor *mapped_below_c = &mask_t<convolution_below_c>;
Processor *mapped_all_c = &mask_t<convolution_all_c>;

} } } } }