#ifndef __MV_FLOW__
#define __MV_FLOW__

#include "MVInterface.h"
#include "SimpleResize.h"
#include "yuy2planes.h"

class MVFlow : public GenericVideoFilter, public MVFilter {

private:

   MVClip mvClip;
   int time256;
   int mode;
   bool fields;
//   PClip pelclip;
   bool isse;
   bool planar;

   PClip finest; // v2.0

   BYTE *VXFullY; // fullframe vector mask
   BYTE *VXFullUV;
   BYTE *VYFullY;
   BYTE *VYFullUV;

   BYTE *VXSmallY; // Small vector mask
   BYTE *VXSmallUV;
   BYTE *VYSmallY;
   BYTE *VYSmallUV;

	int nBlkXP;
	 int nBlkYP;
	 int nWidthP;
	 int nHeightP;
	 int nWidthPUV;
	 int nHeightPUV;

   int VPitchY;
   int VPitchUV;

   int nWidthUV;
   int nHeightUV;

	 int nHPaddingUV;
	 int nVPaddingUV;

   int *LUTV; // luckup table for v

   SimpleResize *upsizer;
   SimpleResize *upsizerUV;
	void Fetch(BYTE * pdst, int dst_pitch, const BYTE *pref, int ref_pitch,  BYTE *VXFull, int VXPitch,  BYTE *VYFull, int VYPitch, int width, int height, int time256);
	void Shift(BYTE * pdst, int dst_pitch, const BYTE *pref, int ref_pitch,  BYTE *VXFull, int VXPitch,  BYTE *VYFull, int VYPitch, int width, int height, int time256);
	void Create_LUTV(int time256, int *LUTV);

	YUY2Planes * DstPlanes;

public:
	MVFlow(PClip _child, PClip _super, PClip _vectors, int _time256, int _mode, bool _fields,
                int nSCD1, int nSCD2, bool isse, bool _planar, IScriptEnvironment* env);
	~MVFlow();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

#endif
