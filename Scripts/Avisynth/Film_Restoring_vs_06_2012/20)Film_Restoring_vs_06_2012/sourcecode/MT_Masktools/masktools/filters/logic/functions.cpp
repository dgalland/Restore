#include "filter.h"

using namespace Filtering;

typedef Byte (Operator)(Byte, Byte);

static inline Byte and(Byte a, Byte b) { return a & b; }
static inline Byte or(Byte a, Byte b) { return a | b; }
static inline Byte andn(Byte a, Byte b) { return a & ~b; }
static inline Byte xor(Byte a, Byte b) { return a ^ b; }
static inline Byte min(Byte a, Byte b) { return a < b ? a : b; }
static inline Byte max(Byte a, Byte b) { return a > b ? a : b; }

template <Operator op>
void logic_t(Byte *pDst, int nDstPitch, const Byte *pSrc, int nSrcPitch, int nWidth, int nHeight)
{
   for ( int y = 0; y < nHeight; y++ )
   {
      for ( int x = 0; x < nWidth; x++ )
         pDst[x] = op(pDst[x], pSrc[x]);
      pDst += nDstPitch;
      pSrc += nSrcPitch;
   }
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

Processor *and_c  = &logic_t<and>;
Processor *or_c   = &logic_t<or>;
Processor *andn_c = &logic_t<andn>;
Processor *xor_c  = &logic_t<xor>;
Processor *min_c  = &logic_t<min>;
Processor *max_c  = &logic_t<max>;

} } } }