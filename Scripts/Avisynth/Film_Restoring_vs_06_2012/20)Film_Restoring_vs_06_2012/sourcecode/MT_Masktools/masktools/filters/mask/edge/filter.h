#ifndef __Mt_Edgemask_H__
#define __Mt_Edgemask_H__

#include "../../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Edge {

typedef void(Processor)(Byte *pDst, int nDstPitch, const Byte *pSrc, int nSrcPitch, const Word matrix[10], int nLowThreshold, int nHighThreshold, int nWidth, int nHeight);

extern Processor *convolution_c;
extern Processor *convolution8_mmx;
extern Processor *convolution8_sse2;

extern Processor *sobel_c;
extern Processor *sobel8_mmx;
extern Processor *sobel8_sse2;

extern Processor *roberts_c;
extern Processor *roberts8_mmx;
extern Processor *roberts8_sse2;

extern Processor *laplace_c;
extern Processor *laplace8_mmx;
extern Processor *laplace8_sse2;

extern Processor *cartoon_c;
extern Processor *prewitt_c;

extern Processor *morpho_c;
extern Processor *morpho8_isse;
extern Processor *morpho8_sse2;

class Filter : public MaskTools::Filter<ChildFilter>
{
   int nLowThresholds[3];
   int nHighThresholds[3];
   Word matrix[10];

   ProcessorList<Processor> processors;

protected:

   virtual void process(int n, const Plane<Byte> &dst, int nPlane)
   {
      processors.best_processor( constraints[nPlane] )( dst, dst.pitch(), frames[0].plane(nPlane), frames[0].plane(nPlane).pitch(), matrix, nLowThresholds[nPlane], nHighThresholds[nPlane], dst.width(), dst.height() );
   }

public:
   Filter(const Parameters &parameters) : MaskTools::Filter<ChildFilter>( parameters )
   {
      nLowThresholds[0] = clip<int, int>( parameters["thY1"].toInt(), 0, 255 );
      nLowThresholds[1] = nLowThresholds[2] = clip<int, int>( parameters["thC1"].toInt(), 0, 255 );
      nHighThresholds[0] = clip<int, int>( parameters["thY2"].toInt(), 0, 255 );
      nHighThresholds[1] = nHighThresholds[2] = clip<int, int>( parameters["thC2"].toInt(), 0, 255 );

      /* add the processors */
      if ( parameters["mode"].toString() == "sobel" )
      {
         print(LOG_DEBUG, "Edge : using sobel detector");
         processors.push_back(Filtering::Processor<Processor>(sobel_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
         processors.push_back(Filtering::Processor<Processor>(sobel8_mmx, Constraint(CPU_MMX, 8, 1, 1, 1), 1));
         processors.push_back(Filtering::Processor<Processor>(sobel8_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 2));
      }
      else if ( parameters["mode"].toString() == "roberts" )
      {
         print(LOG_DEBUG, "Edge : using roberts detector");
         processors.push_back(Filtering::Processor<Processor>(roberts_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
         processors.push_back(Filtering::Processor<Processor>(roberts8_mmx, Constraint(CPU_MMX, 8, 1, 1, 1), 1));
         processors.push_back(Filtering::Processor<Processor>(roberts8_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 2));
      }
      else if ( parameters["mode"].toString() == "laplace" )
      {
         print(LOG_DEBUG, "Edge : using laplace detector");
         processors.push_back(Filtering::Processor<Processor>(laplace_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
         processors.push_back(Filtering::Processor<Processor>(laplace8_mmx, Constraint(CPU_MMX, 8, 1, 1, 1), 1));
         processors.push_back(Filtering::Processor<Processor>(laplace8_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 2));
      }
      else if ( parameters["mode"].toString() == "cartoon" )
      {
         print(LOG_DEBUG, "Edge : using cartoon detector");
         processors.push_back(Filtering::Processor<Processor>(cartoon_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      }
      else if ( parameters["mode"].toString() == "min/max" )
      {
         print(LOG_DEBUG, "Edge : using morphologic detector");
         processors.push_back(Filtering::Processor<Processor>(morpho_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
         processors.push_back(Filtering::Processor<Processor>(morpho8_isse, Constraint(CPU_ISSE, 8, 1, 1, 1), 1));
         processors.push_back(Filtering::Processor<Processor>(morpho8_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 2));
      }
      else if ( parameters["mode"].toString() == "prewitt" )
      {
         print(LOG_DEBUG, "Edge : using prewitt detector");
         processors.push_back(Filtering::Processor<Processor>(prewitt_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      }
      else
      {
         print(LOG_DEBUG, "Edge : using custom detector");
         std::list<Parser::Symbol> coeffs = Parser::getDefaultParser().parse(parameters["mode"].toString(), " ").getExpression();
         memset(matrix, 0, sizeof(matrix));
         for ( int i = 0; i < 9; i++ )
         {
            if ( !coeffs.size() )
            {
               error = "invalid mode";
               return;
            }

            print( LOG_DEBUG, "%i\n", coeffs.size());

            matrix[9] += abs(matrix[i] = Word(coeffs.front().getValue(0,0,0)));
            coeffs.pop_front();
         }

         int nSum = matrix[9];

         if ( coeffs.size() )
            matrix[9] = Word(coeffs.front().getValue(0,0,0));
         else
         {
            matrix[9] = 0;
            /* choose the upper power of 2, if possible */
            for ( int i = 0; i < 8; i++ )
            {
               if ( (1 << i) >= nSum )
               {
                  matrix[9] = 1 << i;
                  break;
               }
            }
            if ( !matrix[9] )
               matrix[9] = nSum;
         }

         if ( !matrix[9] )
            matrix[9] = 1;

         print(LOG_DEBUG, "Edge : %3i %3i %3i\n"
                        "       %3i %3i %3i\n"
                        "       %3i %3i %3i\n"
                        "normalize: %3i", matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9]);
         processors.push_back(Filtering::Processor<Processor>(convolution_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
         /* insert mmx version only if matrix[9] is a valid power of 2 */
         if ( matrix[9] == 1 || matrix[9] == 2 || matrix[9] == 4 || matrix[9] == 8 || matrix[9] == 16 || matrix[9] == 32 || matrix[9] == 64 || matrix[9] == 128 )
         {
            processors.push_back(Filtering::Processor<Processor>(convolution8_mmx, Constraint(CPU_MMX, 8, 1, 1, 1), 1));
            processors.push_back(Filtering::Processor<Processor>(convolution8_sse2, Constraint(CPU_SSE2, 8, 1, 1, 1), 2));
         }
      }
   }

   InputConfiguration &input_configuration() const { return OneFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_edge";

      signature.add(Parameter(TYPE_CLIP, ""));
      signature.add(Parameter(String("sobel"), "mode"));
      signature.add(Parameter(10, "thY1"));
      signature.add(Parameter(10, "thY2"));
      signature.add(Parameter(10, "thC1"));
      signature.add(Parameter(10, "thC2"));

      return add_defaults( signature );
   }

};

} } } } }

#endif