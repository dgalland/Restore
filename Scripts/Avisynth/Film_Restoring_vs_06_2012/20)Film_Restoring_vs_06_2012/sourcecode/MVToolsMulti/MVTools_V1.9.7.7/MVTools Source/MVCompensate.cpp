// Make a motion compensate temporal denoiser
// Author: Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick (YUY2, overlap, edges processing)
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

#include "MVCompensate.h"
#include "Padding.h"
#include "Interpolation.h"
#include "commonfunctions.h"

MVCompensate::MVCompensate(PClip _child, PClip vectors, bool sc, int _mode, int _thres, int _thresC, bool _fields, PClip _pelclip, 
                           int _nIdx, int nSCD1, int nSCD2, bool _mmx, bool _isse, IScriptEnvironment* env) :
              GenericVideoFilter(_child),
              mvClip(vectors, nSCD1, nSCD2, env, true),
              MVFilter(vectors, "MVCompensate", env),
              pelclip(_pelclip)
{
    isse = _isse;
    mmx = _mmx;
    scBehavior = sc;
    mode = _mode;
    fields = _fields;

    if (mode==0) mode = 1; // for compatibility
    if (mode==2)
        env->ThrowError("MVCompensate: mode=2 is not supported");

    if (fields && nPel<2 && !vi.IsFieldBased())
        env->ThrowError("MVCompensate: fields option is for fieldbased video and pel > 1");

    nIdx = _nIdx;
    thSAD  = _thres*mvClip.GetThSCD1()/nSCD1; // normalize to block SAD
    thSADC = _thresC*mvClip.GetThSCD1()/nSCD1; // normalize to block SAD

    if (isse) {
        switch (nBlkSizeX)
        {
        case 32:
            if (nBlkSizeY==16) {
                BLITLUMA = Copy_C<32,16>;
                OVERSLUMA = Overlaps32x16_mmx;
                if (yRatioUV==2) {
                    BLITCHROMA = Copy_C<16,8>; // idem
                    OVERSCHROMA = Overlaps16x8_mmx;
                } else {
                    BLITCHROMA = Copy_C<16, 16>; // idem
                    OVERSCHROMA = Overlaps16x16_mmx;
                }
            } 
            break;
        case 16:
            if (nBlkSizeY==16) {
                BLITLUMA = Copy_C<16, 16>;
                OVERSLUMA = Overlaps16x16_mmx;
                if (yRatioUV==2) {
                    BLITCHROMA = Copy_C<8, 8>; // idem
                    OVERSCHROMA = Overlaps8x8_mmx;
                } else {
                    BLITCHROMA = Copy_C<8,16>; // idem
                    OVERSCHROMA = Overlaps8x16_mmx;
                }
            } else if (nBlkSizeY==8) {
                BLITLUMA = Copy_C<16,8>;
                OVERSLUMA = Overlaps16x8_mmx;
                if (yRatioUV==2) {
                    BLITCHROMA = Copy_C<8,4>; // idem
                    OVERSCHROMA = Overlaps8x4_mmx;
                } else {
                    BLITCHROMA = Copy_C<8, 8>; // idem
                    OVERSCHROMA = Overlaps8x8_mmx;
                }
            }
            break;
        case 4:
            BLITLUMA = Copy_C<4, 4>; // "mmx" version could be used, but it's more like a debugging version
            OVERSLUMA = Overlaps4x4_mmx;
            if (yRatioUV==2) {
                BLITCHROMA = Copy_C<2, 2>; // idem
                OVERSCHROMA = Overlaps2x2_mmx;
            } else {
                BLITCHROMA = Copy_C<2,4>; // idem
                OVERSCHROMA = Overlaps2x4_mmx;
            }
            break;
        case 8:
        default:
            if (nBlkSizeY==8) {
                BLITLUMA = Copy_C<8, 8>;
                OVERSLUMA = Overlaps8x8_mmx;
                if (yRatioUV==2) {
                    BLITCHROMA = Copy_C<4, 4>; // idem
                    OVERSCHROMA = Overlaps4x4_mmx;
                }
                else {
                    BLITCHROMA = Copy_C<4,8>; // idem
                    OVERSCHROMA = Overlaps4x8_mmx;
                }
            } else if (nBlkSizeY==4) { // 8x4
                BLITLUMA = Copy_C<8,4>;
                OVERSLUMA = Overlaps8x4_mmx;
                if (yRatioUV==2) {
                    BLITCHROMA = Copy_C<4,2>; // idem
                    OVERSCHROMA = Overlaps4x2_mmx;
                } else {
                    BLITCHROMA = Copy_C<4, 4>; // idem
                    OVERSCHROMA =Overlaps4x4_mmx;
                }
            }
            break;
        }
    }
    else {
        switch (nBlkSizeX)
        {
        case 32:
            if (nBlkSizeY==16) {
                BLITLUMA = Copy_C<32,16>;
                OVERSLUMA = Overlaps_C<32, 16>;
                if (yRatioUV==2) {
                    BLITCHROMA = Copy_C<16,8>; // idem
                    OVERSCHROMA = Overlaps_C<16, 8>;
                } else {
                    BLITCHROMA = Copy_C<16, 16>; // idem
                    OVERSCHROMA = Overlaps_C<16, 16>;
                }
            } 
            break;
        case 16:
            if (nBlkSizeY==16) {
                BLITLUMA = Copy_C<16, 16>;
                OVERSLUMA = Overlaps_C<16, 16>;
                if (yRatioUV==2) {
                    BLITCHROMA = Copy_C<8, 8>; // idem
                    OVERSCHROMA = Overlaps_C<8, 8>;
                } else {
                    BLITCHROMA = Copy_C<8,16>; // idem
                    OVERSCHROMA = Overlaps_C<8, 16>;
                }
            } else if (nBlkSizeY==8) {
                BLITLUMA = Copy_C<16,8>;
                OVERSLUMA = Overlaps_C<16, 8>;
                if (yRatioUV==2) {
                    BLITCHROMA = Copy_C<8,4>; // idem
                    OVERSCHROMA = Overlaps_C<8, 4>;
                } else {
                    BLITCHROMA = Copy_C<8, 8>; // idem
                    OVERSCHROMA = Overlaps_C<8, 8>;
                }
            }
            break;
        case 4:
            BLITLUMA = Copy_C<4, 4>; // "mmx" version could be used, but it's more like a debugging version
            OVERSLUMA = Overlaps_C<4, 4>;
            if (yRatioUV==2) {
                BLITCHROMA = Copy_C<2, 2>; // idem
                OVERSCHROMA = Overlaps_C<2, 2>;
            } else {
                BLITCHROMA = Copy_C<2,4>; // idem
                OVERSCHROMA = Overlaps_C<2, 4>;
            }
            break;
        case 8:
        default:
            if (nBlkSizeY==8) {
                BLITLUMA = Copy_C<8, 8>;
                OVERSLUMA = Overlaps_C<8, 8>;
                if (yRatioUV==2) {
                    BLITCHROMA = Copy_C<4, 4>; // idem
                    OVERSCHROMA = Overlaps_C<4, 4>;
                }
                else {
                    BLITCHROMA = Copy_C<4,8>; // idem
                    OVERSCHROMA = Overlaps_C<4, 8>;
                }
            } else if (nBlkSizeY==4) { // 8x4
                BLITLUMA = Copy_C<8,4>;
                OVERSLUMA = Overlaps_C<8, 4>;
                if (yRatioUV==2) {
                    BLITCHROMA = Copy_C<4,2>; // idem
                    OVERSCHROMA = Overlaps_C<4, 2>;
                } else {
                    BLITCHROMA = Copy_C<4, 4>; // idem
                    OVERSCHROMA = Overlaps_C<4, 4>;
                }
            }
            break;
        }
    }
	usePelClipHere = false;
    if (pelclip && (nPel > 1)) {
        if (pelclip->GetVideoInfo().width != nWidth*nPel || pelclip->GetVideoInfo().height != nHeight*nPel)
            env->ThrowError("MVCompensate: pelclip frame size must be Pel of source!");
        else
            usePelClipHere = true;
    }

    mvCore->AddFrames(nIdx, 2, mvClip.GetLevelCount(), nWidth, nHeight, nPel, nHPadding, nVPadding,
                      YUVPLANES, isse, yRatioUV);
    DstPlanes =  new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);

    dstShortPitch = ((nWidth + 15)/16)*16;
    dstShortSize  = dstShortPitch*nHeight;
    if (nOverlapX >0 || nOverlapY>0) {
        OverWins = new OverlapWindows(nBlkSizeX, nBlkSizeY, nOverlapX, nOverlapY);
        OverWinsUV = new OverlapWindows(nBlkSizeX/2, nBlkSizeY/yRatioUV, nOverlapX/2, nOverlapY/yRatioUV);
        DstShort = new unsigned short[3*dstShortSize];
    }
}

MVCompensate::~MVCompensate()
{
    if (DstPlanes) delete DstPlanes;

    if (nOverlapX >0 || nOverlapY >0) {
        if (OverWins) delete OverWins;
        if (OverWinsUV) delete OverWinsUV;
        if (DstShort) delete [] DstShort;
    }
}


PVideoFrame __stdcall MVCompensate::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src	= child->GetFrame(n, env);
    PVideoFrame dst = env->NewVideoFrame(vi);

    int ySubUV = (yRatioUV == 2) ? 1 : 0;
    int nLogPel = ilog2(nPel); //shift

    PVideoFrame mvn = mvClip.GetFrame(n, env);
    mvClip.Update(mvn, env);

    int offset = ( mvClip.IsBackward() ) ? 1 : -1;
    offset *= mvClip.GetDeltaFrame();

    int sharp = mvClip.GetSharp();

    if ( mvClip.IsUsable() ) {
        // convert to frames
        unsigned char *pDst[3];
        int nDstPitches[3];
        DstPlanes->ConvertVideoFrameToPlanes(&dst, pDst, nDstPitches);

        // get source frame
        PMVGroupOfFrames pSrcGOF = mvCore->GetFrame(nIdx, n);
        if (!pSrcGOF->IsProcessed()) {
            PVideoFrame src2x;
            src = child->GetFrame(n, env);
            pSrcGOF->SetParity(child->GetParity(n));
            if (usePelClipHere) 
	            src2x = pelclip->GetFrame(n, env);
            ProcessFrameIntoGroupOfFrames(&pSrcGOF, &src, &src2x, sharp, pixelType, nHeight, nWidth, nPel, isse);
        }

        // get reference frames
        int FrameNum=n+offset;
        PMVGroupOfFrames pRefGOF = mvCore->GetFrame(nIdx, FrameNum);
        if (!pRefGOF->IsProcessed()) {
            PVideoFrame ref, ref2x;
            ref = child->GetFrame(FrameNum, env);
            pRefGOF->SetParity(child->GetParity(FrameNum));
            if (usePelClipHere)
                ref2x= pelclip->GetFrame(FrameNum, env);

            ProcessFrameIntoGroupOfFrames(&pRefGOF, &ref, &ref2x, sharp, pixelType, nHeight, nWidth, nPel, isse);
        }

        MVPlane *pPlanes[3], *pSrcPlanes[3];

        pPlanes[0] = pRefGOF->GetFrame(0)->GetPlane(YPLANE);
        pPlanes[1] = pRefGOF->GetFrame(0)->GetPlane(UPLANE);
        pPlanes[2] = pRefGOF->GetFrame(0)->GetPlane(VPLANE);
        pSrcPlanes[0] = pSrcGOF->GetFrame(0)->GetPlane(YPLANE);
        pSrcPlanes[1] = pSrcGOF->GetFrame(0)->GetPlane(UPLANE);
        pSrcPlanes[2] = pSrcGOF->GetFrame(0)->GetPlane(VPLANE);

        PROFILE_START(MOTION_PROFILE_COMPENSATION);

        unsigned char *pDstCur[3];
        pDstCur[0] = pDst[0];
        pDstCur[1] = pDst[1];
        pDstCur[2] = pDst[2];

        int fieldShift = 0;
        if (fields && nPel > 1 && (offset %2 != 0)) {
            fieldShift = (pSrcGOF->Parity() && !pRefGOF->Parity()) ? 
                            nPel/2 : ((pRefGOF->Parity()  && !pSrcGOF->Parity()) ? -(nPel/2) : 0);
            // vertical shift of fields for fieldbased video at finest level pel2
        }

        int nWidth_B = nBlkX*(nBlkSizeX - nOverlapX) + nOverlapX;
        int nHeight_B = nBlkY*(nBlkSizeY - nOverlapY) + nOverlapY;

        // -----------------------------------------------------------------------------
        if (nOverlapX==0 && nOverlapY==0) {
            for (int by=0; by<nBlkY; ++by) {
                int xx = 0;
                int blx=0, bly=0;
                for (int bx=0; bx<nBlkX; ++bx) {
                    int i = by*nBlkX + bx;
                    const FakeBlockData &block = mvClip.GetBlock(0, i);
                    blx = block.GetX() * nPel + block.GetMV().x;
                    bly = block.GetY() * nPel + block.GetMV().y + fieldShift;
                    if (block.GetSAD() < thSAD) {
                        // luma
                        BLITLUMA(pDstCur[0] + xx, nDstPitches[0], pPlanes[0]->GetPointer(blx, bly), pPlanes[0]->GetPitch(), isse);
                    }
                    else
                    {
                        int blxsrc = bx * (nBlkSizeX) * nPel;
                        int blysrc = by * (nBlkSizeY) * nPel  + fieldShift;

                        BLITLUMA(pDstCur[0] + xx, nDstPitches[0], pSrcPlanes[0]->GetPointer(blxsrc, blysrc), 
                                 pSrcPlanes[0]->GetPitch(), isse);
                    }
                    if (block.GetSAD() < thSADC) {
                        // chroma u
                        BLITCHROMA(pDstCur[1] + (xx>>1), nDstPitches[1], pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV), 
                                   pPlanes[1]->GetPitch(), isse);
                        // chroma v
                        BLITCHROMA(pDstCur[2] + (xx>>1), nDstPitches[2], pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV), 
                                   pPlanes[2]->GetPitch(), isse);
                    }
                    else
                    {
                        int blxsrc = bx * (nBlkSizeX) * nPel;
                        int blysrc = by * (nBlkSizeY) * nPel  + fieldShift;

                        // chroma u
                        BLITCHROMA(pDstCur[1] + (xx>>1), nDstPitches[1], pSrcPlanes[1]->GetPointer(blxsrc>>1, blysrc>>ySubUV), 
                                   pSrcPlanes[1]->GetPitch(), isse);
                        // chroma v
                        BLITCHROMA(pDstCur[2] + (xx>>1), nDstPitches[2], pSrcPlanes[2]->GetPointer(blxsrc>>1, blysrc>>ySubUV), 
                                   pSrcPlanes[2]->GetPitch(), isse);
                    }
                    xx += (nBlkSizeX);
                }
                if (nWidth_B < nWidth) // right non-covered region
                {
                    int lastx = std::min<int>(nBlkSizeX, pPlanes[0]->GetWidth() - (blx>>nLogPel));
                    // luma
                    PlaneCopy(pDstCur[0] + nWidth_B, nDstPitches[0], pPlanes[0]->GetPointer(blx, bly) + lastx,
                              pPlanes[0]->GetPitch(), nWidth-nWidth_B, nBlkSizeY, isse);
                    // chroma u
                    PlaneCopy(pDstCur[1] + (nWidth_B>>1), nDstPitches[1], pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1),
                              pPlanes[1]->GetPitch(), (nWidth-nWidth_B)>>1, nBlkSizeY>>ySubUV, isse);
                    // chroma v
                    PlaneCopy(pDstCur[2] + (nWidth_B>>1), nDstPitches[2], pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1),
                              pPlanes[2]->GetPitch(), (nWidth-nWidth_B)>>1, nBlkSizeY>>ySubUV, isse);
                }
                pDstCur[0] += (nBlkSizeY) * (nDstPitches[0]);
                pDstCur[1] += ( nBlkSizeY>>ySubUV) * (nDstPitches[1]);
                pDstCur[2] += ( nBlkSizeY>>ySubUV) * (nDstPitches[2]) ;

                if (by == nBlkY-1 && nHeight_B < nHeight) // bottom uncovered region
                {
                    xx = 0;
                    for (int bx=0; bx<nBlkX; ++bx) {
                        int i = by*nBlkX + bx;
                        const FakeBlockData &block = mvClip.GetBlock(0, i);
                        blx = block.GetX() * nPel + block.GetMV().x;
                        bly = block.GetY() * nPel + block.GetMV().y + fieldShift;

                        int lasty = std::min<int>(nBlkSizeY, pPlanes[0]->GetHeight() - (bly>>nLogPel));

                        // luma
                        PlaneCopy(pDstCur[0] + xx, nDstPitches[0],
                                  pPlanes[0]->GetPointer(blx, bly) + lasty*pPlanes[0]->GetPitch(),
                                  pPlanes[0]->GetPitch(), nBlkSizeX, nHeight-nHeight_B, isse);
                        // chroma u
                        PlaneCopy(pDstCur[1] + (xx>>1), nDstPitches[1],
                                  pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV)+ (lasty>>ySubUV)*pPlanes[1]->GetPitch(),
                                  pPlanes[1]->GetPitch(), (nBlkSizeX)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
                        // chroma v
                        PlaneCopy(pDstCur[2] + (xx>>1), nDstPitches[2],
                                  pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV)+ (lasty>>ySubUV)*pPlanes[2]->GetPitch(),
                                  pPlanes[2]->GetPitch(), (nBlkSizeX)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
                        xx += (nBlkSizeX);
                    }
                    if (nWidth_B < nWidth) // right bottom non-covered region
                    {
                        int lastx = std::min<int>(nBlkSizeX, pPlanes[0]->GetWidth() - (blx>>nLogPel));
                        int lasty = std::min<int>(nBlkSizeY, pPlanes[0]->GetHeight() - (bly>>nLogPel));
                        // luma
                        PlaneCopy(pDstCur[0] + nWidth_B, nDstPitches[0],
                                  pPlanes[0]->GetPointer(blx, bly) + lastx + lasty*pPlanes[0]->GetPitch(),
                                  pPlanes[0]->GetPitch(), nWidth-nWidth_B, nHeight-nHeight_B, isse);
                        // chroma u
                        PlaneCopy(pDstCur[1] + (nWidth_B>>1), nDstPitches[1],
                                  pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1) + (lasty>>ySubUV)*pPlanes[1]->GetPitch(),
                                  pPlanes[1]->GetPitch(), (nWidth-nWidth_B)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
                        // chroma v
                        PlaneCopy(pDstCur[2] + (nWidth_B>>1), nDstPitches[2],
                                  pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1) + (lasty>>ySubUV)*pPlanes[2]->GetPitch(),
                                  pPlanes[2]->GetPitch(), (nWidth-nWidth_B)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
                    }
                }
            }
        }
        // -----------------------------------------------------------------
        else // overlap
        {
            unsigned short *pDstShort = DstShort;
            MemZoneSet(reinterpret_cast<unsigned char*>(pDstShort), 0, nWidth_B*2, nHeight_B, 0, 0, dstShortPitch*2);
            unsigned short *pDstShortU = DstShort+dstShortSize;
            MemZoneSet(reinterpret_cast<unsigned char*>(pDstShortU), 0, nWidth_B, nHeight_B>>ySubUV, 0, 0, dstShortPitch*2);
            unsigned short *pDstShortV = DstShort+dstShortSize*2;
            MemZoneSet(reinterpret_cast<unsigned char*>(pDstShortV), 0, nWidth_B, nHeight_B>>ySubUV, 0, 0, dstShortPitch*2);

            for (int by=0; by<nBlkY; ++by) {
                int xx = 0;
                int blx=0, bly=0;
                for (int bx=0; bx<nBlkX; ++bx) {
                    // select window
                    if (by==0 && bx==0) {				winOver = OverWins->GetWindow(OW_TL); winOverUV = OverWinsUV->GetWindow(OW_TL); }
                    else if (by==0 && bx==nBlkX-1) {	winOver = OverWins->GetWindow(OW_TR); winOverUV = OverWinsUV->GetWindow(OW_TR); }
                    else if (by==0) {					winOver = OverWins->GetWindow(OW_TM); winOverUV = OverWinsUV->GetWindow(OW_TM); }
                    else if (by==nBlkY-1 && bx==0) {	winOver = OverWins->GetWindow(OW_BL); winOverUV = OverWinsUV->GetWindow(OW_BL); }
                    else if (by==nBlkY-1 && bx==nBlkX-1) { winOver = OverWins->GetWindow(OW_BR); winOverUV = OverWinsUV->GetWindow(OW_BR); }
                    else if (by==nBlkY-1) {				winOver = OverWins->GetWindow(OW_BM); winOverUV = OverWinsUV->GetWindow(OW_BM); }
                    else if (bx==0) {					winOver = OverWins->GetWindow(OW_ML); winOverUV = OverWinsUV->GetWindow(OW_ML); }
                    else if (bx==nBlkX-1) {				winOver = OverWins->GetWindow(OW_MR); winOverUV = OverWinsUV->GetWindow(OW_MR); }
                    else {								winOver = OverWins->GetWindow(OW_MM); winOverUV = OverWinsUV->GetWindow(OW_MM); }

                    int i = by*nBlkX + bx;
                    const FakeBlockData &block = mvClip.GetBlock(0, i);

                    blx = block.GetX() * nPel + block.GetMV().x;
                    bly = block.GetY() * nPel + block.GetMV().y  + fieldShift;

                    if (block.GetSAD() < thSAD) {
                        // luma
                        OVERSLUMA(pDstShort + xx, dstShortPitch, pPlanes[0]->GetPointer(blx, bly), pPlanes[0]->GetPitch(), 
                                  winOver, nBlkSizeX);
                    }
                    else // bad compensation, use src
                    {
                        int blxsrc = bx * (nBlkSizeX - nOverlapX) * nPel;
                        int blysrc = by * (nBlkSizeY - nOverlapY) * nPel  + fieldShift;

                        OVERSLUMA(pDstShort + xx, dstShortPitch, pSrcPlanes[0]->GetPointer(blxsrc, blysrc), 
                                  pSrcPlanes[0]->GetPitch(), winOver, nBlkSizeX);
                    }
                    if (block.GetSAD() < thSADC) {
                        // chroma u
                        OVERSCHROMA(pDstShortU + (xx>>1), dstShortPitch, pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV), 
                                    pPlanes[1]->GetPitch(), winOverUV, nBlkSizeX/2);
                        // chroma v
                        OVERSCHROMA(pDstShortV + (xx>>1), dstShortPitch, pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV), 
                                    pPlanes[2]->GetPitch(), winOverUV, nBlkSizeX/2);
                    }
                    else // bad compensation, use src
                    {
                        int blxsrc = bx * (nBlkSizeX - nOverlapX) * nPel;
                        int blysrc = by * (nBlkSizeY - nOverlapY) * nPel  + fieldShift;

                        // chroma u
                        OVERSCHROMA(pDstShortU + (xx>>1), dstShortPitch, pSrcPlanes[1]->GetPointer(blxsrc>>1, blysrc>>ySubUV), pSrcPlanes[1]->GetPitch(), 
                                    winOverUV, nBlkSizeX/2);
                        // chroma v
                        OVERSCHROMA(pDstShortV + (xx>>1), dstShortPitch, pSrcPlanes[2]->GetPointer(blxsrc>>1, blysrc>>ySubUV), pSrcPlanes[2]->GetPitch(), 
                                    winOverUV, nBlkSizeX/2);
                    }
                    xx += (nBlkSizeX - nOverlapX);

                }
                if (nWidth_B < nWidth) // right non-covered region
                {
                    int lastx = std::min<int>(nBlkSizeX, pPlanes[0]->GetWidth() - (blx>>nLogPel));
                    // luma
                    PlaneCopy(pDstCur[0] + nWidth_B, nDstPitches[0], pPlanes[0]->GetPointer(blx, bly) + lastx, pPlanes[0]->GetPitch(), 
                              nWidth-nWidth_B, nBlkSizeY, isse);
                    // chroma u
                    PlaneCopy(pDstCur[1] + (nWidth_B>>1), nDstPitches[1], pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1), 
                              pPlanes[1]->GetPitch(), (nWidth-nWidth_B)>>1, nBlkSizeY>>ySubUV, isse);
                    // chroma v
                    PlaneCopy(pDstCur[2] + (nWidth_B>>1), nDstPitches[2], pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1), 
                              pPlanes[2]->GetPitch(), (nWidth-nWidth_B)>>1, nBlkSizeY>>ySubUV, isse);
                }
                pDstShort  += dstShortPitch*(nBlkSizeY - nOverlapY);
                pDstShortU += dstShortPitch*((nBlkSizeY - nOverlapY)>>ySubUV);
                pDstShortV += dstShortPitch*((nBlkSizeY - nOverlapY)>>ySubUV);
                pDstCur[0] += (nBlkSizeY - nOverlapY) * (nDstPitches[0]);
                pDstCur[1] += ((nBlkSizeY - nOverlapY)>>ySubUV) * (nDstPitches[1]);
                pDstCur[2] += ((nBlkSizeY - nOverlapY)>>ySubUV) * (nDstPitches[2]);

                if (by == nBlkY-1 && nHeight_B < nHeight) // bottom uncovered region
                {
                    pDstCur[0] += ( nOverlapY) * (nDstPitches[0]);
                    pDstCur[1] += ( nOverlapY>>ySubUV) * (nDstPitches[1]) ;
                    pDstCur[2] += ( nOverlapY>>ySubUV) * (nDstPitches[2]);
                    xx = 0;
                    for (int bx=0; bx<nBlkX; bx++) {
                        int i = by*nBlkX + bx;
                        const FakeBlockData &block = mvClip.GetBlock(0, i);
                        blx = block.GetX() * nPel + block.GetMV().x;
                        bly = block.GetY() * nPel + block.GetMV().y + fieldShift;

                        int lasty = std::min<int>(nBlkSizeY, pPlanes[0]->GetHeight() - (bly>>nLogPel)); // padding ?
                        // luma
                        PlaneCopy(pDstCur[0] + xx, nDstPitches[0],
                                  pPlanes[0]->GetPointer(blx, bly) + lasty*pPlanes[0]->GetPitch(),
                                  pPlanes[0]->GetPitch(), nBlkSizeX, nHeight-nHeight_B, isse);
                        // chroma u
                        PlaneCopy(pDstCur[1] + (xx>>1), nDstPitches[1],
                                  pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV)+ (lasty>>ySubUV)*pPlanes[1]->GetPitch(),
                                  pPlanes[1]->GetPitch(), (nBlkSizeX)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
                        // chroma v
                        PlaneCopy(pDstCur[2] + (xx>>1), nDstPitches[2],
                                  pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV)+ (lasty>>ySubUV)*pPlanes[2]->GetPitch(),
                                  pPlanes[2]->GetPitch(), (nBlkSizeX)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
                        xx += (nBlkSizeX - nOverlapX);

                    }
                    if (nWidth_B < nWidth) // right bottom non-covered region
                    {
                        int lastx = std::min<int>(nBlkSizeX, pPlanes[0]->GetWidth() - (blx>>nLogPel));
                        int lasty = std::min<int>(nBlkSizeY, pPlanes[0]->GetHeight() - (bly>>nLogPel));
                        // luma
                        PlaneCopy(pDstCur[0] + nWidth_B, nDstPitches[0],
                                  pPlanes[0]->GetPointer(blx, bly) + lastx + lasty*pPlanes[0]->GetPitch(),
                                  pPlanes[0]->GetPitch(), nWidth-nWidth_B, nHeight-nHeight_B, isse);
                        // chroma u
                        PlaneCopy(pDstCur[1] + (nWidth_B>>1), nDstPitches[1],
                                  pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1) + (lasty>>ySubUV)*pPlanes[1]->GetPitch(),
                                  pPlanes[1]->GetPitch(), (nWidth-nWidth_B)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
                        // chroma v
                        PlaneCopy(pDstCur[2] + (nWidth_B>>1), nDstPitches[2],
                                  pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1) + (lasty>>ySubUV)*pPlanes[2]->GetPitch(),
                                  pPlanes[2]->GetPitch(), (nWidth-nWidth_B)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
                    }
                }
            }
            Short2Bytes(pDst[0], nDstPitches[0], DstShort,                  dstShortPitch, nWidth_B, nHeight_B);
            Short2Bytes(pDst[1], nDstPitches[1], &DstShort[dstShortSize],   dstShortPitch, nWidth_B>>1, nHeight_B>>ySubUV);
            Short2Bytes(pDst[2], nDstPitches[2], &DstShort[2*dstShortSize], dstShortPitch, nWidth_B>>1, nHeight_B>>ySubUV);
        }
        __asm emms; // (we may use double-float somewhere) Fizick
        PROFILE_STOP(MOTION_PROFILE_COMPENSATION);

        // convert back from planes
        unsigned char *pDstYUY2 = dst->GetWritePtr();
        int nDstPitchYUY2 = dst->GetPitch();
        DstPlanes->YUY2FromPlanes(pDstYUY2, nDstPitchYUY2);
    }
    else {
        if ( !scBehavior && ( n + offset < vi.num_frames ) && ( n + offset >= 0 )) {
            dst = child->GetFrame(n + offset, env);
        }
        else
            dst = src;
    }
    return dst;
}

void MVCompensate::ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
                                                 int const Sharp, int const pixelType, int const nHeight, int const nWidth, 
                                                 int const nPel, bool const isse)
{
    (*srcGOF)->ConvertVideoFrame(src, src2x, pixelType, nWidth, nHeight, nPel, isse);

    PROFILE_START(MOTION_PROFILE_INTERPOLATION);
    (*srcGOF)->Pad(YUVPLANES);

    if (*src2x) {
        MVFrame *srcFrames = (*srcGOF)->GetFrame(0);
        MVPlane *srcPlaneY = srcFrames->GetPlane(YPLANE);
        srcPlaneY->RefineExt((*srcGOF)->GetVF2xYPtr(), (*srcGOF)->GetVF2xYPitch());
        MVPlane *srcPlaneU = srcFrames->GetPlane(UPLANE);
        srcPlaneU->RefineExt((*srcGOF)->GetVF2xUPtr(), (*srcGOF)->GetVF2xUPitch());
        MVPlane *srcPlaneV = srcFrames->GetPlane(VPLANE);
        srcPlaneV->RefineExt((*srcGOF)->GetVF2xVPtr(), (*srcGOF)->GetVF2xVPitch());
    }
    else
        (*srcGOF)->Refine(YUVPLANES, Sharp);

    PROFILE_STOP(MOTION_PROFILE_INTERPOLATION);

    // set processed
    (*srcGOF)->SetProcessed();
}
