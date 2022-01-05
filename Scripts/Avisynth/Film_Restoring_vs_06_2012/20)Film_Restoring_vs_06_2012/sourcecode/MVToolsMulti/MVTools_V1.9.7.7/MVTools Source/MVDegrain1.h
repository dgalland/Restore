#ifndef __MV_DEGRAIN1__
#define __MV_DEGRAIN1__

#include "MVInterface.h"
#include "MVDegrainBase.h"

/*! \brief Filter that denoise the picture
 */
class MVDegrain1 : public MVDegrainBase {
private:

public:
    MVDegrain1(PClip _child, PClip mvbw, PClip mvfw, int _thSAD, int _thSADC, int _YUVplanes, int _nLimit,
               PClip _pelclip, int _nIdx, int nSCD1, int nSCD2, bool mmx, bool isse, int _MaxThreads,
               int _PreFetch, int _SadMode, IScriptEnvironment* env);
    ~MVDegrain1();
};

#endif

