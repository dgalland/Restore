#ifndef __Mt_Inflate_H__
#define __Mt_Inflate_H__

#include "../../../filters/morphologic/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inflate {

extern Processor *inflate_c;

class Filter : public Morphologic::Filter
{
public:
   Filter(const Parameters &parameters) : Morphologic::Filter(parameters)
   {
      /* add the processors */
      processors.push_back(Filtering::Processor<Processor>(inflate_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
   }

   static Signature Filter::filter_signature()
   {
      Signature signature = "mt_inflate";

      signature.add( Parameter(TYPE_CLIP, "") );
      signature.add( Parameter(255, "thY") );
      signature.add( Parameter(255, "thC") );

      return add_defaults( signature );
   }
};

} } } } } // namespace Inflate, Morphologic, Filter, MaskTools, Filtering

#endif