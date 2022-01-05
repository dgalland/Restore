// Author: Manao
// updated by Fizick
// See legal notice in Copying.txt for more information
//
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

#include "MVIncrease.h"

MVIncrease::MVIncrease(PClip _child, PClip vectors, int _vertical, int _horizontal, bool _sc, int _nIdx,
                       int nSCD1, int nSCD2, bool mmx, bool isse, IScriptEnvironment* env) :
            GenericVideoFilter(_child),
            mvClip(vectors, nSCD1, nSCD2, env, true),
            MVFilter(vectors, "MVIncrease", env)
{
    if (!vi.IsYV12())
        env->ThrowError("MVIncrease : Color format must be YV12");

    nIdx = _nIdx;
    vertical = _vertical;
    horizontal = _horizontal;
    scBehavior = _sc;
    nBlkSizeX2 = nBlkSizeX / 2;
    nBlkSizeY2 = nBlkSizeY / 2;

    //	if (nBlkSizeY != nBlkSizeX)
    //      env->ThrowError("MVIncrease : nBlkSizeY and nBlkSizeX must be equal");

    if (nOverlapX != 0 || nOverlapY != 0)
        env->ThrowError("MVIncrease : overlap must be 0");

    if (( vertical < 1 ) || ( vertical > 2 ))
        env->ThrowError("MVIncrease : Vertical can only be 1 or 2");
    if (( horizontal < 1 ) || ( horizontal > 2 ))
        env->ThrowError("MVIncrease : Horizontal can only be 1 or 2");

    #ifndef DYNAMIC_COMPILE
    if ( nBlkSizeX == 4 ) {
        if (nBlkSizeY==4) {
            if ( ( horizontal == 2 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<8,8>;
                BLITCHROMA = Copy_C<4,4>; }
            else if (( horizontal == 1 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<4, 8>;
                BLITCHROMA = Copy_C<2, 4>; }
            else if (( horizontal == 2 ) && ( vertical == 1 ))  {
                BLITLUMA = Copy_C<8, 4>;
                BLITCHROMA = Copy_C<4, 2>; }
        } else if (nBlkSizeY==8) {
            if (( horizontal == 2 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<8,16>;
                BLITCHROMA = Copy_C<4,8>; }
            else if (( horizontal == 1 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<4, 16>;
                BLITCHROMA = Copy_C<2, 8>; }
            else if (( horizontal == 2 )&& ( vertical == 1 ))  {
                BLITLUMA = Copy_C<8, 8>;
                BLITCHROMA = Copy_C<4, 4>; }
        }
    }
    else if ( nBlkSizeX == 8 ) {
        if (nBlkSizeY==8) {
            if ( ( horizontal == 2 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<16,16>;
                BLITCHROMA = Copy_C<8,8>; }
            else if (( horizontal == 1 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<16,8>;
                BLITCHROMA = Copy_C<8,4>; }
            else if (( horizontal == 2 ) && ( vertical == 1 ))  {
                BLITLUMA = Copy_C<8,16>;
                BLITCHROMA = Copy_C<4,8>; }
        } else if (nBlkSizeY==4) {
            if ( ( horizontal == 2 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<16,8>;
                BLITCHROMA = Copy_C<8,4>; }
            else if (( horizontal == 1 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<8,8>;
                BLITCHROMA = Copy_C<4,4>; }
            else if (( horizontal == 2 )&& ( vertical == 1 ))  {
                BLITLUMA = Copy_C<16, 4>;
                BLITCHROMA = Copy_C<8, 2>; }
        }
    }
    else if ( nBlkSizeX == 16 ) {
        if (nBlkSizeY==16) {
            if ( ( horizontal == 2 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<32, 32>;
                BLITCHROMA = Copy_C<16,16>; }
            else if (( horizontal == 1 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<16, 32>;
                BLITCHROMA = Copy_C<8,16>;  }
            else if (( horizontal == 2 )&& ( vertical == 1 ))  {
                BLITLUMA = Copy_C<32,16>;
                BLITCHROMA = Copy_C<16,8>;  }
        } else if (nBlkSizeY==8) {
            if ( ( horizontal == 2 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<32,16>;
                BLITCHROMA = Copy_C<16,8>; }
            else if (( horizontal == 1 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<16,16>;
                BLITCHROMA = Copy_C<8,8>; }
            else if (( horizontal == 2 )&& ( vertical == 1 ))  {
                BLITLUMA = Copy_C<32, 8>;
                BLITCHROMA = Copy_C<16, 4>; }
        } else if (nBlkSizeY==2) {
            if ( ( horizontal == 2 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<32,4>;
                BLITCHROMA = Copy_C<16,2>; }
            else if (( horizontal == 1 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<16,4>;
                BLITCHROMA = Copy_C<8,2>; }
            else if (( horizontal == 2 )&& ( vertical == 1 ))  {
                BLITLUMA = Copy_C<32, 2>;
                BLITCHROMA = Copy_C<16, 1>; }
        }
    }
    else if ( nBlkSizeX == 32 ) {
        if (nBlkSizeY==16) {
            if ( ( horizontal == 2 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<64, 32>;
                BLITCHROMA = Copy_C<32,16>; }
            else if (( horizontal == 1 ) && ( vertical == 2 ))  {
                BLITLUMA = Copy_C<32, 32>;
                BLITCHROMA = Copy_C<16,16>;  }
            else if (( horizontal == 2 )&& ( vertical == 1 ))  {
                BLITLUMA = Copy_C<64,16>;
                BLITCHROMA = Copy_C<32,8>;  }
        }
    }

    #else

    copyLuma = new SWCopy(nBlkSizeX * horizontal, nBlkSizeY * vertical);
    copyChroma = new SWCopy(nBlkSizeX2 * horizontal, nBlkSizeY2 * vertical);

    #endif

    mvCore->AddFrames(nIdx, 3, 1, vi.width, vi.height,
                      nPel, nHPadding * horizontal, nVPadding * vertical, YUVPLANES, isse, yRatioUV);

    DstPlanes =  new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);
}

MVIncrease::~MVIncrease()
{
#ifdef DYNAMIC_COMPILE
    if (copyLuma)   delete copyLuma;
    if (copyChroma) delete copyChroma;
#endif

    if (DstPlanes) delete DstPlanes;
}

PVideoFrame __stdcall MVIncrease::GetFrame(int n, IScriptEnvironment *env)
{
    int shiftPel = (nPel==4) ? 2 : (nPel==2) ? 1 : 0;
    int shiftH = (horizontal==4) ? 2 : (horizontal==2) ? 1 : 0;
    int shiftV = (vertical==4) ? 2 : (vertical==2) ? 1 : 0;

    PVideoFrame dst;
    PVideoFrame src	= child->GetFrame(n, env);

    PVideoFrame mvn = mvClip.GetFrame(n, env);
    mvClip.Update(mvn, env);

    int off = ( mvClip.IsBackward() ) ? 1 : -1;
    off *= mvClip.GetDeltaFrame();

    int sharp = mvClip.GetSharp();

    if ( mvClip.IsUsable() )
    {
        // convert to planes
        dst = env->NewVideoFrame(vi);
        unsigned char *pDst[3];
        int nDstPitches[3];
        DstPlanes->ConvertVideoFrameToPlanes(&dst, pDst, nDstPitches);

        //      if (( vertical == 1 ) && ( horizontal == 1 ))
        //         return src;

        PMVGroupOfFrames pRefGOF = mvCore->GetFrame(nIdx, n + off);
        if (!pRefGOF->IsProcessed()) {
            PVideoFrame ref, ref2x;
            ref = child->GetFrame(n + off, env);
            pRefGOF->SetParity(child->GetParity(n + off));
//            if (usePelClipHere) 
//                ref2x = pelclip->GetFrame(n+off, env);
            ProcessFrameIntoGroupOfFrames(&pRefGOF, &ref, &ref2x, mvClip.GetSharp(), pixelType, 
                                          nHeight, nWidth, nPel, isse);
        }

        MVPlane *pPlanes[3];

        pPlanes[0] = pRefGOF->GetFrame(0)->GetPlane(YPLANE);
        pPlanes[1] = pRefGOF->GetFrame(0)->GetPlane(UPLANE);
        pPlanes[2] = pRefGOF->GetFrame(0)->GetPlane(VPLANE);

        PROFILE_START(MOTION_PROFILE_COMPENSATION);

        for ( int k = 0; k < mvClip.GetBlkCount(); k++ ) {
            const FakeBlockData &block = mvClip.GetBlock(0, k);
#ifdef DYNAMIC_COMPILE
            copyLuma->Call(pDst[0], pPitches[0],
                           pPlanes[0]->GetPointer((block.GetX() * nPel + block.GetMV().x) * horizontal,
                           (block.GetY() * nPel + block.GetMV().y) * vertical),
                           pPlanes[0]->GetPitch());

            copyChroma->Call(pDst[1], pPitches[1],
                             pPlanes[1]->GetPointer(((block.GetX() * nPel + block.GetMV().x) * horizontal) >> 1,
                             ((block.GetY() * nPel + block.GetMV().y) * vertical) >> 1),
                             pPlanes[1]->GetPitch());

            copyChroma->Call(pDst[2], pPitches[2],
                             pPlanes[2]->GetPointer(((block.GetX() * nPel + block.GetMV().x) * horizontal) >> 1,
                             ((block.GetY() * nPel + block.GetMV().y) * vertical) >> 1),
                             pPlanes[2]->GetPitch());
#else
            // luma
            BLITLUMA(pDst[0], nDstPitches[0],
                     pPlanes[0]->GetPointer(((block.GetX() << shiftPel) + block.GetMV().x) << shiftH,
                     ((block.GetY() << shiftPel) + block.GetMV().y) << shiftV),
                     pPlanes[0]->GetPitch(), isse);

            // chroma u
            BLITCHROMA(pDst[1], nDstPitches[1],
                       pPlanes[1]->GetPointer((((block.GetX() << shiftPel) + block.GetMV().x) << shiftH) >> 1,
                       (((block.GetY() << shiftPel) + block.GetMV().y) << shiftV) >> 1),
                       pPlanes[1]->GetPitch(), isse);

            // chroma v
            BLITCHROMA(pDst[2], nDstPitches[2],
                       pPlanes[2]->GetPointer((((block.GetX() << shiftPel) + block.GetMV().x) << shiftH) >> 1,
                       (((block.GetY() << shiftPel) + block.GetMV().y) << shiftV) >> 1),
                       pPlanes[2]->GetPitch(), isse);
#endif

            // update pDsts
            pDst[0] += nBlkSizeX << shiftH;
            pDst[1] += nBlkSizeX2 << shiftH;
            pDst[2] += nBlkSizeX2 << shiftH;

            if ( !((k + 1) % nBlkX)  ) {
                pDst[0] += nBlkSizeY  * vertical * nDstPitches[0] - nBlkSizeX * nBlkX * horizontal;
                pDst[1] += nBlkSizeY2 * vertical * nDstPitches[1] - (nBlkSizeX2 * nBlkX) * horizontal;
                pDst[2] += nBlkSizeY2 * vertical * nDstPitches[2] - (nBlkSizeX2 * nBlkX) * horizontal;
            }
        }
        _asm emms;
        PROFILE_STOP(MOTION_PROFILE_COMPENSATION);
    }
    else {
        if ( !scBehavior && ( n + off < vi.num_frames ) && ( n + off >= 0 )) 
            dst = child->GetFrame(n + off, env);
        else
            dst = src;
    }
    return dst;
}

void MVIncrease::ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
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