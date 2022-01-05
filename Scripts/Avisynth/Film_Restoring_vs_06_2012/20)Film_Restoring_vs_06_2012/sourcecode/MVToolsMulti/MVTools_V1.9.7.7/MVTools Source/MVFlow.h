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
    PClip pelclip;
    bool isse;
    bool mmx;

    bool usePelClipHere;

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

    int pel2PitchY, pel2HeightY, pel2PitchUV, pel2HeightUV, pel2OffsetY, pel2OffsetUV;

    BYTE * pel2PlaneY; // big plane
    BYTE * pel2PlaneU;
    BYTE * pel2PlaneV;

    int LUTV[256]; // luckup table for v

    SimpleResize *upsizer;
    SimpleResize *upsizerUV;
    void Fetch(BYTE * pdst, int dst_pitch, const BYTE *pref, int ref_pitch,  BYTE *VXFull, int VXPitch,  BYTE *VYFull, int VYPitch, int width, int height, int time256);
    void Shift(BYTE * pdst, int dst_pitch, const BYTE *pref, int ref_pitch,  BYTE *VXFull, int VXPitch,  BYTE *VYFull, int VYPitch, int width, int height, int time256);
    void Create_LUTV(int time256, int *LUTV);

    YUY2Planes * DstPlanes;

public:
    MVFlow(PClip _child, PClip _vectors, int _time256, int _mode, bool _fields, PClip _pelclip, int _nIdx,
           int nSCD1, int nSCD2, bool mmx, bool isse, IScriptEnvironment* env);
    ~MVFlow();
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
    void ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
                                       int const Sharp, int const pixelType, int const nHeight, int const nWidth, 
                                       int const nPel, bool const isse);
};

#endif
