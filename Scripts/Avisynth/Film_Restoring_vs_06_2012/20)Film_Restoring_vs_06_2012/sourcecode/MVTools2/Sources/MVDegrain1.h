#ifndef __MV_DEGRAIN1__
#define __MV_DEGRAIN1__

#include "MVInterface.h"
#include "CopyCode.h"
#include "yuy2planes.h"
#include "overlap.h"

typedef void (Denoise1Function)(BYTE *pDst, int nDstPitch, const BYTE *pSrc, int nSrcPitch,
						const BYTE *pRefB, int BPitch, const BYTE *pRefF, int FPitch,
						int WSrc, int WRefB, int WRefF);

/*! \brief Filter that Denoise the picture
 */
class MVDegrain1 : public GenericVideoFilter, public MVFilter {
private:

   MVClip mvClipB;
   MVClip mvClipF;
   int thSAD;
   int thSADC;
   int YUVplanes;
   int nLimit;
   bool isse;
   bool planar;

   PClip super; // v2.0
    MVGroupOfFrames *pRefBGOF, *pRefFGOF;
    int nSuperModeYUV;

	YUY2Planes * DstPlanes;
	YUY2Planes * SrcPlanes;

	short *winOver;
	short *winOverUV;

	OverlapWindows *OverWins;
	OverlapWindows *OverWinsUV;

	OverlapsFunction *OVERSLUMA;
	OverlapsFunction *OVERSCHROMA;
	Denoise1Function *DEGRAINLUMA;
	Denoise1Function *DEGRAINCHROMA;

	unsigned char *tmpBlock;
	unsigned short * DstShort;
	int dstShortPitch;

public:
	MVDegrain1(PClip _child, PClip _super, PClip _mvbw, PClip _mvfw, int _thSAD, int _thSADC, int _YUVplanes, int nLimit,
		int nSCD1, int nSCD2, bool isse, bool _planar, IScriptEnvironment* env);
	~MVDegrain1();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

template <int width, int height>
void Degrain1_C(BYTE *pDst, int nDstPitch, const BYTE *pSrc, int nSrcPitch,
						const BYTE *pRefB, int BPitch, const BYTE *pRefF, int FPitch,
						int WSrc, int WRefB, int WRefF)
{
	for (int h=0; h<height; h++)
	{
		for (int x=0; x<width; x++)
		{
			 pDst[x] = (pRefF[x]*WRefF + pSrc[x]*WSrc + pRefB[x]*WRefB + 128)>>8;// weighted (by SAD) average
		}
		pDst += nDstPitch;
		pSrc += nSrcPitch;
		pRefB += BPitch;
		pRefF += FPitch;
	}
}

#endif
