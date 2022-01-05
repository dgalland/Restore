// Pixels flow motion interplolation function
// Copyright(c)2005-2006 A.G.Balakhnin aka Fizick

// See legal notice in Copying.txt for more information

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
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

#include "MVFlowInter.h"
#include "CopyCode.h"
#include "MaskFun.h"
#include "Padding.h"
#include "CopyCode.h"

#include <algorithm>

MVFlowInter::MVFlowInter(PClip _child, PClip _mvbw, PClip _mvfw,  int _time256, double _ml, double _mSAD, 
                         double _mSADC, PClip _pelclip, int _nIdx, int nSCD1, int nSCD2, bool _mmx, bool _isse, 
                         IScriptEnvironment* env) :
             GenericVideoFilter(_child),
             MVFilter(_mvfw, "MVFlowInter", env),
             mvClipB(_mvbw, nSCD1, nSCD2, env, true),
             mvClipF(_mvfw, nSCD1, nSCD2, env, true),
             pelclip(_pelclip)
{
    time256 = _time256;
    ml = _ml;
    mSAD  = _mSAD;
    mSADC = _mSADC;
    nIdx = _nIdx;
    mmx = _mmx;
    isse = _isse;

    normSAD1024  = unsigned int( 1024 * 255 / (mSAD * nBlkSizeX*nBlkSizeY) ); //normalize
    normSADC1024 = unsigned int( 1024 * 255 / (mSADC * nBlkSizeX*nBlkSizeY) ); //normalize

    if (!mvClipB.IsBackward())
        env->ThrowError("MVFlowInter: wrong backward vectors");
    if (mvClipF.IsBackward())
        env->ThrowError("MVFlowInter: wrong forward vectors");

    CheckSimilarity(mvClipB, "mvbw", env);
    CheckSimilarity(mvClipF, "mvfw", env);

    mvCore->AddFrames(nIdx, 3, mvClipB.GetLevelCount(), nWidth, nHeight,
                      nPel, nHPadding, nVPadding, YUVPLANES, isse, yRatioUV);

    usePelClipHere = false;
    if (pelclip && (nPel > 1)) {
        if (pelclip->GetVideoInfo().width != nWidth*nPel || pelclip->GetVideoInfo().height != nHeight*nPel)
            env->ThrowError("MVFlowInter: pelclip frame size must be Pel of source!");
        else
            usePelClipHere = true;
    }
    // may be padded for full frame cover
    nBlkXP = (nBlkX*(nBlkSizeX - nOverlapX) + nOverlapX < nWidth) ? nBlkX+1 : nBlkX;
    nBlkYP = (nBlkY*(nBlkSizeY - nOverlapY) + nOverlapY < nHeight) ? nBlkY+1 : nBlkY;
    nWidthP = nBlkXP*(nBlkSizeX - nOverlapX) + nOverlapX;
    nHeightP = nBlkYP*(nBlkSizeY - nOverlapY) + nOverlapY;
    // for YV12
    nWidthPUV = nWidthP/2;
    nHeightPUV = nHeightP/yRatioUV;
    nHeightUV = nHeight/yRatioUV;
    nWidthUV = nWidth/2;

    nHPaddingUV = nHPadding/2;
    nVPaddingUV = nVPadding/yRatioUV;

    VPitchY = (nWidthP + 15) & (~15);
    VPitchUV = (nWidthPUV + 15) & (~15);

    VXFullYB = new BYTE [nHeightP*VPitchY];
    VXFullUVB = new BYTE [nHeightPUV*VPitchUV];
    VYFullYB = new BYTE [nHeightP*VPitchY];
    VYFullUVB = new BYTE [nHeightPUV*VPitchUV];

    VXFullYF = new BYTE [nHeightP*VPitchY];
    VXFullUVF = new BYTE [nHeightPUV*VPitchUV];
    VYFullYF = new BYTE [nHeightP*VPitchY];
    VYFullUVF = new BYTE [nHeightPUV*VPitchUV];

    VXSmallYB = new BYTE [nBlkXP*nBlkYP];
    VYSmallYB = new BYTE [nBlkXP*nBlkYP];
    VXSmallUVB = new BYTE [nBlkXP*nBlkYP];
    VYSmallUVB = new BYTE [nBlkXP*nBlkYP];

    VXSmallYF = new BYTE [nBlkXP*nBlkYP];
    VYSmallYF = new BYTE [nBlkXP*nBlkYP];
    VXSmallUVF = new BYTE [nBlkXP*nBlkYP];
    VYSmallUVF = new BYTE [nBlkXP*nBlkYP];

    VXFullYBB = new BYTE [nHeightP*VPitchY];
    VXFullUVBB = new BYTE [nHeightPUV*VPitchUV];
    VYFullYBB = new BYTE [nHeightP*VPitchY];
    VYFullUVBB = new BYTE [nHeightPUV*VPitchUV];

    VXFullYFF = new BYTE [nHeightP*VPitchY];
    VXFullUVFF = new BYTE [nHeightPUV*VPitchUV];
    VYFullYFF = new BYTE [nHeightP*VPitchY];
    VYFullUVFF = new BYTE [nHeightPUV*VPitchUV];

    VXSmallYBB = new BYTE [nBlkXP*nBlkYP];
    VYSmallYBB = new BYTE [nBlkXP*nBlkYP];
    VXSmallUVBB = new BYTE [nBlkXP*nBlkYP];
    VYSmallUVBB = new BYTE [nBlkXP*nBlkYP];

    VXSmallYFF = new BYTE [nBlkXP*nBlkYP];
    VYSmallYFF = new BYTE [nBlkXP*nBlkYP];
    VXSmallUVFF = new BYTE [nBlkXP*nBlkYP];
    VYSmallUVFF = new BYTE [nBlkXP*nBlkYP];

    MaskSmallB = new BYTE [nBlkXP*nBlkYP];
    MaskFullYB = new BYTE [nHeightP*VPitchY];
    MaskFullUVB = new BYTE [nHeightPUV*VPitchUV];

    MaskSmallF = new BYTE [nBlkXP*nBlkYP];
    MaskFullYF = new BYTE [nHeightP*VPitchY];
    MaskFullUVF = new BYTE [nHeightPUV*VPitchUV];

    SADMaskSmallB = new BYTE [nBlkXP*nBlkYP];
    SADMaskSmallF = new BYTE [nBlkXP*nBlkYP];

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
        pel2PlaneYF = new BYTE [pel2PitchY*pel2HeightY];
        pel2PlaneUF = new BYTE [pel2PitchUV*pel2HeightUV];
        pel2PlaneVF = new BYTE [pel2PitchUV*pel2HeightUV];
    }

    int CPUF_Resize = env->GetCPUFlags();
    if (!isse) CPUF_Resize = (CPUF_Resize & !CPUF_INTEGER_SSE) & !CPUF_SSE2;

    upsizer = new SimpleResize(nWidthP, nHeightP, nBlkXP, nBlkYP, CPUF_Resize);
    upsizerUV = new SimpleResize(nWidthPUV, nHeightPUV, nBlkXP, nBlkYP, CPUF_Resize);

    Create_LUTV(time256, LUTVB, LUTVF);

    SrcPlanes =  new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);
    RefPlanes =  new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);
    DstPlanes =  new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);
}

MVFlowInter::~MVFlowInter()
{
    if (SrcPlanes) delete SrcPlanes;
    if (RefPlanes) delete RefPlanes;
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

    if (VXFullYBB)   delete [] VXFullYBB;
    if (VXFullUVBB)  delete [] VXFullUVBB;
    if (VYFullYBB)   delete [] VYFullYBB;
    if (VYFullUVBB)  delete [] VYFullUVBB;
    if (VXSmallYBB)  delete [] VXSmallYBB;
    if (VYSmallYBB)  delete [] VYSmallYBB;
    if (VXSmallUVBB) delete [] VXSmallUVBB;
    if (VYSmallUVBB) delete [] VYSmallUVBB;
    if (VXFullYFF)   delete [] VXFullYFF;
    if (VXFullUVFF)  delete [] VXFullUVFF;
    if (VYFullYFF)   delete [] VYFullYFF;
    if (VYFullUVFF)  delete [] VYFullUVFF;
    if (VXSmallYFF)  delete [] VXSmallYFF;
    if (VYSmallYFF)  delete [] VYSmallYFF;
    if (VXSmallUVFF) delete [] VXSmallUVFF;
    if (VYSmallUVFF) delete [] VYSmallUVFF;

    if (MaskSmallB)  delete [] MaskSmallB;
    if (MaskFullYB)  delete [] MaskFullYB;
    if (MaskFullUVB) delete [] MaskFullUVB;
    if (MaskSmallF)  delete [] MaskSmallF;
    if (MaskFullYF)  delete [] MaskFullYF;
    if (MaskFullUVF) delete [] MaskFullUVF;

    if (SADMaskSmallB) delete [] SADMaskSmallB;
    if (SADMaskSmallF) delete [] SADMaskSmallF;
    if (nPel>1) {
        if (pel2PlaneYB) delete [] pel2PlaneYB;
        if (pel2PlaneUB) delete [] pel2PlaneUB;
        if (pel2PlaneVB) delete [] pel2PlaneVB;
        if (pel2PlaneYF) delete [] pel2PlaneYF;
        if (pel2PlaneUF) delete [] pel2PlaneUF;
        if (pel2PlaneVF) delete [] pel2PlaneVF;
    }
}

//-------------------------------------------------------------------------
PVideoFrame __stdcall MVFlowInter::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src	= child->GetFrame(n, env);
    PVideoFrame dst = env->NewVideoFrame(vi);

    // convert to frames
    unsigned char *pDst[3];
    int nDstPitches[3];
    DstPlanes->ConvertVideoFrameToPlanes(&dst, pDst, nDstPitches);

    int off = mvClipB.GetDeltaFrame(); // integer offset of reference frame

    // get reference frame
    PVideoFrame ref = child->GetFrame(n+off, env);//  ref for  compensation

    PVideoFrame mvB = mvClipB.GetFrame(n, env);
    mvClipB.Update(mvB, env);// backward from next to current
    PVideoFrame mvF = mvClipF.GetFrame(n+off, env);
    mvClipF.Update(mvF, env);// forward from current to next

    if ( mvClipB.IsUsable()  && mvClipF.IsUsable()) {
        PMVGroupOfFrames pRefGOFF = mvCore->GetFrame(nIdx, n); // forward ref
        PMVGroupOfFrames pRefGOFB = mvCore->GetFrame(nIdx, n + off); // backward ref

        if (!pRefGOFF->IsProcessed()) {
            PVideoFrame src2x;
            pRefGOFF->SetParity(child->GetParity(n));
            if (usePelClipHere) {
                src2x = pelclip->GetFrame(n, env);
            }
            ProcessFrameIntoGroupOfFrames(&pRefGOFF, &src, &src2x, mvClipF.GetSharp(), pixelType, 
                                          nHeight, nWidth, nPel, isse);
        }

        if (!pRefGOFB->IsProcessed()) {
            PVideoFrame ref2x;
            pRefGOFB->SetParity(child->GetParity(n + off));
            if (usePelClipHere) {
                ref2x = pelclip->GetFrame(n+off, env);
            }
            ProcessFrameIntoGroupOfFrames(&pRefGOFB, &ref, &ref2x, mvClipB.GetSharp(), pixelType, 
                                          nHeight, nWidth, nPel, isse);
        }

        MVPlane *pPlanesB[3];
        MVPlane *pPlanesF[3];

        pPlanesB[0] = pRefGOFB->GetFrame(0)->GetPlane(YPLANE);
        pPlanesB[1] = pRefGOFB->GetFrame(0)->GetPlane(UPLANE);
        pPlanesB[2] = pRefGOFB->GetFrame(0)->GetPlane(VPLANE);

        pPlanesF[0] = pRefGOFF->GetFrame(0)->GetPlane(YPLANE);
        pPlanesF[1] = pRefGOFF->GetFrame(0)->GetPlane(UPLANE);
        pPlanesF[2] = pRefGOFF->GetFrame(0)->GetPlane(VPLANE);

        if (nPel>=2) {
            if (usePelClipHere) { // simply padding 2x planes
                PlaneCopy(pel2PlaneYB + nHPadding*nPel + nVPadding*nPel * pel2PitchY, pel2PitchY,
                          pRefGOFB->GetVF2xYPtr(), pRefGOFB->GetVF2xYPitch(), nWidth*nPel, nHeight*nPel, isse);
                Padding::PadReferenceFrame(pel2PlaneYB, pel2PitchY, nHPadding*nPel, nVPadding*nPel, nWidth*nPel, 
                                           nHeight*nPel);
                PlaneCopy(pel2PlaneUB + nHPaddingUV*nPel + nVPaddingUV*nPel * pel2PitchUV, pel2PitchUV,
                          pRefGOFB->GetVF2xUPtr(), pRefGOFB->GetVF2xUPitch(), nWidthUV*nPel, nHeightUV*nPel, isse);
                Padding::PadReferenceFrame(pel2PlaneUB, pel2PitchUV, nHPaddingUV*nPel, nVPaddingUV*nPel, nWidthUV*nPel, 
                                           nHeightUV*nPel);
                PlaneCopy(pel2PlaneVB + nHPaddingUV*nPel + nVPaddingUV*nPel * pel2PitchUV, pel2PitchUV,
                          pRefGOFB->GetVF2xVPtr(), pRefGOFB->GetVF2xVPitch(), nWidthUV*nPel, nHeightUV*nPel, isse);
                Padding::PadReferenceFrame(pel2PlaneVB, pel2PitchUV, nHPaddingUV*nPel, nVPaddingUV*nPel, nWidthUV*nPel, 
                                           nHeightUV*nPel);
                PlaneCopy(pel2PlaneYF + nHPadding*nPel + nVPadding*nPel * pel2PitchY, pel2PitchY,
                          pRefGOFF->GetVF2xYPtr(), pRefGOFF->GetVF2xYPitch(), nWidth*nPel, nHeight*nPel, isse);
                Padding::PadReferenceFrame(pel2PlaneYB, pel2PitchY, nHPadding*nPel, nVPadding*nPel, nWidth*nPel, 
                                           nHeight*nPel);
                PlaneCopy(pel2PlaneUF + nHPaddingUV*nPel + nVPaddingUV*nPel * pel2PitchUV, pel2PitchUV,
                          pRefGOFF->GetVF2xUPtr(), pRefGOFF->GetVF2xUPitch(), nWidthUV*nPel, nHeightUV*nPel, isse);
                Padding::PadReferenceFrame(pel2PlaneUB, pel2PitchUV, nHPaddingUV*nPel, nVPaddingUV*nPel, nWidthUV*nPel, 
                                           nHeightUV*nPel);
                PlaneCopy(pel2PlaneVF + nHPaddingUV*nPel + nVPaddingUV*nPel * pel2PitchUV, pel2PitchUV,
                          pRefGOFF->GetVF2xVPtr(), pRefGOFF->GetVF2xVPitch(), nWidthUV*nPel, nHeightUV*nPel, isse);
                Padding::PadReferenceFrame(pel2PlaneVF, pel2PitchUV, nHPaddingUV*nPel, nVPaddingUV*nPel, nWidthUV*nPel, 
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
                Merge4PlanesToBig(pel2PlaneYF, pel2PitchY, pPlanesF[0]->GetAbsolutePointer(0,0),
                                  pPlanesF[0]->GetAbsolutePointer(1,0), pPlanesF[0]->GetAbsolutePointer(0,1),
                                  pPlanesF[0]->GetAbsolutePointer(1,1), pPlanesF[0]->GetExtendedWidth(),
                                  pPlanesF[0]->GetExtendedHeight(), pPlanesF[0]->GetPitch(), isse);
                Merge4PlanesToBig(pel2PlaneUF, pel2PitchUV, pPlanesF[1]->GetAbsolutePointer(0,0),
                                  pPlanesF[1]->GetAbsolutePointer(1,0), pPlanesF[1]->GetAbsolutePointer(0,1),
                                  pPlanesF[1]->GetAbsolutePointer(1,1), pPlanesF[1]->GetExtendedWidth(),
                                  pPlanesF[1]->GetExtendedHeight(), pPlanesF[1]->GetPitch(), isse);
                Merge4PlanesToBig(pel2PlaneVF, pel2PitchUV, pPlanesF[2]->GetAbsolutePointer(0,0),
                                  pPlanesF[2]->GetAbsolutePointer(1,0), pPlanesF[2]->GetAbsolutePointer(0,1),
                                  pPlanesF[2]->GetAbsolutePointer(1,1), pPlanesF[2]->GetExtendedWidth(),
                                  pPlanesF[2]->GetExtendedHeight(), pPlanesF[2]->GetPitch(), isse);
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
                                   pPlanesB[0]->GetExtendedWidth(), pPlanesB[0]->GetExtendedHeight(), 
                                   pPlanesB[0]->GetPitch(), isse);
                Merge16PlanesToBig(pel2PlaneUB, pel2PitchUV,
                                   pPlanesB[1]->GetAbsolutePointer(0,0), pPlanesB[1]->GetAbsolutePointer(1,0),
                                   pPlanesB[1]->GetAbsolutePointer(2,0), pPlanesB[1]->GetAbsolutePointer(3,0),
                                   pPlanesB[1]->GetAbsolutePointer(1,0), pPlanesB[1]->GetAbsolutePointer(1,1),
                                   pPlanesB[1]->GetAbsolutePointer(1,2), pPlanesB[1]->GetAbsolutePointer(1,3),
                                   pPlanesB[1]->GetAbsolutePointer(2,0), pPlanesB[1]->GetAbsolutePointer(2,1),
                                   pPlanesB[1]->GetAbsolutePointer(2,2), pPlanesB[1]->GetAbsolutePointer(2,3),
                                   pPlanesB[1]->GetAbsolutePointer(3,0), pPlanesB[1]->GetAbsolutePointer(3,1),
                                   pPlanesB[1]->GetAbsolutePointer(3,2), pPlanesB[1]->GetAbsolutePointer(3,3),
                                   pPlanesB[1]->GetExtendedWidth(), pPlanesB[1]->GetExtendedHeight(), 
                                   pPlanesB[1]->GetPitch(), isse);
                Merge16PlanesToBig(pel2PlaneVB, pel2PitchUV,
                                   pPlanesB[2]->GetAbsolutePointer(0,0), pPlanesB[2]->GetAbsolutePointer(1,0),
                                   pPlanesB[2]->GetAbsolutePointer(2,0), pPlanesB[2]->GetAbsolutePointer(3,0),
                                   pPlanesB[2]->GetAbsolutePointer(1,0), pPlanesB[2]->GetAbsolutePointer(1,1),
                                   pPlanesB[2]->GetAbsolutePointer(1,2), pPlanesB[2]->GetAbsolutePointer(1,3),
                                   pPlanesB[2]->GetAbsolutePointer(2,0), pPlanesB[2]->GetAbsolutePointer(2,1),
                                   pPlanesB[2]->GetAbsolutePointer(2,2), pPlanesB[2]->GetAbsolutePointer(2,3),
                                   pPlanesB[2]->GetAbsolutePointer(3,0), pPlanesB[2]->GetAbsolutePointer(3,1),
                                   pPlanesB[2]->GetAbsolutePointer(3,2), pPlanesB[2]->GetAbsolutePointer(3,3),
                                   pPlanesB[2]->GetExtendedWidth(), pPlanesB[2]->GetExtendedHeight(), 
                                   pPlanesB[2]->GetPitch(), isse);
                Merge16PlanesToBig(pel2PlaneYF, pel2PitchY,
                                   pPlanesF[0]->GetAbsolutePointer(0,0), pPlanesF[0]->GetAbsolutePointer(1,0),
                                   pPlanesF[0]->GetAbsolutePointer(2,0), pPlanesF[0]->GetAbsolutePointer(3,0),
                                   pPlanesF[0]->GetAbsolutePointer(1,0), pPlanesF[0]->GetAbsolutePointer(1,1),
                                   pPlanesF[0]->GetAbsolutePointer(1,2), pPlanesF[0]->GetAbsolutePointer(1,3),
                                   pPlanesF[0]->GetAbsolutePointer(2,0), pPlanesF[0]->GetAbsolutePointer(2,1),
                                   pPlanesF[0]->GetAbsolutePointer(2,2), pPlanesF[0]->GetAbsolutePointer(2,3),
                                   pPlanesF[0]->GetAbsolutePointer(3,0), pPlanesF[0]->GetAbsolutePointer(3,1),
                                   pPlanesF[0]->GetAbsolutePointer(3,2), pPlanesF[0]->GetAbsolutePointer(3,3),
                                   pPlanesF[0]->GetExtendedWidth(), pPlanesF[0]->GetExtendedHeight(), 
                                   pPlanesF[0]->GetPitch(), isse);
                Merge16PlanesToBig(pel2PlaneUF, pel2PitchUV,
                                   pPlanesF[1]->GetAbsolutePointer(0,0), pPlanesF[1]->GetAbsolutePointer(1,0),
                                   pPlanesF[1]->GetAbsolutePointer(2,0), pPlanesF[1]->GetAbsolutePointer(3,0),
                                   pPlanesF[1]->GetAbsolutePointer(1,0), pPlanesF[1]->GetAbsolutePointer(1,1),
                                   pPlanesF[1]->GetAbsolutePointer(1,2), pPlanesF[1]->GetAbsolutePointer(1,3),
                                   pPlanesF[1]->GetAbsolutePointer(2,0), pPlanesF[1]->GetAbsolutePointer(2,1),
                                   pPlanesF[1]->GetAbsolutePointer(2,2), pPlanesF[1]->GetAbsolutePointer(2,3),
                                   pPlanesF[1]->GetAbsolutePointer(3,0), pPlanesF[1]->GetAbsolutePointer(3,1),
                                   pPlanesF[1]->GetAbsolutePointer(3,2), pPlanesF[1]->GetAbsolutePointer(3,3),
                                   pPlanesF[1]->GetExtendedWidth(), pPlanesF[1]->GetExtendedHeight(),
                                   pPlanesF[1]->GetPitch(), isse);
                Merge16PlanesToBig(pel2PlaneVF, pel2PitchUV,
                                   pPlanesF[2]->GetAbsolutePointer(0,0), pPlanesF[2]->GetAbsolutePointer(1,0),
                                   pPlanesF[2]->GetAbsolutePointer(2,0), pPlanesF[2]->GetAbsolutePointer(3,0),
                                   pPlanesF[2]->GetAbsolutePointer(1,0), pPlanesF[2]->GetAbsolutePointer(1,1),
                                   pPlanesF[2]->GetAbsolutePointer(1,2), pPlanesF[2]->GetAbsolutePointer(1,3),
                                   pPlanesF[2]->GetAbsolutePointer(2,0), pPlanesF[2]->GetAbsolutePointer(2,1),
                                   pPlanesF[2]->GetAbsolutePointer(2,2), pPlanesF[2]->GetAbsolutePointer(2,3),
                                   pPlanesF[2]->GetAbsolutePointer(3,0), pPlanesF[2]->GetAbsolutePointer(3,1),
                                   pPlanesF[2]->GetAbsolutePointer(3,2), pPlanesF[2]->GetAbsolutePointer(3,3),
                                   pPlanesF[2]->GetExtendedWidth(), pPlanesF[2]->GetExtendedHeight(), pPlanesF[2]->GetPitch(), isse);
                }
        }


        // make  vector vx and vy small masks
        // 1. ATTENTION: vectors are assumed SHORT (|vx|, |vy| < 127) !
        // 2. they will be zeroed if not
        // 3. added 128 to all values
        MakeVectorSmallMasks(mvClipB, nBlkX, nBlkY, VXSmallYB, nBlkXP, VYSmallYB, nBlkXP);
        MakeVectorSmallMasks(mvClipF, nBlkX, nBlkY, VXSmallYF, nBlkXP, VYSmallYF, nBlkXP);
        if (nBlkXP > nBlkX) // fill right
        {
            for (int j=0; j<nBlkY; j++) {
                VXSmallYB[j*nBlkXP + nBlkX] = std::min<BYTE>(VXSmallYB[j*nBlkXP + nBlkX-1],128);
                VYSmallYB[j*nBlkXP + nBlkX] = VYSmallYB[j*nBlkXP + nBlkX-1];
                VXSmallYF[j*nBlkXP + nBlkX] = std::min<BYTE>(VXSmallYF[j*nBlkXP + nBlkX-1],128);
                VYSmallYF[j*nBlkXP + nBlkX] = VYSmallYF[j*nBlkXP + nBlkX-1];
            }
        }
        if (nBlkYP > nBlkY) // fill bottom
        {
            for (int i=0; i<nBlkXP; i++) {
                VXSmallYB[nBlkXP*nBlkY +i] = VXSmallYB[nBlkXP*(nBlkY-1) +i];
                VYSmallYB[nBlkXP*nBlkY +i] = std::min<BYTE>(VYSmallYB[nBlkXP*(nBlkY-1) +i],128);
                VXSmallYF[nBlkXP*nBlkY +i] = VXSmallYF[nBlkXP*(nBlkY-1) +i];
                VYSmallYF[nBlkXP*nBlkY +i] = std::min<BYTE>(VYSmallYF[nBlkXP*(nBlkY-1) +i],128);
            }
        }
        VectorSmallMaskYToHalfUV(VXSmallYB, nBlkXP, nBlkYP, VXSmallUVB, 2);
        VectorSmallMaskYToHalfUV(VYSmallYB, nBlkXP, nBlkYP, VYSmallUVB, yRatioUV);
        VectorSmallMaskYToHalfUV(VXSmallYF, nBlkXP, nBlkYP, VXSmallUVF, 2);
        VectorSmallMaskYToHalfUV(VYSmallYF, nBlkXP, nBlkYP, VYSmallUVF, yRatioUV);

        // analyse vectors field to detect occlusion
        //	  double occNormB = (256-time256)/(256*ml);
        MakeVectorOcclusionMaskTime(mvClipB, nBlkX, nBlkY, 1/ml, 1.0, nPel, MaskSmallB, nBlkXP, 
                                    (256-time256), nBlkSizeX - nOverlapX, nBlkSizeY - nOverlapY);
        //	  double occNormF = time256/(256*ml);
        MakeVectorOcclusionMaskTime(mvClipF, nBlkX, nBlkY, 1/ml, 1.0, nPel, MaskSmallF, nBlkXP, 
                                    time256, nBlkSizeX - nOverlapX, nBlkSizeY - nOverlapY);
        if (nBlkXP > nBlkX) // fill right
        {
            for (int j=0; j<nBlkY; j++) {
                MaskSmallB[j*nBlkXP + nBlkX] = MaskSmallB[j*nBlkXP + nBlkX-1];
                MaskSmallF[j*nBlkXP + nBlkX] = MaskSmallF[j*nBlkXP + nBlkX-1];
            }
        }
        if (nBlkYP > nBlkY) // fill bottom
        {
            for (int i=0; i<nBlkXP; i++) {
                MaskSmallB[nBlkXP*nBlkY +i] = MaskSmallB[nBlkXP*(nBlkY-1) +i];
                MaskSmallF[nBlkXP*nBlkY +i] = MaskSmallF[nBlkXP*(nBlkY-1) +i];
            }
        }

        int dummyplane = PLANAR_Y; // use luma plane resizer code for all planes if we resize from luma small mask
        upsizer->SimpleResizeDo(VXFullYB, nWidthP, nHeightP, VPitchY, VXSmallYB, nBlkXP, nBlkXP, dummyplane);
        upsizer->SimpleResizeDo(VYFullYB, nWidthP, nHeightP, VPitchY, VYSmallYB, nBlkXP, nBlkXP, dummyplane);
        upsizerUV->SimpleResizeDo(VXFullUVB, nWidthPUV, nHeightPUV, VPitchUV, VXSmallUVB, nBlkXP, nBlkXP, dummyplane);
        upsizerUV->SimpleResizeDo(VYFullUVB, nWidthPUV, nHeightPUV, VPitchUV, VYSmallUVB, nBlkXP, nBlkXP, dummyplane);

        upsizer->SimpleResizeDo(VXFullYF, nWidthP, nHeightP, VPitchY, VXSmallYF, nBlkXP, nBlkXP, dummyplane);
        upsizer->SimpleResizeDo(VYFullYF, nWidthP, nHeightP, VPitchY, VYSmallYF, nBlkXP, nBlkXP, dummyplane);
        upsizerUV->SimpleResizeDo(VXFullUVF, nWidthPUV, nHeightPUV, VPitchUV, VXSmallUVF, nBlkXP, nBlkXP, dummyplane);
        upsizerUV->SimpleResizeDo(VYFullUVF, nWidthPUV, nHeightPUV, VPitchUV, VYSmallUVF, nBlkXP, nBlkXP, dummyplane);

        upsizer->SimpleResizeDo(MaskFullYB, nWidthP, nHeightP, VPitchY, MaskSmallB, nBlkXP, nBlkXP, dummyplane);
        upsizerUV->SimpleResizeDo(MaskFullUVB, nWidthPUV, nHeightPUV, VPitchUV, MaskSmallB, nBlkXP, nBlkXP, dummyplane);

        upsizer->SimpleResizeDo(MaskFullYF, nWidthP, nHeightP, VPitchY, MaskSmallF, nBlkXP, nBlkXP, dummyplane);
        upsizerUV->SimpleResizeDo(MaskFullUVF, nWidthPUV, nHeightPUV, VPitchUV, MaskSmallF, nBlkXP, nBlkXP, dummyplane);

        // Get motion info from more frames for occlusion areas, and reverse backward and forward
        PVideoFrame mvBB = mvClipB.GetFrame(n+off, env);
        mvClipB.Update(mvBB, env);// backward from next next to next
        PVideoFrame mvFF = mvClipF.GetFrame(n, env);
        mvClipF.Update(mvFF, env);// forward from prev to cur

        if ( mvClipB.IsUsable()  && mvClipF.IsUsable() ) {
            // get vector mask from extra frames
            MakeVectorSmallMasks(mvClipB, nBlkX, nBlkY, VXSmallYBB, nBlkXP, VYSmallYBB, nBlkXP);
            MakeVectorSmallMasks(mvClipF, nBlkX, nBlkY, VXSmallYFF, nBlkXP, VYSmallYFF, nBlkXP);
            if (nBlkXP > nBlkX) // fill right
            {
                for (int j=0; j<nBlkY; j++) {
                    VXSmallYBB[j*nBlkXP + nBlkX] = std::min<BYTE>(VXSmallYBB[j*nBlkXP + nBlkX-1],128);
                    VYSmallYBB[j*nBlkXP + nBlkX] = VYSmallYBB[j*nBlkXP + nBlkX-1];
                    VXSmallYFF[j*nBlkXP + nBlkX] = std::min<BYTE>(VXSmallYFF[j*nBlkXP + nBlkX-1],128);
                    VYSmallYFF[j*nBlkXP + nBlkX] = VYSmallYFF[j*nBlkXP + nBlkX-1];
                }
            }
            if (nBlkYP > nBlkY) // fill bottom
            {
                for (int i=0; i<nBlkXP; i++) {
                    VXSmallYBB[nBlkXP*nBlkY +i] = VXSmallYBB[nBlkXP*(nBlkY-1) +i];
                    VYSmallYBB[nBlkXP*nBlkY +i] = std::min<BYTE>(VYSmallYBB[nBlkXP*(nBlkY-1) +i],128);
                    VXSmallYFF[nBlkXP*nBlkY +i] = VXSmallYFF[nBlkXP*(nBlkY-1) +i];
                    VYSmallYFF[nBlkXP*nBlkY +i] = std::min<BYTE>(VYSmallYFF[nBlkXP*(nBlkY-1) +i],128);
                }
            }
            VectorSmallMaskYToHalfUV(VXSmallYBB, nBlkXP, nBlkYP, VXSmallUVBB, 2);
            VectorSmallMaskYToHalfUV(VYSmallYBB, nBlkXP, nBlkYP, VYSmallUVBB, yRatioUV);
            VectorSmallMaskYToHalfUV(VXSmallYFF, nBlkXP, nBlkYP, VXSmallUVFF, 2);
            VectorSmallMaskYToHalfUV(VYSmallYFF, nBlkXP, nBlkYP, VYSmallUVFF, yRatioUV);

            // upsize vectors to full frame
            upsizer->SimpleResizeDo(VXFullYBB, nWidthP, nHeightP, VPitchY, VXSmallYBB, nBlkXP, nBlkXP, dummyplane);
            upsizer->SimpleResizeDo(VYFullYBB, nWidthP, nHeightP, VPitchY, VYSmallYBB, nBlkXP, nBlkXP, dummyplane);
            upsizerUV->SimpleResizeDo(VXFullUVBB, nWidthPUV, nHeightPUV, VPitchUV, VXSmallUVBB, nBlkXP, nBlkXP, dummyplane);
            upsizerUV->SimpleResizeDo(VYFullUVBB, nWidthPUV, nHeightPUV, VPitchUV, VYSmallUVBB, nBlkXP, nBlkXP, dummyplane);

            upsizer->SimpleResizeDo(VXFullYFF, nWidthP, nHeightP, VPitchY, VXSmallYFF, nBlkXP, nBlkXP, dummyplane);
            upsizer->SimpleResizeDo(VYFullYFF, nWidthP, nHeightP, VPitchY, VYSmallYFF, nBlkXP, nBlkXP, dummyplane);
            upsizerUV->SimpleResizeDo(VXFullUVFF, nWidthPUV, nHeightPUV, VPitchUV, VXSmallUVFF, nBlkXP, nBlkXP, dummyplane);
            upsizerUV->SimpleResizeDo(VYFullUVFF, nWidthPUV, nHeightPUV, VPitchUV, VYSmallUVFF, nBlkXP, nBlkXP, dummyplane);

            if (nPel>=2) {
                FlowInterExtra(pDst[0], nDstPitches[0], pel2PlaneYB + pel2OffsetY, pel2PlaneYF + pel2OffsetY, pel2PitchY,
                               VXFullYB, VXFullYF, VYFullYB, VYFullYF, MaskFullYB, MaskFullYF, VPitchY,
                               nWidth, nHeight, time256, nPel, LUTVB, LUTVF, VXFullYBB, VXFullYFF, VYFullYBB, VYFullYFF);
                FlowInterExtra(pDst[1], nDstPitches[1], pel2PlaneUB + pel2OffsetUV, pel2PlaneUF + pel2OffsetUV, pel2PitchUV,
                               VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, MaskFullUVB, MaskFullUVF, VPitchUV,
                               nWidthUV, nHeightUV, time256, nPel, LUTVB, LUTVF, VXFullUVBB, VXFullUVFF, VYFullUVBB, VYFullUVFF);
                FlowInterExtra(pDst[2], nDstPitches[2], pel2PlaneVB + pel2OffsetUV, pel2PlaneVF + pel2OffsetUV, pel2PitchUV,
                               VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, MaskFullUVB, MaskFullUVF, VPitchUV,
                               nWidthUV, nHeightUV, time256, nPel, LUTVB, LUTVF, VXFullUVBB, VXFullUVFF, VYFullUVBB, VYFullUVFF);

            }
            else if (nPel==1) {
                FlowInterExtra(pDst[0], nDstPitches[0], pPlanesB[0]->GetPointer(0,0), pPlanesF[0]->GetPointer(0,0), pPlanesB[0]->GetPitch(),
                               VXFullYB, VXFullYF, VYFullYB, VYFullYF, MaskFullYB, MaskFullYF, VPitchY,
                               nWidth, nHeight, time256, nPel, LUTVB, LUTVF, VXFullYBB, VXFullYFF, VYFullYBB, VYFullYFF);
                FlowInterExtra(pDst[1], nDstPitches[1], pPlanesB[1]->GetPointer(0,0), pPlanesF[1]->GetPointer(0,0), pPlanesB[1]->GetPitch(),
                               VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, MaskFullUVB, MaskFullUVF, VPitchUV,
                               nWidthUV, nHeightUV, time256, nPel, LUTVB, LUTVF, VXFullUVBB, VXFullUVFF, VYFullUVBB, VYFullUVFF);
                FlowInterExtra(pDst[2], nDstPitches[2], pPlanesB[2]->GetPointer(0,0), pPlanesF[2]->GetPointer(0,0), pPlanesB[2]->GetPitch(),
                               VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, MaskFullUVB, MaskFullUVF, VPitchUV,
                               nWidthUV, nHeightUV, time256, nPel, LUTVB, LUTVF, VXFullUVBB, VXFullUVFF, VYFullUVBB, VYFullUVFF);
            }
        }
        else // bad extra frames, use old method without extra frames
        {
            if (nPel>=2) {
                FlowInter(pDst[0], nDstPitches[0], pel2PlaneYB + pel2OffsetY, pel2PlaneYF + pel2OffsetY, pel2PitchY,
                          VXFullYB, VXFullYF, VYFullYB, VYFullYF, MaskFullYB, MaskFullYF, VPitchY,
                          nWidth, nHeight, time256, nPel, LUTVB, LUTVF);
                FlowInter(pDst[1], nDstPitches[1], pel2PlaneUB + pel2OffsetUV, pel2PlaneUF + pel2OffsetUV, pel2PitchUV,
                          VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, MaskFullUVB, MaskFullUVF, VPitchUV,
                          nWidthUV, nHeightUV, time256, nPel, LUTVB, LUTVF);
                FlowInter(pDst[2], nDstPitches[2], pel2PlaneVB + pel2OffsetUV, pel2PlaneVF + pel2OffsetUV, pel2PitchUV,
                          VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, MaskFullUVB, MaskFullUVF, VPitchUV,
                          nWidthUV, nHeightUV, time256, nPel, LUTVB, LUTVF);
            }
            else if (nPel==1) {
                FlowInter(pDst[0], nDstPitches[0], pPlanesB[0]->GetPointer(0,0), pPlanesF[0]->GetPointer(0,0), pPlanesB[0]->GetPitch(),
                          VXFullYB, VXFullYF, VYFullYB, VYFullYF, MaskFullYB, MaskFullYF, VPitchY,
                          nWidth, nHeight, time256, nPel, LUTVB, LUTVF);
                FlowInter(pDst[1], nDstPitches[1], pPlanesB[1]->GetPointer(0,0), pPlanesF[1]->GetPointer(0,0), pPlanesB[1]->GetPitch(),
                          VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, MaskFullUVB, MaskFullUVF, VPitchUV,
                          nWidthUV, nHeightUV, time256, nPel, LUTVB, LUTVF);
                FlowInter(pDst[2], nDstPitches[2], pPlanesB[2]->GetPointer(0,0), pPlanesF[2]->GetPointer(0,0), pPlanesB[2]->GetPitch(),
                          VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, MaskFullUVB, MaskFullUVF, VPitchUV,
                          nWidthUV, nHeightUV, time256, nPel, LUTVB, LUTVF);
            }
        }
    }
    else {
        //	   return src;
        // poor estimation, let's blend src with ref frames
        unsigned char const *pRef[3], *pSrc[3];
        int nRefPitches[3], nSrcPitches[3];
        SrcPlanes->ConvertVideoFrameToPlanes(&src, pSrc, nSrcPitches);
        RefPlanes->ConvertVideoFrameToPlanes(&ref, pRef, nRefPitches);

        // blend with time weight
        Blend(pDst[0], pSrc[0], pRef[0], nHeight, nWidth, nDstPitches[0], nSrcPitches[0], nRefPitches[0], time256, isse);
        Blend(pDst[1], pSrc[1], pRef[1], nHeightUV, nWidthUV, nDstPitches[1], nSrcPitches[1], nRefPitches[1], time256, isse);
        Blend(pDst[2], pSrc[2], pRef[2], nHeightUV, nWidthUV, nDstPitches[2], nSrcPitches[2], nRefPitches[2], time256, isse);
    }

    // convert back from planes
    unsigned char *pDstYUY2 = dst->GetWritePtr();
    int nDstPitchYUY2 = dst->GetPitch();
    DstPlanes->YUY2FromPlanes(pDstYUY2, nDstPitchYUY2);

    return dst;
}

void MVFlowInter::ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
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
