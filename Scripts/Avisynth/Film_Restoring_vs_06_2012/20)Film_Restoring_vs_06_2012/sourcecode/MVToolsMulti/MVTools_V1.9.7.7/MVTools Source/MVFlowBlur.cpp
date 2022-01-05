// Pixels flow motion blur function
// Copyright(c)2005 A.G.Balakhnin aka Fizick

// See legal notice in Copying.txt for more information

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation;  version 2 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .

#include "MVFlowBlur.h"
#include "CopyCode.h"
#include "MaskFun.h"
#include "Padding.h"

MVFlowBlur::MVFlowBlur(PClip _child, PClip _mvbw, PClip _mvfw,  int _blur256, int _prec, PClip _pelclip, int _nIdx,
                       int nSCD1, int nSCD2, bool _mmx, bool _isse, IScriptEnvironment* env) :
            GenericVideoFilter(_child),
            MVFilter(_mvfw, "MVFlowBlur", env),
            mvClipB(_mvbw, nSCD1, nSCD2, env, true),
            mvClipF(_mvfw, nSCD1, nSCD2, env, true),
            pelclip(_pelclip)
{
    blur256 = _blur256;
    prec = _prec;
    nIdx = _nIdx;
    mmx = _mmx;
    isse = _isse;

    CheckSimilarity(mvClipB, "mvbw", env);
    CheckSimilarity(mvClipF, "mvfw", env);

    mvCore->AddFrames(nIdx, 3, mvClipB.GetLevelCount(), nWidth, nHeight,
                      nPel, nHPadding, nVPadding, YUVPLANES, isse, yRatioUV);
    //     mvCore->AddFrames(nIdx, MV_BUFFER_FRAMES, mvClipF.GetLevelCount(), nWidth, nHeight,
    //                        nPel, nBlkSize, nBlkSize, YUVPLANES, isse, yRatioUV);
    usePelClipHere = false;
    if (pelclip && (nPel > 1)) {
        if (pelclip->GetVideoInfo().width != nWidth*nPel || pelclip->GetVideoInfo().height != nHeight*nPel)
            env->ThrowError("MVFlowBlur: pelclip frame size must be 2X of source!");
        else
            usePelClipHere = true;
    }

    nHeightUV = nHeight/yRatioUV;
    nWidthUV = nWidth/2;// for YV12
    nHPaddingUV = nHPadding/2;
    nVPaddingUV = nHPadding/yRatioUV;

    VPitchY = nWidth;
    VPitchUV= nWidthUV;

    VXFullYB = new BYTE [nHeight*VPitchY];
    VXFullUVB = new BYTE [nHeightUV*VPitchUV];
    VYFullYB = new BYTE [nHeight*VPitchY];
    VYFullUVB = new BYTE [nHeightUV*VPitchUV];

    VXFullYF = new BYTE [nHeight*VPitchY];
    VXFullUVF = new BYTE [nHeightUV*VPitchUV];
    VYFullYF = new BYTE [nHeight*VPitchY];
    VYFullUVF = new BYTE [nHeightUV*VPitchUV];

    VXSmallYB = new BYTE [nBlkX*nBlkY];
    VYSmallYB = new BYTE [nBlkX*nBlkY];
    VXSmallUVB = new BYTE [nBlkX*nBlkY];
    VYSmallUVB = new BYTE [nBlkX*nBlkY];

    VXSmallYF = new BYTE [nBlkX*nBlkY];
    VYSmallYF = new BYTE [nBlkX*nBlkY];
    VXSmallUVF = new BYTE [nBlkX*nBlkY];
    VYSmallUVF = new BYTE [nBlkX*nBlkY];

    MaskSmallB = new BYTE [nBlkX*nBlkY];
    MaskFullYB = new BYTE [nHeight*VPitchY];
    MaskFullUVB = new BYTE [nHeightUV*VPitchUV];

    MaskSmallF = new BYTE [nBlkX*nBlkY];
    MaskFullYF = new BYTE [nHeight*VPitchY];
    MaskFullUVF = new BYTE [nHeightUV*VPitchUV];


    int pel2WidthY = (nWidth + 2*nHPadding)*nPel;
    pel2HeightY = (nHeight + 2*nVPadding)*nPel;
    int pel2WidthUV = (nWidthUV + 2*nHPaddingUV)*nPel;
    pel2HeightUV = (nHeightUV + 2*nVPaddingUV)*nPel;
    pel2PitchY = (pel2WidthY + 15) & (~15);
    pel2PitchUV = (pel2WidthUV + 15) & (~15);
    pel2OffsetY = pel2PitchY * nVPadding*nPel + nHPadding*nPel;
    pel2OffsetUV = pel2PitchUV * nVPaddingUV*nPel + nHPaddingUV*nPel;
    if (nPel>1) {
        pel2PlaneYB = new BYTE [pel2PitchY*pel2HeightY];
        pel2PlaneUB = new BYTE [pel2PitchUV*pel2HeightUV];
        pel2PlaneVB = new BYTE [pel2PitchUV*pel2HeightUV];
    }

    int CPUF_Resize = env->GetCPUFlags();
    if (!isse) CPUF_Resize = (CPUF_Resize & !CPUF_INTEGER_SSE) & !CPUF_SSE2;

    upsizer = new SimpleResize(nWidth, nHeight, nBlkX, nBlkY, CPUF_Resize);
    upsizerUV = new SimpleResize(nWidthUV, nHeightUV, nBlkX, nBlkY, CPUF_Resize);

    DstPlanes = new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);
}

MVFlowBlur::~MVFlowBlur()
{
    if (DstPlanes) delete DstPlanes;

    if (upsizer)   delete upsizer;
    if (upsizerUV) delete upsizerUV;

    if (VXFullYB)   delete [] VXFullYB;
    if (VXFullUVB)  delete [] VXFullUVB;
    if (VYFullYB)   delete [] VYFullYB;
    if (VYFullUVB)  delete [] VYFullUVB;
    if (VXSmallYB)  delete [] VXSmallYB;
    if (VYSmallYB)  delete [] VYSmallYB;
    if (VXSmallUVB) delete [] VXSmallUVB;
    if (VYSmallUVB) delete [] VYSmallUVB;
    if (VXFullYF)   delete [] VXFullYF;
    if (VXFullUVF)  delete [] VXFullUVF;
    if (VYFullYF)   delete [] VYFullYF;
    if (VYFullUVF)  delete [] VYFullUVF;
    if (VXSmallYF)  delete [] VXSmallYF;
    if (VYSmallYF)  delete [] VYSmallYF;
    if (VXSmallUVF) delete [] VXSmallUVF;
    if (VYSmallUVF) delete [] VYSmallUVF;

    if (MaskSmallB)  delete [] MaskSmallB;
    if (MaskFullYB)  delete [] MaskFullYB;
    if (MaskFullUVB) delete [] MaskFullUVB;
    if (MaskSmallF)  delete [] MaskSmallF;
    if (MaskFullYF)  delete [] MaskFullYF;
    if (MaskFullUVF) delete [] MaskFullUVF;

    if (nPel>1) {
        if (pel2PlaneYB) delete [] pel2PlaneYB;
        if (pel2PlaneUB) delete [] pel2PlaneUB;
        if (pel2PlaneVB) delete [] pel2PlaneVB;
    }
}


void MVFlowBlur::FlowBlur(BYTE * pdst, int dst_pitch, const BYTE *pref, int ref_pitch,
                          BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF,
                          int VPitch, int width, int height, int blurmax256, int prec)
{
    // very slow, but precise motion blur
    if (nPel==1) {
        for (int h=0; h<height; h++) {
            for (int w=0; w<width; w++) {
                int bluredsum = pref[w];
                int vxF0 = ((VXFullF[w]-128)*blur256);
                int vyF0 = ((VYFullF[w]-128)*blur256);
                int mF = (std::max<int>(abs(vxF0), abs(vyF0))/prec)>>8;
                if (mF>0) {
                    vxF0 /= mF;
                    vyF0 /= mF;
                    int vxF = vxF0;
                    int vyF = vyF0;
                    for (int i=0; i<mF; i++) {
                        int dstF = pref[(vyF>>8)*ref_pitch + (vxF>>8) + w];
                        bluredsum += dstF;
                        vxF += vxF0;
                        vyF += vyF0;
                    }
                }
                int vxB0 = ((VXFullB[w]-128)*blur256);
                int vyB0 = ((VYFullB[w]-128)*blur256);
                int mB = (std::max<int>(abs(vxB0), abs(vyB0))/prec)>>8;
                if (mB>0) {
                    vxB0 /= mB;
                    vyB0 /= mB;
                    int vxB = vxB0;
                    int vyB = vyB0;
                    for (int i=0; i<mB; i++) {
                        int dstB = pref[(vyB>>8)*ref_pitch + (vxB>>8) + w];
                        bluredsum += dstB;
                        vxB += vxB0;
                        vyB += vyB0;
                    }
                }
                pdst[w] = bluredsum/(mF+mB+1);
            }
            pdst += dst_pitch;
            pref += ref_pitch;
            VXFullB += VPitch;
            VYFullB += VPitch;
            VXFullF += VPitch;
            VYFullF += VPitch;
        }
    }
    else if (nPel==2) {
        for (int h=0; h<height; h++) {
            for (int w=0; w<width; w++) {
                int bluredsum = pref[w<<1];
                int vxF0 = ((VXFullF[w]-128)*blur256);
                int vyF0 = ((VYFullF[w]-128)*blur256);
                int mF = (std::max<int>(abs(vxF0), abs(vyF0))/prec)>>8;
                if (mF>0) {
                    vxF0 /= mF;
                    vyF0 /= mF;
                    int vxF = vxF0;
                    int vyF = vyF0;
                    for (int i=0; i<mF; i++) {
                        int dstF = pref[(vyF>>8)*ref_pitch + (vxF>>8) + (w<<1)];
                        bluredsum += dstF;
                        vxF += vxF0;
                        vyF += vyF0;
                    }
                }
                int vxB0 = ((VXFullB[w]-128)*blur256);
                int vyB0 = ((VYFullB[w]-128)*blur256);
                int mB = (std::max<int>(abs(vxB0), abs(vyB0))/prec)>>8;
                if (mB>0) {
                    vxB0 /= mB;
                    vyB0 /= mB;
                    int vxB = vxB0;
                    int vyB = vyB0;
                    for (int i=0; i<mB; i++) {
                        int dstB = pref[(vyB>>8)*ref_pitch + (vxB>>8) + (w<<1)];
                        bluredsum += dstB;
                        vxB += vxB0;
                        vyB += vyB0;
                    }
                }
                pdst[w] = bluredsum/(mF+mB+1);
            }
            pdst += dst_pitch;
            pref += (ref_pitch<<1);
            VXFullB += VPitch;
            VYFullB += VPitch;
            VXFullF += VPitch;
            VYFullF += VPitch;
        }
    }
    else if (nPel==4) {
        for (int h=0; h<height; h++) {
            for (int w=0; w<width; w++) {
                int bluredsum = pref[w<<2];
                int vxF0 = ((VXFullF[w]-128)*blur256);
                int vyF0 = ((VYFullF[w]-128)*blur256);
                int mF = (std::max<int>(abs(vxF0), abs(vyF0))/prec)>>8;
                if (mF>0) {
                    vxF0 /= mF;
                    vyF0 /= mF;
                    int vxF = vxF0;
                    int vyF = vyF0;
                    for (int i=0; i<mF; i++) {
                        int dstF = pref[(vyF>>8)*ref_pitch + (vxF>>8) + (w<<2)];
                        bluredsum += dstF;
                        vxF += vxF0;
                        vyF += vyF0;
                    }
                }
                int vxB0 = ((VXFullB[w]-128)*blur256);
                int vyB0 = ((VYFullB[w]-128)*blur256);
                int mB = (std::max<int>(abs(vxB0), abs(vyB0))/prec)>>8;
                if (mB>0) {
                    vxB0 /= mB;
                    vyB0 /= mB;
                    int vxB = vxB0;
                    int vyB = vyB0;
                    for (int i=0; i<mB; i++) {
                        int dstB = pref[(vyB>>8)*ref_pitch + (vxB>>8) + (w<<2)];
                        bluredsum += dstB;
                        vxB += vxB0;
                        vyB += vyB0;
                    }
                }
                pdst[w] = bluredsum/(mF+mB+1);
            }
            pdst += dst_pitch;
            pref += (ref_pitch<<2);
            VXFullB += VPitch;
            VYFullB += VPitch;
            VXFullF += VPitch;
            VYFullF += VPitch;
        }
    }
}
//-------------------------------------------------------------------------
PVideoFrame __stdcall MVFlowBlur::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src	= child->GetFrame(n, env);

    int off = mvClipB.GetDeltaFrame(); // integer offset of reference frame
    if (!mvClipB.IsBackward())
        off = - off;

    PVideoFrame mvB = mvClipB.GetFrame(n-off, env);
    mvClipB.Update(mvB, env);// backward from current to prev
    PVideoFrame mvF = mvClipF.GetFrame(n+off, env);
    mvClipF.Update(mvF, env);// forward from current to next

    int sharp = mvClipB.GetSharp();

    if ( mvClipB.IsUsable()  && mvClipF.IsUsable() ) {
        // get reference frames
        PMVGroupOfFrames pRefGOF = mvCore->GetFrame(nIdx, n);
        if (!pRefGOF->IsProcessed()) {
            PVideoFrame ref, ref2x;
            ref = child->GetFrame(n, env);
            pRefGOF->SetParity(child->GetParity(n));
            if (usePelClipHere)
                ref2x= pelclip->GetFrame(n, env);

            ProcessFrameIntoGroupOfFrames(&pRefGOF, &ref, &ref2x, sharp, pixelType, nHeight, nWidth, nPel, isse);
        }

        PVideoFrame dst = env->NewVideoFrame(vi);
        // convert to frames
        unsigned char *pDst[3];
        int nDstPitches[3];
        DstPlanes->ConvertVideoFrameToPlanes(&dst, pDst, nDstPitches);

        MVPlane *pPlanesB[3];

        pPlanesB[0] = pRefGOF->GetFrame(0)->GetPlane(YPLANE);
        pPlanesB[1] = pRefGOF->GetFrame(0)->GetPlane(UPLANE);
        pPlanesB[2] = pRefGOF->GetFrame(0)->GetPlane(VPLANE);

        if (nPel>=2)
        {
            if (usePelClipHere) { // simply padding 2x planes
                PlaneCopy(pel2PlaneYB + nHPadding*nPel + nVPadding*nPel * pel2PitchY, pel2PitchY,
                          pRefGOF->GetVFYPtr(), pRefGOF->GetVFYPitch(), nWidth*nPel, nHeight*nPel, isse);
                Padding::PadReferenceFrame(pel2PlaneYB, pel2PitchY, nHPadding*nPel, nVPadding*nPel, nWidth*nPel, 
                                           nHeight*nPel);
                PlaneCopy(pel2PlaneUB + nHPaddingUV*nPel + nVPaddingUV*nPel * pel2PitchUV, pel2PitchUV,
                          pRefGOF->GetVFUPtr(), pRefGOF->GetVFUPitch(), nWidthUV*nPel, nHeightUV*nPel, isse);
                Padding::PadReferenceFrame(pel2PlaneUB, pel2PitchUV, nHPaddingUV*nPel, nVPaddingUV*nPel, nWidthUV*nPel, 
                                           nHeightUV*nPel);
                PlaneCopy(pel2PlaneVB + nHPaddingUV*nPel + nVPaddingUV*nPel * pel2PitchUV, pel2PitchUV,
                          pRefGOF->GetVFVPtr(), pRefGOF->GetVFVPitch(), nWidthUV*nPel, nHeightUV*nPel, isse);
                Padding::PadReferenceFrame(pel2PlaneVB, pel2PitchUV, nHPaddingUV*nPel, nVPaddingUV*nPel, nWidthUV*nPel, 
                                           nHeightUV*nPel);
            }
            else if (nPel==2) {
                // merge refined planes to big single plane
                Merge4PlanesToBig(pel2PlaneYB, pel2PitchY, pPlanesB[0]->GetAbsolutePointer(0,0),
                                  pPlanesB[0]->GetAbsolutePointer(1,0), pPlanesB[0]->GetAbsolutePointer(0,1),
                                  pPlanesB[0]->GetAbsolutePointer(1,1), pPlanesB[0]->GetExtendedWidth(),
                                  pPlanesB[0]->GetExtendedHeight(), pPlanesB[0]->GetPitch(), isse);
                Merge4PlanesToBig(pel2PlaneUB, pel2PitchUV, pPlanesB[1]->GetAbsolutePointer(0,0),
                                  pPlanesB[1]->GetAbsolutePointer(1,0), pPlanesB[1]->GetAbsolutePointer(0,1),
                                  pPlanesB[1]->GetAbsolutePointer(1,1), pPlanesB[1]->GetExtendedWidth(),
                                  pPlanesB[1]->GetExtendedHeight(), pPlanesB[1]->GetPitch(), isse);
                Merge4PlanesToBig(pel2PlaneVB, pel2PitchUV, pPlanesB[2]->GetAbsolutePointer(0,0),
                                  pPlanesB[2]->GetAbsolutePointer(1,0), pPlanesB[2]->GetAbsolutePointer(0,1),
                                  pPlanesB[2]->GetAbsolutePointer(1,1), pPlanesB[2]->GetExtendedWidth(),
                                  pPlanesB[2]->GetExtendedHeight(), pPlanesB[2]->GetPitch(), isse);
            }
            else if (nPel==4) {
                // merge refined planes to big single plane
                Merge16PlanesToBig(pel2PlaneYB, pel2PitchY,
                                   pPlanesB[0]->GetAbsolutePointer(0,0), pPlanesB[0]->GetAbsolutePointer(1,0),
                                   pPlanesB[0]->GetAbsolutePointer(2,0), pPlanesB[0]->GetAbsolutePointer(3,0),
                                   pPlanesB[0]->GetAbsolutePointer(1,0), pPlanesB[0]->GetAbsolutePointer(1,1),
                                   pPlanesB[0]->GetAbsolutePointer(1,2), pPlanesB[0]->GetAbsolutePointer(1,3),
                                   pPlanesB[0]->GetAbsolutePointer(2,0), pPlanesB[0]->GetAbsolutePointer(2,1),
                                   pPlanesB[0]->GetAbsolutePointer(2,2), pPlanesB[0]->GetAbsolutePointer(2,3),
                                   pPlanesB[0]->GetAbsolutePointer(3,0), pPlanesB[0]->GetAbsolutePointer(3,1),
                                   pPlanesB[0]->GetAbsolutePointer(3,2), pPlanesB[0]->GetAbsolutePointer(3,3),
                                   pPlanesB[0]->GetExtendedWidth(), pPlanesB[0]->GetExtendedHeight(), pPlanesB[0]->GetPitch(), isse);
                Merge16PlanesToBig(pel2PlaneUB, pel2PitchUV,
                                   pPlanesB[1]->GetAbsolutePointer(0,0), pPlanesB[1]->GetAbsolutePointer(1,0),
                                   pPlanesB[1]->GetAbsolutePointer(2,0), pPlanesB[1]->GetAbsolutePointer(3,0),
                                   pPlanesB[1]->GetAbsolutePointer(1,0), pPlanesB[1]->GetAbsolutePointer(1,1),
                                   pPlanesB[1]->GetAbsolutePointer(1,2), pPlanesB[1]->GetAbsolutePointer(1,3),
                                   pPlanesB[1]->GetAbsolutePointer(2,0), pPlanesB[1]->GetAbsolutePointer(2,1),
                                   pPlanesB[1]->GetAbsolutePointer(2,2), pPlanesB[1]->GetAbsolutePointer(2,3),
                                   pPlanesB[1]->GetAbsolutePointer(3,0), pPlanesB[1]->GetAbsolutePointer(3,1),
                                   pPlanesB[1]->GetAbsolutePointer(3,2), pPlanesB[1]->GetAbsolutePointer(3,3),
                                   pPlanesB[1]->GetExtendedWidth(), pPlanesB[1]->GetExtendedHeight(), pPlanesB[1]->GetPitch(), isse);
                Merge16PlanesToBig(pel2PlaneVB, pel2PitchUV,
                                   pPlanesB[2]->GetAbsolutePointer(0,0), pPlanesB[2]->GetAbsolutePointer(1,0),
                                   pPlanesB[2]->GetAbsolutePointer(2,0), pPlanesB[2]->GetAbsolutePointer(3,0),
                                   pPlanesB[2]->GetAbsolutePointer(1,0), pPlanesB[2]->GetAbsolutePointer(1,1),
                                   pPlanesB[2]->GetAbsolutePointer(1,2), pPlanesB[2]->GetAbsolutePointer(1,3),
                                   pPlanesB[2]->GetAbsolutePointer(2,0), pPlanesB[2]->GetAbsolutePointer(2,1),
                                   pPlanesB[2]->GetAbsolutePointer(2,2), pPlanesB[2]->GetAbsolutePointer(2,3),
                                   pPlanesB[2]->GetAbsolutePointer(3,0), pPlanesB[2]->GetAbsolutePointer(3,1),
                                   pPlanesB[2]->GetAbsolutePointer(3,2), pPlanesB[2]->GetAbsolutePointer(3,3),
                                   pPlanesB[2]->GetExtendedWidth(), pPlanesB[2]->GetExtendedHeight(), pPlanesB[2]->GetPitch(), isse);
            }
        }


        // make  vector vx and vy small masks
        // 1. ATTENTION: vectors are assumed SHORT (|vx|, |vy| < 127) !
        // 2. they will be zeroed if not
        // 3. added 128 to all values
        MakeVectorSmallMasks(mvClipB, nBlkX, nBlkY, VXSmallYB, nBlkX, VYSmallYB, nBlkX);
        VectorSmallMaskYToHalfUV(VXSmallYB, nBlkX, nBlkY, VXSmallUVB, 2);
        VectorSmallMaskYToHalfUV(VYSmallYB, nBlkX, nBlkY, VYSmallUVB, yRatioUV);

        MakeVectorSmallMasks(mvClipF, nBlkX, nBlkY, VXSmallYF, nBlkX, VYSmallYF, nBlkX);
        VectorSmallMaskYToHalfUV(VXSmallYF, nBlkX, nBlkY, VXSmallUVF, 2);
        VectorSmallMaskYToHalfUV(VYSmallYF, nBlkX, nBlkY, VYSmallUVF, yRatioUV);

        // analyse vectors field to detect occlusion

        // upsize (bilinear interpolate) vector masks to fullframe size


        int dummyplane = PLANAR_Y; // use luma plane resizer code for all planes if we resize from luma small mask
        upsizer->SimpleResizeDo(VXFullYB, nWidth, nHeight, VPitchY, VXSmallYB, nBlkX, nBlkX, dummyplane);
        upsizer->SimpleResizeDo(VYFullYB, nWidth, nHeight, VPitchY, VYSmallYB, nBlkX, nBlkX, dummyplane);
        upsizerUV->SimpleResizeDo(VXFullUVB, nWidthUV, nHeightUV, VPitchUV, VXSmallUVB, nBlkX, nBlkX, dummyplane);
        upsizerUV->SimpleResizeDo(VYFullUVB, nWidthUV, nHeightUV, VPitchUV, VYSmallUVB, nBlkX, nBlkX, dummyplane);

        upsizer->SimpleResizeDo(VXFullYF, nWidth, nHeight, VPitchY, VXSmallYF, nBlkX, nBlkX, dummyplane);
        upsizer->SimpleResizeDo(VYFullYF, nWidth, nHeight, VPitchY, VYSmallYF, nBlkX, nBlkX, dummyplane);
        upsizerUV->SimpleResizeDo(VXFullUVF, nWidthUV, nHeightUV, VPitchUV, VXSmallUVF, nBlkX, nBlkX, dummyplane);
        upsizerUV->SimpleResizeDo(VYFullUVF, nWidthUV, nHeightUV, VPitchUV, VYSmallUVF, nBlkX, nBlkX, dummyplane);


        if (nPel>=2) {
            FlowBlur(pDst[0], nDstPitches[0], pel2PlaneYB + pel2OffsetY, pel2PitchY,
                     VXFullYB, VXFullYF, VYFullYB, VYFullYF, VPitchY,
                     nWidth, nHeight, blur256, prec);
            FlowBlur(pDst[1], nDstPitches[1], pel2PlaneUB + pel2OffsetUV, pel2PitchUV,
                     VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, VPitchUV,
                     nWidthUV, nHeightUV, blur256, prec);
            FlowBlur(pDst[2], nDstPitches[2], pel2PlaneVB + pel2OffsetUV, pel2PitchUV,
                     VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, VPitchUV,
                     nWidthUV, nHeightUV, blur256, prec);

        }
        else if (nPel==1) {
            FlowBlur(pDst[0], nDstPitches[0], pPlanesB[0]->GetPointer(0,0), pPlanesB[0]->GetPitch(),
                     VXFullYB, VXFullYF, VYFullYB, VYFullYF, VPitchY,
                     nWidth, nHeight, blur256, prec);
            FlowBlur(pDst[1], nDstPitches[1], pPlanesB[1]->GetPointer(0,0), pPlanesB[1]->GetPitch(),
                     VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, VPitchUV,
                     nWidthUV, nHeightUV, blur256, prec);
            FlowBlur(pDst[2], nDstPitches[2], pPlanesB[2]->GetPointer(0,0), pPlanesB[2]->GetPitch(),
                     VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, VPitchUV,
                     nWidthUV, nHeightUV, blur256, prec);
        }

        // convert back from planes
        unsigned char *pDstYUY2 = dst->GetWritePtr();
        int nDstPitchYUY2 = dst->GetPitch();
        DstPlanes->YUY2FromPlanes(pDstYUY2, nDstPitchYUY2);

        return dst;
    }
    else
    {
        return src;
    }
}

void MVFlowBlur::ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
                                               int const Sharp, int const pixelType, int const nHeight, int const nWidth, 
                                               int const nPel, bool const isse)
{
    (*srcGOF)->ConvertVideoFrame(src, src2x, pixelType, nWidth, nHeight, nPel, isse);

    PROFILE_START(MOTION_PROFILE_INTERPOLATION);

    if (*src2x) {
        // do nothing
    }
    else {
        (*srcGOF)->Pad(YUVPLANES);
        (*srcGOF)->Refine(YUVPLANES, Sharp);
    }

    PROFILE_STOP(MOTION_PROFILE_INTERPOLATION);

    // set processed
    (*srcGOF)->SetProcessed();
}