// Pixels flow motion interpolation function to change FPS
// with using second clip cropped by halfblock along diagonal (crop(4,4,-4,-4))
// Copyright(c)2005 A.G.Balakhnin aka Fizick

// See legal notice in Copying.txt for more information

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.
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

#include "MVFlowFps2.h"
#include "CopyCode.h"
#include "MaskFun.h"
#include "Padding.h"

MVFlowFps2::MVFlowFps2(PClip _child, PClip _mvbw, PClip _mvfw, PClip _mvbw2, PClip _mvfw2, unsigned int _num, unsigned int _den,
                       int _maskmode, double _ml, double _mSAD, double _mSADC, PClip _pelclip, int _nIdx, int _nIdx2, int nSCD1,
                       int nSCD2, bool _mmx, bool _isse, IScriptEnvironment* env) :
            GenericVideoFilter(_child),
            MVFilter(_mvfw, "MVFlowFps2", env),
            mvClipB(_mvbw, nSCD1, nSCD2, env, true),
            mvClipF(_mvfw, nSCD1, nSCD2, env, true),
            mvClipB2(_mvbw2, nSCD1, nSCD2, env, true),
            mvClipF2(_mvfw2, nSCD1, nSCD2, env, true),
            pelclip(_pelclip)
{
    numeratorOld = vi.fps_numerator;
    denominatorOld = vi.fps_denominator;

    if (_num != 0 && _den != 0) {
        numerator = _num;
        denominator = _den;
    }
    else if (numeratorOld < (1<<30)) {
        numerator = (numeratorOld<<1); // double fps by default
        denominator = denominatorOld;
    }
    else // very big numerator
    {
        numerator = numeratorOld;
        denominator = (denominatorOld>>1);// double fps by default
    }

    vi.fps_numerator = numerator;
    vi.fps_denominator = denominator;
    vi.num_frames = (int)(1 + __int64(vi.num_frames-1) * __int64(numerator)*__int64(denominatorOld)/(__int64(denominator)*__int64(numeratorOld) ) );

    maskmode = _maskmode;
    ml = _ml;
    mSAD  = _mSAD;
    mSADC = _mSADC;
    nIdx = _nIdx;
    nIdx2 = _nIdx2; // how to use (or not use) second idx???
    if (nIdx2 == nIdx)
        env->ThrowError("MVFlowFps2: idx and idx2 must be different");

    mmx = _mmx;
    isse = _isse;

    nBlkXCropped = mvClipB2.GetBlkX();
    nBlkYCropped = mvClipB2.GetBlkY();

    if (mvClipB.GetOverlapX() != 0 || mvClipB.GetOverlapX() != 0)
        env->ThrowError("MVFlowFps2: overlap is not supported");

    if (!mvClipB.IsBackward())
        env->ThrowError("MVFlowFps2: wrong backward vectors");
    if (mvClipF.IsBackward())
        env->ThrowError("MVFlowFps2: wrong forward vectors");
    if (!mvClipB2.IsBackward())
        env->ThrowError("MVFlowFps2: wrong backward2 vectors");
    if (mvClipF2.IsBackward())
        env->ThrowError("MVFlowFps2: wrong forward2 vectors");

    CheckSimilarity(mvClipB, "mvbw", env);
    CheckSimilarity(mvClipF, "mvfw", env);
    //   CheckSimilarity(mvClipB2, "mvbw2", env); // width,height are different (cropped)
    //   CheckSimilarity(mvClipF2, "mvfw2", env);

    normSAD1024  = unsigned int( 1024 * 255 / (mSAD * nBlkSizeX*nBlkSizeY) ); //normalize
    normSAD1024C = unsigned int( 1024 * 255 / (mSADC * nBlkSizeX*nBlkSizeY) ); //normalize

    mvCore->AddFrames(nIdx, 3, mvClipB.GetLevelCount(), nWidth, nHeight,
                      nPel, nHPadding, nVPadding, YUVPLANES, isse, yRatioUV);
    //     mvCore->AddFrames(nIdx, MV_BUFFER_FRAMES, mvClipF.GetLevelCount(), nWidth, nHeight,
    //                        nPel, nBlkSize, nBlkSize, YUVPLANES, isse, yRatioUV);

    mvCore->AddFrames(nIdx2, 3, mvClipB2.GetLevelCount(), nWidth, nHeight,
                      nPel, nHPadding, nVPadding, YUVPLANES, isse, yRatioUV);
    //     mvCore->AddFrames(nIdx2, MV_BUFFER_FRAMES, mvClipF2.GetLevelCount(), nWidth, nHeight,
    //                        nPel, nBlkSize, nBlkSize, YUVPLANES, isse, yRatioUV);

    usePelClipHere = false;
    if (pelclip && (nPel > 1)) {
        if (pelclip->GetVideoInfo().width != nWidth*nPel || pelclip->GetVideoInfo().height != nHeight*nPel)
            env->ThrowError("MVFlowFps2: pelclip frame size must be Pel of source!");
        else
            usePelClipHere = true;
    }
    nHeightUV = nHeight/yRatioUV; // for YV12
    nWidthUV = nWidth/2;

    nHPaddingUV = nHPadding/2;
    nVPaddingUV = nVPadding/yRatioUV;

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

    VXSmallYB2 = new BYTE [nBlkX*nBlkY];
    VYSmallYB2 = new BYTE [nBlkX*nBlkY];

    VXSmallYF = new BYTE [nBlkX*nBlkY];
    VYSmallYF = new BYTE [nBlkX*nBlkY];

    VXSmallYF2 = new BYTE [nBlkX*nBlkY];
    VYSmallYF2 = new BYTE [nBlkX*nBlkY];

    VXDoubledYB = new BYTE [nBlkX*nBlkY*4];
    VYDoubledYB = new BYTE [nBlkX*nBlkY*4];

    VXDoubledYB2 = new BYTE [nBlkX*nBlkY*4];
    VYDoubledYB2 = new BYTE [nBlkX*nBlkY*4];

    VXDoubledYF = new BYTE [nBlkX*nBlkY*4];
    VYDoubledYF = new BYTE [nBlkX*nBlkY*4];

    VXDoubledYF2 = new BYTE [nBlkX*nBlkY*4];
    VYDoubledYF2 = new BYTE [nBlkX*nBlkY*4];

    VXCombinedYF = new BYTE [nBlkX*nBlkY*4];
    VYCombinedYF = new BYTE [nBlkX*nBlkY*4];
    VXCombinedUVF = new BYTE [nBlkX*nBlkY*4];
    VYCombinedUVF = new BYTE [nBlkX*nBlkY*4];

    VXCombinedYB = new BYTE [nBlkX*nBlkY*4];
    VYCombinedYB = new BYTE [nBlkX*nBlkY*4];
    VXCombinedUVB = new BYTE [nBlkX*nBlkY*4];
    VYCombinedUVB = new BYTE [nBlkX*nBlkY*4];

    VXFullYBB = new BYTE [nHeight*VPitchY];
    VXFullUVBB = new BYTE [nHeightUV*VPitchUV];
    VYFullYBB = new BYTE [nHeight*VPitchY];
    VYFullUVBB = new BYTE [nHeightUV*VPitchUV];

    VXFullYFF = new BYTE [nHeight*VPitchY];
    VXFullUVFF = new BYTE [nHeightUV*VPitchUV];
    VYFullYFF = new BYTE [nHeight*VPitchY];
    VYFullUVFF = new BYTE [nHeightUV*VPitchUV];

    VXSmallYBB = new BYTE [nBlkX*nBlkY];
    VYSmallYBB = new BYTE [nBlkX*nBlkY];
    VXSmallUVBB = new BYTE [nBlkX*nBlkY];
    VYSmallUVBB = new BYTE [nBlkX*nBlkY];

    VXSmallYFF = new BYTE [nBlkX*nBlkY];
    VYSmallYFF = new BYTE [nBlkX*nBlkY];
    VXSmallUVFF = new BYTE [nBlkX*nBlkY];
    VYSmallUVFF = new BYTE [nBlkX*nBlkY];

    MaskDoubledB = new BYTE [nBlkX*nBlkY*4];
    MaskFullYB = new BYTE [nHeight*VPitchY];
    MaskFullUVB = new BYTE [nHeightUV*VPitchUV];

    MaskDoubledF = new BYTE [nBlkX*nBlkY*4];
    MaskFullYF = new BYTE [nHeight*VPitchY];
    MaskFullUVF = new BYTE [nHeightUV*VPitchUV];

    SADMaskSmallB = new BYTE [nBlkX*nBlkY];
    SADMaskSmallF = new BYTE [nBlkX*nBlkY];
    SADMaskDoubledB = new BYTE [nBlkX*nBlkY*4];
    SADMaskDoubledF = new BYTE [nBlkX*nBlkY*4];

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

    upsizer = new SimpleResize(nWidth, nHeight, nBlkX*2, nBlkY*2, CPUF_Resize);
    upsizerUV = new SimpleResize(nWidthUV, nHeightUV, nBlkX*2, nBlkY*2, CPUF_Resize);

    upsizer2 = new SimpleResize(nBlkX*2, nBlkY*2, nBlkX, nBlkY, CPUF_Resize);

    upsizer1 = new SimpleResize(nWidth, nHeight, nBlkX, nBlkY, CPUF_Resize);
    upsizer1UV = new SimpleResize(nWidthUV, nHeightUV, nBlkX, nBlkY, CPUF_Resize);

    nleftLast = -1000;
    nrightLast = -1000;

    DstPlanes =  new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);
    SrcPlanes =  new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);
    RefPlanes =  new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);
}

MVFlowFps2::~MVFlowFps2()
{
    if (DstPlanes) delete DstPlanes;
    if (SrcPlanes) delete DstPlanes;
    if (RefPlanes) delete DstPlanes;

    if (upsizer2)  delete upsizer2;
    if (upsizer)   delete upsizer;
    if (upsizerUV) delete upsizerUV;

    if (VXFullYB)   delete [] VXFullYB;
    if (VXFullUVB)  delete [] VXFullUVB;
    if (VYFullYB)   delete [] VYFullYB;
    if (VYFullUVB)  delete [] VYFullUVB;
    if (VXSmallYB)  delete [] VXSmallYB;
    if (VYSmallYB)  delete [] VYSmallYB;
    if (VXSmallYB2) delete [] VXSmallYB2;
    if (VYSmallYB2) delete [] VYSmallYB2;
    if (VXFullYF)   delete [] VXFullYF;
    if (VXFullUVF)  delete [] VXFullUVF;
    if (VYFullYF)   delete [] VYFullYF;
    if (VYFullUVF)  delete [] VYFullUVF;
    if (VXSmallYF)  delete [] VXSmallYF;
    if (VYSmallYF)  delete [] VYSmallYF;
    if (VXSmallYF2) delete [] VXSmallYF2;
    if (VYSmallYF2) delete [] VYSmallYF2;

    if (VXDoubledYF)  delete [] VXDoubledYF;
    if (VYDoubledYF)  delete [] VYDoubledYF;
    if (VXDoubledYF2) delete [] VXDoubledYF2;
    if (VYDoubledYF2) delete [] VYDoubledYF2;

    if (VXDoubledYB)  delete [] VXDoubledYB;
    if (VYDoubledYB)  delete [] VYDoubledYB;
    if (VXDoubledYB2) delete [] VXDoubledYB2;
    if (VYDoubledYB2) delete [] VYDoubledYB2;

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

    if (MaskDoubledB) delete [] MaskDoubledB;
    if (MaskFullYB)   delete [] MaskFullYB;
    if (MaskFullUVB)  delete [] MaskFullUVB;
    if (MaskDoubledF) delete [] MaskDoubledF;
    if (MaskFullYF)   delete [] MaskFullYF;
    if (MaskFullUVF)  delete [] MaskFullUVF;

    if (VXCombinedYB)  delete [] VXCombinedYB;
    if (VYCombinedYB)  delete [] VYCombinedYB;
    if (VXCombinedUVB) delete [] VXCombinedUVB;
    if (VYCombinedUVB) delete [] VYCombinedUVB;
    if (VXCombinedYF)  delete [] VXCombinedYF;
    if (VYCombinedYF)  delete [] VYCombinedYF;
    if (VXCombinedUVF) delete [] VXCombinedUVF;
    if (VYCombinedUVF) delete [] VYCombinedUVF;

    if (SADMaskSmallB)   delete [] SADMaskSmallB;
    if (SADMaskSmallF)   delete [] SADMaskSmallF;
    if (SADMaskDoubledB) delete [] SADMaskDoubledB;
    if (SADMaskDoubledF) delete [] SADMaskDoubledF;

    if (nPel>1) {
        if (pel2PlaneYB) delete [] pel2PlaneYB;
        if (pel2PlaneUB) delete [] pel2PlaneUB;
        if (pel2PlaneVB) delete [] pel2PlaneVB;
        if (pel2PlaneYF) delete [] pel2PlaneYF;
        if (pel2PlaneUF) delete [] pel2PlaneUF;
        if (pel2PlaneVF) delete [] pel2PlaneVF;
    }
}

void CombineShiftedMask(BYTE *VXDoubled, BYTE *VXDoubledShifted, BYTE *VXCombined, int nBlkX2, int nBlkY2)
{ // diagonal shift by halfblock, i.e. by 1 here
    for (int w=0; w<nBlkX2; w++)
        VXCombined[w] = VXDoubled[w];
    VXDoubled += nBlkX2;
    VXCombined += nBlkX2;
    for (int h=1; h<nBlkY2-1; h++) {
        VXCombined[0] = VXDoubled[0];
        for (int w=1; w<nBlkX2-1; w++) {
            VXCombined[w] = (VXDoubled[w] + VXDoubledShifted[w-1] )>>1;
        }
        VXCombined[nBlkX2-1] = VXDoubled[nBlkX2-1];
        VXDoubled += nBlkX2;
        VXDoubledShifted += nBlkX2;
        VXCombined += nBlkX2;
    }
    for (int w=0; w<nBlkX2; w++)
        VXCombined[w] = VXDoubled[w];
    VXDoubled += nBlkX2;
    VXCombined += nBlkX2;
}

void FillCroppedMaskPad(BYTE *VSmallY2, int widthCropped, int heightCropped, int width, int height)
{
    // we use cropped second clip
    // it is cropped in all sides.
    // but we store it's vector mask in array with non-cropped sizes
    // here we pad right and bottom part to zero vector
    // zero vector value is 128
    for (int h=0; h<heightCropped; h++) {
        for (int w=widthCropped; w<width; w++)
            VSmallY2[w] = 128;
        VSmallY2 += width;
    }

    for (int h=heightCropped; h<height; h++) {
        for (int w=0; w<width; w++)
            VSmallY2[w] = 128;
        VSmallY2 += width;
    }
}

//-------------------------------------------------------------------------
PVideoFrame __stdcall MVFlowFps2::GetFrame(int n, IScriptEnvironment* env)
{
    int nleft = (int) ( (__int64(n)* __int64(denominator)* __int64(numeratorOld))/( __int64(numerator)*__int64(denominatorOld)) );
    // intermediate product may be very large! Now I know how to multiply int64
    time256 = int( (double(n)*double(denominator)*double(numeratorOld)/double(numerator)/double(denominatorOld) - nleft)*256 + 0.5);

    int off = mvClipB.GetDeltaFrame(); // integer offset of reference frame
    // usually off must be = 1
    if (off > 1)
        time256 = time256/off;

    int nright = nleft+off;

    if (time256==0)
        return child->GetFrame(nleft, env); // simply left
    else if (time256==256)
        return child->GetFrame(nright, env); // simply right

    Create_LUTV(time256, LUTVB, LUTVF); // lookup table

    PVideoFrame dst = env->NewVideoFrame(vi);
    // convert to frames
    unsigned char *pDst[3];
    int nDstPitches[3];
    DstPlanes->ConvertVideoFrameToPlanes(&dst, pDst, nDstPitches);

    PVideoFrame mvB = mvClipB.GetFrame(nleft, env);
    mvClipB.Update(mvB, env);// backward from next to current
    PVideoFrame mvF = mvClipF.GetFrame(nright, env);
    mvClipF.Update(mvF, env);// forward from current to next

    PVideoFrame mvB2 = mvClipB2.GetFrame(nleft, env);
    mvClipB2.Update(mvB2, env);// backward from next to current
    PVideoFrame mvF2 = mvClipF2.GetFrame(nright, env);
    mvClipF2.Update(mvF2, env);// forward from current to next

    int sharp = mvClipB.GetSharp();

    if ( mvClipB.IsUsable() && mvClipF.IsUsable() && mvClipB2.IsUsable() && mvClipF2.IsUsable() ) {
        // get reference frames
        PMVGroupOfFrames pRefGOFF = mvCore->GetFrame(nIdx, nleft);
        if (!pRefGOFF->IsProcessed()) {
            PVideoFrame src, src2x;
            src= child->GetFrame(nleft, env);
            pRefGOFF->SetParity(child->GetParity(nleft));
            if (usePelClipHere)
                src2x= pelclip->GetFrame(nleft, env);
            ProcessFrameIntoGroupOfFrames(&pRefGOFF, &src, &src2x, sharp, pixelType, nHeight, nWidth, nPel, isse);
        }

        PMVGroupOfFrames pRefGOFB = mvCore->GetFrame(nIdx, nright);
        if (!pRefGOFB->IsProcessed()) {
            PVideoFrame ref, ref2x;
            ref = child->GetFrame(nright, env);
            pRefGOFB->SetParity(child->GetParity(nright));
            if (usePelClipHere)
                ref2x= pelclip->GetFrame(nright, env);
            ProcessFrameIntoGroupOfFrames(&pRefGOFB, &ref, &ref2x, sharp, pixelType, nHeight, nWidth, nPel, isse);
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
            // merge refined planes to big single plane
            if (nright != nrightLast) {
                if (usePelClipHere) {
                    PlaneCopy(pel2PlaneYB + nHPadding*nPel + nVPadding*nPel * pel2PitchY, pel2PitchY,
                              pRefGOFB->GetVFYPtr(), pRefGOFB->GetVFYPitch(), nWidth*nPel, nHeight*nPel, isse);
                    Padding::PadReferenceFrame(pel2PlaneYB, pel2PitchY, nHPadding*nPel, nVPadding*nPel, nWidth*nPel, 
                                               nHeight*nPel);
                    PlaneCopy(pel2PlaneUB + nHPaddingUV*nPel + nVPaddingUV*nPel * pel2PitchUV, pel2PitchUV,
                              pRefGOFB->GetVFUPtr(), pRefGOFB->GetVFUPitch(), nWidthUV*nPel, nHeightUV*nPel, isse);
                    Padding::PadReferenceFrame(pel2PlaneUB, pel2PitchUV, nHPaddingUV*nPel, nVPaddingUV*nPel, nWidthUV*nPel,
                                               nHeightUV*nPel);
                    PlaneCopy(pel2PlaneVB + nHPaddingUV*nPel + nVPaddingUV*nPel * pel2PitchUV, pel2PitchUV,
                              pRefGOFB->GetVFVPtr(), pRefGOFB->GetVFVPitch(), nWidthUV*nPel, nHeightUV*nPel, isse);
                    Padding::PadReferenceFrame(pel2PlaneVB, pel2PitchUV, nHPaddingUV*nPel, nVPaddingUV*nPel, nWidthUV*nPel, 
                                               nHeightUV*nPel);
                }
                else if (nPel==2) {
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
                else if (nPel==4)
                {
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
                }
            }
            if(nleft != nleftLast) {
                if (usePelClipHere)
                { // simply padding 2x planes
                    PlaneCopy(pel2PlaneYF + nHPadding*nPel + nVPadding*nPel * pel2PitchY, pel2PitchY,
                              pRefGOFF->GetVFYPtr(), pRefGOFF->GetVFYPitch(), nWidth*nPel, nHeight*nPel, isse);
                    Padding::PadReferenceFrame(pel2PlaneYB, pel2PitchY, nHPadding*nPel, nVPadding*nPel, nWidth*nPel,
                                               nHeight*nPel);
                    PlaneCopy(pel2PlaneUF + nHPaddingUV*nPel + nVPaddingUV*nPel * pel2PitchUV, pel2PitchUV,
                              pRefGOFF->GetVFUPtr(), pRefGOFF->GetVFUPitch(), nWidthUV*nPel, nHeightUV*nPel, isse);
                    Padding::PadReferenceFrame(pel2PlaneUB, pel2PitchUV, nHPaddingUV*nPel, nVPaddingUV*nPel, nWidthUV*nPel, 
                                               nHeightUV*nPel);
                    PlaneCopy(pel2PlaneVF + nHPaddingUV*nPel + nVPaddingUV*nPel * pel2PitchUV, pel2PitchUV,
                              pRefGOFF->GetVFVPtr(), pRefGOFF->GetVFVPitch(), nWidthUV*nPel, nHeightUV*nPel, isse);
                    Padding::PadReferenceFrame(pel2PlaneVF, pel2PitchUV, nHPaddingUV*nPel, nVPaddingUV*nPel, nWidthUV*nPel, 
                                               nHeightUV*nPel);
                }
                else if (nPel==2) {
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
                                       pPlanesF[2]->GetExtendedWidth(), pPlanesF[2]->GetExtendedHeight(),
                                       pPlanesF[2]->GetPitch(), isse);
                }
            }
        }

        int dummyplane = PLANAR_Y; // use luma plane resizer code for all planes if we resize from luma small mask

        // make  vector vx and vy small masks
        // 1. ATTENTION: vectors are assumed SHORT (|vx|, |vy| < 127) !
        // 2. they will be zeroed if not
        // 3. added 128 to all values
        MakeVectorSmallMasks(mvClipB, nBlkX, nBlkY, VXSmallYB, nBlkX, VYSmallYB, nBlkX);
        // upsize (bilinear interpolate) vector masks to doubled size
        upsizer2->SimpleResizeDo(VXDoubledYB, nBlkX*2, nBlkY*2, nBlkX*2, VXSmallYB, nBlkX, nBlkX, dummyplane);
        upsizer2->SimpleResizeDo(VYDoubledYB, nBlkX*2, nBlkY*2, nBlkX*2, VYSmallYB, nBlkX, nBlkX, dummyplane);
        //second - shifted clip
        MakeVectorSmallMasks(mvClipB2, nBlkXCropped, nBlkYCropped, VXSmallYB2, nBlkX, VYSmallYB2, nBlkX);
        FillCroppedMaskPad(VXSmallYB2, nBlkXCropped, nBlkYCropped, nBlkX, nBlkY);
        FillCroppedMaskPad(VYSmallYB2, nBlkXCropped, nBlkYCropped, nBlkX, nBlkY);
        // upsize (bilinear interpolate) vector masks to doubled size
        upsizer2->SimpleResizeDo(VXDoubledYB2, nBlkX*2, nBlkY*2, nBlkX*2, VXSmallYB2, nBlkX, nBlkX, dummyplane);
        upsizer2->SimpleResizeDo(VYDoubledYB2, nBlkX*2, nBlkY*2, nBlkX*2, VYSmallYB2, nBlkX, nBlkX, dummyplane);

        // combine non-shifted and shifted doubled mask to form combined average mask
        CombineShiftedMask(VXDoubledYB, VXDoubledYB2, VXCombinedYB, nBlkX*2, nBlkY*2);
        VectorSmallMaskYToHalfUV(VXCombinedYB, nBlkX*2, nBlkY*2, VXCombinedUVB, 2);
        CombineShiftedMask(VYDoubledYB, VYDoubledYB2, VYCombinedYB, nBlkX*2, nBlkY*2);
        VectorSmallMaskYToHalfUV(VYCombinedYB, nBlkX*2, nBlkY*2, VYCombinedUVB, yRatioUV);

        // analyse vectors field to detect occlusion
        //		VectorMasksToOcclusionMask(VXCombinedYB, VYCombinedYB, nBlkX*2, nBlkY*2, 2*(256-time256)/(256*ml), 1.0, nPel, MaskDoubledB);
        VectorMasksToOcclusionMaskTime(VXCombinedYB, VYCombinedYB, nBlkX*2, nBlkY*2, 2/ml, 1.0, nPel, MaskDoubledB, nBlkX*2, (256-time256), nBlkSizeX/2, nBlkSizeY/2); // ??

        upsizer->SimpleResizeDo(VXFullYB, nWidth, nHeight, VPitchY, VXCombinedYB, nBlkX*2, nBlkX*2, dummyplane);
        upsizer->SimpleResizeDo(VYFullYB, nWidth, nHeight, VPitchY, VYCombinedYB, nBlkX*2, nBlkX*2, dummyplane);
        upsizerUV->SimpleResizeDo(VXFullUVB, nWidthUV, nHeightUV, VPitchUV, VXCombinedUVB, nBlkX*2, nBlkX*2, dummyplane);
        upsizerUV->SimpleResizeDo(VYFullUVB, nWidthUV, nHeightUV, VPitchUV, VYCombinedUVB, nBlkX*2, nBlkX*2, dummyplane);
        /*
        // make SAD small mask
        for ( int j=0; j < nBlkY; j++)
            for (int i=0; i<nBlkX; i++) {
                SADMaskSmallB[j*nBlkX + i] = SADToMask(mvClipB.GetBlock(0, j*nBlkX + i).GetSAD(), normSAD1024);
            }
        // upsize (bilinear interpolate) SAD mask to doubled size
        upsizer2->SimpleResizeDo(SADMaskDoubledB, nBlkX*2, nBlkY*2, nBlkX*2, SADMaskSmallB, nBlkX, nBlkX, dummyplane);
        // make summary bad mask (occlusion or big SAD)
        for ( int j=0; j < nBlkY*2; j++)
            for (int i=0; i<nBlkX*2; i++) {
                int ji = j*nBlkX*2 + i;
                MaskDoubledB[ji] = 255 - (((255-MaskDoubledB[ji])*(255-SADMaskDoubledB[ji]) + 255)>>8);
            }
        */
        upsizer->SimpleResizeDo(MaskFullYB, nWidth, nHeight, VPitchY, MaskDoubledB, nBlkX*2, nBlkX*2, dummyplane);
        upsizerUV->SimpleResizeDo(MaskFullUVB, nWidthUV, nHeightUV, VPitchUV, MaskDoubledB, nBlkX*2, nBlkX*2, dummyplane);

        nrightLast = nright;

        // make  vector vx and vy small masks
        // 1. ATTENTION: vectors are assumed SHORT (|vx|, |vy| < 127) !
        // 2. they will be zeroed if not
        // 3. added 128 to all values
        MakeVectorSmallMasks(mvClipF, nBlkX, nBlkY, VXSmallYF, nBlkX, VYSmallYF, nBlkX);
        // upsize (bilinear interpolate) vector masks to doubled size
        upsizer2->SimpleResizeDo(VXDoubledYF, nBlkX*2, nBlkY*2, nBlkX*2, VXSmallYF, nBlkX, nBlkX, dummyplane);
        upsizer2->SimpleResizeDo(VYDoubledYF, nBlkX*2, nBlkY*2, nBlkX*2, VYSmallYF, nBlkX, nBlkX, dummyplane);
        // second - shifted clip
        MakeVectorSmallMasks(mvClipF2, nBlkXCropped, nBlkYCropped, VXSmallYF2, nBlkX, VYSmallYF2, nBlkX);
        FillCroppedMaskPad(VXSmallYF2, nBlkXCropped, nBlkYCropped, nBlkX, nBlkY);
        FillCroppedMaskPad(VYSmallYF2, nBlkXCropped, nBlkYCropped, nBlkX, nBlkY);
        // upsize (bilinear interpolate) vector masks to fullframe size
        upsizer2->SimpleResizeDo(VXDoubledYF2, nBlkX*2, nBlkY*2, nBlkX*2, VXSmallYF2, nBlkX, nBlkX, dummyplane);
        upsizer2->SimpleResizeDo(VYDoubledYF2, nBlkX*2, nBlkY*2, nBlkX*2, VYSmallYF2, nBlkX, nBlkX, dummyplane);

        // combine non-shifted and shifted doubled mask to form combined average mask
        CombineShiftedMask(VXDoubledYF, VXDoubledYF2, VXCombinedYF, nBlkX*2, nBlkY*2);
        VectorSmallMaskYToHalfUV(VXCombinedYF, nBlkX*2, nBlkY*2, VXCombinedUVF, 2);
        CombineShiftedMask(VYDoubledYF, VYDoubledYF2, VYCombinedYF, nBlkX*2, nBlkY*2);
        VectorSmallMaskYToHalfUV(VYCombinedYF, nBlkX*2, nBlkY*2, VYCombinedUVF, yRatioUV);

        // analyse vectors field to detect occlusion
        //		VectorMasksToOcclusionMask(VXCombinedYF, VYCombinedYF, nBlkX*2, nBlkY*2, 2*time256/(256*ml), 1.0, nPel, MaskDoubledF);
        VectorMasksToOcclusionMaskTime(VXCombinedYF, VYCombinedYF, nBlkX*2, nBlkY*2, 2/ml, 1.0, nPel, MaskDoubledF, nBlkX*2, time256, nBlkSizeX/2, nBlkSizeY/2);

        upsizer->SimpleResizeDo(VXFullYF, nWidth, nHeight, VPitchY, VXCombinedYF, nBlkX*2, nBlkX*2, dummyplane);
        upsizer->SimpleResizeDo(VYFullYF, nWidth, nHeight, VPitchY, VYCombinedYF, nBlkX*2, nBlkX*2, dummyplane);
        upsizerUV->SimpleResizeDo(VXFullUVF, nWidthUV, nHeightUV, VPitchUV, VXCombinedUVF, nBlkX*2, nBlkX*2, dummyplane);
        upsizerUV->SimpleResizeDo(VYFullUVF, nWidthUV, nHeightUV, VPitchUV, VYCombinedUVF, nBlkX*2, nBlkX*2, dummyplane);
        /*
        // make SAD small mask
        for ( int j=0; j < nBlkY; j++)
            for (int i=0; i<nBlkX; i++) {
                SADMaskSmallF[j*nBlkX + i] = SADToMask(mvClipF.GetBlock(0, j*nBlkX + i).GetSAD(), normSAD1024);
            }
        // upsize (bilinear interpolate) SAD mask to doubled size
        upsizer2->SimpleResizeDo(SADMaskDoubledF, nBlkX*2, nBlkY*2, nBlkX*2, SADMaskSmallF, nBlkX, nBlkX, dummyplane);
        // make summary bad mask (occlusion or big SAD)
        for ( int j=0; j < nBlkY*2; j++)
            for (int i=0; i<nBlkX*2; i++) {
                int ji = j*nBlkX*2 + i;
                MaskDoubledF[ji] = 255 - (((255-MaskDoubledF[ji])*(255-SADMaskDoubledF[ji]) + 255)>>8);
            }
        */
        upsizer->SimpleResizeDo(MaskFullYF, nWidth, nHeight, VPitchY, MaskDoubledF, nBlkX*2, nBlkX*2, dummyplane);
        upsizerUV->SimpleResizeDo(MaskFullUVF, nWidthUV, nHeightUV, VPitchUV, MaskDoubledF, nBlkX*2, nBlkX*2, dummyplane);

        nleftLast = nleft;

        // Get motion info from more frames for occlusion areas
        PVideoFrame mvBB = mvClipB.GetFrame(nright, env);
        mvClipB.Update(mvBB, env);// backward from next next to next
        PVideoFrame mvFF = mvClipF.GetFrame(nleft, env);
        mvClipF.Update(mvFF, env);// forward from prev to cur

        if ( mvClipB.IsUsable()  && mvClipF.IsUsable() && maskmode==2) {
            // get vector mask from extra frames
            MakeVectorSmallMasks(mvClipB, nBlkX, nBlkY, VXSmallYBB, nBlkX, VYSmallYBB, nBlkX);
            MakeVectorSmallMasks(mvClipF, nBlkX, nBlkY, VXSmallYFF, nBlkX, VYSmallYFF, nBlkX);

            VectorSmallMaskYToHalfUV(VXSmallYBB, nBlkX, nBlkY, VXSmallUVBB, 2);
            VectorSmallMaskYToHalfUV(VYSmallYBB, nBlkX, nBlkY, VYSmallUVBB, yRatioUV);
            VectorSmallMaskYToHalfUV(VXSmallYFF, nBlkX, nBlkY, VXSmallUVFF, 2);
            VectorSmallMaskYToHalfUV(VYSmallYFF, nBlkX, nBlkY, VYSmallUVFF, yRatioUV);

            // upsize vectors to full frame
            upsizer1->SimpleResizeDo(VXFullYBB, nWidth, nHeight, VPitchY, VXSmallYBB, nBlkX, nBlkX, dummyplane);
            upsizer1->SimpleResizeDo(VYFullYBB, nWidth, nHeight, VPitchY, VYSmallYBB, nBlkX, nBlkX, dummyplane);
            upsizer1UV->SimpleResizeDo(VXFullUVBB, nWidthUV, nHeightUV, VPitchUV, VXSmallUVBB, nBlkX, nBlkX, dummyplane);
            upsizer1UV->SimpleResizeDo(VYFullUVBB, nWidthUV, nHeightUV, VPitchUV, VYSmallUVBB, nBlkX, nBlkX, dummyplane);

            upsizer1->SimpleResizeDo(VXFullYFF, nWidth, nHeight, VPitchY, VXSmallYFF, nBlkX, nBlkX, dummyplane);
            upsizer1->SimpleResizeDo(VYFullYFF, nWidth, nHeight, VPitchY, VYSmallYFF, nBlkX, nBlkX, dummyplane);
            upsizer1UV->SimpleResizeDo(VXFullUVFF, nWidthUV, nHeightUV, VPitchUV, VXSmallUVFF, nBlkX, nBlkX, dummyplane);
            upsizer1UV->SimpleResizeDo(VYFullUVFF, nWidthUV, nHeightUV, VPitchUV, VYSmallUVFF, nBlkX, nBlkX, dummyplane);

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
        else if (maskmode==1) // bad extra frames, use old method without extra frames
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
        else // maskmode=0, faster simple method
        {

            PROFILE_START(MOTION_PROFILE_FLOWINTER);
            if (nPel>=2) {
                FlowInterSimple(pDst[0], nDstPitches[0], pel2PlaneYB + pel2OffsetY, pel2PlaneYF + pel2OffsetY, pel2PitchY,
                                VXFullYB, VXFullYF, VYFullYB, VYFullYF, MaskFullYB, MaskFullYF, VPitchY,
                                nWidth, nHeight, time256, nPel, LUTVB, LUTVF);
                FlowInterSimple(pDst[1], nDstPitches[1], pel2PlaneUB + pel2OffsetUV, pel2PlaneUF + pel2OffsetUV, pel2PitchUV,
                                VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, MaskFullUVB, MaskFullUVF, VPitchUV,
                                nWidthUV, nHeightUV, time256, nPel, LUTVB, LUTVF);
                FlowInterSimple(pDst[2], nDstPitches[2], pel2PlaneVB + pel2OffsetUV, pel2PlaneVF + pel2OffsetUV, pel2PitchUV,
                                VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, MaskFullUVB, MaskFullUVF, VPitchUV,
                                nWidthUV, nHeightUV, time256, nPel, LUTVB, LUTVF);
            }
            else if (nPel==1) {
                FlowInterSimple(pDst[0], nDstPitches[0], pPlanesB[0]->GetPointer(0,0), pPlanesF[0]->GetPointer(0,0), pPlanesB[0]->GetPitch(),
                                VXFullYB, VXFullYF, VYFullYB, VYFullYF, MaskFullYB, MaskFullYF, VPitchY,
                                nWidth, nHeight, time256, nPel, LUTVB, LUTVF);
                FlowInterSimple(pDst[1], nDstPitches[1], pPlanesB[1]->GetPointer(0,0), pPlanesF[1]->GetPointer(0,0), pPlanesB[1]->GetPitch(),
                                VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, MaskFullUVB, MaskFullUVF, VPitchUV,
                                nWidthUV, nHeightUV, time256, nPel, LUTVB, LUTVF);
                FlowInterSimple(pDst[2], nDstPitches[2], pPlanesB[2]->GetPointer(0,0), pPlanesF[2]->GetPointer(0,0), pPlanesB[2]->GetPitch(),
                                VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, MaskFullUVB, MaskFullUVF, VPitchUV,
                                nWidthUV, nHeightUV, time256, nPel, LUTVB, LUTVF);
            }
            PROFILE_STOP(MOTION_PROFILE_FLOWINTER);
        }
    }
    else {
        //	   return src;
        // poor estimation, let's blend src with ref frames

        PVideoFrame src	= child->GetFrame(nleft, env);
        PVideoFrame ref = child->GetFrame(nright, env);//  right frame for  compensation

        // convert to frames
        unsigned char const *pSrc[3], *pRef[3];
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

void MVFlowFps2::ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
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