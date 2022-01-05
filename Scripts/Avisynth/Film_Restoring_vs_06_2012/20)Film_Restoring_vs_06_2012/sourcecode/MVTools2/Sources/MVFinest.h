#ifndef __MV_FINEST__
#define __MV_FINEST__

#include "MVInterface.h"
#include "yuy2planes.h"

class MVFinest : public GenericVideoFilter {

private:

   int mode;
   bool fields;
//   PClip pelclip;
   bool isse;


   int nPel;

   PClip super; // v2.0
    int nSuperWidth;
    int nSuperHeight;
    int nSuperHPad;
    int nSuperVPad;
    MVGroupOfFrames *pRefGOF;
//   bool usePelClipHere;


//	YUY2Planes * RefPlanes;
//	YUY2Planes * DstPlanes;


public:
	MVFinest(PClip _super, bool isse, IScriptEnvironment* env);
	~MVFinest();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

#endif
