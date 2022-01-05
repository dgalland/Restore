// Pixels flow motion function
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

#include "MVFlow.h"
#include "CopyCode.h"
#include "MaskFun.h"
#include "Padding.h"


MVFlow::MVFlow(PClip _child, PClip _mvec, int _time256, int _mode, bool _fields, PClip _pelclip, int _nIdx, 
               int nSCD1, int nSCD2, bool _mmx, bool _isse, IScriptEnvironment* env) :
        GenericVideoFilter(_child),
        MVFilter(_mvec, "MVFlow", env),
        mvClip(_mvec, nSCD1, nSCD2, env, true),
        pelclip(_pelclip)
{
    time256 = _time256;
    mode = _mode;
    nIdx = _nIdx;
    mmx = _mmx;
    isse = _isse;
    fields = _fields;

    mvCore->AddFrames(nIdx, 2, mvClip.GetLevelCount(), nWidth, nHeight, nPel, nHPadding, nVPadding, 
                      YUVPLANES, isse, yRatioUV);

    usePelClipHere = false;
    if (pelclip && (nPel >= 2)) {
        if (pelclip->GetVideoInfo().width != nWidth*nPel || pelclip->GetVideoInfo().height != nHeight*nPel)
            env->ThrowError("MVFlow: pelclip frame size must be Pel size of source!");
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
    //  char debugbuf[128];
    //   wsprintf(debugbuf,"MVFlow: nBlkX=%d, nOverlap=%d, nBlkXP=%d, nWidth=%d, nWidthP=%d, VPitchY=%d",nBlkX, nOverlap, nBlkXP, nWidth, nWidthP, VPitchY);
    //   OutputDebugString(debugbuf);

    VXFullY = new BYTE [nHeightP*VPitchY];
    VXFullUV = new BYTE [nHeightPUV*VPitchUV];

    VYFullY = new BYTE [nHeightP*VPitchY];
    VYFullUV = new BYTE [nHeightPUV*VPitchUV];

    VXSmallY = new BYTE [nBlkXP*nBlkYP];
    VYSmallY = new BYTE [nBlkXP*nBlkYP];
    VXSmallUV = new BYTE [nBlkXP*nBlkYP];
    VYSmallUV = new BYTE [nBlkXP*nBlkYP];


    int pel2WidthY = (nWidth + 2*nHPadding)*nPel; // and pitch
    pel2HeightY = (nHeight + 2*nVPadding)*nPel;
    int pel2WidthUV = (nWidthUV + 2*nHPaddingUV)*nPel;
    pel2HeightUV = (nHeightUV + 2*nVPaddingUV)*nPel;
    pel2PitchY = (pel2WidthY + 15) & (~15);
    pel2PitchUV = (pel2WidthUV + 15) & (~15);
    pel2OffsetY = pel2PitchY * nVPadding*nPel + nHPadding*nPel;
    pel2OffsetUV = pel2PitchUV * nVPaddingUV*nPel + nHPaddingUV*nPel;
    if (nPel>1) {
        pel2PlaneY = new BYTE [pel2PitchY*pel2HeightY];
        pel2PlaneU = new BYTE [pel2PitchUV*pel2HeightUV];
        pel2PlaneV = new BYTE [pel2PitchUV*pel2HeightUV];
    }

    int CPUF_Resize = env->GetCPUFlags();
    if (!isse) CPUF_Resize = (CPUF_Resize & !CPUF_INTEGER_SSE) & !CPUF_SSE2;

    upsizer = new SimpleResize(nWidthP, nHeightP, nBlkXP, nBlkYP, CPUF_Resize);
    upsizerUV = new SimpleResize(nWidthPUV, nHeightPUV, nBlkXP, nBlkYP, CPUF_Resize);

    Create_LUTV(time256, LUTV);

    DstPlanes = new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);
}

MVFlow::~MVFlow()
{
    if (DstPlanes) delete DstPlanes;

    if (upsizer)   delete upsizer;
    if (upsizerUV) delete upsizerUV;

    if (VXFullY)   delete [] VXFullY;
    if (VXFullUV)  delete [] VXFullUV;
    if (VYFullY)   delete [] VYFullY;
    if (VYFullUV)  delete [] VYFullUV;
    if (VXSmallY)  delete [] VXSmallY;
    if (VYSmallY)  delete [] VYSmallY;
    if (VXSmallUV) delete [] VXSmallUV;
    if (VYSmallUV) delete [] VYSmallUV;

    if (nPel>1) {
        if (pel2PlaneY) delete [] pel2PlaneY;
        if (pel2PlaneU) delete [] pel2PlaneU;
        if (pel2PlaneV) delete [] pel2PlaneV;
    }
}

void MVFlow::Create_LUTV(int time256, int *LUTV)
{
    for (int v=0; v<256; ++v)
        LUTV[v] = ((v-128)*time256)/256;
}

void MVFlow::Shift(BYTE * pdst, int dst_pitch, const BYTE *pref, int ref_pitch,  BYTE *VXFull, int VXPitch,  
                   BYTE *VYFull, int VYPitch, int width, int height, int time256)
{
    // shift mode
    if (nPel==1) {
        for (int h=0; h<height; h++) {
            for (int w=0; w<width; w++) {
                int vx = -((VXFull[w]-128)*time256)/256;
                int vy = -((VYFull[w]-128)*time256)/256;
                int href = h + vy;
                int wref = w + vx;
                if (href>=0 && href<height && wref>=0 && wref<width)// bound check if not padded
                    pdst[vy*dst_pitch + vx + w] = pref[w];
            }
            pref += ref_pitch;
            pdst += dst_pitch;
            VXFull += VXPitch;
            VYFull += VYPitch;
        }
    }
    else if (nPel==2) {
        for (int h=0; h<height; h++) {
            for (int w=0; w<width; w++) {
                // very simple half-pixel using,  must be by image interpolation really (later)
                int vx = -((VXFull[w]-128)*time256)/512;
                int vy = -((VYFull[w]-128)*time256)/512;
                int href = h + vy;
                int wref = w + vx;
                if (href>=0 && href<height && wref>=0 && wref<width)// bound check if not padded
                    pdst[vy*dst_pitch + vx + w] = pref[w];
            }
            pref += ref_pitch;
            pdst += dst_pitch;
            VXFull += VXPitch;
            VYFull += VYPitch;
        }
    }
    else if (nPel==4) {
        for (int h=0; h<height; h++) {
            for (int w=0; w<width; w++) {
                // very simple half-pixel using,  must be by image interpolation really (later)
                int vx = -((VXFull[w]-128)*time256)/1024;
                int vy = -((VYFull[w]-128)*time256)/1024;
                int href = h + vy;
                int wref = w + vx;
                if (href>=0 && href<height && wref>=0 && wref<width)// bound check if not padded
                    pdst[vy*dst_pitch + vx + w] = pref[w];
            }
            pref += ref_pitch;
            pdst += dst_pitch;
            VXFull += VXPitch;
            VYFull += VYPitch;
        }
    }
}

void MVFlow::Fetch(BYTE * pdst, int dst_pitch, const BYTE *pref, int ref_pitch,  BYTE *VXFull, int VXPitch,  
                   BYTE *VYFull, int VYPitch, int width, int height, int time256)
{
    // fetch mode
    if (nPel==1) {
        for (int h=0; h<height; h++) {
            for (int w=0; w<width; w++) {
                /*
                int vx = ((VXFull[w]-128)*time256)>>8; //fast but not quite correct
                int vy = ((VYFull[w]-128)*time256)>>8; // shift of negative values result in not same as division

                int vx = ((VXFull[w]-128)*time256)/256; //correct
                int vy = ((VYFull[w]-128)*time256)/256;
                
                int vx = VXFull[w]-128;
                if (vx < 0) //vx =+;
                    vx = -((-vx*time256)>>8);
                else
                    vx = (vx*time256)>>8;

                int vy = VYFull[w]-128;
                if (vy < 0)	//vy++;
                    vy = -((-vy*time256)>>8);
                else
                    vy = (vy*time256)>>8;
                */
                int vx = LUTV[VXFull[w]];
                int vy = LUTV[VYFull[w]];

                pdst[w] = pref[vy*ref_pitch + vx + w];
            }
            pref += ref_pitch;
            pdst += dst_pitch;
            VXFull += VXPitch;
            VYFull += VYPitch;
        }
    }
    else if (nPel==2) {
        for (int h=0; h<height; h++) {
            for (int w=0; w<width; w++)  {
                // use interpolated image

                /*
                int vx = ((VXFull[w]-128)*time256)>>8;
                int vy = ((VYFull[w]-128)*time256)>>8;

                int vx = ((VXFull[w]-128)*time256)/256; //correct
                int vy = ((VYFull[w]-128)*time256)/256;

                int vx = VXFull[w]-128;
                if (vx < 0) //	vx++;
                    vx = -((-vx*time256)>>8);
                else
                    vx = (vx*time256)>>8;

                int vy = VYFull[w]-128;
                if (vy < 0) //	vy++;
                    vy = -((-vy*time256)>>8);
                else
                    vy = (vy*time256)>>8;
                */
                int vx = LUTV[VXFull[w]];
                int vy = LUTV[VYFull[w]];

                pdst[w] = pref[vy*ref_pitch + vx + (w<<1)];
            }
            pref += (ref_pitch)<<1;
            pdst += dst_pitch;
            VXFull += VXPitch;
            VYFull += VYPitch;
        }
    }
    else if (nPel==4) {
        for (int h=0; h<height; h++) {
            for (int w=0; w<width; w++) {
                // use interpolated image

                /*
                int vx = ((VXFull[w]-128)*time256)>>8;
                int vy = ((VYFull[w]-128)*time256)>>8;

                int vx = ((VXFull[w]-128)*time256)/256; //correct
                int vy = ((VYFull[w]-128)*time256)/256;

                int vx = VXFull[w]-128;
                if (vx < 0) //	vx++;
                    vx = -((-vx*time256)>>8);
                else
                    vx = (vx*time256)>>8;

                int vy = VYFull[w]-128;
                if (vy < 0) //	vy++;
                    vy = -((-vy*time256)>>8);
                else
                    vy = (vy*time256)>>8;
                */
                int vx = LUTV[VXFull[w]];
                int vy = LUTV[VYFull[w]];

                pdst[w] = pref[vy*ref_pitch + vx + (w<<2)];
            }
            pref += (ref_pitch)<<2;
            pdst += dst_pitch;
            VXFull += VXPitch;
            VYFull += VYPitch;
        }
    }
}

//-------------------------------------------------------------------------
PVideoFrame __stdcall MVFlow::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src	= child->GetFrame(n, env);
    bool paritySrc = child->GetParity(n);

    int nref;
    int off = mvClip.GetDeltaFrame(); // integer offset of reference frame
    if ( mvClip.IsBackward() ) {
        nref = n + off;
    }
    else {
        nref = n - off;
    }

    PVideoFrame mvn = mvClip.GetFrame(n, env);
    mvClip.Update(mvn, env);// backward from next to current

    int sharp = mvClip.GetSharp();

    if ( mvClip.IsUsable()) {
        // get reference frames
        PMVGroupOfFrames pRefGOF = mvCore->GetFrame(nIdx, nref);
        if (!pRefGOF->IsProcessed()) {
            PVideoFrame ref, ref2x;
            ref = child->GetFrame(nref, env);
            pRefGOF->SetParity(child->GetParity(nref));
            if (usePelClipHere)
                ref2x= pelclip->GetFrame(nref, env);

            ProcessFrameIntoGroupOfFrames(&pRefGOF, &ref, &ref2x, sharp, pixelType, nHeight, nWidth, nPel, isse);
        }

        PVideoFrame dst = env->NewVideoFrame(vi);
        // convert to frames
        unsigned char *pDst[3];
        int nDstPitches[3];
        DstPlanes->ConvertVideoFrameToPlanes(&dst, pDst, nDstPitches);

        MVPlane *pPlanes[3];

        pPlanes[0] = pRefGOF->GetFrame(0)->GetPlane(YPLANE);
        pPlanes[1] = pRefGOF->GetFrame(0)->GetPlane(UPLANE);
        pPlanes[2] = pRefGOF->GetFrame(0)->GetPlane(VPLANE);

        if (nPel>=2 ){
            if (usePelClipHere) { // simply padding 2x or 4x planes
                PlaneCopy(pel2PlaneY + pel2OffsetY, pel2PitchY,
                          pRefGOF->GetVFYPtr(), pRefGOF->GetVFYPitch(), nWidth*nPel, nHeight*nPel, isse);
                Padding::PadReferenceFrame(pel2PlaneY, pel2PitchY, nHPadding*nPel, nVPadding*nPel, nWidth*nPel, 
                                           nHeight*nPel);
                PlaneCopy(pel2PlaneU + pel2OffsetUV, pel2PitchUV,
                          pRefGOF->GetVFUPtr(), pRefGOF->GetVFUPitch(), nWidthUV*nPel, nHeightUV*nPel, isse);
                Padding::PadReferenceFrame(pel2PlaneU, pel2PitchUV, nHPaddingUV*nPel, nVPaddingUV*nPel, nWidthUV*nPel, 
                                           nHeightUV*nPel);
                PlaneCopy(pel2PlaneV + pel2OffsetUV, pel2PitchUV,
                          pRefGOF->GetVFVPtr(), pRefGOF->GetVFVPitch(), nWidthUV*nPel, nHeightUV*nPel, isse);
                Padding::PadReferenceFrame(pel2PlaneV, pel2PitchUV, nHPaddingUV*nPel, nVPaddingUV*nPel, nWidthUV*nPel, 
                                           nHeightUV*nPel);
            }
            else if (nPel==2) {
                // merge refined planes to big single plane
                Merge4PlanesToBig(pel2PlaneY, pel2PitchY, pPlanes[0]->GetAbsolutePointer(0,0),
                                  pPlanes[0]->GetAbsolutePointer(1,0), pPlanes[0]->GetAbsolutePointer(0,1),
                                  pPlanes[0]->GetAbsolutePointer(1,1), pPlanes[0]->GetExtendedWidth(),
                                  pPlanes[0]->GetExtendedHeight(), pPlanes[0]->GetPitch(), isse);
                Merge4PlanesToBig(pel2PlaneU, pel2PitchUV, pPlanes[1]->GetAbsolutePointer(0,0),
                                  pPlanes[1]->GetAbsolutePointer(1,0), pPlanes[1]->GetAbsolutePointer(0,1),
                                  pPlanes[1]->GetAbsolutePointer(1,1), pPlanes[1]->GetExtendedWidth(),
                                  pPlanes[1]->GetExtendedHeight(), pPlanes[1]->GetPitch(), isse);
                Merge4PlanesToBig(pel2PlaneV, pel2PitchUV, pPlanes[2]->GetAbsolutePointer(0,0),
                                  pPlanes[2]->GetAbsolutePointer(1,0), pPlanes[2]->GetAbsolutePointer(0,1),
                                  pPlanes[2]->GetAbsolutePointer(1,1), pPlanes[2]->GetExtendedWidth(),
                                  pPlanes[2]->GetExtendedHeight(), pPlanes[2]->GetPitch(), isse);
            }
            else if (nPel==4) {
                // merge refined planes to big single plane
                Merge16PlanesToBig(pel2PlaneY, pel2PitchY,
                                   pPlanes[0]->GetAbsolutePointer(0,0), pPlanes[0]->GetAbsolutePointer(1,0),
                                   pPlanes[0]->GetAbsolutePointer(2,0), pPlanes[0]->GetAbsolutePointer(3,0),
                                   pPlanes[0]->GetAbsolutePointer(1,0), pPlanes[0]->GetAbsolutePointer(1,1),
                                   pPlanes[0]->GetAbsolutePointer(1,2), pPlanes[0]->GetAbsolutePointer(1,3),
                                   pPlanes[0]->GetAbsolutePointer(2,0), pPlanes[0]->GetAbsolutePointer(2,1),
                                   pPlanes[0]->GetAbsolutePointer(2,2), pPlanes[0]->GetAbsolutePointer(2,3),
                                   pPlanes[0]->GetAbsolutePointer(3,0), pPlanes[0]->GetAbsolutePointer(3,1),
                                   pPlanes[0]->GetAbsolutePointer(3,2), pPlanes[0]->GetAbsolutePointer(3,3),
                                   pPlanes[0]->GetExtendedWidth(), pPlanes[0]->GetExtendedHeight(), pPlanes[0]->GetPitch(), isse);
                Merge16PlanesToBig(pel2PlaneU, pel2PitchUV,
                                   pPlanes[1]->GetAbsolutePointer(0,0), pPlanes[1]->GetAbsolutePointer(1,0),
                                   pPlanes[1]->GetAbsolutePointer(2,0), pPlanes[1]->GetAbsolutePointer(3,0),
                                   pPlanes[1]->GetAbsolutePointer(1,0), pPlanes[1]->GetAbsolutePointer(1,1),
                                   pPlanes[1]->GetAbsolutePointer(1,2), pPlanes[1]->GetAbsolutePointer(1,3),
                                   pPlanes[1]->GetAbsolutePointer(2,0), pPlanes[1]->GetAbsolutePointer(2,1),
                                   pPlanes[1]->GetAbsolutePointer(2,2), pPlanes[1]->GetAbsolutePointer(2,3),
                                   pPlanes[1]->GetAbsolutePointer(3,0), pPlanes[1]->GetAbsolutePointer(3,1),
                                   pPlanes[1]->GetAbsolutePointer(3,2), pPlanes[1]->GetAbsolutePointer(3,3),
                                   pPlanes[1]->GetExtendedWidth(), pPlanes[1]->GetExtendedHeight(), pPlanes[1]->GetPitch(), isse);
                Merge16PlanesToBig(pel2PlaneV, pel2PitchUV,
                                   pPlanes[2]->GetAbsolutePointer(0,0), pPlanes[2]->GetAbsolutePointer(1,0),
                                   pPlanes[2]->GetAbsolutePointer(2,0), pPlanes[2]->GetAbsolutePointer(3,0),
                                   pPlanes[2]->GetAbsolutePointer(1,0), pPlanes[2]->GetAbsolutePointer(1,1),
                                   pPlanes[2]->GetAbsolutePointer(1,2), pPlanes[2]->GetAbsolutePointer(1,3),
                                   pPlanes[2]->GetAbsolutePointer(2,0), pPlanes[2]->GetAbsolutePointer(2,1),
                                   pPlanes[2]->GetAbsolutePointer(2,2), pPlanes[2]->GetAbsolutePointer(2,3),
                                   pPlanes[2]->GetAbsolutePointer(3,0), pPlanes[2]->GetAbsolutePointer(3,1),
                                   pPlanes[2]->GetAbsolutePointer(3,2), pPlanes[2]->GetAbsolutePointer(3,3),
                                   pPlanes[2]->GetExtendedWidth(), pPlanes[2]->GetExtendedHeight(), pPlanes[2]->GetPitch(), isse);
            }
        }

        //		 char debugbuf[100];
        //		 for (int V=100; V<145; V++)
        //		 {
        //			 int vx1 = ((V-128)*time256)>>8;
        //				int vx2 = ((V*time256+127)>>8) - (time256>>1);
        //				int vx3 = ((V*time256)/256) - (time256/2);
        //				int vx4 = ((V-128)*time256)/256;
        //		 		int vx5 = V-128;
        //				if (vx5 < 0) vx5++;
        //				vx5 = (vx5*time256)>>8;
        //
        //			 sprintf(debugbuf,"MVFlow: V=%d %d %d %d %d %d", V,vx1,vx2,vx3,vx4,vx5);
        //			 OutputDebugString(debugbuf);
        //		 }
        //
        // make  vector vx and vy small masks
        // 1. ATTENTION: vectors are assumed SHORT (|vx|, |vy| < 127) !
        // 2. they will be zeroed if not
        // 3. added 128 to all values
        MakeVectorSmallMasks(mvClip, nBlkX, nBlkY, VXSmallY, nBlkXP, VYSmallY, nBlkXP);
        if (nBlkXP > nBlkX) // fill right
        {
            for (int j=0; j<nBlkY; j++) {
                VXSmallY[j*nBlkXP + nBlkX] = std::min<BYTE>(VXSmallY[j*nBlkXP + nBlkX-1],128);
                VYSmallY[j*nBlkXP + nBlkX] = VYSmallY[j*nBlkXP + nBlkX-1];
            }
        }
        if (nBlkYP > nBlkY) // fill bottom
        {
            for (int i=0; i<nBlkXP; i++) {
                VXSmallY[nBlkXP*nBlkY +i] = VXSmallY[nBlkXP*(nBlkY-1) +i];
                VYSmallY[nBlkXP*nBlkY +i] = std::min<BYTE>(VYSmallY[nBlkXP*(nBlkY-1) +i],128);
            }
        }

        int fieldShift = 0;
        if (fields && (nPel >= 2) && (off %2 != 0)) {
            fieldShift = (paritySrc && !pRefGOF->Parity()) ? (nPel/2) : ((pRefGOF->Parity() && !paritySrc) ? -(nPel/2) : 0);
            // vertical shift of fields for fieldbased video at finest level pel2
        }

        for (int j=0; j<nBlkYP; j++) {
            for (int i=0; i<nBlkXP; i++) {
            VYSmallY[j*nBlkXP + i] += fieldShift;
            }
        }

        VectorSmallMaskYToHalfUV(VXSmallY, nBlkXP, nBlkYP, VXSmallUV, 2);
        VectorSmallMaskYToHalfUV(VYSmallY, nBlkXP, nBlkYP, VYSmallUV, yRatioUV);

        // upsize (bilinear interpolate) vector masks to fullframe size

        int dummyplane = PLANAR_Y; // use luma plane resizer code for all planes if we resize from luma small mask
        upsizer->SimpleResizeDo(VXFullY, nWidthP, nHeightP, VPitchY, VXSmallY, nBlkXP, nBlkXP, dummyplane);
        upsizer->SimpleResizeDo(VYFullY, nWidthP, nHeightP, VPitchY, VYSmallY, nBlkXP, nBlkXP, dummyplane);
        upsizerUV->SimpleResizeDo(VXFullUV, nWidthPUV, nHeightPUV, VPitchUV, VXSmallUV, nBlkXP, nBlkXP, dummyplane);
        upsizerUV->SimpleResizeDo(VYFullUV, nWidthPUV, nHeightPUV, VPitchUV, VYSmallUV, nBlkXP, nBlkXP, dummyplane);


        if (mode==0) // fetch mode
        {
            if (nPel>=2)  {
                Fetch(pDst[0], nDstPitches[0], pel2PlaneY + pel2OffsetY, pel2PitchY, VXFullY, VPitchY, VYFullY, 
                      VPitchY, nWidth, nHeight, time256); //padded
                Fetch(pDst[1], nDstPitches[1], pel2PlaneU + pel2OffsetUV, pel2PitchUV, VXFullUV, VPitchUV, VYFullUV, 
                      VPitchUV, nWidthUV, nHeightUV, time256);
                Fetch(pDst[2], nDstPitches[2], pel2PlaneV + pel2OffsetUV, pel2PitchUV, VXFullUV, VPitchUV, VYFullUV, 
                      VPitchUV, nWidthUV, nHeightUV, time256);
            }
            else //(nPel==1)
            {
                Fetch(pDst[0], nDstPitches[0], pPlanes[0]->GetPointer(0,0), pPlanes[0]->GetPitch(), VXFullY, VPitchY, VYFullY, 
                      VPitchY, nWidth, nHeight, time256); //padded
                Fetch(pDst[1], nDstPitches[1], pPlanes[1]->GetPointer(0,0), pPlanes[1]->GetPitch(), VXFullUV, VPitchUV, VYFullUV, 
                      VPitchUV, nWidthUV, nHeightUV, time256);
                Fetch(pDst[2], nDstPitches[2], pPlanes[2]->GetPointer(0,0), pPlanes[2]->GetPitch(), VXFullUV, VPitchUV, VYFullUV, 
                      VPitchUV, nWidthUV, nHeightUV, time256);
            }
        }
        else if (mode==1) // shift mode
        {
            MemZoneSet(pDst[0], 0, nWidth, nHeight, 0, 0, nDstPitches[0]);
            MemZoneSet(pDst[1], 0, nWidthUV, nHeightUV, 0, 0, nDstPitches[1]);
            MemZoneSet(pDst[2], 0, nWidthUV, nHeightUV, 0, 0, nDstPitches[2]);
            Shift(pDst[0], nDstPitches[0], pPlanes[0]->GetPointer(0,0), pPlanes[0]->GetPitch(), 
                  VXFullY, VPitchY, VYFullY, VPitchY, nWidth, nHeight, time256);
            Shift(pDst[1], nDstPitches[1], pPlanes[1]->GetPointer(0,0), pPlanes[1]->GetPitch(), 
                  VXFullUV, VPitchUV, VYFullUV, VPitchUV, nWidthUV, nHeightUV, time256);
            Shift(pDst[2], nDstPitches[2], pPlanes[2]->GetPointer(0,0), pPlanes[2]->GetPitch(), 
                  VXFullUV, VPitchUV, VYFullUV, VPitchUV, nWidthUV, nHeightUV, time256);
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

void MVFlow::ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
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