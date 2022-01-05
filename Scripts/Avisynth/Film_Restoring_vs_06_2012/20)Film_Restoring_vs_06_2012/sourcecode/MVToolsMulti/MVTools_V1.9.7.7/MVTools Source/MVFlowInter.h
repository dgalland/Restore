#ifndef __MV_FLOWINTER__
#define __MV_FLOWINTER__

#include "MVInterface.h"
#include "SimpleResize.h"
#include "yuy2planes.h"

class MVFlowInter : public GenericVideoFilter, public MVFilter {
private:

    MVClip mvClipB;
    MVClip mvClipF;
    int time256;
    double ml;
    double mSAD, mSADC;
    PClip pelclip;
    bool isse;
    bool mmx;

    bool usePelClipHere;

    unsigned int normSAD1024, normSADC1024;

    // fullframe vector mask
    BYTE *VXFullYB; //backward
    BYTE *VXFullUVB;
    BYTE *VYFullYB;
    BYTE *VYFullUVB;
    BYTE *VXFullYF;  // forward
    BYTE *VXFullUVF;
    BYTE *VYFullYF;
    BYTE *VYFullUVF;
    BYTE *VXFullYBB; //backward backward
    BYTE *VXFullUVBB;
    BYTE *VYFullYBB;
    BYTE *VYFullUVBB;
    BYTE *VXFullYFF; // forward forward
    BYTE *VXFullUVFF;
    BYTE *VYFullYFF;
    BYTE *VYFullUVFF;

    // Small vector mask
    BYTE *VXSmallYB;
    BYTE *VXSmallUVB;
    BYTE *VYSmallYB;
    BYTE *VYSmallUVB;
    BYTE *VXSmallYF;
    BYTE *VXSmallUVF;
    BYTE *VYSmallYF;
    BYTE *VYSmallUVF;
    BYTE *VXSmallYBB;
    BYTE *VXSmallUVBB;
    BYTE *VYSmallYBB;
    BYTE *VYSmallUVBB;
    BYTE *VXSmallYFF;
    BYTE *VXSmallUVFF;
    BYTE *VYSmallYFF;
    BYTE *VYSmallUVFF;

    BYTE *MaskSmallB;
    BYTE *MaskFullYB;
    BYTE *MaskFullUVB;
    BYTE *MaskSmallF;
    BYTE *MaskFullYF;
    BYTE *MaskFullUVF;

    BYTE *SADMaskSmallB;
    BYTE *SADMaskSmallF;

    int nWidthUV;
    int nHeightUV;
    int	VPitchY, VPitchUV;
    int nBlkXP;
    int nBlkYP;
    int nWidthP;
    int nHeightP;
    int nWidthPUV;
    int nHeightPUV;

    int nHPaddingUV;
    int nVPaddingUV;

    int LUTVB[256]; // lookup tables
    int LUTVF[256];

    int pel2PitchY, pel2HeightY, pel2PitchUV, pel2HeightUV, pel2OffsetY, pel2OffsetUV;

    BYTE * pel2PlaneYB; // big plane
    BYTE * pel2PlaneUB;
    BYTE * pel2PlaneVB;
    BYTE * pel2PlaneYF; // big plane
    BYTE * pel2PlaneUF;
    BYTE * pel2PlaneVF;

    SimpleResize *upsizer;
    SimpleResize *upsizerUV;
//	void FlowInter(BYTE * pdst, int dst_pitch, const BYTE *prefB, const BYTE *prefF, int ref_pitch,
//			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
//			   int VPitch, int width, int height, int time256);
//	void FlowInterExtra(BYTE * pdst, int dst_pitch, const BYTE *prefB, const BYTE *prefF, int ref_pitch,
//			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
//			   int VPitch, int width, int height, int time256, BYTE *VXFullBB, BYTE *VXFullFF, BYTE *VYFullBB, BYTE *VYFullFF);
//	void Create_LUTV(int time256, int *LUTVB, int *LUTVF);

    YUY2Planes * SrcPlanes;
    YUY2Planes * RefPlanes;
    YUY2Planes * DstPlanes;

public:
    MVFlowInter(PClip _child, PClip _mvbw, PClip _mvfw, int _time256, double _ml, double _mSAD,
                double _mSADC, PClip _pelclip, int _nIdx, int nSCD1, int nSCD2, bool mmx, bool isse,
                IScriptEnvironment* env);
    ~MVFlowInter();
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
    void ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
                                       int const Sharp, int const pixelType, int const nHeight, int const nWidth, 
                                       int const nPel, bool const isse);
};

#endif
