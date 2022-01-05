#ifndef __Mt_Lut_Functions_H__
#define __Mt_Lut_Functions_H__

namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut {

typedef enum {

   NONIZER = 0,
   AVERAGER = 1,
   MINIMIZER = 2,
   MAXIMIZER = 3,
   DEVIATER = 4,
   RANGIZER = 5,
   MEDIANIZER = 6,
   MEDIANIZER4 = 7,
   MEDIANIZER6 = 8,
   MEDIANIZER2 = 9,

   NUM_MODES,

};

#define EXPRESSION_SINGLE( base, mode1, mode2 ) &base< mode2 >
#define EXPRESSION_DUAL( base, mode1, mode2 ) &base< mode1, mode2 >

#define MPROCESSOR(expr, param, mode) \
{ \
   expr( param, mode, Nonizer ), \
   expr( param, mode, Averager<int> ), \
   expr( param, mode, Minimizer ), \
   expr( param, mode, Maximizer ), \
   expr( param, mode, Deviater<int> ), \
   expr( param, mode, Rangizer ), \
   expr( param, mode, Medianizer ), \
   expr( param, mode, MedianizerBetter<4> ), \
   expr( param, mode, MedianizerBetter<6> ), \
   expr( param, mode, MedianizerBetter<2> ), \
}

#define MPROCESSOR_SINGLE( base )     MPROCESSOR( EXPRESSION_SINGLE, base, )
#define MPROCESSOR_DUAL( base, mode ) MPROCESSOR( EXPRESSION_DUAL, base, mode )

static inline int ModeToInt(const String &mode)
{
   if ( mode == "average" || mode == "avg" )
      return AVERAGER;
   else if ( mode == "standard deviation" || mode == "std" )
      return DEVIATER;
   else if ( mode == "minimum" || mode == "min" )
      return MINIMIZER;
   else if ( mode == "maximum" || mode == "max" )
      return MAXIMIZER;
   else if ( mode == "range" )
      return RANGIZER;
   else if ( mode == "median" || mode == "med" )
      return MEDIANIZER4;
   else
      return NONIZER;
}

class Nonizer {

public:
   Nonizer(int nSize) {}
   void reset(int nValue = 255) {}
   void add(int nValue) {}
   Byte finalize() const { return 0; }
};

class Minimizer {

   int nMin;

public:

   Minimizer(int nSize) {}
   void reset(int nValue = 255) { nMin = nValue; }
   void add(int nValue) { nMin = min<int>( nMin, nValue ); }
   Byte finalize() const { return nMin; }

};

class Medianizer {

   int nSize;
   int elements[256];

public:

   Medianizer(int nSize) { }
   void reset() { memset( elements, 0, sizeof( elements ) ); nSize = 0; }
   void reset(int nValue) { reset(); add( nValue ); }
   void add(int nValue) { elements[nValue]++; nSize++; }
   Byte finalize() const
   {
      int nCount = 0;
      int nIdx = -1;
      const int nLowHalf = (nSize + 1) >> 1;
      while ( nCount < nLowHalf )
         nCount += elements[++nIdx];

      if ( nSize & 1 ) /* nSize odd -> median belong to the middle class */
         return nIdx;
      else
      {
         if ( nCount >= nLowHalf + 1 ) /* nSize even, but middle class owns both median elements */
            return nIdx;
         else /* nSize even, middle class owns only one element, it's the lowest, so we search for the next one */
         {
            int nSndIdx = nIdx;
            while ( !elements[++nSndIdx] ) {}
            return (nIdx + nSndIdx + 1) >> 1; /* and we return the average */
         }
      }
   }
};

template<int n>
class MedianizerBetter
{
   int nSize;
   int elements[256];
   int elements2[256 >> n];

public:
   MedianizerBetter(int nSize) {}
   void reset() { nSize = 0; memset( elements, 0, sizeof( elements ) ); memset( elements2, 0, sizeof( elements2 ) ); }
   void add(int nValue) { elements[nValue]++; elements2[nValue>>n]++; nSize++; }
   void reset(int nValue) { reset(); add(nValue); }
   Byte finalize() const
   {
      int nCount = 0;
      int nIdx = -1;
      const int nLowHalf = (nSize + 1) >> 1;

      while ( nCount < nLowHalf )
         nCount += elements2[++nIdx]; /* low resolution search */

      nCount -= elements2[nIdx];
      nIdx <<= n;
      nIdx--;

      while ( nCount < nLowHalf )
         nCount += elements[++nIdx]; /* high resolution search */

      if ( nSize & 1 ) /* nSize odd -> median belong to the middle class */
         return nIdx;
      else
      {
         if ( nCount >= nLowHalf + 1 ) /* nSize even, but middle class owns both median elements */
            return nIdx;
         else /* nSize even, middle class owns only one element, it's the lowest, so we search for the next one */
         {
            int nSndIdx = nIdx;
            while ( !elements[++nSndIdx] ) {}
            return (nIdx + nSndIdx + 1) >> 1; /* and we return the average */
         }
      }
   }
};

class Maximizer {

   int nMax;

public:

   Maximizer(int nSize) { }
   void reset(int nValue = 0) { nMax = nValue; }
   void add(int nValue) { nMax = max<int>( nMax, nValue ); }
   Byte finalize() const { return nMax; }

};

class Rangizer {

   int nMin, nMax;

public:

   Rangizer(int nSize) {}
   void reset(int nValue) { nMin = nMax = nValue; }
   void reset() { nMin = 255; nMax = 0; }
   void add(int nValue) { nMin = min<int>( nMin, nValue ); nMax = max<int>( nMax, nValue ); }
   Byte finalize() const { return nMax - nMin; }

};

template<typename T>
class Averager
{

   T nSum, nCount;

public:

   Averager(int nSize) { }
   void reset() { nSum = 0; nCount = 0; }
   void reset(int nValue) { nSum = nValue; nCount = 1; }
   void add(int nValue) { nSum += nValue; nCount++; }
   Byte finalize() const { return rounded_division<T>( nSum, nCount ); }

};

template<typename T>
class Deviater
{
   T nSum, nSum2, nCount;

public:

   Deviater(int nSize) { }
   void reset() { nSum = 0; nSum2 = 0; nCount = 0; }
   void reset(int nValue) { nSum = nValue; nSum2 = nValue * nValue; nCount = 1; }
   void add(int nValue) { nSum += nValue; nSum2 += nValue * nValue; nCount++; }
   Byte finalize() const { return convert<Byte, Double>( sqrt( ( Double( nSum2 ) * nCount - Double( nSum ) * Double( nSum ) ) / ( Double( nCount ) * nCount ) ) ); }

};

} } } }

#endif
