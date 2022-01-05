#ifndef __MV_CHANGECOMPENSATE__
#define __MV_CHANGECOMPENSATE__

#include "MVInterface.h"
#include "yuy2planes.h"

/*! \brief Filter that denoise the picture
 */
class MVChangeCompensate : public GenericVideoFilter {
private:
    PClip newCompensation;
    int nWidth, nHeight, nLevelCount;
    int pixelType;
    int yRatioUV;
    YUY2Planes * NewPlanes;
    bool isse;

protected:
public:
    MVChangeCompensate(PClip _child, PClip video, bool _isse, IScriptEnvironment* env);
    ~MVChangeCompensate();
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

#endif