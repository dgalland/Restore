#ifndef __MV_FLOWFPS__
#define __MV_FLOWFPS__

#include "MVInterface.h"
#include "SimpleResize.h"
#include "yuy2planes.h"

class MVFlowFps : public GenericVideoFilter, public MVFilter {
private:

    MVClip mvClipB;
    MVClip mvClipF;
    unsigned int numerator;
    unsigned int denominator;
    unsigned int numeratorOld;
    unsigned int denominatorOld;
    int maskmode;
    double ml;
    double mSAD, mSADC;
    PClip pelclip;
    bool isse;
    bool mmx;

    bool usePelClipHere;

    unsigned int normSAD1024, normSAD1024C;

    int time256;
    int nleftLast;
    int nrightLast;

    // fullframe vector mask
    BYTE *VXFullYB; //backward
    BYTE *VXFullUVB;
    BYTE *VYFullYB;
    BYTE *VYFullUVB;
    BYTE *VXFullYF;
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

    int LUTVB[256]; // lookup table
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

    YUY2Planes * DstPlanes;
    YUY2Planes * SrcPlanes;
    YUY2Planes * RefPlanes;

public:
    MVFlowFps(PClip _child, PClip _mvbw, PClip _mvfw, unsigned int _num, unsigned int _den, int _maskmode, double _ml, 
              double _mSAD, double _mSADC, PClip _pelclip, int _nIdx, int nSCD1, int nSCD2, bool mmx, bool isse, 
              IScriptEnvironment* env);
    ~MVFlowFps();
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
    void ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
                                       int const Sharp, int const pixelType, int const nHeight, int const nWidth, 
                                       int const nPel, bool const isse);
};

#endif
