#ifndef __Mt_Binarize_H__
#define __Mt_Binarize_H__

#include "../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Binarize {

typedef void(Processor)(Byte *pDst, int nDstPitch, int nThreshold, int nWidth, int nHeight);

Processor upper_c;
Processor lower_c;

extern "C" Processor Binarize_upper8_mmx;
extern "C" Processor Binarize_lower8_mmx;
extern "C" Processor Binarize_upper8_isse;
extern "C" Processor Binarize_lower8_isse;
extern "C" Processor Binarize_upper8_3dnow;
extern "C" Processor Binarize_lower8_3dnow;
extern "C" Processor Binarize_upper8_sse2;
extern "C" Processor Binarize_lower8_sse2;
extern "C" Processor Binarize_upper8_asse2;
extern "C" Processor Binarize_lower8_asse2;

class Filter : public MaskTools::Filter<InPlaceFilter>
{
   int nThreshold;
   ProcessorList<Processor> processors;

protected:
   virtual void process(int n, const Plane<Byte> &dst, int nPlane)
   {
      processors.best_processor( constraints[nPlane] )( dst, dst.pitch(), nThreshold, dst.width(), dst.height() );
   }

public:
   Filter(const Parameters &parameters) : MaskTools::Filter<InPlaceFilter>(parameters)
   {
      nThreshold = clip<int, int>( parameters["threshold"].toInt(), 0, 255 );

      /* add the processors */
      if ( parameters["upper"].toBool() )
      {
         processors.push_back( Filtering::Processor<Processor>( upper_c, Constraint( CPU_NONE, 1, 1, 1, 1 ), 0 ) );
         processors.push_back( Filtering::Processor<Processor>( Binarize_upper8_mmx, Constraint( CPU_MMX , 8, 1, 1, 1 ), 1 ) );
         processors.push_back( Filtering::Processor<Processor>( Binarize_upper8_isse, Constraint( CPU_ISSE , 8, 1, 1, 1 ), 2 ) );
         processors.push_back( Filtering::Processor<Processor>( Binarize_upper8_3dnow, Constraint( CPU_3DNOW , 8, 1, 1, 1 ), 3 ) );
         processors.push_back( Filtering::Processor<Processor>( Binarize_upper8_sse2, Constraint( CPU_SSE2 , 8, 1, 1, 1 ), 4 ) );
         processors.push_back( Filtering::Processor<Processor>( Binarize_upper8_asse2, Constraint( CPU_SSE2 , 8, 1, 16, 16 ), 5 ) );
      }
      else
      {
         processors.push_back( Filtering::Processor<Processor>( lower_c, Constraint( CPU_NONE, 1, 1, 1, 1 ), 0 ) );
         processors.push_back( Filtering::Processor<Processor>( Binarize_lower8_mmx, Constraint( CPU_MMX , 8, 1, 1, 1 ), 1 ) );
         processors.push_back( Filtering::Processor<Processor>( Binarize_lower8_isse, Constraint( CPU_ISSE , 8, 1, 1, 1 ), 2 ) );
         processors.push_back( Filtering::Processor<Processor>( Binarize_lower8_3dnow, Constraint( CPU_3DNOW , 8, 1, 1, 1 ), 3 ) );
         processors.push_back( Filtering::Processor<Processor>( Binarize_lower8_sse2, Constraint( CPU_SSE2 , 8, 1, 1, 1 ), 4 ) );
         processors.push_back( Filtering::Processor<Processor>( Binarize_lower8_asse2, Constraint( CPU_SSE2 , 8, 1, 16, 16 ), 5 ) );
      }
   }

   InputConfiguration &input_configuration() const { return InPlaceOneFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_binarize";

      signature.add( Parameter( TYPE_CLIP, "" ) );
      signature.add( Parameter( 128, "threshold" ) );
      signature.add( Parameter( false, "upper" ) );

      return add_defaults( signature );
   }
};

} } } } // namespace Binarize, Filter, MaskTools, Filtering

#endif