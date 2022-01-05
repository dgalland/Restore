#ifndef __MV_FLOWFPS2__
#define __MV_FLOWFPS2__

#include "MVInterface.h"
#include "SimpleResize.h"
#include "yuy2planes.h"

class MVFlowFps2 : public GenericVideoFilter, public MVFilter {
private:

    MVClip mvClipB;
    MVClip mvClipF;
    MVClip mvClipB2;
    MVClip mvClipF2;
    unsigned int numerator;
    unsigned int denominator;
    unsigned int numeratorOld;
    unsigned int denominatorOld;
    int maskmode;
    double ml;
    double mSAD, mSADC;
    PClip pelclip;
    int nIdx2;
    bool isse;
    bool mmx;

    bool usePelClipHere;

    unsigned int normSAD1024, normSAD1024C;

    int time256;
    int nleftLast;
    int nrightLast;

    int nBlkXCropped;
    int nBlkYCropped;

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
    BYTE *VYSmallYB;
    BYTE *VXSmallYF;
    BYTE *VYSmallYF;

    BYTE *VXSmallYB2;
    BYTE *VYSmallYB2;
    BYTE *VXSmallYF2;
    BYTE *VYSmallYF2;

    BYTE *VXSmallYBB;
    BYTE *VXSmallUVBB;
    BYTE *VYSmallYBB;
    BYTE *VYSmallUVBB;
    BYTE *VXSmallYFF;
    BYTE *VXSmallUVFF;
    BYTE *VYSmallYFF;
    BYTE *VYSmallUVFF;

    BYTE *MaskDoubledB;
    BYTE *MaskFullYB;
    BYTE *MaskFullUVB;
    BYTE *MaskDoubledF;
    BYTE *MaskFullYF;
    BYTE *MaskFullUVF;

    // doubled small vector mask
    BYTE *VXDoubledYB;
    BYTE *VYDoubledYB;
    BYTE *VXDoubledYF;
    BYTE *VYDoubledYF;

    BYTE *VXDoubledYB2;
    BYTE *VYDoubledYB2;
    BYTE *VXDoubledYF2;
    BYTE *VYDoubledYF2;

    BYTE *VXCombinedYB;
    BYTE *VXCombinedUVB;
    BYTE *VYCombinedYB;
    BYTE *VYCombinedUVB;
    BYTE *VXCombinedYF;
    BYTE *VXCombinedUVF;
    BYTE *VYCombinedYF;
    BYTE *VYCombinedUVF;

    BYTE *SADMaskSmallB;
    BYTE *SADMaskSmallF;
    BYTE *SADMaskDoubledB;
    BYTE *SADMaskDoubledF;

    int nWidthUV;
    int nHeightUV;
    int	VPitchY, VPitchUV;

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

    SimpleResize *upsizer2;
    SimpleResize *upsizer;
    SimpleResize *upsizerUV;
    SimpleResize *upsizer1;
    SimpleResize *upsizer1UV;
//	void FlowInter(BYTE * pdst, int dst_pitch, const BYTE *prefB, const BYTE *prefF, int ref_pitch,
//			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
//			   int VPitch, int width, int height, int time256);

	YUY2Planes * DstPlanes;
    YUY2Planes * SrcPlanes;
    YUY2Planes * RefPlanes;

public:
    MVFlowFps2(PClip _child, PClip _mvbw, PClip _mvfw, PClip _mvbw2, PClip _mvfw2, unsigned int _num, unsigned int _den,
               int _maskmode, double _ml, double _mSAD, double _mSADC, PClip _pelclip, int _nIdx, int _nIdx2, int nSCD1, 
               int nSCD2, bool mmx, bool isse, IScriptEnvironment* env);
    ~MVFlowFps2();
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

    void ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
                                       int const Sharp, int const pixelType, int const nHeight, int const nWidth, 
                                       int const nPel, bool const isse);
};

#endif
