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
    int mode;
    int thSAD, thSADC;
    bool fields;
    PClip pelclip;
    bool isse;
    bool mmx;

    bool usePelClipHere;

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
    int dstShortPitch, dstShortSize;

public:
    MVCompensate(PClip _child, PClip vectors, bool sc, int _mode, int _thres, int _thresC, bool _fields, PClip _pelclip, int _nIdx,
                 int nSCD1, int nSCD2, bool mmx, bool isse, IScriptEnvironment* env);
    ~MVCompensate();
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
    void ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
                                       int const Sharp, int const pixelType, int const nHeight, int const nWidth, 
                                       int const nPel, bool const isse);
};

#endif
