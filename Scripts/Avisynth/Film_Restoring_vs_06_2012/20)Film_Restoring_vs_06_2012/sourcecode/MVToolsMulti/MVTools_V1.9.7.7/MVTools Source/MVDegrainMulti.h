#ifndef __MV_DEGRAINMULTI__
#define __MV_DEGRAINMULTI__

#include "MVInterface.h"
#include "MVDegrainBase.h"

/*! \brief Filter that denoise the picture
 */
class MVDegrainMulti : public MVDegrainBase {
private:
    unsigned int RefFrames; // number of reference frames
public:
	MVDegrainMulti(PClip _child, PClip mvMulti, int _RefFrames, int _thSAD, int _thSADC, int _YUVplanes, int _nLimit,
			       PClip _pelclip, int _nIdx, int nSCD1, int nSCD2, bool mmx, bool isse, int _MaxThreads, int _PreFetch,
                   int _SadMode, IScriptEnvironment* env);
	~MVDegrainMulti();
};

#endif

