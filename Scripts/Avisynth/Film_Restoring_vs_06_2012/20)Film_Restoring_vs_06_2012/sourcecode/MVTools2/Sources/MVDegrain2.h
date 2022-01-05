#ifndef __MV_DEGRAIN2__
#define __MV_DEGRAIN2__

#include "MVInterface.h"
#include "CopyCode.h"
#include "yuy2planes.h"
#include "overlap.h"

typedef void (Denoise2Function)(BYTE *pDst, int nDstPitch, const BYTE *pSrc, int nSrcPitch,
						const BYTE *pRefB, int BPitch, const BYTE *pRefF, int FPitch,
						const BYTE *pRefB2, int B2Pitch, const BYTE *pRefF2, int F2Pitch,
						int WSrc, int WRefB, int WRefF, int WRefB2, int WRefF2);

/*! \brief Filter that denoise the picture
 */
class MVDegrain2 : public GenericVideoFilter, public MVFilter {
private:

   MVClip mvClipB;
   MVClip mvClipF;
   MVClip mvClipB2;
   MVClip mvClipF2;
   int thSAD;
   int thSADC;
   int YUVplanes;
   int nLimit;
   bool isse;
   bool planar;

   PClip super; // v2.0
   int nSuperModeYUV;
    MVGroupOfFrames *pRefBGOF, *pRefFGOF;
    MVGroupOfFrames *pRefB2GOF, *pRefF2GOF;

	YUY2Planes * DstPlanes;
	YUY2Planes * SrcPlanes;

	short *winOver;
	short *winOverUV;

	OverlapWindows *OverWins;
	OverlapWindows *OverWinsUV;

	OverlapsFunction *OVERSLUMA;
	OverlapsFunction *OVERSCHROMA;
	Denoise2Function *DEGRAINLUMA;
	Denoise2Function *DEGRAINCHROMA;

	unsigned char *tmpBlock;
	unsigned short * DstShort;
	int dstShortPitch;

public:
	MVDegrain2(PClip _child, PClip _super, PClip _mvbw2, PClip _mvbw, PClip _mvfw, PClip _mvfw2, int _thSAD, int _thSADC, int _YUVplanes, int _nLimit,
		int nSCD1, int nSCD2, bool isse, bool _planar, IScriptEnvironment* env);
	~MVDegrain2();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

template<int blockWidth, int blockHeight>
void Degrain2_C(BYTE *pDst, int nDstPitch, const BYTE *pSrc, int nSrcPitch,
						const BYTE *pRefB, int BPitch, const BYTE *pRefF, int FPitch,
						const BYTE *pRefB2, int B2Pitch, const BYTE *pRefF2, int F2Pitch,
						int WSrc, int WRefB, int WRefF, int WRefB2, int WRefF2)
{
	for (int h=0; h<blockHeight; h++)
	{
		for (int x=0; x<blockWidth; x++)
		{
			 pDst[x] = (pRefF[x]*WRefF + pSrc[x]*WSrc + pRefB[x]*WRefB + pRefF2[x]*WRefF2 + pRefB2[x]*WRefB2 + 128)>>8;
		}
		pDst += nDstPitch;
		pSrc += nSrcPitch;
		pRefB += BPitch;
		pRefF += FPitch;
		pRefB2 += B2Pitch;
		pRefF2 += F2Pitch;
	}
}


#endif
