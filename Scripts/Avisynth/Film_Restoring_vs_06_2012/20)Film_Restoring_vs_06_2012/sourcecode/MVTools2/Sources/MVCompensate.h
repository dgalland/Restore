#ifndef __MV_COMPENSATE__
#define __MV_COMPENSATE__

#include "MVInterface.h"
#include "CopyCode.h"
#include "yuy2planes.h"
#include "overlap.h"

/*! \brief Filter that compensate the picture
 */
class MVCompensate : public GenericVideoFilter, public MVFilter {
private:

   MVClip mvClip;
   bool scBehavior;
   int recursion;
   int thSAD;
   bool fields;
   PClip super;
   bool isse;
   bool planar;

   COPYFunction *BLITLUMA;
   COPYFunction *BLITCHROMA;

	YUY2Planes * DstPlanes;

	short *winOver;
	short *winOverUV;

	OverlapWindows *OverWins;
	OverlapWindows *OverWinsUV;

	OverlapsFunction *OVERSLUMA;
	OverlapsFunction *OVERSCHROMA;
	unsigned short * DstShort;
	unsigned short * DstShortU;
	unsigned short * DstShortV;
	int dstShortPitch;
	int dstShortPitchUV;

    int nSuperWidth;
    int nSuperHeight;
    int nSuperHPad;
    int nSuperVPad;
    MVGroupOfFrames *pRefGOF, *pSrcGOF;

    unsigned char *pLoop[3];
    int nLoopPitches[3];

public:
	MVCompensate(PClip _child, PClip _super, PClip vectors, bool sc, float _recursionPercent, int _thres, bool _fields,
                int nSCD1, int nSCD2, bool isse, bool _planar, IScriptEnvironment* env);
	~MVCompensate();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

#endif
