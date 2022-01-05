#ifndef __MV_SCDETECTION__
#define __MV_SCDETECTION__


#include "MVInterface.h"


class MVSCDetection : public GenericVideoFilter, public MVFilter {
private:

	MVClip mvClip;
   unsigned char sceneChangeValue;

public:
	MVSCDetection(PClip _child, PClip vectors, int nSceneChangeValue, int nSCD1, int nSCD2, bool isse, IScriptEnvironment* env);
	~MVSCDetection();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

#endif
