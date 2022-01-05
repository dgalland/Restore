#ifndef __MV_DEGRAIN3__
#define __MV_DEGRAIN3__

#include "MVInterface.h"
#include "CopyCode.h"
#include "yuy2planes.h"
#include "overlap.h"

typedef void (Denoise3Function)(BYTE *pDst, int nDstPitch, const BYTE *pSrc, int nSrcPitch,
						const BYTE *pRefB, int BPitch, const BYTE *pRefF, int FPitch,
						const BYTE *pRefB2, int B2Pitch, const BYTE *pRefF2, int F2Pitch,
						const BYTE *pRefB3, int B3Pitch, const BYTE *pRefF3, int F3Pitch,
						int WSrc, int WRefB, int WRefF, int WRefB2, int WRefF2, int WRefB3, int WRefF3);

/*! \brief Filter that denoise the picture
 */
class MVDegrain3 : public GenericVideoFilter, public MVFilter {
private:

   MVClip mvClipB;
   MVClip mvClipF;
   MVClip mvClipB2;
   MVClip mvClipF2;
   MVClip mvClipB3;
   MVClip mvClipF3;
   int thSAD;
   int thSADC;
   int YUVplanes;
   int nLimit;
   PClip super;
   bool isse;
   bool planar;

    int nSuperModeYUV;

	YUY2Planes * DstPlanes;
	YUY2Planes * SrcPlanes;

	short *winOver;
	short *winOverUV;

	OverlapWindows *OverWins;
	OverlapWindows *OverWinsUV;

	OverlapsFunction *OVERSLUMA;
	OverlapsFunction *OVERSCHROMA;
	Denoise3Function *DEGRAINLUMA;
	Denoise3Function *DEGRAINCHROMA;

	unsigned char *tmpBlock;
	unsigned short * DstShort;
	int dstShortPitch;

    MVGroupOfFrames *pRefBGOF, *pRefFGOF;
    MVGroupOfFrames *pRefB2GOF, *pRefF2GOF;
    MVGroupOfFrames *pRefB3GOF, *pRefF3GOF;

public:
	MVDegrain3(PClip _child, PClip _super, PClip _mvbw, PClip _mvfw, PClip _mvbw2,  PClip _mvfw2, PClip _mvbw3,  PClip _mvfw3,
	int _thSAD, int _thSADC, int _YUVplanes, int _nLimit,
		int nSCD1, int nSCD2, bool isse, bool _planar, IScriptEnvironment* env);
	~MVDegrain3();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

template<int blockWidth, int blockHeight>
void Degrain3_C(BYTE *pDst, int nDstPitch, const BYTE *pSrc, int nSrcPitch,
						const BYTE *pRefB, int BPitch, const BYTE *pRefF, int FPitch,
						const BYTE *pRefB2, int B2Pitch, const BYTE *pRefF2, int F2Pitch,
						const BYTE *pRefB3, int B3Pitch, const BYTE *pRefF3, int F3Pitch,
						int WSrc, int WRefB, int WRefF, int WRefB2, int WRefF2, int WRefB3, int WRefF3)
{
	for (int h=0; h<blockHeight; h++)
	{
		for (int x=0; x<blockWidth; x++)
		{
			 pDst[x] = (pRefF[x]*WRefF + pSrc[x]*WSrc + pRefB[x]*WRefB + pRefF2[x]*WRefF2 + pRefB2[x]*WRefB2 + pRefF3[x]*WRefF3 + pRefB3[x]*WRefB3 + 128)>>8;
		}
		pDst += nDstPitch;
		pSrc += nSrcPitch;
		pRefB += BPitch;
		pRefF += FPitch;
		pRefB2 += B2Pitch;
		pRefF2 += F2Pitch;
		pRefB3 += B3Pitch;
		pRefF3 += F3Pitch;
	}
}

#endif
