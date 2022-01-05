// Define the BlockData class

// I borrowed a lot of code from XviD's sources here, so I thank all the developpers
// of this wonderful codec

// See legal notice in Copying.txt for more information

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .

#ifndef __MV_INTERFACES_H__
#define __MV_INTERFACES_H__

//#define _STLP_USE_STATIC_LIB

#pragma pack(16)
#pragma warning(disable:4103) // disable pack to change alignment warning ( stlport related )
#pragma warning(disable:4800) // disable warning about bool to int unefficient conversion
#pragma warning(disable:4996) // disable warning about insecure deprecated string functions


#define ALIGN_PLANES 64 // all luma/chroma planes of all frames will have the effective frame area
						// aligned to this (source plane can be accessed with aligned loads, 64 required for effective use of x264 sad on Core2) 1.9.5
#define ALIGN_SOURCEBLOCK 16	// ALIGN_PLANES aligns the sourceblock UNLESS overlap != 0 OR special case: MMX function AND Block=16, Overlap = 8
								// ALIGN_SOURCEBLOCK creates aligned copy of Source block
//this options make things usually slower
#define ALLOW_DCT				// complex check in lumaSAD & DCT code in SearchMV / PseudoEPZ
//#define	ONLY_CHECK_NONDEFAULT_MV // make the check if it is no default reference (zero, global,...)


#define DEBUG_CLIENTBLOCK

#include "crtdbg.h"

#include "windows.h"
//#include <vector>
//#include <string>
#include "avisynth.h"

//#define MOTION_DEBUG          // allows to output debug information to the debug output
//#define MOTION_PROFILE        // allows to make a profiling of the motion estimation

#define MOTION_MAGIC_KEY 0x564D //'MV' is IMHO better 31415926 :)

struct VECTOR {
	int x;
	int y;
   int sad;
};

#define N_PER_BLOCK 3

typedef unsigned char Uint8;

enum MVPlaneSet {
   YPLANE = 1,
   UPLANE = 2,
   VPLANE = 4,
   YUPLANES = 3,
   YVPLANES = 5,
   UVPLANES = 6,
   YUVPLANES = 7
};

/*! \brief Search type : defines the algorithm used for minimizing the SAD */
enum SearchType {
	ONETIME = 1,
	NSTEP = 2,
	LOGARITHMIC = 4,
	EXHAUSTIVE = 8,
	HEX2SEARCH = 16, // v.2
	UMHSEARCH = 32   // v.2
};

/*! \brief Macros for accessing easily frame pointers & pitch */
#define YRPLAN(a) (a)->GetReadPtr(PLANAR_Y)
#define YWPLAN(a) (a)->GetWritePtr(PLANAR_Y)
#define URPLAN(a) (a)->GetReadPtr(PLANAR_U)
#define UWPLAN(a) (a)->GetWritePtr(PLANAR_U)
#define VRPLAN(a) (a)->GetReadPtr(PLANAR_V)
#define VWPLAN(a) (a)->GetWritePtr(PLANAR_V)
#define YPITCH(a) (a)->GetPitch(PLANAR_Y)
#define UPITCH(a) (a)->GetPitch(PLANAR_U)
#define VPITCH(a) (a)->GetPitch(PLANAR_V)

/*! \brief usefull macros */
#define MAX(a,b) (((a) < (b)) ? (b) : (a))
#define MIN(a,b) (((a) > (b)) ? (b) : (a))

//#define MOTION_CALC_SRC_LUMA        0x00000001
//#define MOTION_CALC_REF_LUMA        0x00000002
//#define MOTION_CALC_VAR             0x00000004
#define MOTION_CALC_BLOCK           0x00000008
#define MOTION_USE_MMX              0x00000010
#define MOTION_USE_ISSE             0x00000020
#define MOTION_IS_BACKWARD          0x00000040
#define MOTION_SMALLEST_PLANE       0x00000080
//#define MOTION_COMPENSATE_LUMA      0x00000100
//#define MOTION_COMPENSATE_CHROMA_U  0x00000200
//#define MOTION_COMPENSATE_CHROMA_V  0x00000400
#define MOTION_USE_CHROMA_MOTION    0x00000800

//cpu capability flags from x264 cpu.h //added in 1.9.5.4
#define CPU_CACHELINE_32   0x00001000  /* avoid memory loads that span the border between two cachelines */
#define CPU_CACHELINE_64   0x00002000  /* 32/64 is the size of a cacheline in bytes */
#define CPU_MMX            0x00004000
#define CPU_MMXEXT         0x00008000  /* MMX2 aka MMXEXT aka ISSE */
#define CPU_SSE            0x00010000
#define CPU_SSE2           0x00020000
#define CPU_SSE2_IS_SLOW   0x00040000  /* avoid most SSE2 functions on Athlon64 */
#define CPU_SSE2_IS_FAST   0x00080000  /* a few functions are only faster on Core2 and Phenom */
#define CPU_SSE3           0x00100000
#define CPU_SSSE3          0x00200000
#define CPU_PHADD_IS_FAST  0x00400000  /* pre-Penryn Core2 have a uselessly slow PHADD instruction */
#define CPU_SSE4           0x00800000  /* SSE4.1 */
//force MVAnalyse to use a different function for SAD / SADCHROMA (debug)
#define MOTION_USE_SSD     0x01000000
#define MOTION_USE_SATD    0x02000000

#define MV_DEFAULT_SCD1             400 // increased in v1.4.1
#define MV_DEFAULT_SCD2             130

//#define MV_BUFFER_FRAMES 10

static const VECTOR zeroMV = { 0, 0, -1 };

struct SuperParams64struct // MVSuper parameters packed to 64 bit num_audio_samples
{
  unsigned short nHeight;
  unsigned char nHPad;
  unsigned char nVPad;
  unsigned char nPel;
  unsigned char nModeYUV;
  unsigned char nLevels;
  unsigned char param;
};

typedef SuperParams64struct SuperParams64Bits;

#ifdef FORCE_INLINE // set compiler variable for Release with fastest run speed, but slow compile!
#define inline __forceinline
#else
#define inline inline
#endif

#include <stdio.h>	//required for Debug output and outfile, fixed in 1.9.5
/*! \brief debug printing information */
#ifdef MOTION_DEBUG

#include <stdarg.h>

static inline void DebugPrintf(char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   OutputDebugString(buf);
}
#else
static inline void DebugPrintf(char *fmt, ...)
{ }
#endif

// profiling
#define MOTION_PROFILE_ME              0
#define MOTION_PROFILE_LUMA_VAR        1
#define MOTION_PROFILE_COMPENSATION    2
#define MOTION_PROFILE_INTERPOLATION   3
#define MOTION_PROFILE_PREDICTION      4
#define MOTION_PROFILE_2X              5
#define MOTION_PROFILE_MASK            6
#define MOTION_PROFILE_RESIZE          7
#define MOTION_PROFILE_FLOWINTER       8
#define MOTION_PROFILE_YUY2CONVERT     9
#define MOTION_PROFILE_COUNT           10

#ifdef MOTION_PROFILE

extern int ProfTimeTable[MOTION_PROFILE_COUNT];
extern int ProfResults[MOTION_PROFILE_COUNT * 2];
extern __int64 ProfCumulatedResults[MOTION_PROFILE_COUNT * 2];

#define PROFILE_START(x) \
{ \
   __asm { rdtsc }; \
   __asm { mov [ProfTimeTable + 4 * x], eax }; \
}

#define PROFILE_STOP(x) \
{ \
   __asm { rdtsc }; \
   __asm { sub eax, [ProfTimeTable + 4 * x] }; \
   __asm { add eax, [ProfResults + 8 * x] }; \
   __asm { mov [ProfResults + 8 * x], eax }; \
   ProfResults[2 * x + 1]++; \
}

inline static void PROFILE_INIT()
{
   for ( int i = 0; i < MOTION_PROFILE_COUNT; i++ )
   {
      ProfTimeTable[i]                 = 0;
      ProfResults[2 * i]               = 0;
      ProfResults[2 * i + 1]           = 0;
      ProfCumulatedResults[2 * i]      = 0;
      ProfCumulatedResults[2 * i + 1]  = 0;
   }
}
inline static void PROFILE_CUMULATE()
{
   for ( int i = 0; i < MOTION_PROFILE_COUNT; i++ )
   {
      ProfCumulatedResults[2 * i]     += ProfResults[2 * i];
      ProfCumulatedResults[2 * i + 1] += ProfResults[2 * i + 1];
      ProfResults[2 * i]     = 0;
      ProfResults[2 * i + 1] = 0;
   }
}
inline static void PROFILE_SHOW()
{
   __int64 nTotal = 0;
   for ( int i = 0; i < MOTION_PROFILE_COUNT; i++ )
      nTotal += ProfCumulatedResults[2*i];

   DebugPrintf("ME            : %2.1f %%", (double)ProfCumulatedResults[2 * MOTION_PROFILE_ME] * 100.0f / (nTotal + 1));
   DebugPrintf("LUMA & VAR    : %2.1f %%", (double)ProfCumulatedResults[2 * MOTION_PROFILE_LUMA_VAR] * 100.0f / (nTotal + 1));
   DebugPrintf("COMPENSATION  : %2.1f %%", (double)ProfCumulatedResults[2 * MOTION_PROFILE_COMPENSATION] * 100.0f / (nTotal + 1));
   DebugPrintf("INTERPOLATION : %2.1f %%", (double)ProfCumulatedResults[2 * MOTION_PROFILE_INTERPOLATION] * 100.0f / (nTotal + 1));
   DebugPrintf("PREDICTION    : %2.1f %%", (double)ProfCumulatedResults[2 * MOTION_PROFILE_PREDICTION] * 100.0f / (nTotal + 1));
   DebugPrintf("2X            : %2.1f %%", (double)ProfCumulatedResults[2 * MOTION_PROFILE_2X] * 100.0f / (nTotal + 1));
   DebugPrintf("MASK          : %2.1f %%", (double)ProfCumulatedResults[2 * MOTION_PROFILE_MASK] * 100.0f / (nTotal + 1));
   DebugPrintf("RESIZE        : %2.1f %%", (double)ProfCumulatedResults[2 * MOTION_PROFILE_RESIZE] * 100.0f / (nTotal + 1));
   DebugPrintf("FLOWINTER     : %2.1f %%", (double)ProfCumulatedResults[2 * MOTION_PROFILE_FLOWINTER] * 100.0f / (nTotal + 1));
   DebugPrintf("YUY2CONVERT     : %2.1f %%", (double)ProfCumulatedResults[2 * MOTION_PROFILE_YUY2CONVERT] * 100.0f / (nTotal + 1));
}
#else
#define PROFILE_START(x)
#define PROFILE_STOP(x)
#define PROFILE_INIT()
#define PROFILE_CUMULATE()
#define PROFILE_SHOW()
#endif

class FakeBlockData {
	int x;
	int y;
	VECTOR vector;
//	int nSad;
//	int nLength;
//   int nVariance;
//   int nLuma;
//   int nRefLuma;
//   int nPitch;

//   const unsigned char *pRef;

//	inline static int SquareLength(const VECTOR& v)
//	{ return v.x * v.x + v.y * v.y; }

public :
    FakeBlockData();
    FakeBlockData(int _x, int _y);
    ~FakeBlockData();

    void Init(int _x, int _y);
	void Update(const int *array);

	inline int GetX() const { return x; }
	inline int GetY() const { return y; }
	inline VECTOR GetMV() const { return vector; }
	inline int GetSAD() const { return vector.sad; }
//	inline int GetMVLength() const { return nLength; }
//   inline int GetVariance() const { return nVariance; }
//   inline int GetLuma() const { return nLuma; }
//   inline int GetRefLuma() const { return nRefLuma; }
//   inline const unsigned char *GetRef() const { return pRef; }
//   inline int GetPitch() const { return nPitch; }
};

class FakePlaneOfBlocks {

	int nWidth_Bi;
	int nHeight_Bi;
	int nBlkX;
	int nBlkY;
	int nBlkSizeX;
	int nBlkSizeY;
	int nBlkCount;
	int nPel;
	int nLogPel;
	int nScale;
	int nLogScale;
	int nOverlapX;
	int nOverlapY;

	FakeBlockData *blocks;

public :

	FakePlaneOfBlocks(int sizex,  int sizey, int lv, int pel, int overlapx, int overlapy, int nBlkX, int nBlkY);
	~FakePlaneOfBlocks();

	void Update(const int *array);
	bool IsSceneChange(int nTh1, int nTh2) const;

	inline bool IsInFrame(int i) const
	{
		return (( i >= 0 ) && ( i < nBlkCount ));
	}

	inline const FakeBlockData& operator[](const int i) const {
		return (blocks[i]);
	}

	inline int GetBlockCount() const { return nBlkCount; }
	inline int GetReducedWidth() const { return nBlkX; }
	inline int GetReducedHeight() const { return nBlkY; }
	inline int GetWidth() const { return nWidth_Bi; }
	inline int GetHeight() const { return nHeight_Bi; }
	inline int GetScaleLevel() const { return nLogScale; }
	inline int GetEffectiveScale() const { return nScale; }
	inline int GetBlockSizeX() const { return nBlkSizeX; }
	inline int GetBlockSizeY() const { return nBlkSizeY; }
	inline int GetPel() const { return nPel; }
   inline const FakeBlockData& GetBlock(int i) const { return (blocks[i]); }
	inline int GetOverlapX() const { return nOverlapX; }
	inline int GetOverlapY() const { return nOverlapY; }
};

class FakeGroupOfPlanes {
	int nLvCount_;
	bool validity;
   int nWidth_B;
   int nHeight_B;
//   int nOverlap;
   int yRatioUV_B;
	FakePlaneOfBlocks **planes;
//   const unsigned char *compensatedPlane;
//   const unsigned char *compensatedPlaneU;
//   const unsigned char *compensatedPlaneV;
	inline static bool GetValidity(const int *array) { return (array[1] == 1); }
   CRITICAL_SECTION cs;

public :
   FakeGroupOfPlanes();
//	FakeGroupOfPlanes(int w, int h, int size, int lv, int pel);
	~FakeGroupOfPlanes();

   void Create(int _nBlkSizeX, int _nBlkSizeY, int _nLevelCount, int _nPel, int _nOverlapX, int _nOverlapY, int _yRatioUV, int _nBlkX, int _nBlkY);

	void Update(const int *array);
	bool IsSceneChange(int nThSCD1, int nThSCD2) const;

	inline const FakePlaneOfBlocks& operator[](const int i) const {
		return *(planes[i]);
	}


	inline bool IsValid() const { return validity; }
//   inline const unsigned char *GetCompensatedPlane() const { return compensatedPlane; }
//   inline const unsigned char *GetCompensatedPlaneU() const { return compensatedPlaneU; }
//   inline const unsigned char *GetCompensatedPlaneV() const { return compensatedPlaneV; }
   inline int GetPitch() const { return nWidth_B; }
   inline int GetPitchUV() const { return nWidth_B / 2; }

	inline const FakePlaneOfBlocks& GetPlane(int i) const { return *(planes[i]); }
};

//class MVCore;

#define MVANALYSIS_DATA_VERSION 5

class MVAnalysisData
{
public:
   /*! \brief Unique identifier, not very useful */
   int nMagicKey; // placed to head in v.1.2.6

   int nVersion; // MVAnalysisData and outfile format version - added in v1.2.6

   /*! \brief size of a block, in pixel */
   int nBlkSizeX; // horizontal block size

	int nBlkSizeY; // vertical block size - v1.7

   /*! \brief pixel refinement of the motion estimation */
   int nPel;

   /*! \brief number of level for the hierarchal search */
   int nLvCount;

   /*! \brief difference between the index of the reference and the index of the current frame */
   int nDeltaFrame;

   /*! \brief direction of the search ( forward / backward ) */
	bool isBackward;

   /*! \brief diverse flags to set up the search */
   int nFlags;

	/*! \brief Width of the frame */
	int nWidth;

	/*! \brief Height of the frame */
	int nHeight;

   /*! \brief MVFrames idx */
//   int nIdx;

  int nOverlapX; // overlap block size - v1.1

	int nOverlapY; // vertical overlap - v1.7

   int nBlkX; // number of blocks along X

   int nBlkY; // number of blocks along Y

   int pixelType; // color format

   int yRatioUV; // ratio of luma plane height to chroma plane height

   int xRatioUV; // ratio of luma plane height to chroma plane width (fixed to 2 for YV12 and YUY2)

//	int sharp; // pel2 interpolation type

//	bool usePelClip; // use extra clip with upsized 2x frame size

//	MVCore *pmvCore; // last (but is not really useful for written file)

	int nHPadding; // Horizontal padding - v1.8.1

	int nVPadding; // Vertical padding - v1.8.1


public :

   inline void SetFlags(int _nFlags) { nFlags |= _nFlags; }
   inline int GetFlags() const { return nFlags; }
   inline int GetBlkSizeX() const { return nBlkSizeX; }
   inline int GetPel() const { return nPel; }
   inline int GetLevelCount() const { return nLvCount; }
//   inline int GetFramesIdx() const { return nIdx; }
   inline bool IsBackward() const { return isBackward; }
   inline int GetMagicKey() const { return nMagicKey; }
   inline int GetDeltaFrame() const { return nDeltaFrame; }
   inline int GetWidth() const { return nWidth; }
   inline int GetHeight() const { return nHeight; }
   inline bool IsChromaMotion() const { return nFlags & MOTION_USE_CHROMA_MOTION; }
//   inline MVCore *GetMVCore() const { return pmvCore; }
   inline int GetOverlapX() const { return nOverlapX; }
   inline int GetBlkX() const { return nBlkX; }
   inline int GetBlkY() const { return nBlkY; }
   inline int GetPixelType() const { return pixelType; }
   inline int GetYRatioUV() const { return yRatioUV; }
   inline int GetXRatioUV() const { return xRatioUV; }
//   inline int GetSharp() const { return sharp; }
//   inline bool UsePelClip() const { return usePelClip; }
   inline int GetBlkSizeY() const { return nBlkSizeY; }
   inline int GetOverlapY() const { return nOverlapY; }
   inline int GetHPadding() const { return nHPadding; }
   inline int GetVPadding() const { return nVPadding; }

};

class MVClip : public GenericVideoFilter, public FakeGroupOfPlanes, public MVAnalysisData
{
	/*! \brief Number of blocks horizontaly, at the first level */
//	int nBlkX;

	/*! \brief Number of blocks verticaly, at the first level */
//	int nBlkY;

	/*! \brief Number of blocks at the first level */
	int nBlkCount;

   /*! \brief Horizontal padding */
   int nHPadding;

   /*! \brief Vertical padding */
   int nVPadding;

   /*! \brief First Scene Change Detection threshold ( compared against SAD value of the block ) */
   int nSCD1;

   /*! \brief Second Scene Change Detection threshold ( compared against the number of block over the first threshold */
   int nSCD2;

   int nHeaderSize; // offset to data

public :
   MVClip(const PClip &vectors, int nSCD1, int nSCD2, IScriptEnvironment *env);
   ~MVClip();

//   void SetVectorsNeed(bool srcluma, bool refluma, bool var,
//                       bool compy, bool compu, bool compv) const;

//   void Update(int n, IScriptEnvironment *env);
   void Update(PVideoFrame &fn, IScriptEnvironment *env); // v1.4.13

   // encapsulation
//   inline int GetBlkX() const { return nBlkX; }
//   inline int GetBlkY() const { return nBlkY; }
   inline int GetBlkCount() const { return nBlkCount; }
   inline int GetHPadding() const { return nHPadding; }
   inline int GetVPadding() const { return nVPadding; }
   inline int GetThSCD1() const { return nSCD1; }
   inline int GetThSCD2() const { return nSCD2; }
   inline const FakeBlockData& GetBlock(int nLevel, int nBlk) const { return GetPlane(nLevel)[nBlk]; }
   bool IsUsable(int nSCD1, int nSCD2) const;
   bool IsUsable() const { return IsUsable(nSCD1, nSCD2); }
   bool IsSceneChange() const { return FakeGroupOfPlanes::IsSceneChange(nSCD1, nSCD2); }
};

class MVClipArray
{
   int size_;
   MVClip **pmvClips;

public :
   MVClipArray(const AVSValue &vectors, int nSCD1, int nSCD2, IScriptEnvironment *env);
   ~MVClipArray();
//   void Update(int n, IScriptEnvironment *env); // excluded

   inline int size() { return size_; }
   inline MVClip &operator[](int i) { return *(pmvClips[i]); }

};

class MVFilter {
protected:
	/*! \brief Number of blocks horizontaly, at the first level */
	int nBlkX;

	/*! \brief Number of blocks verticaly, at the first level */
	int nBlkY;

	/*! \brief Number of blocks at the first level */
	int nBlkCount;

	/*! \brief Number of blocks at the first level */
	int nBlkSizeX;

	int nBlkSizeY;

   /*! \brief Horizontal padding */
   int nHPadding;

   /*! \brief Vertical padding */
   int nVPadding;

	/*! \brief Width of the frame */
	int nWidth;

	/*! \brief Height of the frame */
	int nHeight;

   /*! \brief MVFrames idx */
   int nIdx;

   /*! \brief pixel refinement of the motion estimation */
   int nPel;

   int nOverlapX;
   int nOverlapY;

   int pixelType;
   int yRatioUV;

   /*! \brief Filter's name */
//   std::string name;
   const char * name; //v1.8 replaced std::string (why it was used?)

   /*! \brief Pointer to the MVCore object */
//   MVCore *mvCore;

   MVFilter(const PClip &vector, const char *filterName, IScriptEnvironment *env);

   void CheckSimilarity(const MVClip &vector, const char *vectorName, IScriptEnvironment *env);
};

//#define MOTION_DELTA_FRAME_BUFFER 5

class MVPlane {
   Uint8 **pPlane;
   int nWidth;
   int nHeight;
   int nExtendedWidth;
   int nExtendedHeight;
   int nPitch;
   int nHPadding;
   int nVPadding;
   int nOffsetPadding;
   int nHPaddingPel;
   int nVPaddingPel;

   int nPel;

   bool isse;

   bool isPadded;
   bool isRefined;
   bool isFilled;

   CRITICAL_SECTION cs;

public :

   MVPlane(int _nWidth, int _nHeight, int _nPel, int _nHPad, int _nVPad, bool _isse);
   ~MVPlane();

   void Update(BYTE* pSrc, int _nPitch);
   void ChangePlane(const Uint8 *pNewPlane, int nNewPitch);
   void Pad();
   void Refine(int interType);
   void RefineExt(const Uint8 *pSrc2x, int nSrc2xPitch, bool isExtPadded); //2.0.08
   void ReduceTo(MVPlane *pReducedPlane, int rfilter);
   void WritePlane(FILE *pFile);

   inline const Uint8 *GetAbsolutePointer(int nX, int nY) const
   {
      if ( nPel == 1 )
         return pPlane[0] + nX + nY * nPitch;
      else if (nPel == 2) {
      int idx = (nX&1) | ((nY&1)<<1);

      nX >>= 1;
      nY >>= 1;

      return pPlane[idx] + nX + nY * nPitch;
     }
      else // nPel = 4
      {
      int idx = (nX&3) | ((nY&3)<<2);

      nX >>= 2;
      nY >>= 2;

      return pPlane[idx] + nX + nY * nPitch;
      }
   }

   inline const Uint8 *GetAbsolutePointerPel1(int nX, int nY) const
   {
         return pPlane[0] + nX + nY * nPitch;
   }

   inline const Uint8 *GetAbsolutePointerPel2(int nX, int nY) const
   {
      int idx = (nX&1) | ((nY&1)<<1);

      nX >>= 1;
      nY >>= 1;

      return pPlane[idx] + nX + nY * nPitch;
   }

   inline const Uint8 *GetAbsolutePointerPel4(int nX, int nY) const
   {
      int idx = (nX&3) | ((nY&3)<<2);

      nX >>= 2;
      nY >>= 2;

      return pPlane[idx] + nX + nY * nPitch;
   }

   inline const Uint8 *GetPointer(int nX, int nY) const
   {
      return GetAbsolutePointer(nX + nHPaddingPel, nY + nVPaddingPel);
   }

   inline const Uint8 *GetPointerPel1(int nX, int nY) const
   {
      return GetAbsolutePointerPel1(nX + nHPaddingPel, nY + nVPaddingPel);
   }

   inline const Uint8 *GetPointerPel2(int nX, int nY) const
   {
      return GetAbsolutePointerPel2(nX + nHPaddingPel, nY + nVPaddingPel);
   }

   inline const Uint8 *GetPointerPel4(int nX, int nY) const
   {
      return GetAbsolutePointerPel4(nX + nHPaddingPel, nY + nVPaddingPel);
   }

   inline const Uint8 *GetAbsolutePelPointer(int nX, int nY) const
   {  return pPlane[0] + nX + nY * nPitch; }

   inline int GetPitch() const { return nPitch; }
   inline int GetWidth() const { return nWidth; }
   inline int GetHeight() const { return nHeight; }
   inline int GetExtendedWidth() const { return nExtendedWidth; }
   inline int GetExtendedHeight() const { return nExtendedHeight; }
   inline int GetHPadding() const { return nHPadding; }
   inline int GetVPadding() const { return nVPadding; }
   inline void ResetState() { isRefined = isFilled = isPadded = false; }

};

class MVFrame {

   MVPlane *pYPlane;
   MVPlane *pUPlane;
   MVPlane *pVPlane;

   int nMode;
   bool isse;
   int yRatioUV;

public:
   MVFrame(int nWidth, int nHeight, int nPel, int nHPad, int nVPad, int _nMode, bool _isse, int _yRatioUV);
   ~MVFrame();

   void Update(int _nMode, BYTE * pSrcY, int pitchY, BYTE * pSrcU, int pitchU, BYTE *pSrcV, int pitchV);
   void ChangePlane(const Uint8 *pNewSrc, int nNewPitch, MVPlaneSet _nMode);
   void Refine(MVPlaneSet _nMode, int interType);
   void Pad(MVPlaneSet _nMode);
   void ReduceTo(MVFrame *pFrame, MVPlaneSet _nMode, int rfilter);
   void ResetState();
   void WriteFrame(FILE *pFile);

   inline MVPlane *GetPlane(MVPlaneSet _nMode)
   {
      if ( nMode & _nMode & YPLANE )
         return pYPlane;

      if ( nMode & _nMode & UPLANE )
         return pUPlane;

      if ( nMode & _nMode & VPLANE )
         return pVPlane;

      return 0;
   }

   inline int GetMode() { return nMode; }

};

class MVFrames;

class MVGroupOfFrames {
   int nLevelCount;
   MVFrame **pFrames;

//   int nFrameIdx;
//   int nRefCount;
   int nWidth;
   int nHeight;
   int nPel;
   int nHPad;
   int nVPad;
   int yRatioUV;

public :

   MVGroupOfFrames(int _nLevelCount, int nWidth, int nHeight, int nPel, int nHPad, int nVPad, int nMode, bool isse, int yRatioUV);
   ~MVGroupOfFrames();
   void Update(int nModeYUV, BYTE * pSrcY, int pitchY, BYTE * pSrcU, int pitchU, BYTE *pSrcV, int pitchV);

   MVFrame *GetFrame(int nLevel);
   void SetPlane(const Uint8 *pNewSrc, int nNewPitch, MVPlaneSet nMode);
//   void SetFrameIdx(int _nFrameIdx);
   void Refine(MVPlaneSet nMode, int interType);
   void Pad(MVPlaneSet nMode);
   void Reduce(MVPlaneSet nMode, int rfilter);
   void ResetState();

//   inline int GetFrameIdx() { return nFrameIdx; }
//   inline int GetRefCount() { return nRefCount; }
//   inline void IncRefCount() { InterlockedIncrement(reinterpret_cast<LONG*>(&nRefCount)); }
//   inline void DecRefCount() { InterlockedDecrement(reinterpret_cast<LONG*>(&nRefCount)); }
};

/*
//smartpointer adopted from avisynth.h PVideoFrame
class PMVGroupOfFrames {


  MVGroupOfFrames* p;

  void Init(MVGroupOfFrames* x) {
    if (x) x->IncRefCount();
    p=x;
  }

  void Set(MVGroupOfFrames* x) {
    if (x) x->IncRefCount();
    if (p) p->DecRefCount();
    p=x;
  }

public:
  PMVGroupOfFrames() { p = 0; }
  PMVGroupOfFrames(const PMVGroupOfFrames& x) { Init(x.p); }
  PMVGroupOfFrames(MVGroupOfFrames* x) { Init(x); }
  void operator=(MVGroupOfFrames* x) { Set(x); }
  void operator=(const PMVGroupOfFrames& x) { Set(x.p); }

  MVGroupOfFrames* operator->() const { return p; }

  // for conditional expressions
  operator void*() const { return p; }
  bool operator!() const { return !p; }

  ~PMVGroupOfFrames() { if (p) p->DecRefCount();}
};

class MVFrames {

   MVGroupOfFrames **pGroupsOfFrames;
   int nNbFrames;
   int nIdx;
   CRITICAL_SECTION cs;

   int nLevelCount;
   int nWidth;
   int nHeight;
   int nPel;
   int nHPad;
   int nVPad;
   int nMode;
   bool isse;
   int yRatioUV;

   class FrameListNode {
   public:
	   FrameListNode(){_next=this;_prev=this;n=-1;}
	   FrameListNode(FrameListNode* prev,FrameListNode* next){next->_prev=prev->_next=this;_next=next;_prev=prev;n=-1;}
	   ~FrameListNode(){_next->_prev=_prev;_prev->_next=_next;if(_next!=this)delete _next;}
		int n;
		void Relink(FrameListNode* prev,FrameListNode* next){_next->_prev=_prev;_prev->_next=_next;next->_prev=prev->_next=this;_next=next;_prev=prev;}

		FrameListNode* Next(){return _next;}
		FrameListNode* Prev(){return _prev;}
   private:
	   	FrameListNode* _next;
		FrameListNode* _prev;
   };
   FrameListNode *startFLN;
   int nFLN;

public:

   MVFrames(int _nIdx, int _nNbFrames, int _nLevelCount, int _nWidth, int _nHeight, int _nPel, int _nHPad, int _nVPad, int _nMode, bool _isse, int _yRatioUV);
   ~MVFrames();

   PMVGroupOfFrames GetNewFrame(int nFrameIdx);
   PMVGroupOfFrames GetFrame(int nFrameIdx);
   inline int GetIdx() { return nIdx; }
   void GrowMVGroupOfFrames(int newsize);

};


class MVFramesList {
   MVFrames *pMVFrames;
   MVFramesList *pNext;

public :

   MVFramesList(MVFramesList *_pNext, int nIdx, int nNbFrames, int nLevelCount, int nWidth, int nHeight, int nPel, int nHPad, int nVPad, int nMode, bool isse, int yRatioUV);
   MVFramesList();
   ~MVFramesList();

   MVFrames *GetFrames(int _nIdx);
};

class MVCore {

   MVFramesList *pFramesList;

   int nAutoIdx;

   int nRefCount;

public:

   MVCore();
   ~MVCore();

   void AddFrames(int nIdx, int nNbFrames, int nLevelCount, int nWidth,
                  int nHeight, int nPel, int nHPad, int nVPad, int nMode, bool isse, int yRatioUV);
   MVFrames *GetFrames(int nIdx);

   inline int GetNextIdx() { nAutoIdx--; return nAutoIdx; }

};
*/
#endif
