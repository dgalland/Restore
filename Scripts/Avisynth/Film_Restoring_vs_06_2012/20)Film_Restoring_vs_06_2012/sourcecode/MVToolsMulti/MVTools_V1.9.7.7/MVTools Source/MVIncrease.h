#ifndef __MV_INCREASE_H__
#define __MV_INCREASE_H__

#include "MVInterface.h"
#include "CopyCode.h"
#include "yuy2planes.h"

//#define DYNAMIC_COMPILE 1

#ifdef DYNAMIC_COMPILE
#include "SWCopyCode.h"
#endif

/*! \brief Filter that denoise the picture
 */
class MVIncrease : public GenericVideoFilter, public MVFilter {
private:

    MVClip mvClip;

#ifdef DYNAMIC_COMPILE
    SWCopy *copyLuma;
    SWCopy *copyChroma;
#endif

    bool scBehavior;
    bool isse;
    bool mmx;
    int vertical;
    int horizontal;

    int nBlkSizeX2;
    int nBlkSizeY2;

    COPYFunction *BLITLUMA;
    COPYFunction *BLITCHROMA;

    YUY2Planes * DstPlanes;

public:
    MVIncrease(PClip _child, PClip vectors, int vertical, int horizontal, bool scBehavior, int nIdx,
               int nSCD1, int nSCD2, bool mmx, bool isse, IScriptEnvironment* env);
    ~MVIncrease();
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
    void ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
                                       int const Sharp, int const pixelType, int const nHeight, int const nWidth, 
                                       int const nPel, bool const isse);
};

#endif
