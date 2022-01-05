#ifndef __Mt_Lutf_H__
#define __Mt_Lutf_H__

#include "../../../common/base/filter.h"
#include "../../../../common/parser/parser.h"

#include "../functions.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut { namespace Frame {

typedef void(Processor)(Byte *pDst, int nDstPitch, const Byte *pSrc, int nSrcPitch, const Byte lut[65536], int nWidth, int nHeight);

extern Processor *processors_array[NUM_MODES];

class Filter : public MaskTools::Filter<InPlaceFilter>
{
   Byte luts[3][65536];

   ProcessorList<Processor> processors;

protected:
   virtual void process(int n, const Plane<Byte> &dst, int nPlane)
   {
      processors.best_processor( constraints[nPlane] )( dst, dst.pitch(), frames[0].plane(nPlane), frames[0].plane(nPlane).pitch(), luts[nPlane], dst.width(), dst.height() );
   }

public:
   Filter(const Parameters &parameters) : MaskTools::Filter<InPlaceFilter>( parameters )
   {
      static const char *expr_strs[] = { "yExpr", "uExpr", "vExpr" };

      Parser::Parser parser = Parser::getDefaultParser().addSymbol(Parser::Symbol::X).addSymbol(Parser::Symbol::Y);

      /* compute the luts */
      for ( int i = 0; i < 3; i++ )
      {
         if ( parameters[expr_strs[i]].is_defined() ) 
            parser.parse(parameters[expr_strs[i]].toString(), " ");
         else
            parser.parse(parameters["expr"].toString(), " ");

         Parser::Context ctx(parser.getExpression());

         if ( !ctx.check() )
         {
            error = "invalid expression in the lut";
            return;
         }

         for ( int x = 0; x < 256; x++ )
            for ( int y = 0; y < 256; y++ )
               luts[i][(x<<8)+y] = ctx.compute_byte(x, y);
      }

      processors.push_back( processors_array[ ModeToInt( parameters["mode"].toString() ) ] );
   }

   InputConfiguration &input_configuration() const { return InPlaceTwoFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_lutf";

      signature.add( Parameter( TYPE_CLIP, "" ) );
      signature.add( Parameter( TYPE_CLIP, "" ) );
      signature.add( Parameter( String( "average" ), "mode" ) );
      signature.add( Parameter( String( "y" ), "expr" ) );
      signature.add( Parameter( String( "y" ), "yExpr" ) );
      signature.add( Parameter( String( "y" ), "uExpr" ) );
      signature.add( Parameter( String( "y" ), "vExpr" ) );

      return add_defaults( signature );
   }
};

} } } } }

#endif