#ifndef __MV_FLOWBLUR__
#define __MV_FLOWBLUR__

#include "MVInterface.h"
#include "SimpleResize.h"
#include "yuy2planes.h"

class MVFlowBlur : public GenericVideoFilter, public MVFilter {
private:

    MVClip mvClipB;
    MVClip mvClipF;
    int blur256; // blur time interval
    int prec; // blur precision (pixels)
    PClip pelclip;
    bool isse;
    bool mmx;

    bool usePelClipHere;

    // fullframe vector mask
    BYTE *VXFullYB; //backward
    BYTE *VXFullUVB;
    BYTE *VYFullYB;
    BYTE *VYFullUVB;
    BYTE *VXFullYF;
    BYTE *VXFullUVF;
    BYTE *VYFullYF;
    BYTE *VYFullUVF;

    // Small vector mask
    BYTE *VXSmallYB;
    BYTE *VXSmallUVB;
    BYTE *VYSmallYB;
    BYTE *VYSmallUVB;
    BYTE *VXSmallYF;
    BYTE *VXSmallUVF;
    BYTE *VYSmallYF;
    BYTE *VYSmallUVF;

    BYTE *MaskSmallB;
    BYTE *MaskFullYB;
    BYTE *MaskFullUVB;
    BYTE *MaskSmallF;
    BYTE *MaskFullYF;
    BYTE *MaskFullUVF;

    int nWidthUV;
    int nHeightUV;
    int	VPitchY, VPitchUV;

    int nHPaddingUV;
    int nVPaddingUV;

    int pel2PitchY, pel2HeightY, pel2PitchUV, pel2HeightUV, pel2OffsetY, pel2OffsetUV;

    BYTE * pel2PlaneYB; // big plane
    BYTE * pel2PlaneUB;
    BYTE * pel2PlaneVB;
    BYTE * pel2PlaneYF; // big plane
    BYTE * pel2PlaneUF;
    BYTE * pel2PlaneVF;

    SimpleResize *upsizer;
    SimpleResize *upsizerUV;
    void FlowBlur(BYTE * pdst, int dst_pitch, const BYTE *prefB, int ref_pitch,
                  BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF,
                  int VPitch, int width, int height, int time256, int nb);

    YUY2Planes * RefPlanes;
    YUY2Planes * DstPlanes;

    YUY2Planes * Ref2xPlanes;

public:
    MVFlowBlur(PClip _child, PClip _mvbw, PClip _mvfw, int _blur256, int _prec, PClip _pelclip, int _nIdx,
               int nSCD1, int nSCD2, bool mmx, bool isse, IScriptEnvironment* env);
    ~MVFlowBlur();
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
    void ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
                                       int const Sharp, int const pixelType, int const nHeight, int const nWidth, 
                                       int const nPel, bool const isse);
};

#endif
