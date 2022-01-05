#ifndef __Mt_Logic_H__
#define __Mt_Logic_H__

#include "../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Logic {

typedef void(Processor)(Byte *pDst, int nDstPitch, const Byte *pSrc1, int nSrc1Pitch, int nWidth, int nHeight);

extern Processor *and_c;
extern Processor *or_c;
extern Processor *andn_c;
extern Processor *xor_c;
extern Processor *min_c;
extern Processor *max_c;

extern "C" Processor Logic_and8_mmx;
extern "C" Processor Logic_andn8_mmx;
extern "C" Processor Logic_or8_mmx;
extern "C" Processor Logic_xor8_mmx;

extern "C" Processor Logic_and8_isse;
extern "C" Processor Logic_andn8_isse;
extern "C" Processor Logic_or8_isse;
extern "C" Processor Logic_xor8_isse;
extern "C" Processor Logic_min8_isse;
extern "C" Processor Logic_max8_isse;

extern "C" Processor Logic_and8_3dnow;
extern "C" Processor Logic_andn8_3dnow;
extern "C" Processor Logic_or8_3dnow;
extern "C" Processor Logic_xor8_3dnow;
extern "C" Processor Logic_min8_3dnow;
extern "C" Processor Logic_max8_3dnow;

extern "C" Processor Logic_and8_sse2;
extern "C" Processor Logic_andn8_sse2;
extern "C" Processor Logic_or8_sse2;
extern "C" Processor Logic_xor8_sse2;
extern "C" Processor Logic_min8_sse2;
extern "C" Processor Logic_max8_sse2;

extern "C" Processor Logic_and8_asse2;
extern "C" Processor Logic_andn8_asse2;
extern "C" Processor Logic_or8_asse2;
extern "C" Processor Logic_xor8_asse2;
extern "C" Processor Logic_min8_asse2;
extern "C" Processor Logic_max8_asse2;

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
      if ( parameters["mode"].toString() == "and" )
      {
         processors.push_back(Filtering::Processor<Processor>(and_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
         processors.push_back(Filtering::Processor<Processor>(Logic_and8_mmx, Constraint(CPU_MMX, 8, 1, 1, 1), 1));
         processors.push_back(Filtering::Processor<Processor>(Logic_and8_isse, Constraint(CPU_ISSE, 8, 1, 1, 1), 3));
         processors.push_back(Filtering::Processor<Processor>(Logic_and8_3dnow, Constraint(CPU_3DNOW, 8, 1, 1, 1), 4));
         processors.push_back(Filtering::Processor<Processor>(Logic_and8_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 5));
         processors.push_back(Filtering::Processor<Processor>(Logic_and8_asse2, Constraint(CPU_SSE2, 8, 1, 16, 16), 6));
      }
      else if ( parameters["mode"].toString() == "andn" )
      {
         processors.push_back(Filtering::Processor<Processor>(andn_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
         processors.push_back(Filtering::Processor<Processor>(Logic_andn8_mmx, Constraint(CPU_MMX, 8, 1, 1, 1), 1));
         processors.push_back(Filtering::Processor<Processor>(Logic_andn8_isse, Constraint(CPU_ISSE, 8, 1, 1, 1), 3));
         processors.push_back(Filtering::Processor<Processor>(Logic_andn8_3dnow, Constraint(CPU_3DNOW, 8, 1, 1, 1), 4));
         processors.push_back(Filtering::Processor<Processor>(Logic_andn8_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 5));
         processors.push_back(Filtering::Processor<Processor>(Logic_andn8_asse2, Constraint(CPU_SSE2, 8, 1, 16, 16), 6));
      }
      else if ( parameters["mode"].toString() == "or" )
      {
         processors.push_back(Filtering::Processor<Processor>(or_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
         processors.push_back(Filtering::Processor<Processor>(Logic_or8_mmx, Constraint(CPU_MMX, 8, 1, 1, 1), 1));
         processors.push_back(Filtering::Processor<Processor>(Logic_or8_isse, Constraint(CPU_ISSE, 8, 1, 1, 1), 3));
         processors.push_back(Filtering::Processor<Processor>(Logic_or8_3dnow, Constraint(CPU_3DNOW, 8, 1, 1, 1), 4));
         processors.push_back(Filtering::Processor<Processor>(Logic_or8_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 5));
         processors.push_back(Filtering::Processor<Processor>(Logic_or8_asse2, Constraint(CPU_SSE2, 8, 1, 16, 16), 6));
      }
      else if ( parameters["mode"].toString() == "xor" )
      {
         processors.push_back(Filtering::Processor<Processor>(xor_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
         processors.push_back(Filtering::Processor<Processor>(Logic_xor8_mmx, Constraint(CPU_MMX, 8, 1, 1, 1), 1));
         processors.push_back(Filtering::Processor<Processor>(Logic_xor8_isse, Constraint(CPU_ISSE, 8, 1, 1, 1), 3));
         processors.push_back(Filtering::Processor<Processor>(Logic_xor8_3dnow, Constraint(CPU_3DNOW, 8, 1, 1, 1), 4));
         processors.push_back(Filtering::Processor<Processor>(Logic_xor8_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 5));
         processors.push_back(Filtering::Processor<Processor>(Logic_xor8_asse2, Constraint(CPU_SSE2, 8, 1, 16, 16), 6));
      }
      else if ( parameters["mode"].toString() == "min" )
      {
         processors.push_back(Filtering::Processor<Processor>(min_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
         processors.push_back(Filtering::Processor<Processor>(Logic_min8_isse, Constraint(CPU_ISSE, 8, 1, 1, 1), 1));
         processors.push_back(Filtering::Processor<Processor>(Logic_min8_3dnow, Constraint(CPU_3DNOW, 8, 1, 1, 1), 3));
         processors.push_back(Filtering::Processor<Processor>(Logic_min8_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 5));
         processors.push_back(Filtering::Processor<Processor>(Logic_min8_asse2, Constraint(CPU_SSE2, 8, 1, 16, 16), 6));
      }
      else if ( parameters["mode"].toString() == "max" )
      {
         processors.push_back(Filtering::Processor<Processor>(max_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
         processors.push_back(Filtering::Processor<Processor>(Logic_max8_isse, Constraint(CPU_ISSE, 8, 1, 1, 1), 1));
         processors.push_back(Filtering::Processor<Processor>(Logic_max8_3dnow, Constraint(CPU_3DNOW, 8, 1, 1, 1), 3));
         processors.push_back(Filtering::Processor<Processor>(Logic_max8_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 5));
         processors.push_back(Filtering::Processor<Processor>(Logic_max8_asse2, Constraint(CPU_SSE2, 8, 1, 16, 16), 6));
      }
      else
      {
         error = "unknown mode";
         return;
      }
   }

   InputConfiguration &input_configuration() const { return InPlaceTwoFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_logic";

      signature.add(Parameter(TYPE_CLIP, ""));
      signature.add(Parameter(TYPE_CLIP, ""));
      signature.add(Parameter(String(""), "mode"));

      return add_defaults( signature );
   }

};


} } } }

#endif