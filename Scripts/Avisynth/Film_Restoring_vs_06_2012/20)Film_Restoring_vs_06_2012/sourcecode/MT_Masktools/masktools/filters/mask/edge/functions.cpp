#include "filter.h"
#include "../../../filters/mask/functions.h"

using namespace Filtering;

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

static inline Byte convolution(Byte a11, Byte a21, Byte a31, Byte a12, Byte a22, Byte a32, Byte a13, Byte a23, Byte a33, const Word matrix[10], int nLowThreshold, int nHighThreshold)
{
   return threshold<Byte, int>(abs((a11 * matrix[0] + a21 * matrix[1] + a31 * matrix[2] + 
                                    a12 * matrix[3] + a22 * matrix[4] + a32 * matrix[5] +
                                    a13 * matrix[6] + a23 * matrix[7] + a33 * matrix[8]) / matrix[9]), nLowThreshold, nHighThreshold);
}

static inline Byte sobel(Byte a11, Byte a21, Byte a31, Byte a12, Byte a22, Byte a32, Byte a13, Byte a23, Byte a33, const Word matrix[10], int nLowThreshold, int nHighThreshold)
{
   return threshold<Byte, int>(abs( (int)a32 + a23 - a12 - a21 ) >> 1, nLowThreshold, nHighThreshold);
}

static inline Byte roberts(Byte a11, Byte a21, Byte a31, Byte a12, Byte a22, Byte a32, Byte a13, Byte a23, Byte a33, const Word matrix[10], int nLowThreshold, int nHighThreshold)
{
   return threshold<Byte, int>(abs( ((int)a22 << 1) - a32 - a23 ) >> 1, nLowThreshold, nHighThreshold);
}

static inline Byte laplace(Byte a11, Byte a21, Byte a31, Byte a12, Byte a22, Byte a32, Byte a13, Byte a23, Byte a33, const Word matrix[10], int nLowThreshold, int nHighThreshold)
{
   return threshold<Byte, int>(abs( ((int)a22 << 3) - a32 - a23 - a11 - a21 - a31 - a12 - a13 - a33 ) >> 3, nLowThreshold, nHighThreshold);
}

static inline Byte morpho(Byte a11, Byte a21, Byte a31, Byte a12, Byte a22, Byte a32, Byte a13, Byte a23, Byte a33, const Word matrix[10], int nLowThreshold, int nHighThreshold)
{
   int nMin = a11, nMax = a11;
   nMin = min<int>( nMin, a21 );
   nMax = max<int>( nMax, a21 );
   nMin = min<int>( nMin, a31 );
   nMax = max<int>( nMax, a31 );
   nMin = min<int>( nMin, a12 );
   nMax = max<int>( nMax, a12 );
   nMin = min<int>( nMin, a22 );
   nMax = max<int>( nMax, a22 );
   nMin = min<int>( nMin, a32 );
   nMax = max<int>( nMax, a32 );
   nMin = min<int>( nMin, a13 );
   nMax = max<int>( nMax, a13 );
   nMin = min<int>( nMin, a23 );
   nMax = max<int>( nMax, a23 );
   nMin = min<int>( nMin, a33 );
   nMax = max<int>( nMax, a33 );

   return threshold<Byte, int>( nMax - nMin, nLowThreshold, nHighThreshold );
}

static inline Byte cartoon(Byte a11, Byte a21, Byte a31, Byte a12, Byte a22, Byte a32, Byte a13, Byte a23, Byte a33, const Word matrix[10], int nLowThreshold, int nHighThreshold)
{
   int val = ((int)a21 << 1) - a22 - a31;
   return val > 0 ? 0 : threshold<Byte, int>( -val, nLowThreshold, nHighThreshold );
}

static inline Byte prewitt(Byte a11, Byte a21, Byte a31, Byte a12, Byte a22, Byte a32, Byte a13, Byte a23, Byte a33, const Word matrix[10], int nLowThreshold, int nHighThreshold)
{
   const int p90 = a11 + a21 + a31 - a13 - a23 - a33;
   const int p180 = a11 + a12 + a13 - a31 - a32 - a33;
   const int p45 = a12 + a11 + a21 - a33 - a32 - a23;
   const int p135 = a13 + a12 + a23 - a31 - a32 - a21;

   const int max1 = max<int>( abs<int>( p90 ), abs<int>( p180 ) );
   const int max2 = max<int>( abs<int>( p45 ), abs<int>( p135 ) );
   const int maxv = max<int>( max1, max2 );

   return threshold<Byte, int>( maxv, nLowThreshold, nHighThreshold );
}

class Thresholds {
   Byte nMinThreshold, nMaxThreshold;
public:
   Thresholds(Byte nMinThreshold, Byte nMaxThreshold) :
   nMinThreshold(nMinThreshold), nMaxThreshold(nMaxThreshold)
   {
   }

   int minpitch() const { return 0; }
   int maxpitch() const { return 0; }
   void nextRow() { }
   Byte min(int x) const { return nMinThreshold; }
   Byte max(int x) const { return nMaxThreshold; }
};

template<Filters::Mask::Operator op>
void mask_t(Byte *pDst, int nDstPitch, const Byte *pSrc, int nSrcPitch, const Word matrix[10], int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
{
   Filters::Mask::generic_c<op, Thresholds>(pDst, nDstPitch, pSrc, nSrcPitch, Thresholds(nLowThreshold, nHighThreshold), matrix, nWidth, nHeight);
}

extern "C" Processor Edge_sobel8_mmx;
extern "C" Processor Edge_sobel8_sse2;
extern "C" Processor Edge_roberts8_mmx;
extern "C" Processor Edge_roberts8_sse2;
extern "C" Processor Edge_laplace8_mmx;
extern "C" Processor Edge_laplace8_sse2;
extern "C" Processor Edge_convolution8_mmx;
extern "C" Processor Edge_convolution8_sse2;
extern "C" Processor Edge_morpho8_isse;
extern "C" Processor Edge_morpho8_sse2;

Processor *convolution_c = &mask_t<convolution>;
Processor *convolution8_mmx = &Edge_convolution8_mmx;
Processor *convolution8_sse2 = &Edge_convolution8_sse2;

Processor *sobel_c = &mask_t<sobel>;
Processor *sobel8_mmx = &Edge_sobel8_mmx;
Processor *sobel8_sse2 = &Edge_sobel8_sse2;

Processor *roberts_c = &mask_t<roberts>;
Processor *roberts8_mmx = &Edge_roberts8_mmx;
Processor *roberts8_sse2 = &Edge_roberts8_sse2;

Processor *laplace_c = &mask_t<laplace>;
Processor *laplace8_mmx = &Edge_laplace8_mmx;
Processor *laplace8_sse2 = &Edge_laplace8_sse2;

Processor *cartoon_c = &mask_t<cartoon>;
Processor *prewitt_c = &mask_t<prewitt>;

Processor *morpho_c = &mask_t<morpho>;
Processor *morpho8_isse = &Edge_morpho8_isse;
Processor *morpho8_sse2 = &Edge_morpho8_sse2;

} } } } }