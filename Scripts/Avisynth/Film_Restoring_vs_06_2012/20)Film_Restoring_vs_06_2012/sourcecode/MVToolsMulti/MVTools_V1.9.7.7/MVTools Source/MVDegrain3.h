#ifndef __MV_DEGRAIN3__
#define __MV_DEGRAIN3__

#include "MVInterface.h"
#include "MVDegrainBase.h"

/*! \brief Filter that denoise the picture
 */
class MVDegrain3 : public MVDegrainBase {
private:

public:
    MVDegrain3(PClip _child, PClip mvbw, PClip mvfw, PClip mvbw2, PClip mvfw2, PClip mvbw3, PClip mvfw3,
               int _thSAD, int _thSADC, int _YUVplanes, int _nLimit, PClip _pelclip, int _nIdx, int nSCD1,
               int nSCD2, bool mmx, bool isse, int _MaxThreads, int _PreFetch, int _SadMode, IScriptEnvironment* env);
    ~MVDegrain3();
};


#endif
