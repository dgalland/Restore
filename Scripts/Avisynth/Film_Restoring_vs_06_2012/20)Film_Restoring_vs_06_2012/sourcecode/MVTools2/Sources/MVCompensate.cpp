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
#include "commonfunctions.h"
#include "maskfun.h"

MVCompensate::MVCompensate(PClip _child, PClip _super, PClip vectors, bool sc, float _recursionPercent, int _thres, bool _fields,
                           int nSCD1, int nSCD2, bool _isse, bool _planar, IScriptEnvironment* env) :
GenericVideoFilter(_child),
mvClip(vectors, nSCD1, nSCD2, env),
MVFilter(vectors, "MCompensate", env),
super(_super)
{
   isse = _isse;
   scBehavior = sc;
   recursion = max(0, min(256, int(_recursionPercent/100*256))); // convert to int scaled 0 to 256
   fields = _fields;
   planar = _planar;

   if (recursion>0 && nPel>1)
	   env->ThrowError("MCompensate: recursion is not supported for Pel>1");

   if (fields && nPel<2 && !vi.IsFieldBased())
	   env->ThrowError("MCompensate: fields option is for fieldbased video and pel > 1");

   thSAD = _thres*mvClip.GetThSCD1()/nSCD1; // normalize to block SAD

   if (isse)
   {
      switch (nBlkSizeX)
      {
      case 32:
      if (nBlkSizeY==16) {
         BLITLUMA = Copy32x16_mmx;
         OVERSLUMA = Overlaps32x16_mmx;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy16x8_mmx;
	         OVERSCHROMA = Overlaps16x8_mmx;
		 } else {
	         BLITCHROMA = Copy16x16_mmx;
	         OVERSCHROMA = Overlaps16x16_mmx;
		 }
      } break;
      case 16:
      if (nBlkSizeY==16) {
         BLITLUMA = Copy16x16_mmx;
         OVERSLUMA = Overlaps16x16_mmx;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy8x8_mmx;
	         OVERSCHROMA = Overlaps8x8_mmx;
		 } else {
	         BLITCHROMA = Copy8x16_mmx;
	         OVERSCHROMA = Overlaps8x16_mmx;
		 }
      } else if (nBlkSizeY==8) {
         BLITLUMA = Copy16x8_mmx;
         OVERSLUMA = Overlaps16x8_mmx;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy8x4_mmx;
	         OVERSCHROMA = Overlaps8x4_mmx;
		 } else {
	         BLITCHROMA = Copy8x8_mmx;
	         OVERSCHROMA = Overlaps8x8_mmx;
		 }
      } else if (nBlkSizeY==2) {
         BLITLUMA = Copy16x2_mmx;
         OVERSLUMA = Overlaps16x2_mmx;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy8x1_mmx;
	         OVERSCHROMA = Overlaps8x1_mmx;
		 } else {
	         BLITCHROMA = Copy8x2_mmx;
	         OVERSCHROMA = Overlaps8x2_mmx;
		 }
      }
         break;
      case 4:
		 BLITLUMA = Copy4x4_mmx;
		 OVERSLUMA = Overlaps4x4_mmx;
		 if (yRatioUV==2) {
			 BLITCHROMA = Copy2x2_mmx;
			 OVERSCHROMA = Overlaps_C<2,2>;
		 } else {
			 BLITCHROMA = Copy2x4_mmx;
			 OVERSCHROMA = Overlaps_C<2,4>;
		 }
         break;
      case 8:
      default:
      if (nBlkSizeY==8) {
         BLITLUMA = Copy8x8_mmx;
         OVERSLUMA = Overlaps8x8_mmx;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy4x4_mmx;
	         OVERSCHROMA = Overlaps4x4_mmx;
		 } else {
	         BLITCHROMA = Copy4x8_mmx;
	         OVERSCHROMA = Overlaps4x8_mmx;
		 }
      } else if (nBlkSizeY==4) { // 8x4
         BLITLUMA = Copy8x4_mmx;
         OVERSLUMA = Overlaps8x4_mmx;
		 if (yRatioUV==2) {
			 BLITCHROMA = Copy4x2_mmx; // idem
	         OVERSCHROMA = Overlaps4x2_mmx;
		 } else {
			 BLITCHROMA = Copy4x4_mmx; // idem
	         OVERSCHROMA = Overlaps4x4_mmx;
		 }
      }
      }
   }
   else // pure C, no isse opimization ("mmx" version could be used, but it's more like a debugging version)
   {
      switch (nBlkSizeX)
      {
      case 32:
      if (nBlkSizeY==16) {
         BLITLUMA = Copy_C<32,16>;         OVERSLUMA = Overlaps_C<32,16>;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy_C<16,8>;	         OVERSCHROMA = Overlaps_C<16,8>;
		 } else {
	         BLITCHROMA = Copy_C<16>;          OVERSCHROMA = Overlaps_C<16,16>;
		 }
      } break;
      case 16:
      if (nBlkSizeY==16) {
         BLITLUMA = Copy_C<16>;         OVERSLUMA = Overlaps_C<16,16>;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy_C<8>;	         OVERSCHROMA = Overlaps_C<8,8>;
		 } else {
	         BLITCHROMA = Copy_C<8,16>;	         OVERSCHROMA = Overlaps_C<8,16>;
		 }
      } else if (nBlkSizeY==8) {
         BLITLUMA = Copy_C<16,8>;         OVERSLUMA = Overlaps_C<16,8>;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy_C<8,4>;	         OVERSCHROMA = Overlaps_C<8,4>;
		 } else {
	         BLITCHROMA = Copy_C<8>;	         OVERSCHROMA = Overlaps_C<8,8>;
		 }
      } else if (nBlkSizeY==2) {
         BLITLUMA = Copy_C<16,2>;         OVERSLUMA = Overlaps_C<16,2>;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy_C<8,1>;	         OVERSCHROMA = Overlaps_C<8,1>;
		 } else {
	         BLITCHROMA = Copy_C<8,2>;	         OVERSCHROMA = Overlaps_C<8,2>;
		 }
      }
         break;
      case 4:
         BLITLUMA = Copy_C<4>;         OVERSLUMA = Overlaps_C<4,4>;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy_C<2>;			 OVERSCHROMA = Overlaps_C<2,2>;
		 } else {
	         BLITCHROMA = Copy_C<2,4>;			 OVERSCHROMA = Overlaps_C<2,4>;
		 }
         break;
      case 8:
      default:
      if (nBlkSizeY==8) {
         BLITLUMA = Copy_C<8>;         OVERSLUMA = Overlaps_C<8,8>;
		 if (yRatioUV==2) {
			 BLITCHROMA = Copy_C<4>;	         OVERSCHROMA = Overlaps_C<4,4>;
		 }
		 else {
			 BLITCHROMA = Copy_C<4,8>;	         OVERSCHROMA = Overlaps_C<4,8>;
		 }
      } else if (nBlkSizeY==4) { // 8x4
         BLITLUMA = Copy_C<8,4>;         OVERSLUMA = Overlaps_C<8,4>;
		 if (yRatioUV==2) {
			 BLITCHROMA = Copy_C<4,2>;	         OVERSCHROMA = Overlaps_C<4,2>;
		 } else {
			 BLITCHROMA = Copy_C<4>;	         OVERSCHROMA = Overlaps_C<4,4>;
		 }
      }
      }
   }


// get parameters of prepared super clip - v2.0
	SuperParams64Bits params;
    memcpy(&params, &super->GetVideoInfo().num_audio_samples, 8);
    int nHeightS = params.nHeight;
    nSuperHPad = params.nHPad;
    nSuperVPad = params.nVPad;
    int nSuperPel = params.nPel;
    int nSuperModeYUV = params.nModeYUV;
    int nSuperLevels = params.nLevels;

    pRefGOF = new MVGroupOfFrames(nSuperLevels, nWidth, nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, isse, yRatioUV);
    pSrcGOF = new MVGroupOfFrames(nSuperLevels, nWidth, nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, isse, yRatioUV);
    nSuperWidth = super->GetVideoInfo().width;
    nSuperHeight = super->GetVideoInfo().height;

    if (nHeight != nHeightS || nHeight != vi.height || nWidth != nSuperWidth-nSuperHPad*2 || nWidth != vi.width)
    		env->ThrowError("MCompensate : wring source or super frame size");


   if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 && !planar)
   {
		DstPlanes =  new YUY2Planes(nWidth, nHeight);
   }
   dstShortPitch = ((nWidth + 15)/16)*16;
   dstShortPitchUV = (((nWidth>>1) + 15)/16)*16;
   if (nOverlapX >0 || nOverlapY>0)
   {
	OverWins = new OverlapWindows(nBlkSizeX, nBlkSizeY, nOverlapX, nOverlapY);
	OverWinsUV = new OverlapWindows(nBlkSizeX/2, nBlkSizeY/yRatioUV, nOverlapX/2, nOverlapY/yRatioUV);
	DstShort = new unsigned short[dstShortPitch*nHeight];
	DstShortU = new unsigned short[dstShortPitchUV*nHeight];
	DstShortV = new unsigned short[dstShortPitchUV*nHeight];
   }

    if (recursion>0)
    {
  		if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
		{
				nLoopPitches[0]  = ((nSuperWidth*2 + 15)/16)*16;
				nLoopPitches[1]  = nLoopPitches[0];
				nLoopPitches[2]  = nLoopPitches[1];
				pLoop[0] = new unsigned char [nLoopPitches[0]*nSuperHeight];
				pLoop[1] = pLoop[0] + nSuperWidth;
				pLoop[2] = pLoop[1] + nSuperWidth/2;
		}
		else
		{
			 nLoopPitches[0] = ((nSuperWidth + 15)/16)*16;
			 nLoopPitches[1] = ((nSuperWidth/2 + 15)/16)*16;
			 nLoopPitches[2] = nLoopPitches[1];
			 pLoop[0] = new unsigned char [nLoopPitches[0]*nSuperHeight];
			 pLoop[1] = new unsigned char [nLoopPitches[1]*nSuperHeight];
			 pLoop[2] = new unsigned char [nLoopPitches[2]*nSuperHeight];
		}
    }

}

MVCompensate::~MVCompensate()
{
   if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2  && !planar)
   {
	delete DstPlanes;
   }
   if (nOverlapX >0 || nOverlapY >0)
   {
	   delete OverWins;
	   delete OverWinsUV;
	   delete [] DstShort;
	   delete [] DstShortU;
	   delete [] DstShortV;
   }
   delete pRefGOF; // v2.0
   delete pSrcGOF;

   if (recursion>0)
   {
       delete [] pLoop[1];
  		if ( (pixelType & VideoInfo::CS_YUY2) != VideoInfo::CS_YUY2 )
  		{
            delete [] pLoop[2];
            delete [] pLoop[3];
  		}
   }

}


PVideoFrame __stdcall MVCompensate::GetFrame(int n, IScriptEnvironment* env)
{
	int nWidth_B = nBlkX*(nBlkSizeX - nOverlapX) + nOverlapX;
	int nHeight_B = nBlkY*(nBlkSizeY - nOverlapY) + nOverlapY;

	BYTE *pDst[3], *pDstCur[3];
	const BYTE *pRef[3];
    int nDstPitches[3], nRefPitches[3];
	const BYTE *pSrc[3], *pSrcCur[3];
	int nSrcPitches[3];
	unsigned char *pDstYUY2;
	int nDstPitchYUY2;
	unsigned short *pDstShort;
	unsigned short *pDstShortU;
	unsigned short *pDstShortV;
	int blx, bly;
	int nOffset[3];

    nOffset[0] = nHPadding + nVPadding * nLoopPitches[0];
    nOffset[1] = nHPadding / 2 + (nVPadding / yRatioUV) * nLoopPitches[1];
    nOffset[2] = nOffset[1];

	int ySubUV = (yRatioUV == 2) ? 1 : 0;
	int nLogPel = ilog2(nPel); //shift

	PVideoFrame mvn = mvClip.GetFrame(n, env);
   mvClip.Update(mvn, env);
   mvn = 0; // free

	PVideoFrame	src	= super->GetFrame(n, env);
   PVideoFrame dst = env->NewVideoFrame(vi);

    int off, nref;
   if (mvClip.GetDeltaFrame() > 0)
   {
     off = ( mvClip.IsBackward() ) ? 1 : -1;
   off *= mvClip.GetDeltaFrame();
     nref = n + off;
   }
   else
   {
       nref = - mvClip.GetDeltaFrame(); // positive frame number (special static mode)
   }


   if ( mvClip.IsUsable() )
   {
 		if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
		{
			if (!planar)
			{
			pDstYUY2 = dst->GetWritePtr();
			nDstPitchYUY2 = dst->GetPitch();
			pDst[0] = DstPlanes->GetPtr();
			pDst[1] = DstPlanes->GetPtrU();
			pDst[2] = DstPlanes->GetPtrV();
			nDstPitches[0]  = DstPlanes->GetPitch();
			nDstPitches[1]  = DstPlanes->GetPitchUV();
			nDstPitches[2]  = DstPlanes->GetPitchUV();
			}
			else
			{
                pDst[0] = dst->GetWritePtr();
                pDst[1] = pDst[0] + nWidth;
                pDst[2] = pDst[1] + nWidth/2;
                nDstPitches[0] = dst->GetPitch();
                nDstPitches[1] = nDstPitches[0];
                nDstPitches[2] = nDstPitches[0];
			}
                pSrc[0] = src->GetReadPtr();
                pSrc[1] = pSrc[0] + nSuperWidth;
                pSrc[2] = pSrc[1] + nSuperWidth/2;
                nSrcPitches[0] = src->GetPitch();
                nSrcPitches[1] = nSrcPitches[0];
                nSrcPitches[2] = nSrcPitches[0];
		}
		else
		{
			 pDst[0] = YWPLAN(dst);
			 pDst[1] = UWPLAN(dst);
			 pDst[2] = VWPLAN(dst);
			 nDstPitches[0] = YPITCH(dst);
			 nDstPitches[1] = UPITCH(dst);
			 nDstPitches[2] = VPITCH(dst);
			 pSrc[0] = YRPLAN(src);
			 pSrc[1] = URPLAN(src);
			 pSrc[2] = VRPLAN(src);
			 nSrcPitches[0] = YPITCH(src);
			 nSrcPitches[1] = UPITCH(src);
			 nSrcPitches[2] = VPITCH(src);
		}


         PVideoFrame ref = super->GetFrame(nref, env);

  		if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
		{
				pRef[0] = ref->GetReadPtr();
				pRef[1] = pRef[0] + nSuperWidth;
				pRef[2] = pRef[1] + nSuperWidth/2;
				nRefPitches[0]  = ref->GetPitch();
				nRefPitches[1]  = nRefPitches[0];
				nRefPitches[2]  = nRefPitches[0];
		}
		else
		{
			 pRef[0] = YRPLAN(ref);
			 pRef[1] = URPLAN(ref);
			 pRef[2] = VRPLAN(ref);
			 nRefPitches[0] = YPITCH(ref);
			 nRefPitches[1] = UPITCH(ref);
			 nRefPitches[2] = VPITCH(ref);
		}

		if (recursion>0)
		{
            Blend(pLoop[0], pLoop[0], pRef[0], nSuperHeight, nSuperWidth, nLoopPitches[0], nLoopPitches[0], nRefPitches[0], 256-recursion, isse);
            Blend(pLoop[1], pLoop[1], pRef[1], nSuperHeight/yRatioUV, nSuperWidth/2, nLoopPitches[1], nLoopPitches[1], nRefPitches[1], 256-recursion, isse);
            Blend(pLoop[2], pLoop[2], pRef[2], nSuperHeight/yRatioUV, nSuperWidth/2, nLoopPitches[2], nLoopPitches[2], nRefPitches[2], 256-recursion, isse);
            pRefGOF->Update(YUVPLANES, (BYTE*)pLoop[0], nLoopPitches[0], (BYTE*)pLoop[1], nLoopPitches[1], (BYTE*)pLoop[2], nLoopPitches[2]);
		}
        else
            pRefGOF->Update(YUVPLANES, (BYTE*)pRef[0], nRefPitches[0], (BYTE*)pRef[1], nRefPitches[1], (BYTE*)pRef[2], nRefPitches[2]);// v2.0
            pSrcGOF->Update(YUVPLANES, (BYTE*)pSrc[0], nSrcPitches[0], (BYTE*)pSrc[1], nSrcPitches[1], (BYTE*)pSrc[2], nSrcPitches[2]);


         MVPlane *pPlanes[3];

         pPlanes[0] = pRefGOF->GetFrame(0)->GetPlane(YPLANE);
         pPlanes[1] = pRefGOF->GetFrame(0)->GetPlane(UPLANE);
         pPlanes[2] = pRefGOF->GetFrame(0)->GetPlane(VPLANE);

         MVPlane *pSrcPlanes[3];

         pSrcPlanes[0] = pSrcGOF->GetFrame(0)->GetPlane(YPLANE);
         pSrcPlanes[1] = pSrcGOF->GetFrame(0)->GetPlane(UPLANE);
         pSrcPlanes[2] = pSrcGOF->GetFrame(0)->GetPlane(VPLANE);

         PROFILE_START(MOTION_PROFILE_COMPENSATION);
		 pDstCur[0] = pDst[0];
		 pDstCur[1] = pDst[1];
		 pDstCur[2] = pDst[2];
		 pSrcCur[0] = pSrc[0];
		 pSrcCur[1] = pSrc[1];
		 pSrcCur[2] = pSrc[2];

		int fieldShift = 0;
		if (fields && nPel > 1 && ((nref-n) %2 != 0))
		{
			bool paritySrc = child->GetParity(n);
	 		bool parityRef = child->GetParity(nref);
			fieldShift = (paritySrc && !parityRef) ? nPel/2 : ( (parityRef && !paritySrc) ? -(nPel/2) : 0);
			// vertical shift of fields for fieldbased video at finest level pel2
		}
// -----------------------------------------------------------------------------
	  if (nOverlapX==0 && nOverlapY==0)
	  {
		for (int by=0; by<nBlkY; by++)
		{
			int xx = 0;
			for (int bx=0; bx<nBlkX; bx++)
			{
					int i = by*nBlkX + bx;
					const FakeBlockData &block = mvClip.GetBlock(0, i);
					blx = block.GetX() * nPel + block.GetMV().x;
					bly = block.GetY() * nPel + block.GetMV().y + fieldShift;
					if (block.GetSAD() < thSAD)
					{
					// luma
					BLITLUMA(pDstCur[0] + xx, nDstPitches[0], pPlanes[0]->GetPointer(blx, bly), pPlanes[0]->GetPitch());
					// chroma u
					if(pPlanes[1]) BLITCHROMA(pDstCur[1] + (xx>>1), nDstPitches[1], pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV), pPlanes[1]->GetPitch());
					// chroma v
					if(pPlanes[2]) BLITCHROMA(pDstCur[2] + (xx>>1), nDstPitches[2], pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV), pPlanes[2]->GetPitch());
					}
					else
					{
						int blxsrc = bx * (nBlkSizeX) * nPel;
						int blysrc = by * (nBlkSizeY) * nPel  + fieldShift;

						BLITLUMA(pDstCur[0] + xx, nDstPitches[0], pSrcPlanes[0]->GetPointer(blxsrc, blysrc), pSrcPlanes[0]->GetPitch());
						// chroma u
						if(pSrcPlanes[1]) BLITCHROMA(pDstCur[1] + (xx>>1), nDstPitches[1], pSrcPlanes[1]->GetPointer(blxsrc>>1, blysrc>>ySubUV), pSrcPlanes[1]->GetPitch());
						// chroma v
						if(pSrcPlanes[2]) BLITCHROMA(pDstCur[2] + (xx>>1), nDstPitches[2], pSrcPlanes[2]->GetPointer(blxsrc>>1, blysrc>>ySubUV), pSrcPlanes[2]->GetPitch());
					}


					xx += (nBlkSizeX);

			}
			if (nWidth_B < nWidth) // right non-covered region
			{
				int lastx = min(nBlkSizeX, pPlanes[0]->GetWidth() - (blx>>nLogPel));
				// luma
				BitBlt(pDstCur[0] + nWidth_B, nDstPitches[0], pPlanes[0]->GetPointer(blx, bly) + lastx,
				   pPlanes[0]->GetPitch(), nWidth-nWidth_B, nBlkSizeY, isse);
				// chroma u
				if(pPlanes[1]) BitBlt(pDstCur[1] + (nWidth_B>>1), nDstPitches[1], pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1),
				   pPlanes[1]->GetPitch(), (nWidth-nWidth_B)>>1, nBlkSizeY>>ySubUV, isse);
				// chroma v
				if(pPlanes[2]) BitBlt(pDstCur[2] + (nWidth_B>>1), nDstPitches[2], pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1),
				   pPlanes[2]->GetPitch(), (nWidth-nWidth_B)>>1, nBlkSizeY>>ySubUV, isse);
			}
               pDstCur[0] += (nBlkSizeY) * (nDstPitches[0]);
               pDstCur[1] += ( nBlkSizeY>>ySubUV) * (nDstPitches[1]);
               pDstCur[2] += ( nBlkSizeY>>ySubUV) * (nDstPitches[2]) ;
               pSrcCur[0] += (nBlkSizeY) * (nSrcPitches[0]);
               pSrcCur[1] += ( nBlkSizeY>>ySubUV) * (nSrcPitches[1]);
               pSrcCur[2] += ( nBlkSizeY>>ySubUV) * (nSrcPitches[2]) ;

			if (by == nBlkY-1 && nHeight_B < nHeight) // bottom uncovered region
			{
				int xx = 0;
				for (int bx=0; bx<nBlkX; bx++)
				{
					int i = by*nBlkX + bx;
					const FakeBlockData &block = mvClip.GetBlock(0, i);
					blx = block.GetX() * nPel + block.GetMV().x;
					bly = block.GetY() * nPel + block.GetMV().y + fieldShift;

					int lasty = min(nBlkSizeY, pPlanes[0]->GetHeight() - (bly>>nLogPel));

					// luma
					BitBlt(pDstCur[0] + xx, nDstPitches[0],
					   pPlanes[0]->GetPointer(blx, bly) + lasty*pPlanes[0]->GetPitch(),
					   pPlanes[0]->GetPitch(), nBlkSizeX, nHeight-nHeight_B, isse);
					// chroma u
					if(pPlanes[1]) BitBlt(pDstCur[1] + (xx>>1), nDstPitches[1],
					   pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV)+ (lasty>>ySubUV)*pPlanes[1]->GetPitch(),
					   pPlanes[1]->GetPitch(), (nBlkSizeX)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
					// chroma v
					if(pPlanes[2]) BitBlt(pDstCur[2] + (xx>>1), nDstPitches[2],
					   pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV)+ (lasty>>ySubUV)*pPlanes[2]->GetPitch(),
					   pPlanes[2]->GetPitch(), (nBlkSizeX)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
					xx += (nBlkSizeX);

				}
				if (nWidth_B < nWidth) // right bottom non-covered region
				{
					int lastx = min(nBlkSizeX, pPlanes[0]->GetWidth() - (blx>>nLogPel));
					int lasty = min(nBlkSizeY, pPlanes[0]->GetHeight() - (bly>>nLogPel));
					// luma
					BitBlt(pDstCur[0] + nWidth_B, nDstPitches[0],
					   pPlanes[0]->GetPointer(blx, bly) + lastx + lasty*pPlanes[0]->GetPitch(),
					   pPlanes[0]->GetPitch(), nWidth-nWidth_B, nHeight-nHeight_B, isse);
					// chroma u
					if(pPlanes[1]) BitBlt(pDstCur[1] + (nWidth_B>>1), nDstPitches[1],
					   pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1) + (lasty>>ySubUV)*pPlanes[1]->GetPitch(),
					   pPlanes[1]->GetPitch(), (nWidth-nWidth_B)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
					// chroma v
					if(pPlanes[2]) BitBlt(pDstCur[2] + (nWidth_B>>1), nDstPitches[2],
					   pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1) + (lasty>>ySubUV)*pPlanes[2]->GetPitch(),
					   pPlanes[2]->GetPitch(), (nWidth-nWidth_B)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
				}
			}
		}
	  }
// -----------------------------------------------------------------
	  else // overlap
	  {
		pDstShort = DstShort;
		MemZoneSet(reinterpret_cast<unsigned char*>(DstShort), 0, nWidth_B*2, nHeight_B, 0, 0, dstShortPitch*2);
		pDstShortU = DstShortU;
		if(pPlanes[1]) MemZoneSet(reinterpret_cast<unsigned char*>(DstShortU), 0, nWidth_B, nHeight_B>>ySubUV, 0, 0, dstShortPitchUV*2);
		pDstShortV = DstShortV;
		if(pPlanes[2]) MemZoneSet(reinterpret_cast<unsigned char*>(DstShortV), 0, nWidth_B, nHeight_B>>ySubUV, 0, 0, dstShortPitchUV*2);

		for (int by=0; by<nBlkY; by++)
		{
			int wby = ((by + nBlkY - 3)/(nBlkY - 2))*3;
			int xx = 0;
			for (int bx=0; bx<nBlkX; bx++)
			{
					// select window
                    int wbx = (bx + nBlkX - 3)/(nBlkX - 2);
                    winOver = OverWins->GetWindow(wby + wbx);
                    winOverUV = OverWinsUV->GetWindow(wby + wbx);

					int i = by*nBlkX + bx;
					const FakeBlockData &block = mvClip.GetBlock(0, i);

					blx = block.GetX() * nPel + block.GetMV().x;
					bly = block.GetY() * nPel + block.GetMV().y  + fieldShift;

					if (block.GetSAD() < thSAD)
					{
					// luma
					OVERSLUMA(pDstShort + xx, dstShortPitch, pPlanes[0]->GetPointer(blx, bly), pPlanes[0]->GetPitch(), winOver, nBlkSizeX);
					// chroma u
					if(pPlanes[1]) OVERSCHROMA(pDstShortU + (xx>>1), dstShortPitchUV, pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV), pPlanes[1]->GetPitch(), winOverUV, nBlkSizeX/2);
					// chroma v
					if(pPlanes[2]) OVERSCHROMA(pDstShortV + (xx>>1), dstShortPitchUV, pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV), pPlanes[2]->GetPitch(), winOverUV, nBlkSizeX/2);
					}
					else // bad compensation, use src
					{
						int blxsrc = bx * (nBlkSizeX - nOverlapX) * nPel;
						int blysrc = by * (nBlkSizeY - nOverlapY) * nPel  + fieldShift;

						OVERSLUMA(pDstShort + xx, dstShortPitch, pSrcPlanes[0]->GetPointer(blxsrc, blysrc), pSrcPlanes[0]->GetPitch(), winOver, nBlkSizeX);
						// chroma u
						if(pSrcPlanes[1]) OVERSCHROMA(pDstShortU + (xx>>1), dstShortPitchUV, pSrcPlanes[1]->GetPointer(blxsrc>>1, blysrc>>ySubUV), pSrcPlanes[1]->GetPitch(), winOverUV, nBlkSizeX/2);
						// chroma v
						if(pSrcPlanes[2]) OVERSCHROMA(pDstShortV + (xx>>1), dstShortPitchUV, pSrcPlanes[2]->GetPointer(blxsrc>>1, blysrc>>ySubUV), pSrcPlanes[2]->GetPitch(), winOverUV, nBlkSizeX/2);
					}

					xx += (nBlkSizeX - nOverlapX);

			}
			if (nWidth_B < nWidth) // right non-covered region
			{
				int lastx = min(nBlkSizeX, pPlanes[0]->GetWidth() - (blx>>nLogPel));
				// luma
				BitBlt(pDstCur[0] + nWidth_B, nDstPitches[0], pPlanes[0]->GetPointer(blx, bly) + lastx, pPlanes[0]->GetPitch(), nWidth-nWidth_B, nBlkSizeY, isse);
				// chroma u
				if(pPlanes[1]) BitBlt(pDstCur[1] + (nWidth_B>>1), nDstPitches[1], pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1), pPlanes[1]->GetPitch(), (nWidth-nWidth_B)>>1, nBlkSizeY>>ySubUV, isse);
				// chroma v
				if(pPlanes[2]) BitBlt(pDstCur[2] + (nWidth_B>>1), nDstPitches[2], pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1), pPlanes[2]->GetPitch(), (nWidth-nWidth_B)>>1, nBlkSizeY>>ySubUV, isse);
			}
               pDstShort += dstShortPitch*(nBlkSizeY - nOverlapY);
               pDstShortU += dstShortPitchUV*((nBlkSizeY - nOverlapY)>>ySubUV);
               pDstShortV += dstShortPitchUV*((nBlkSizeY - nOverlapY)>>ySubUV);
               pDstCur[0] += (nBlkSizeY - nOverlapY) * (nDstPitches[0]);
               pDstCur[1] += ((nBlkSizeY - nOverlapY)>>ySubUV) * (nDstPitches[1]);
               pDstCur[2] += ((nBlkSizeY - nOverlapY)>>ySubUV) * (nDstPitches[2]);
               pSrcCur[0] += (nBlkSizeY - nOverlapY) * (nSrcPitches[0]);
               pSrcCur[1] += ((nBlkSizeY - nOverlapY)>>ySubUV) * (nSrcPitches[1]);
               pSrcCur[2] += ((nBlkSizeY - nOverlapY)>>ySubUV) * (nSrcPitches[2]) ;

			if (by == nBlkY-1 && nHeight_B < nHeight) // bottom uncovered region
			{
               pDstCur[0] += ( nOverlapY) * (nDstPitches[0]);
               pDstCur[1] += ( nOverlapY>>ySubUV) * (nDstPitches[1]) ;
               pDstCur[2] += ( nOverlapY>>ySubUV) * (nDstPitches[2]);
				int xx = 0;
				for (int bx=0; bx<nBlkX; bx++)
				{
					int i = by*nBlkX + bx;
					const FakeBlockData &block = mvClip.GetBlock(0, i);
					blx = block.GetX() * nPel + block.GetMV().x;
					bly = block.GetY() * nPel + block.GetMV().y + fieldShift;

					int lasty = min(nBlkSizeY, pPlanes[0]->GetHeight() - (bly>>nLogPel)); // padding ?
					// luma
					BitBlt(pDstCur[0] + xx, nDstPitches[0],
					   pPlanes[0]->GetPointer(blx, bly) + lasty*pPlanes[0]->GetPitch(),
					   pPlanes[0]->GetPitch(), nBlkSizeX, nHeight-nHeight_B, isse);
					// chroma u
					if(pPlanes[1]) BitBlt(pDstCur[1] + (xx>>1), nDstPitches[1],
					   pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV)+ (lasty>>ySubUV)*pPlanes[1]->GetPitch(),
					   pPlanes[1]->GetPitch(), (nBlkSizeX)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
					// chroma v
					if(pPlanes[2]) BitBlt(pDstCur[2] + (xx>>1), nDstPitches[2],
					   pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV)+ (lasty>>ySubUV)*pPlanes[2]->GetPitch(),
					   pPlanes[2]->GetPitch(), (nBlkSizeX)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
					xx += (nBlkSizeX - nOverlapX);

				}
				if (nWidth_B < nWidth) // right bottom non-covered region
				{
					int lastx = min(nBlkSizeX, pPlanes[0]->GetWidth() - (blx>>nLogPel));
					int lasty = min(nBlkSizeY, pPlanes[0]->GetHeight() - (bly>>nLogPel));
					// luma
					BitBlt(pDstCur[0] + nWidth_B, nDstPitches[0],
					   pPlanes[0]->GetPointer(blx, bly) + lastx + lasty*pPlanes[0]->GetPitch(),
					   pPlanes[0]->GetPitch(), nWidth-nWidth_B, nHeight-nHeight_B, isse);
					// chroma u
					if(pPlanes[1]) BitBlt(pDstCur[1] + (nWidth_B>>1), nDstPitches[1],
					   pPlanes[1]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1) + (lasty>>ySubUV)*pPlanes[1]->GetPitch(),
					   pPlanes[1]->GetPitch(), (nWidth-nWidth_B)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
					// chroma v
					if(pPlanes[2]) BitBlt(pDstCur[2] + (nWidth_B>>1), nDstPitches[2],
					   pPlanes[2]->GetPointer(blx>>1, bly>>ySubUV) + (lastx>>1) + (lasty>>ySubUV)*pPlanes[2]->GetPitch(),
					   pPlanes[2]->GetPitch(), (nWidth-nWidth_B)>>1, (nHeight-nHeight_B)>>ySubUV, isse);
				}
			}

		}
		Short2Bytes(pDst[0], nDstPitches[0], DstShort, dstShortPitch, nWidth_B, nHeight_B);
		if(pPlanes[1]) Short2Bytes(pDst[1], nDstPitches[1], DstShortU, dstShortPitchUV, nWidth_B>>1, nHeight_B>>ySubUV);
		if(pPlanes[2]) Short2Bytes(pDst[2], nDstPitches[2], DstShortV, dstShortPitchUV, nWidth_B>>1, nHeight_B>>ySubUV);
	  }
         PROFILE_STOP(MOTION_PROFILE_COMPENSATION);

      // if we're in in-loop recursive mode, we copy the frame
      if ( recursion>0 )
      {
         env->BitBlt(pLoop[0] + nOffset[0], nLoopPitches[0], pDst[0], nDstPitches[0], nWidth, nHeight);
         env->BitBlt(pLoop[1] + nOffset[1], nLoopPitches[1], pDst[1], nDstPitches[1], nWidth / 2, nHeight / yRatioUV);
         env->BitBlt(pLoop[2] + nOffset[2], nLoopPitches[2], pDst[2], nDstPitches[2], nWidth / 2, nHeight / yRatioUV);
      }

	if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 && !planar)
	{
		YUY2FromPlanes(pDstYUY2, nDstPitchYUY2, nWidth, nHeight,
					  pDst[0], nDstPitches[0], pDst[1], pDst[2], nDstPitches[1], isse);
	}
   }
   else {
      if ( !scBehavior && ( nref < vi.num_frames ) && ( nref >= 0 ))
         src = super->GetFrame(nref, env);
      else
         src = super->GetFrame(n, env);

 		if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
		{
			if (!planar)
			{
			pDstYUY2 = dst->GetWritePtr();
			nDstPitchYUY2 = dst->GetPitch();
			pDst[0] = DstPlanes->GetPtr();
			pDst[1] = DstPlanes->GetPtrU();
			pDst[2] = DstPlanes->GetPtrV();
			nDstPitches[0]  = DstPlanes->GetPitch();
			nDstPitches[1]  = DstPlanes->GetPitchUV();
			nDstPitches[2]  = DstPlanes->GetPitchUV();
			}
			else
			{
                pDst[0] = dst->GetWritePtr();
                pDst[1] = pDst[0] + nWidth;
                pDst[2] = pDst[1] + nWidth/2;
                nDstPitches[0] = dst->GetPitch();
                nDstPitches[1] = nDstPitches[0];
                nDstPitches[2] = nDstPitches[0];
			}
                pSrc[0] = src->GetReadPtr();
                pSrc[1] = pSrc[0] + nSuperWidth;
                pSrc[2] = pSrc[1] + nSuperWidth/2;
                nSrcPitches[0] = src->GetPitch();
                nSrcPitches[1] = nSrcPitches[0];
                nSrcPitches[2] = nSrcPitches[0];
		}
		else
		{
			 pDst[0] = YWPLAN(dst);
			 pDst[1] = UWPLAN(dst);
			 pDst[2] = VWPLAN(dst);
			 nDstPitches[0] = YPITCH(dst);
			 nDstPitches[1] = UPITCH(dst);
			 nDstPitches[2] = VPITCH(dst);
			 pSrc[0] = YRPLAN(src);
			 pSrc[1] = URPLAN(src);
			 pSrc[2] = VRPLAN(src);
			 nSrcPitches[0] = YPITCH(src);
			 nSrcPitches[1] = UPITCH(src);
			 nSrcPitches[2] = VPITCH(src);
		}

    nOffset[0] = nHPadding + nVPadding * nSrcPitches[0];
    nOffset[1] = nHPadding / 2 + (nVPadding / yRatioUV) * nSrcPitches[1];
    nOffset[2] = nOffset[1];

         env->BitBlt(pDst[0], nDstPitches[0], pSrc[0] + nOffset[0], nSrcPitches[0], nWidth, nHeight);
         env->BitBlt(pDst[1], nDstPitches[1], pSrc[1] + nOffset[1], nSrcPitches[1], nWidth / 2, nHeight / yRatioUV);
         env->BitBlt(pDst[2], nDstPitches[2], pSrc[2] + nOffset[2], nSrcPitches[2], nWidth / 2, nHeight / yRatioUV);

	if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 && !planar)
	{
		YUY2FromPlanes(pDstYUY2, nDstPitchYUY2, nWidth, nHeight,
					  pDst[0], nDstPitches[0], pDst[1], pDst[2], nDstPitches[1], isse);
	}

      if ( recursion>0 )
      {
         env->BitBlt(pLoop[0], nLoopPitches[0], pSrc[0], nSrcPitches[0], nWidth, nHeight);
         env->BitBlt(pLoop[1], nLoopPitches[1], pSrc[1], nSrcPitches[1], nWidth / 2, nHeight / yRatioUV);
         env->BitBlt(pLoop[2], nLoopPitches[2], pSrc[2], nSrcPitches[2], nWidth / 2, nHeight / yRatioUV);
      }


   }
		 __asm emms; // (we may use double-float somewhere) Fizick
   return dst;
}
