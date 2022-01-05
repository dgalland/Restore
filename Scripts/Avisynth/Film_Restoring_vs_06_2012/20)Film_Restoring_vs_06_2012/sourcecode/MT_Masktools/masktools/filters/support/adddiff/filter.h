#ifndef __Mt_AddDiff_H__
#define __Mt_AddDiff_H__

#include "../../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support { namespace AddDiff {

typedef void(Processor)(Byte *pDst, int nDstPitch, const Byte *pSrc, int nSrcPitch, int nWidth, int nHeight);

Processor adddiff_c;
extern "C" Processor AddDiff_adddiff8_mmx;
extern "C" Processor AddDiff_adddiff8_isse;
extern "C" Processor AddDiff_adddiff8_3dnow;
extern "C" Processor AddDiff_adddiff8_sse2;
extern "C" Processor AddDiff_adddiff8_asse2;

class Filter : public MaskTools::Filter<InPlaceFilter>
{

   ProcessorList<Processor> processors;

protected:

   virtual void process(int n, const Plane<Byte> &dst, int nPlane)
   {
      processors.best_processor( constraints[nPlane] )( dst, dst.pitch(), frames[0].plane(nPlane), frames[0].plane(nPlane).pitch(), dst.width(), dst.height() );
   }

public:
   Filter(const Parameters &parameters) : MaskTools::Filter<InPlaceFilter>( parameters )
   {
      /* add the processors */
      processors.push_back(Filtering::Processor<Processor>(&adddiff_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      processors.push_back(Filtering::Processor<Processor>(&AddDiff_adddiff8_mmx, Constraint(CPU_MMX, 8, 1, 1, 1), 1));
      processors.push_back(Filtering::Processor<Processor>(&AddDiff_adddiff8_isse, Constraint(CPU_ISSE, 8, 1, 1, 1), 2));
      processors.push_back(Filtering::Processor<Processor>(&AddDiff_adddiff8_3dnow, Constraint(CPU_3DNOW, 8, 1, 1, 1), 3));
      processors.push_back(Filtering::Processor<Processor>(&AddDiff_adddiff8_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 4));
      processors.push_back(Filtering::Processor<Processor>(&AddDiff_adddiff8_asse2, Constraint(CPU_SSE2, 8, 1, 16, 16), 5));
   }

   InputConfiguration &input_configuration() const { return InPlaceTwoFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_adddiff";

      signature.add(Parameter(TYPE_CLIP, ""));
      signature.add(Parameter(TYPE_CLIP, ""));

      return add_defaults( signature );
   }

};

} } } } }

#endif