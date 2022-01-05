// Make a motion compensate temporal denoiser
// Copyright(c)2006 A.G.Balakhnin aka Fizick
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

#include "MVDegrain3.h"
#include "Padding.h"
#include "Interpolation.h"

MVDegrain3::MVDegrain3(PClip _child, PClip _super, PClip mvbw, PClip mvfw, PClip mvbw2,  PClip mvfw2, PClip mvbw3,  PClip mvfw3,
                        int _thSAD, int _thSADC, int _YUVplanes, int _nLimit,
					   int _nSCD1, int _nSCD2, bool _isse, bool _planar, IScriptEnvironment* env) :
GenericVideoFilter(_child),
MVFilter(mvfw3, "MDegrain3", env),
mvClipF3(mvfw3, _nSCD1, _nSCD2, env),
mvClipF2(mvfw2, _nSCD1, _nSCD2, env),
mvClipF(mvfw, _nSCD1, _nSCD2, env),
mvClipB(mvbw, _nSCD1, _nSCD2, env),
mvClipB2(mvbw2, _nSCD1, _nSCD2, env),
mvClipB3(mvbw3, _nSCD1, _nSCD2, env),
super(_super)
{
   thSAD = _thSAD*mvClipB.GetThSCD1()/_nSCD1; // normalize to block SAD
   thSADC = _thSADC*mvClipB.GetThSCD1()/_nSCD1; // chroma
   YUVplanes = _YUVplanes;
   nLimit = _nLimit;
   isse = _isse;
   planar = _planar;

   CheckSimilarity(mvClipB, "mvbw", env);
   CheckSimilarity(mvClipF, "mvfw", env);
   CheckSimilarity(mvClipF2, "mvfw2", env);
   CheckSimilarity(mvClipB2, "mvbw2", env);
   CheckSimilarity(mvClipF3, "mvfw3", env);
   CheckSimilarity(mvClipB3, "mvbw3", env);

// get parameters of prepared super clip - v2.0
	SuperParams64Bits params;
    memcpy(&params, &super->GetVideoInfo().num_audio_samples, 8);
    int nHeightS = params.nHeight;
    int nSuperHPad = params.nHPad;
    int nSuperVPad = params.nVPad;
    int nSuperPel = params.nPel;
    nSuperModeYUV = params.nModeYUV;
    int nSuperLevels = params.nLevels;
    pRefBGOF = new MVGroupOfFrames(nSuperLevels, nWidth, nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, isse, yRatioUV);
    pRefFGOF = new MVGroupOfFrames(nSuperLevels, nWidth, nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, isse, yRatioUV);
    pRefB2GOF = new MVGroupOfFrames(nSuperLevels, nWidth, nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, isse, yRatioUV);
    pRefF2GOF = new MVGroupOfFrames(nSuperLevels, nWidth, nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, isse, yRatioUV);
    pRefB3GOF = new MVGroupOfFrames(nSuperLevels, nWidth, nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, isse, yRatioUV);
    pRefF3GOF = new MVGroupOfFrames(nSuperLevels, nWidth, nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, isse, yRatioUV);
    int nSuperWidth = super->GetVideoInfo().width;
    int nSuperHeight = super->GetVideoInfo().height;

    if (nHeight != nHeightS || nHeight != vi.height || nWidth != nSuperWidth-nSuperHPad*2 || nWidth != vi.width)
    		env->ThrowError("MDegrain3 : wrong source or super frame size");

   if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 && !planar)
   {
		DstPlanes =  new YUY2Planes(nWidth, nHeight);
		SrcPlanes =  new YUY2Planes(nWidth, nHeight);
   }
   dstShortPitch = ((nWidth + 15)/16)*16;
   if (nOverlapX >0 || nOverlapY>0)
   {
	OverWins = new OverlapWindows(nBlkSizeX, nBlkSizeY, nOverlapX, nOverlapY);
	OverWinsUV = new OverlapWindows(nBlkSizeX/2, nBlkSizeY/yRatioUV, nOverlapX/2, nOverlapY/yRatioUV);
	DstShort = new unsigned short[dstShortPitch*nHeight];
   }

   if ( isse )
   {
	switch (nBlkSizeX)
      {
      case 32:
      if (nBlkSizeY==16) {          OVERSLUMA = Overlaps32x16_mmx;  DEGRAINLUMA = Degrain3_C<32,16>;
		 if (yRatioUV==2) {         OVERSCHROMA = Overlaps16x8_mmx; DEGRAINCHROMA = Degrain3_C<16,8>;		 }
		 else {                     OVERSCHROMA = Overlaps16x16_mmx;DEGRAINCHROMA = Degrain3_C<16,16>;		 }
      } break;
      case 16:
      if (nBlkSizeY==16) {          OVERSLUMA = Overlaps16x16_mmx; DEGRAINLUMA = Degrain3_C<16,16>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps8x8_mmx; DEGRAINCHROMA = Degrain3_C<8,8>;		 }
		 else {	                    OVERSCHROMA = Overlaps8x16_mmx;DEGRAINCHROMA = Degrain3_C<8,16>;		 }
      } else if (nBlkSizeY==8) {    OVERSLUMA = Overlaps16x8_mmx;  DEGRAINLUMA = Degrain3_C<16,8>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps8x4_mmx; DEGRAINCHROMA = Degrain3_C<8,4>;		 }
		 else {	                    OVERSCHROMA = Overlaps8x8_mmx; DEGRAINCHROMA = Degrain3_C<8,8>;		 }
      } else if (nBlkSizeY==2) {    OVERSLUMA = Overlaps16x2_mmx;  DEGRAINLUMA = Degrain3_C<16,2>;
		 if (yRatioUV==2) {         OVERSCHROMA = Overlaps8x1_mmx; DEGRAINCHROMA = Degrain3_C<8,1>;		 }
		 else {	                    OVERSCHROMA = Overlaps8x2_mmx; DEGRAINCHROMA = Degrain3_C<8,2>;		 }
      }
         break;
      case 4:
                                    OVERSLUMA = Overlaps4x4_mmx;    DEGRAINLUMA = Degrain3_C<4,4>;
		 if (yRatioUV==2) {			OVERSCHROMA = Overlaps_C<2,2>;	DEGRAINCHROMA = Degrain3_C<2,2>;		 }
		 else {			            OVERSCHROMA = Overlaps_C<2,4>;    DEGRAINCHROMA = Degrain3_C<2,4>;		 }
         break;
      case 8:
      default:
      if (nBlkSizeY==8) {           OVERSLUMA = Overlaps8x8_mmx;    DEGRAINLUMA = Degrain3_C<8,8>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps4x4_mmx;  DEGRAINCHROMA = Degrain3_C<4,4>;		 }
		 else {	                    OVERSCHROMA = Overlaps4x8_mmx;  DEGRAINCHROMA = Degrain3_C<4,8>;		 }
      }else if (nBlkSizeY==4) {     OVERSLUMA = Overlaps8x4_mmx;	DEGRAINLUMA = Degrain3_C<8,4>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps4x2_mmx;	DEGRAINCHROMA = Degrain3_C<4,2>;		 }
		 else {	                    OVERSCHROMA = Overlaps4x4_mmx;  DEGRAINCHROMA = Degrain3_C<4,4>;		 }
      }
      }
   }
   else
   {
	switch (nBlkSizeX)
      {
      case 32:
      if (nBlkSizeY==16) {          OVERSLUMA = Overlaps_C<32,16>;  DEGRAINLUMA = Degrain3_C<32,16>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps_C<16,8>; DEGRAINCHROMA = Degrain3_C<16,8>;		 }
		 else {	                    OVERSCHROMA = Overlaps_C<16,16>;DEGRAINCHROMA = Degrain3_C<16,16>;		 }
      } break;
      case 16:
      if (nBlkSizeY==16) {          OVERSLUMA = Overlaps_C<16,16>;  DEGRAINLUMA = Degrain3_C<16,16>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps_C<8,8>;  DEGRAINCHROMA = Degrain3_C<8,8>;		 }
		 else {	                    OVERSCHROMA = Overlaps_C<8,16>; DEGRAINCHROMA = Degrain3_C<8,16>;		 }
      } else if (nBlkSizeY==8) {    OVERSLUMA = Overlaps_C<16,8>;   DEGRAINLUMA = Degrain3_C<16,8>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps_C<8,4>;  DEGRAINCHROMA = Degrain3_C<8,4>;		 }
		 else {	                    OVERSCHROMA = Overlaps_C<8,8>;  DEGRAINCHROMA = Degrain3_C<8,8>;		 }
      } else if (nBlkSizeY==2) {    OVERSLUMA = Overlaps_C<16,2>;   DEGRAINLUMA = Degrain3_C<16,2>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps_C<8,1>;  DEGRAINCHROMA = Degrain3_C<8,1>;		 }
		 else {	                    OVERSCHROMA = Overlaps_C<8,2>;  DEGRAINCHROMA = Degrain3_C<8,2>;		 }
      }
         break;
      case 4:
                                    OVERSLUMA = Overlaps_C<4,4>;    DEGRAINLUMA = Degrain3_C<4,4>;
		 if (yRatioUV==2) {			OVERSCHROMA = Overlaps_C<2,2>;  DEGRAINCHROMA = Degrain3_C<2,2>;		 }
		 else {			            OVERSCHROMA = Overlaps_C<2,4>;  DEGRAINCHROMA = Degrain3_C<2,4>;		 }
         break;
      case 8:
      default:
      if (nBlkSizeY==8) {           OVERSLUMA = Overlaps_C<8,8>;    DEGRAINLUMA = Degrain3_C<8,8>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps_C<4,4>;  DEGRAINCHROMA = Degrain3_C<4,4>;		 }
		 else {	                    OVERSCHROMA = Overlaps_C<4,8>;  DEGRAINCHROMA = Degrain3_C<4,8>;		 }
      }else if (nBlkSizeY==4) {     OVERSLUMA = Overlaps_C<8,4>;    DEGRAINLUMA = Degrain3_C<8,4>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps_C<4,2>;  DEGRAINCHROMA = Degrain3_C<4,2>;		 }
		 else {	                    OVERSCHROMA = Overlaps_C<4,4>;  DEGRAINCHROMA = Degrain3_C<4,4>;		 }
      }
      }
   }

   	tmpBlock = new BYTE[32*32];
}


MVDegrain3::~MVDegrain3()
{
   if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
   {
	delete DstPlanes;
	delete SrcPlanes;
   }
   if (nOverlapX >0 || nOverlapY>0)
   {
	   delete OverWins;
	   delete OverWinsUV;
	   delete [] DstShort;
   }
   delete [] tmpBlock;
   delete pRefBGOF;
   delete pRefFGOF;
   delete pRefB2GOF;
   delete pRefF2GOF;
   delete pRefB3GOF;
   delete pRefF3GOF;
}




PVideoFrame __stdcall MVDegrain3::GetFrame(int n, IScriptEnvironment* env)
{
	int nWidth_B = nBlkX*(nBlkSizeX - nOverlapX) + nOverlapX;
	int nHeight_B = nBlkY*(nBlkSizeY - nOverlapY) + nOverlapY;

   BYTE *pDst[3], *pDstCur[3];
   const BYTE *pSrcCur[3];
    const BYTE *pSrc[3];
	const BYTE *pRefB[3];
	const BYTE *pRefF[3];
	const BYTE *pRefB2[3];
	const BYTE *pRefF2[3];
	const BYTE *pRefB3[3];
	const BYTE *pRefF3[3];
    int nDstPitches[3], nSrcPitches[3];
	int nRefBPitches[3], nRefFPitches[3];
	int nRefB2Pitches[3], nRefF2Pitches[3];
	int nRefB3Pitches[3], nRefF3Pitches[3];
	unsigned char *pDstYUY2;
	const unsigned char *pSrcYUY2;
	int nDstPitchYUY2;
	int nSrcPitchYUY2;
	bool isUsableB, isUsableF, isUsableB2, isUsableF2, isUsableB3, isUsableF3;
	int tmpPitch = nBlkSizeX;
	int WSrc, WRefB, WRefF, WRefB2, WRefF2, WRefB3, WRefF3;
	unsigned short *pDstShort;

	PVideoFrame mvF3 = mvClipF3.GetFrame(n, env);
   mvClipF3.Update(mvF3, env);
   isUsableF3 = mvClipF3.IsUsable();
   mvF3 = 0; // v2.0.9.2 -  it seems, we do not need in vectors clip anymore when we finished copiing them to fakeblockdatas
	PVideoFrame mvF2 = mvClipF2.GetFrame(n, env);
   mvClipF2.Update(mvF2, env);
   isUsableF2 = mvClipF2.IsUsable();
   mvF2 =0;
	PVideoFrame mvF = mvClipF.GetFrame(n, env);
   mvClipF.Update(mvF, env);
   isUsableF = mvClipF.IsUsable();
   mvF =0;
	PVideoFrame mvB = mvClipB.GetFrame(n, env);
   mvClipB.Update(mvB, env);
   isUsableB = mvClipB.IsUsable();
   mvB =0;
	PVideoFrame mvB2 = mvClipB2.GetFrame(n, env);
   mvClipB2.Update(mvB2, env);
   isUsableB2 = mvClipB2.IsUsable();
   mvB2 =0;
	PVideoFrame mvB3 = mvClipB3.GetFrame(n, env);
   mvClipB3.Update(mvB3, env);
   isUsableB3 = mvClipB3.IsUsable();
   mvB3 =0;

//   if ( mvClipB.IsUsable() && mvClipF.IsUsable() && mvClipB2.IsUsable() && mvClipF2.IsUsable() )
//   {
 	PVideoFrame	src	= child->GetFrame(n, env);
       PVideoFrame dst = env->NewVideoFrame(vi);
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
			pSrcYUY2 = src->GetReadPtr();
			nSrcPitchYUY2 = src->GetPitch();
			pSrc[0] = SrcPlanes->GetPtr();
			pSrc[1] = SrcPlanes->GetPtrU();
			pSrc[2] = SrcPlanes->GetPtrV();
			nSrcPitches[0]  = SrcPlanes->GetPitch();
			nSrcPitches[1]  = SrcPlanes->GetPitchUV();
			nSrcPitches[2]  = SrcPlanes->GetPitchUV();
			YUY2ToPlanes(pSrcYUY2, nSrcPitchYUY2, nWidth, nHeight,
				pSrc[0], nSrcPitches[0], pSrc[1], pSrc[2], nSrcPitches[1], isse);
			}
			else
			{
                pDst[0] = dst->GetWritePtr();
                pDst[1] = pDst[0] + nWidth;
                pDst[2] = pDst[1] + nWidth/2;
                nDstPitches[0] = dst->GetPitch();
                nDstPitches[1] = nDstPitches[0];
                nDstPitches[2] = nDstPitches[0];
                pSrc[0] = src->GetReadPtr();
                pSrc[1] = pSrc[0] + nWidth;
                pSrc[2] = pSrc[1] + nWidth/2;
                nSrcPitches[0] = src->GetPitch();
                nSrcPitches[1] = nSrcPitches[0];
                nSrcPitches[2] = nSrcPitches[0];
			}
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

		PVideoFrame refB, refF, refB2, refF2, refB3, refF3;

        // reorder ror regular frames order in v2.0.9.2
		if (isUsableF3)
		{
			int offF3 = ( mvClipF3.IsBackward() ) ? 1 : -1;
			offF3 *= mvClipF3.GetDeltaFrame();
			refF3 = super->GetFrame(n + offF3, env);
		}
		if (isUsableF2)
		{
			int offF2 = ( mvClipF2.IsBackward() ) ? 1 : -1;
			offF2 *= mvClipF2.GetDeltaFrame();
			refF2 = super->GetFrame(n + offF2, env);
		}
		if (isUsableF)
		{
			int offF = ( mvClipF.IsBackward() ) ? 1 : -1;
			 offF *= mvClipF.GetDeltaFrame();
			refF = super->GetFrame(n + offF, env);
		}
		if (isUsableB)
		{
			int offB = ( mvClipB.IsBackward() ) ? 1 : -1;
			offB *= mvClipB.GetDeltaFrame();
			refB = super->GetFrame(n + offB, env);
		}
		if (isUsableB2)
		{
			int offB2 = ( mvClipB2.IsBackward() ) ? 1 : -1;
			offB2 *= mvClipB2.GetDeltaFrame();
			refB2 = super->GetFrame(n + offB2, env);
		}
		if (isUsableB3)
		{
			int offB3 = ( mvClipB3.IsBackward() ) ? 1 : -1;
			offB3 *= mvClipB3.GetDeltaFrame();
			refB3 = super->GetFrame(n + offB3, env);
		}

  		if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
		{
			if (isUsableF3)
			{
				pRefF3[0] = refF3->GetReadPtr();
				pRefF3[1] = pRefF3[0] + refF3->GetRowSize()/2;
				pRefF3[2] = pRefF3[1] + refF3->GetRowSize()/4;
				nRefF3Pitches[0]  = refF3->GetPitch();
				nRefF3Pitches[1]  = nRefF3Pitches[0];
				nRefF3Pitches[2]  = nRefF3Pitches[0];
			}
			if (isUsableF2)
			{
				pRefF2[0] = refF2->GetReadPtr();
				pRefF2[1] = pRefF2[0] + refF2->GetRowSize()/2;
				pRefF2[2] = pRefF2[1] + refF2->GetRowSize()/4;
				nRefF2Pitches[0]  = refF2->GetPitch();
				nRefF2Pitches[1]  = nRefF2Pitches[0];
				nRefF2Pitches[2]  = nRefF2Pitches[0];
			}
			if (isUsableF)
			{
				pRefF[0] = refF->GetReadPtr();
				pRefF[1] = pRefF[0] + refF->GetRowSize()/2;
				pRefF[2] = pRefF[1] + refF->GetRowSize()/4;
				nRefFPitches[0]  = refF->GetPitch();
				nRefFPitches[1]  = nRefFPitches[0];
				nRefFPitches[2]  = nRefFPitches[0];
			}
			if (isUsableB)
			{
				pRefB[0] = refB->GetReadPtr();
				pRefB[1] = pRefB[0] + refB->GetRowSize()/2;
				pRefB[2] = pRefB[1] + refB->GetRowSize()/4;
				nRefBPitches[0]  = refB->GetPitch();
				nRefBPitches[1]  = nRefBPitches[0];
				nRefBPitches[2]  = nRefBPitches[0];
			}
			if (isUsableB2)
			{
				pRefB2[0] = refB2->GetReadPtr();
				pRefB2[1] = pRefB2[0] + refB2->GetRowSize()/2;
				pRefB2[2] = pRefB2[1] + refB2->GetRowSize()/4;
				nRefB2Pitches[0]  = refB2->GetPitch();
				nRefB2Pitches[1]  = nRefB2Pitches[0];
				nRefB2Pitches[2]  = nRefB2Pitches[0];
			}
			if (isUsableB3)
			{
				pRefB3[0] = refB3->GetReadPtr();
				pRefB3[1] = pRefB3[0] + refB3->GetRowSize()/2;
				pRefB3[2] = pRefB3[1] + refB3->GetRowSize()/4;
				nRefB3Pitches[0]  = refB3->GetPitch();
				nRefB3Pitches[1]  = nRefB3Pitches[0];
				nRefB3Pitches[2]  = nRefB3Pitches[0];
			}
		}
		else
		{
			if (isUsableF3)
			{
			 pRefF3[0] = YRPLAN(refF3);
			 pRefF3[1] = URPLAN(refF3);
			 pRefF3[2] = VRPLAN(refF3);
			 nRefF3Pitches[0] = YPITCH(refF3);
			 nRefF3Pitches[1] = UPITCH(refF3);
			 nRefF3Pitches[2] = VPITCH(refF3);
			}
			if (isUsableF2)
			{
			 pRefF2[0] = YRPLAN(refF2);
			 pRefF2[1] = URPLAN(refF2);
			 pRefF2[2] = VRPLAN(refF2);
			 nRefF2Pitches[0] = YPITCH(refF2);
			 nRefF2Pitches[1] = UPITCH(refF2);
			 nRefF2Pitches[2] = VPITCH(refF2);
			}
			if (isUsableF)
			{
			 pRefF[0] = YRPLAN(refF);
			 pRefF[1] = URPLAN(refF);
			 pRefF[2] = VRPLAN(refF);
			 nRefFPitches[0] = YPITCH(refF);
			 nRefFPitches[1] = UPITCH(refF);
			 nRefFPitches[2] = VPITCH(refF);
			}
			if (isUsableB)
			{
			 pRefB[0] = YRPLAN(refB);
			 pRefB[1] = URPLAN(refB);
			 pRefB[2] = VRPLAN(refB);
			 nRefBPitches[0] = YPITCH(refB);
			 nRefBPitches[1] = UPITCH(refB);
			 nRefBPitches[2] = VPITCH(refB);
			}
			if (isUsableB2)
			{
			 pRefB2[0] = YRPLAN(refB2);
			 pRefB2[1] = URPLAN(refB2);
			 pRefB2[2] = VRPLAN(refB2);
			 nRefB2Pitches[0] = YPITCH(refB2);
			 nRefB2Pitches[1] = UPITCH(refB2);
			 nRefB2Pitches[2] = VPITCH(refB2);
			}
			if (isUsableB3)
			{
			 pRefB3[0] = YRPLAN(refB3);
			 pRefB3[1] = URPLAN(refB3);
			 pRefB3[2] = VRPLAN(refB3);
			 nRefB3Pitches[0] = YPITCH(refB3);
			 nRefB3Pitches[1] = UPITCH(refB3);
			 nRefB3Pitches[2] = VPITCH(refB3);
			}
		}
            pRefF3GOF->Update(YUVplanes, (BYTE*)pRefF3[0], nRefF3Pitches[0], (BYTE*)pRefF3[1], nRefF3Pitches[1], (BYTE*)pRefF3[2], nRefF3Pitches[2]);
            pRefF2GOF->Update(YUVplanes, (BYTE*)pRefF2[0], nRefF2Pitches[0], (BYTE*)pRefF2[1], nRefF2Pitches[1], (BYTE*)pRefF2[2], nRefF2Pitches[2]);
            pRefFGOF->Update(YUVplanes, (BYTE*)pRefF[0], nRefFPitches[0], (BYTE*)pRefF[1], nRefFPitches[1], (BYTE*)pRefF[2], nRefFPitches[2]);
            pRefBGOF->Update(YUVplanes, (BYTE*)pRefB[0], nRefBPitches[0], (BYTE*)pRefB[1], nRefBPitches[1], (BYTE*)pRefB[2], nRefBPitches[2]);// v2.0
            pRefB2GOF->Update(YUVplanes, (BYTE*)pRefB2[0], nRefB2Pitches[0], (BYTE*)pRefB2[1], nRefB2Pitches[1], (BYTE*)pRefB2[2], nRefB2Pitches[2]);// v2.0
            pRefB3GOF->Update(YUVplanes, (BYTE*)pRefB3[0], nRefB3Pitches[0], (BYTE*)pRefB3[1], nRefB3Pitches[1], (BYTE*)pRefB3[2], nRefB3Pitches[2]);// v2.0

         MVPlane *pPlanesB[3];
         MVPlane *pPlanesF[3];
         MVPlane *pPlanesB2[3];
         MVPlane *pPlanesF2[3];
         MVPlane *pPlanesB3[3];
         MVPlane *pPlanesF3[3];

		if (isUsableF3)
		{
			if (YUVplanes & YPLANE)
			 pPlanesF3[0] = pRefF3GOF->GetFrame(0)->GetPlane(YPLANE);
			if (YUVplanes & UPLANE)
			 pPlanesF3[1] = pRefF3GOF->GetFrame(0)->GetPlane(UPLANE);
			if (YUVplanes & VPLANE)
			 pPlanesF3[2] = pRefF3GOF->GetFrame(0)->GetPlane(VPLANE);
		}
		if (isUsableF2)
		{
			if (YUVplanes & YPLANE)
			 pPlanesF2[0] = pRefF2GOF->GetFrame(0)->GetPlane(YPLANE);
			if (YUVplanes & UPLANE)
			 pPlanesF2[1] = pRefF2GOF->GetFrame(0)->GetPlane(UPLANE);
			if (YUVplanes & VPLANE)
			 pPlanesF2[2] = pRefF2GOF->GetFrame(0)->GetPlane(VPLANE);
		}
		if (isUsableF)
		{
			if (YUVplanes & YPLANE)
	         pPlanesF[0] = pRefFGOF->GetFrame(0)->GetPlane(YPLANE);
			if (YUVplanes & UPLANE)
			 pPlanesF[1] = pRefFGOF->GetFrame(0)->GetPlane(UPLANE);
			if (YUVplanes & VPLANE)
	         pPlanesF[2] = pRefFGOF->GetFrame(0)->GetPlane(VPLANE);
		}
		if (isUsableB)
		{
			if (YUVplanes & YPLANE)
	         pPlanesB[0] = pRefBGOF->GetFrame(0)->GetPlane(YPLANE);
			if (YUVplanes & UPLANE)
	         pPlanesB[1] = pRefBGOF->GetFrame(0)->GetPlane(UPLANE);
			if (YUVplanes & VPLANE)
	         pPlanesB[2] = pRefBGOF->GetFrame(0)->GetPlane(VPLANE);
		}
		if (isUsableB2)
		{
			if (YUVplanes & YPLANE)
	         pPlanesB2[0] = pRefB2GOF->GetFrame(0)->GetPlane(YPLANE);
			if (YUVplanes & UPLANE)
	        pPlanesB2[1] = pRefB2GOF->GetFrame(0)->GetPlane(UPLANE);
			if (YUVplanes & VPLANE)
	         pPlanesB2[2] = pRefB2GOF->GetFrame(0)->GetPlane(VPLANE);
		}
		if (isUsableB3)
		{
			if (YUVplanes & YPLANE)
	         pPlanesB3[0] = pRefB3GOF->GetFrame(0)->GetPlane(YPLANE);
			if (YUVplanes & UPLANE)
	        pPlanesB3[1] = pRefB3GOF->GetFrame(0)->GetPlane(UPLANE);
			if (YUVplanes & VPLANE)
	         pPlanesB3[2] = pRefB3GOF->GetFrame(0)->GetPlane(VPLANE);
		}

         PROFILE_START(MOTION_PROFILE_COMPENSATION);
		 pDstCur[0] = pDst[0];
		 pDstCur[1] = pDst[1];
		 pDstCur[2] = pDst[2];
		 pSrcCur[0] = pSrc[0];
		 pSrcCur[1] = pSrc[1];
		 pSrcCur[2] = pSrc[2];
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

	 // LUMA plane Y

  if (!(YUVplanes & YPLANE))
		BitBlt(pDstCur[0], nDstPitches[0], pSrcCur[0], nSrcPitches[0], nWidth, nHeight, isse);
  else
  {
	  if (nOverlapX==0 && nOverlapY==0)
	  {
		for (int by=0; by<nBlkY; by++)
		{
			int xx = 0;
			for (int bx=0; bx<nBlkX; bx++)
			{
				int i = by*nBlkX + bx;
				const BYTE * pB, *pF, *pB2, *pF2, *pB3, *pF3;
				int npB, npF, npB2, npF2, npB3, npF3;

				if (isUsableB)
				{
					const FakeBlockData &blockB = mvClipB.GetBlock(0, i);
					int blxB = blockB.GetX() * nPel + blockB.GetMV().x;
					int blyB = blockB.GetY() * nPel + blockB.GetMV().y;
					pB = pPlanesB[0]->GetPointer(blxB, blyB);
					npB = pPlanesB[0]->GetPitch();
					int blockSAD = blockB.GetSAD();
					WRefB = DegrainWeight(thSAD, blockSAD);
				}
				else
				{
					pB = pSrcCur[0]+ xx;
					npB = nSrcPitches[0];
					WRefB = 0;
				}
				if (isUsableF)
				{
					const FakeBlockData &blockF = mvClipF.GetBlock(0, i);
					int blxF = blockF.GetX() * nPel + blockF.GetMV().x;
					int blyF = blockF.GetY() * nPel + blockF.GetMV().y;
					pF = pPlanesF[0]->GetPointer(blxF, blyF);
					npF = pPlanesF[0]->GetPitch();
					int blockSAD = blockF.GetSAD();
					WRefF = DegrainWeight(thSAD, blockSAD);
				}
				else
				{
					pF = pSrcCur[0]+ xx;
					npF = nSrcPitches[0];
					WRefF = 0;
				}
				if (isUsableB2)
				{
					const FakeBlockData &blockB2 = mvClipB2.GetBlock(0, i);
					int blxB2 = blockB2.GetX() * nPel + blockB2.GetMV().x;
					int blyB2 = blockB2.GetY() * nPel + blockB2.GetMV().y;
					pB2 = pPlanesB2[0]->GetPointer(blxB2, blyB2);
					npB2 = pPlanesB2[0]->GetPitch();
					int blockSAD = blockB2.GetSAD();
					WRefB2 = DegrainWeight(thSAD, blockSAD);
				}
				else
				{
					pB2 = pSrcCur[0]+ xx;
					npB2 = nSrcPitches[0];
					WRefB2 = 0;
				}
				if (isUsableF2)
				{
					const FakeBlockData &blockF2 = mvClipF2.GetBlock(0, i);
					int blxF2 = blockF2.GetX() * nPel + blockF2.GetMV().x;
					int blyF2 = blockF2.GetY() * nPel + blockF2.GetMV().y;
					pF2 = pPlanesF2[0]->GetPointer(blxF2, blyF2);
					npF2 = pPlanesF2[0]->GetPitch();
					int blockSAD = blockF2.GetSAD();
					WRefF2 = DegrainWeight(thSAD, blockSAD);
				}
				else
				{
					pF2 = pSrcCur[0]+ xx;
					npF2 = nSrcPitches[0];
					WRefF2 = 0;
				}
				if (isUsableB3)
				{
					const FakeBlockData &blockB3 = mvClipB3.GetBlock(0, i);
					int blxB3 = blockB3.GetX() * nPel + blockB3.GetMV().x;
					int blyB3 = blockB3.GetY() * nPel + blockB3.GetMV().y;
					pB3 = pPlanesB3[0]->GetPointer(blxB3, blyB3);
					npB3 = pPlanesB3[0]->GetPitch();
					int blockSAD = blockB3.GetSAD();
					WRefB3 = DegrainWeight(thSAD, blockSAD);
				}
				else
				{
					pB3 = pSrcCur[0]+ xx;
					npB3 = nSrcPitches[0];
					WRefB3 = 0;
				}
				if (isUsableF3)
				{
					const FakeBlockData &blockF3 = mvClipF3.GetBlock(0, i);
					int blxF3 = blockF3.GetX() * nPel + blockF3.GetMV().x;
					int blyF3 = blockF3.GetY() * nPel + blockF3.GetMV().y;
					pF3 = pPlanesF3[0]->GetPointer(blxF3, blyF3);
					npF3 = pPlanesF3[0]->GetPitch();
					int blockSAD = blockF3.GetSAD();
					WRefF3 = DegrainWeight(thSAD, blockSAD);
				}
				else
				{
					pF3 = pSrcCur[0]+ xx;
					npF3 = nSrcPitches[0];
					WRefF3 = 0;
				}
				WSrc = 256;
				int WSum = WRefB + WRefF + WSrc + WRefB2 + WRefF2  + WRefB3 + WRefF3 + 1;
				WRefB = WRefB*256/WSum; // normailize weights to 256
				WRefF = WRefF*256/WSum;
				WRefB2 = WRefB2*256/WSum;
				WRefF2 = WRefF2*256/WSum;
				WRefB3 = WRefB3*256/WSum;
				WRefF3 = WRefF3*256/WSum;
				WSrc = 256 - WRefB - WRefF - WRefB2 - WRefF2 - WRefB3 - WRefF3;
					// luma
				DEGRAINLUMA(pDstCur[0] + xx, nDstPitches[0], pSrcCur[0]+ xx, nSrcPitches[0],
						pB, npB, pF, npF, pB2, npB2, pF2, npF2, pB3, npB3, pF3, npF3,
						WSrc, WRefB, WRefF, WRefB2, WRefF2, WRefB3, WRefF3);

				xx += (nBlkSizeX);

					if (bx == nBlkX-1 && nWidth_B < nWidth) // right non-covered region
					{
						// luma
						BitBlt(pDstCur[0] + nWidth_B, nDstPitches[0],
							pSrcCur[0] + nWidth_B, nSrcPitches[0], nWidth-nWidth_B, nBlkSizeY, isse);
					}
			}
               pDstCur[0] += ( nBlkSizeY ) * (nDstPitches[0]);
               pSrcCur[0] += ( nBlkSizeY ) * (nSrcPitches[0]);

			if (by == nBlkY-1 && nHeight_B < nHeight) // bottom uncovered region
			{
				// luma
				BitBlt(pDstCur[0], nDstPitches[0], pSrcCur[0], nSrcPitches[0], nWidth, nHeight-nHeight_B, isse);
			}
		}
	  }
// -----------------------------------------------------------------
	  else // overlap
	  {
		pDstShort = DstShort;
		MemZoneSet(reinterpret_cast<unsigned char*>(pDstShort), 0, nWidth_B*2, nHeight_B, 0, 0, dstShortPitch*2);

		for (int by=0; by<nBlkY; by++)
		{
			int wby = ((by + nBlkY - 3)/(nBlkY - 2))*3;
			int xx = 0;
			for (int bx=0; bx<nBlkX; bx++)
			{
					// select window
                    int wbx = (bx + nBlkX - 3)/(nBlkX - 2);
                    winOver = OverWins->GetWindow(wby + wbx);

					int i = by*nBlkX + bx;
				const BYTE * pB, *pF, *pB2, *pF2, *pB3, *pF3;
				int npB, npF, npB2, npF2, npB3, npF3;

				if (isUsableB)
				{
					const FakeBlockData &blockB = mvClipB.GetBlock(0, i);
					int blxB = blockB.GetX() * nPel + blockB.GetMV().x;
					int blyB = blockB.GetY() * nPel + blockB.GetMV().y;
					pB = pPlanesB[0]->GetPointer(blxB, blyB);
					npB = pPlanesB[0]->GetPitch();
					int blockSAD = blockB.GetSAD();
					WRefB = DegrainWeight(thSAD, blockSAD);
				}
				else
				{
					pB = pSrcCur[0]+ xx;
					npB = nSrcPitches[0];
					WRefB = 0;
				}
				if (isUsableF)
				{
					const FakeBlockData &blockF = mvClipF.GetBlock(0, i);
					int blxF = blockF.GetX() * nPel + blockF.GetMV().x;
					int blyF = blockF.GetY() * nPel + blockF.GetMV().y;
					pF = pPlanesF[0]->GetPointer(blxF, blyF);
					npF = pPlanesF[0]->GetPitch();
					int blockSAD = blockF.GetSAD();
					WRefF = DegrainWeight(thSAD, blockSAD);
				}
				else
				{
					pF = pSrcCur[0]+ xx;
					npF = nSrcPitches[0];
					WRefF = 0;
				}
				if (isUsableB2)
				{
					const FakeBlockData &blockB2 = mvClipB2.GetBlock(0, i);
					int blxB2 = blockB2.GetX() * nPel + blockB2.GetMV().x;
					int blyB2 = blockB2.GetY() * nPel + blockB2.GetMV().y;
					pB2 = pPlanesB2[0]->GetPointer(blxB2, blyB2);
					npB2 = pPlanesB2[0]->GetPitch();
					int blockSAD = blockB2.GetSAD();
					WRefB2 = DegrainWeight(thSAD, blockSAD);
				}
				else
				{
					pB2 = pSrcCur[0]+ xx;
					npB2 = nSrcPitches[0];
					WRefB2 = 0;
				}
				if (isUsableF2)
				{
					const FakeBlockData &blockF2 = mvClipF2.GetBlock(0, i);
					int blxF2 = blockF2.GetX() * nPel + blockF2.GetMV().x;
					int blyF2 = blockF2.GetY() * nPel + blockF2.GetMV().y;
					pF2 = pPlanesF2[0]->GetPointer(blxF2, blyF2);
					npF2 = pPlanesF2[0]->GetPitch();
					int blockSAD = blockF2.GetSAD();
					WRefF2 = DegrainWeight(thSAD, blockSAD);
				}
				else
				{
					pF2 = pSrcCur[0]+ xx;
					npF2 = nSrcPitches[0];
					WRefF2 = 0;
				}
				if (isUsableB3)
				{
					const FakeBlockData &blockB3 = mvClipB3.GetBlock(0, i);
					int blxB3 = blockB3.GetX() * nPel + blockB3.GetMV().x;
					int blyB3 = blockB3.GetY() * nPel + blockB3.GetMV().y;
					pB3 = pPlanesB3[0]->GetPointer(blxB3, blyB3);
					npB3 = pPlanesB3[0]->GetPitch();
					int blockSAD = blockB3.GetSAD();
					WRefB3 = DegrainWeight(thSAD, blockSAD);
				}
				else
				{
					pB3 = pSrcCur[0]+ xx;
					npB3 = nSrcPitches[0];
					WRefB3 = 0;
				}
				if (isUsableF3)
				{
					const FakeBlockData &blockF3 = mvClipF3.GetBlock(0, i);
					int blxF3 = blockF3.GetX() * nPel + blockF3.GetMV().x;
					int blyF3 = blockF3.GetY() * nPel + blockF3.GetMV().y;
					pF3 = pPlanesF3[0]->GetPointer(blxF3, blyF3);
					npF3 = pPlanesF3[0]->GetPitch();
					int blockSAD = blockF3.GetSAD();
					WRefF3 = DegrainWeight(thSAD, blockSAD);
				}
				else
				{
					pF3 = pSrcCur[0]+ xx;
					npF3 = nSrcPitches[0];
					WRefF3 = 0;
				}
				WSrc = 256;
				int WSum = WRefB + WRefF + WSrc + WRefB2 + WRefF2 + WRefB3 + WRefF3 + 1;
				WRefB = WRefB*256/WSum; // normailize weights to 256
				WRefF = WRefF*256/WSum;
				WRefB2 = WRefB2*256/WSum;
				WRefF2 = WRefF2*256/WSum;
				WRefB3 = WRefB3*256/WSum;
				WRefF3 = WRefF3*256/WSum;
				WSrc = 256 - WRefB - WRefF - WRefB2 - WRefF2 - WRefB3 - WRefF3;
					// luma
					DEGRAINLUMA(tmpBlock, tmpPitch, pSrcCur[0]+ xx, nSrcPitches[0],
						pB, npB, pF, npF, pB2, npB2, pF2, npF2, pB3, npB3, pF3, npF3,
						WSrc, WRefB, WRefF, WRefB2, WRefF2, WRefB3, WRefF3);
				OVERSLUMA(pDstShort + xx, dstShortPitch, tmpBlock, tmpPitch, winOver, nBlkSizeX);

					xx += (nBlkSizeX - nOverlapX);

			}
               pSrcCur[0] += (nBlkSizeY - nOverlapY) * (nSrcPitches[0]);
               pDstShort += (nBlkSizeY - nOverlapY) * dstShortPitch;
		}
		Short2Bytes(pDst[0], nDstPitches[0], DstShort, dstShortPitch, nWidth_B, nHeight_B);
		if (nWidth_B < nWidth)
			BitBlt(pDst[0] + nWidth_B, nDstPitches[0], pSrc[0] + nWidth_B, nSrcPitches[0], nWidth-nWidth_B, nHeight_B, isse);
		if (nHeight_B < nHeight) // bottom noncovered region
			BitBlt(pDst[0] + nHeight_B*nDstPitches[0], nDstPitches[0], pSrc[0] + nHeight_B*nSrcPitches[0], nSrcPitches[0], nWidth, nHeight-nHeight_B, isse);
	  }
	  if (nLimit < 255) {
	  if (isse)
        LimitChanges_mmx(pDst[0], nDstPitches[0], pSrc[0], nSrcPitches[0], nWidth, nHeight, nLimit);
      else
        LimitChanges_c(pDst[0], nDstPitches[0], pSrc[0], nSrcPitches[0], nWidth, nHeight, nLimit);
	  }
  }
//----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
 // CHROMA plane U
  if (!(YUVplanes & UPLANE & nSuperModeYUV)) // v2.0
		BitBlt(pDstCur[1], nDstPitches[1], pSrcCur[1], nSrcPitches[1], nWidth>>1, nHeight/yRatioUV, isse);
  else
  {
	  if (nOverlapX==0 && nOverlapY==0)
	  {
		for (int by=0; by<nBlkY; by++)
		{
			int xx = 0;
			for (int bx=0; bx<nBlkX; bx++)
			{
				int i = by*nBlkX + bx;
				const BYTE * pBU, *pFU, *pB2U, *pF2U, *pB3U, *pF3U;
				int npBU, npFU, npB2U, npF2U, npB3U, npF3U;

				if (isUsableB)
				{
					const FakeBlockData &blockB = mvClipB.GetBlock(0, i);
					int blxB = blockB.GetX() * nPel + blockB.GetMV().x;
					int blyB = blockB.GetY() * nPel + blockB.GetMV().y;
					pBU = pPlanesB[1]->GetPointer(blxB>>1, blyB/yRatioUV);
					npBU = pPlanesB[1]->GetPitch();
					int blockSAD = blockB.GetSAD();
					WRefB = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pBU = pSrcCur[1]+ (xx>>1);
					npBU = nSrcPitches[1];
					WRefB = 0;
				}
				if (isUsableF)
				{
					const FakeBlockData &blockF = mvClipF.GetBlock(0, i);
					int blxF = blockF.GetX() * nPel + blockF.GetMV().x;
					int blyF = blockF.GetY() * nPel + blockF.GetMV().y;
					pFU = pPlanesF[1]->GetPointer(blxF>>1, blyF/yRatioUV);
					npFU = pPlanesF[1]->GetPitch();
					int blockSAD = blockF.GetSAD();
					WRefF = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pFU = pSrcCur[1]+ (xx>>1);
					npFU = nSrcPitches[1];
					WRefF = 0;
				}
				if (isUsableB2)
				{
					const FakeBlockData &blockB2 = mvClipB2.GetBlock(0, i);
					int blxB2 = blockB2.GetX() * nPel + blockB2.GetMV().x;
					int blyB2 = blockB2.GetY() * nPel + blockB2.GetMV().y;
					pB2U = pPlanesB2[1]->GetPointer(blxB2>>1, blyB2/yRatioUV);
					npB2U = pPlanesB2[1]->GetPitch();
					int blockSAD = blockB2.GetSAD();
					WRefB2 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pB2U = pSrcCur[1]+ (xx>>1);
					npB2U = nSrcPitches[1];
					WRefB2 = 0;
				}
				if (isUsableF2)
				{
					const FakeBlockData &blockF2 = mvClipF2.GetBlock(0, i);
					int blxF2 = blockF2.GetX() * nPel + blockF2.GetMV().x;
					int blyF2 = blockF2.GetY() * nPel + blockF2.GetMV().y;
					pF2U = pPlanesF2[1]->GetPointer(blxF2>>1, blyF2/yRatioUV);
					npF2U = pPlanesF2[1]->GetPitch();
					int blockSAD = blockF2.GetSAD();
					WRefF2 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pF2U = pSrcCur[1]+ (xx>>1);
					npF2U = nSrcPitches[1];
					WRefF2 = 0;
				}
				if (isUsableB3)
				{
					const FakeBlockData &blockB3 = mvClipB3.GetBlock(0, i);
					int blxB3 = blockB3.GetX() * nPel + blockB3.GetMV().x;
					int blyB3 = blockB3.GetY() * nPel + blockB3.GetMV().y;
					pB3U = pPlanesB3[1]->GetPointer(blxB3>>1, blyB3/yRatioUV);
					npB3U = pPlanesB3[1]->GetPitch();
					int blockSAD = blockB3.GetSAD();
					WRefB3 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pB3U = pSrcCur[1]+ (xx>>1);
					npB3U = nSrcPitches[1];
					WRefB3 = 0;
				}
				if (isUsableF3)
				{
					const FakeBlockData &blockF3 = mvClipF3.GetBlock(0, i);
					int blxF3 = blockF3.GetX() * nPel + blockF3.GetMV().x;
					int blyF3 = blockF3.GetY() * nPel + blockF3.GetMV().y;
					pF3U = pPlanesF3[1]->GetPointer(blxF3>>1, blyF3/yRatioUV);
					npF3U = pPlanesF3[1]->GetPitch();
					int blockSAD = blockF3.GetSAD();
					WRefF3 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pF3U = pSrcCur[1]+ (xx>>1);
					npF3U = nSrcPitches[1];
					WRefF3 = 0;
				}
				WSrc = 256;
				int WSum = WRefB + WRefF + WSrc + WRefB2 + WRefF2 + WRefB3 + WRefF3 + 1;
				WRefB = WRefB*256/WSum; // normailize weights to 256
				WRefF = WRefF*256/WSum;
				WRefB2 = WRefB2*256/WSum;
				WRefF2 = WRefF2*256/WSum;
				WRefB3 = WRefB3*256/WSum;
				WRefF3 = WRefF3*256/WSum;
				WSrc = 256 - WRefB - WRefF - WRefB2 - WRefF2 - WRefB3 - WRefF3;
					// chroma u
					DEGRAINCHROMA(pDstCur[1] + (xx>>1), nDstPitches[1], pSrcCur[1]+ (xx>>1), nSrcPitches[1],
						pBU, npBU, pFU, npFU, pB2U, npB2U, pF2U, npF2U, pB3U, npB3U, pF3U, npF3U,
						WSrc, WRefB, WRefF, WRefB2, WRefF2, WRefB3, WRefF3);

					xx += (nBlkSizeX);

					if (bx == nBlkX-1 && nWidth_B < nWidth) // right non-covered region
					{
						// chroma u
						BitBlt(pDstCur[1] + (nWidth_B>>1), nDstPitches[1],
							pSrcCur[1] + (nWidth_B>>1), nSrcPitches[1], (nWidth-nWidth_B)>>1, (nBlkSizeY)/yRatioUV, isse);
					}
			}
               pDstCur[1] += ( nBlkSizeY )/yRatioUV * (nDstPitches[1]) ;
               pSrcCur[1] += ( nBlkSizeY )/yRatioUV * (nSrcPitches[1]) ;

			if (by == nBlkY-1 && nHeight_B < nHeight) // bottom uncovered region
			{
				// chroma u
				BitBlt(pDstCur[1], nDstPitches[1], pSrcCur[1], nSrcPitches[1], nWidth>>1, (nHeight-nHeight_B)/yRatioUV, isse);
			}
		}
	  }
// -----------------------------------------------------------------
	  else // overlap
	  {
		pDstShort = DstShort;
		MemZoneSet(reinterpret_cast<unsigned char*>(pDstShort), 0, nWidth_B, nHeight_B/yRatioUV, 0, 0, dstShortPitch*2);
		for (int by=0; by<nBlkY; by++)
		{
			int wby = ((by + nBlkY - 3)/(nBlkY - 2))*3;
			int xx = 0;
			for (int bx=0; bx<nBlkX; bx++)
			{
					// select window
                    int wbx = (bx + nBlkX - 3)/(nBlkX - 2);
                    winOverUV = OverWinsUV->GetWindow(wby + wbx);

					int i = by*nBlkX + bx;
				const BYTE * pBU, *pFU, *pB2U, *pF2U, *pB3U, *pF3U;
				int npBU, npFU, npB2U, npF2U, npB3U, npF3U;

				if (isUsableB)
				{
					const FakeBlockData &blockB = mvClipB.GetBlock(0, i);
					int blxB = blockB.GetX() * nPel + blockB.GetMV().x;
					int blyB = blockB.GetY() * nPel + blockB.GetMV().y;
					pBU = pPlanesB[1]->GetPointer(blxB>>1, blyB/yRatioUV);
					npBU = pPlanesB[1]->GetPitch();
					int blockSAD = blockB.GetSAD();
					WRefB = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pBU = pSrcCur[1]+ (xx>>1);
					npBU = nSrcPitches[1];
					WRefB = 0;
				}
				if (isUsableF)
				{
					const FakeBlockData &blockF = mvClipF.GetBlock(0, i);
					int blxF = blockF.GetX() * nPel + blockF.GetMV().x;
					int blyF = blockF.GetY() * nPel + blockF.GetMV().y;
					pFU = pPlanesF[1]->GetPointer(blxF>>1, blyF/yRatioUV);
					npFU = pPlanesF[1]->GetPitch();
					int blockSAD = blockF.GetSAD();
					WRefF = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pFU = pSrcCur[1]+ (xx>>1);
					npFU = nSrcPitches[1];
					WRefF = 0;
				}
				if (isUsableB2)
				{
					const FakeBlockData &blockB2 = mvClipB2.GetBlock(0, i);
					int blxB2 = blockB2.GetX() * nPel + blockB2.GetMV().x;
					int blyB2 = blockB2.GetY() * nPel + blockB2.GetMV().y;
					pB2U = pPlanesB2[1]->GetPointer(blxB2>>1, blyB2/yRatioUV);
					npB2U = pPlanesB2[1]->GetPitch();
					int blockSAD = blockB2.GetSAD();
					WRefB2 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pB2U = pSrcCur[1]+ (xx>>1);
					npB2U = nSrcPitches[1];
					WRefB2 = 0;
				}
				if (isUsableF2)
				{
					const FakeBlockData &blockF2 = mvClipF2.GetBlock(0, i);
					int blxF2 = blockF2.GetX() * nPel + blockF2.GetMV().x;
					int blyF2 = blockF2.GetY() * nPel + blockF2.GetMV().y;
					pF2U = pPlanesF2[1]->GetPointer(blxF2>>1, blyF2/yRatioUV);
					npF2U = pPlanesF2[1]->GetPitch();
					int blockSAD = blockF2.GetSAD();
					WRefF2 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pF2U = pSrcCur[1]+ (xx>>1);
					npF2U = nSrcPitches[1];
					WRefF2 = 0;
				}
				if (isUsableB3)
				{
					const FakeBlockData &blockB3 = mvClipB3.GetBlock(0, i);
					int blxB3 = blockB3.GetX() * nPel + blockB3.GetMV().x;
					int blyB3 = blockB3.GetY() * nPel + blockB3.GetMV().y;
					pB3U = pPlanesB3[1]->GetPointer(blxB3>>1, blyB3/yRatioUV);
					npB3U = pPlanesB3[1]->GetPitch();
					int blockSAD = blockB3.GetSAD();
					WRefB3 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pB3U = pSrcCur[1]+ (xx>>1);
					npB3U = nSrcPitches[1];
					WRefB3 = 0;
				}
				if (isUsableF3)
				{
					const FakeBlockData &blockF3 = mvClipF3.GetBlock(0, i);
					int blxF3 = blockF3.GetX() * nPel + blockF3.GetMV().x;
					int blyF3 = blockF3.GetY() * nPel + blockF3.GetMV().y;
					pF3U = pPlanesF3[1]->GetPointer(blxF3>>1, blyF3/yRatioUV);
					npF3U = pPlanesF3[1]->GetPitch();
					int blockSAD = blockF3.GetSAD();
					WRefF3 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pF3U = pSrcCur[1]+ (xx>>1);
					npF3U = nSrcPitches[1];
					WRefF3 = 0;
				}
				WSrc = 256;
				int WSum = WRefB + WRefF + WSrc + WRefB2 + WRefF2 + WRefB3 + WRefF3 + 1;
				WRefB = WRefB*256/WSum; // normailize weights to 256
				WRefF = WRefF*256/WSum;
				WRefB2 = WRefB2*256/WSum;
				WRefF2 = WRefF2*256/WSum;
				WRefB3 = WRefB3*256/WSum;
				WRefF3 = WRefF3*256/WSum;
				WSrc = 256 - WRefB - WRefF - WRefB2 - WRefF2 - WRefB3 - WRefF3;
					// chroma u
					DEGRAINCHROMA(tmpBlock, tmpPitch, pSrcCur[1]+ (xx>>1), nSrcPitches[1],
						pBU, npBU, pFU, npFU, pB2U, npB2U, pF2U, npF2U, pB3U, npB3U, pF3U, npF3U,
						WSrc, WRefB, WRefF, WRefB2, WRefF2, WRefB3, WRefF3);
				OVERSCHROMA(pDstShort + (xx>>1), dstShortPitch, tmpBlock, tmpPitch, winOverUV, nBlkSizeX/2);

					xx += (nBlkSizeX - nOverlapX);

			}
               pSrcCur[1] += (nBlkSizeY - nOverlapY)/yRatioUV * (nSrcPitches[1]) ;
              pDstShort += (nBlkSizeY - nOverlapY)/yRatioUV * dstShortPitch;
		}
		Short2Bytes(pDst[1], nDstPitches[1], DstShort, dstShortPitch, nWidth_B>>1, nHeight_B/yRatioUV);
		if (nWidth_B < nWidth)
			BitBlt(pDst[1] + (nWidth_B>>1), nDstPitches[1], pSrc[1] + (nWidth_B>>1), nSrcPitches[1], (nWidth-nWidth_B)>>1, nHeight_B/yRatioUV, isse);
		if (nHeight_B < nHeight) // bottom noncovered region
			BitBlt(pDst[1] + nDstPitches[1]*nHeight_B/yRatioUV, nDstPitches[1], pSrc[1] + nSrcPitches[1]*nHeight_B/yRatioUV, nSrcPitches[1], nWidth>>1, (nHeight-nHeight_B)/yRatioUV, isse);
	  }
	  if (nLimit < 255) {
	  if (isse)
        LimitChanges_mmx(pDst[1], nDstPitches[1], pSrc[1], nSrcPitches[1], nWidth/2, nHeight/yRatioUV, nLimit);
      else
        LimitChanges_c(pDst[1], nDstPitches[1], pSrc[1], nSrcPitches[1], nWidth/2, nHeight/yRatioUV, nLimit);
	  }
  }
//----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------
 // CHROMA plane V
  if (!(YUVplanes & VPLANE & nSuperModeYUV))
		BitBlt(pDstCur[2], nDstPitches[2], pSrcCur[2], nSrcPitches[2], nWidth>>1, nHeight/yRatioUV, isse);
  else
  {
	  if (nOverlapX==0 && nOverlapY==0)
	  {
		for (int by=0; by<nBlkY; by++)
		{
			int xx = 0;
			for (int bx=0; bx<nBlkX; bx++)
			{
				int i = by*nBlkX + bx;
				const BYTE * pBV, *pFV, *pB2V, *pF2V, *pB3V, *pF3V;
				int npBV, npFV, npB2V, npF2V, npB3V, npF3V;

				if (isUsableB)
				{
					const FakeBlockData &blockB = mvClipB.GetBlock(0, i);
					int blxB = blockB.GetX() * nPel + blockB.GetMV().x;
					int blyB = blockB.GetY() * nPel + blockB.GetMV().y;
					pBV = pPlanesB[2]->GetPointer(blxB>>1, blyB/yRatioUV);
					npBV = pPlanesB[2]->GetPitch();
					int blockSAD = blockB.GetSAD();
					WRefB = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pBV = pSrcCur[2]+ (xx>>1);
					npBV = nSrcPitches[2];
					WRefB = 0;
				}
				if (isUsableF)
				{
					const FakeBlockData &blockF = mvClipF.GetBlock(0, i);
					int blxF = blockF.GetX() * nPel + blockF.GetMV().x;
					int blyF = blockF.GetY() * nPel + blockF.GetMV().y;
					pFV = pPlanesF[2]->GetPointer(blxF>>1, blyF/yRatioUV);
					npFV = pPlanesF[2]->GetPitch();
					int blockSAD = blockF.GetSAD();
					WRefF = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pFV = pSrcCur[2]+ (xx>>1);
					npFV = nSrcPitches[2];
					WRefF = 0;
				}
				if (isUsableB2)
				{
					const FakeBlockData &blockB2 = mvClipB2.GetBlock(0, i);
					int blxB2 = blockB2.GetX() * nPel + blockB2.GetMV().x;
					int blyB2 = blockB2.GetY() * nPel + blockB2.GetMV().y;
					pB2V = pPlanesB2[2]->GetPointer(blxB2>>1, blyB2/yRatioUV);
					npB2V = pPlanesB2[2]->GetPitch();
					int blockSAD = blockB2.GetSAD();
					WRefB2 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pB2V = pSrcCur[2]+ (xx>>1);
					npB2V = nSrcPitches[2];
					WRefB2 = 0;
				}
				if (isUsableF2)
				{
					const FakeBlockData &blockF2 = mvClipF2.GetBlock(0, i);
					int blxF2 = blockF2.GetX() * nPel + blockF2.GetMV().x;
					int blyF2 = blockF2.GetY() * nPel + blockF2.GetMV().y;
					pF2V = pPlanesF2[2]->GetPointer(blxF2>>1, blyF2/yRatioUV);
					npF2V = pPlanesF2[2]->GetPitch();
					int blockSAD = blockF2.GetSAD();
					WRefF2 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pF2V = pSrcCur[2]+ (xx>>1);
					npF2V = nSrcPitches[2];
					WRefF2 = 0;
				}
				if (isUsableB3)
				{
					const FakeBlockData &blockB3 = mvClipB3.GetBlock(0, i);
					int blxB3 = blockB3.GetX() * nPel + blockB3.GetMV().x;
					int blyB3 = blockB3.GetY() * nPel + blockB3.GetMV().y;
					pB3V = pPlanesB3[2]->GetPointer(blxB3>>1, blyB3/yRatioUV);
					npB3V = pPlanesB3[2]->GetPitch();
					int blockSAD = blockB3.GetSAD();
					WRefB3 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pB3V = pSrcCur[2]+ (xx>>1);
					npB3V = nSrcPitches[2];
					WRefB3 = 0;
				}
				if (isUsableF3)
				{
					const FakeBlockData &blockF3 = mvClipF3.GetBlock(0, i);
					int blxF3 = blockF3.GetX() * nPel + blockF3.GetMV().x;
					int blyF3 = blockF3.GetY() * nPel + blockF3.GetMV().y;
					pF3V = pPlanesF3[2]->GetPointer(blxF3>>1, blyF3/yRatioUV);
					npF3V = pPlanesF3[2]->GetPitch();
					int blockSAD = blockF3.GetSAD();
					WRefF3 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pF3V = pSrcCur[2]+ (xx>>1);
					npF3V = nSrcPitches[2];
					WRefF3 = 0;
				}
				WSrc = 256;
				int WSum = WRefB + WRefF + WSrc + WRefB2 + WRefF2 + WRefB3 + WRefF3 + 1;
				WRefB = WRefB*256/WSum; // normailize weights to 256
				WRefF = WRefF*256/WSum;
				WRefB2 = WRefB2*256/WSum;
				WRefF2 = WRefF2*256/WSum;
				WRefB3 = WRefB3*256/WSum;
				WRefF3 = WRefF3*256/WSum;
				WSrc = 256 - WRefB - WRefF - WRefB2 - WRefF2 - WRefB3 - WRefF3;
					// chroma v
					DEGRAINCHROMA(pDstCur[2] + (xx>>1), nDstPitches[2], pSrcCur[2]+ (xx>>1), nSrcPitches[2],
						pBV, npBV, pFV, npFV, pB2V, npB2V, pF2V, npF2V, pB3V, npB3V, pF3V, npF3V,
						WSrc, WRefB, WRefF, WRefB2, WRefF2, WRefB3, WRefF3);

					xx += (nBlkSizeX);

					if (bx == nBlkX-1 && nWidth_B < nWidth) // right non-covered region
					{
						// chroma v
						BitBlt(pDstCur[2] + (nWidth_B>>1), nDstPitches[2],
							pSrcCur[2] + (nWidth_B>>1), nSrcPitches[2], (nWidth-nWidth_B)>>1, (nBlkSizeY)/yRatioUV, isse);
					}
			}
               pDstCur[2] += ( nBlkSizeY )/yRatioUV * (nDstPitches[2]) ;
               pSrcCur[2] += ( nBlkSizeY )/yRatioUV * (nSrcPitches[2]) ;

			if (by == nBlkY-1 && nHeight_B < nHeight) // bottom uncovered region
			{
				// chroma v
				BitBlt(pDstCur[2], nDstPitches[2], pSrcCur[2], nSrcPitches[2], nWidth>>1, (nHeight-nHeight_B)/yRatioUV, isse);
			}
		}
	  }
// -----------------------------------------------------------------
	  else // overlap
	  {
		pDstShort = DstShort;
		MemZoneSet(reinterpret_cast<unsigned char*>(pDstShort), 0, nWidth_B, nHeight_B/yRatioUV, 0, 0, dstShortPitch*2);

		for (int by=0; by<nBlkY; by++)
		{
			int wby = ((by + nBlkY - 3)/(nBlkY - 2))*3;
			int xx = 0;
			for (int bx=0; bx<nBlkX; bx++)
			{
					// select window
                    int wbx = (bx + nBlkX - 3)/(nBlkX - 2);
                    winOverUV = OverWinsUV->GetWindow(wby + wbx);

					int i = by*nBlkX + bx;
				const BYTE * pBV, *pFV, *pB2V, *pF2V, *pB3V, *pF3V;
				int npBV, npFV, npB2V, npF2V, npB3V, npF3V;

				if (isUsableB)
				{
					const FakeBlockData &blockB = mvClipB.GetBlock(0, i);
					int blxB = blockB.GetX() * nPel + blockB.GetMV().x;
					int blyB = blockB.GetY() * nPel + blockB.GetMV().y;
					pBV = pPlanesB[2]->GetPointer(blxB>>1, blyB/yRatioUV);
					npBV = pPlanesB[2]->GetPitch();
					int blockSAD = blockB.GetSAD();
					WRefB = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pBV = pSrcCur[2]+ (xx>>1);
					npBV = nSrcPitches[2];
					WRefB = 0;
				}
				if (isUsableF)
				{
					const FakeBlockData &blockF = mvClipF.GetBlock(0, i);
					int blxF = blockF.GetX() * nPel + blockF.GetMV().x;
					int blyF = blockF.GetY() * nPel + blockF.GetMV().y;
					pFV = pPlanesF[2]->GetPointer(blxF>>1, blyF/yRatioUV);
					npFV = pPlanesF[2]->GetPitch();
					int blockSAD = blockF.GetSAD();
					WRefF = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pFV = pSrcCur[2]+ (xx>>1);
					npFV = nSrcPitches[2];
					WRefF = 0;
				}
				if (isUsableB2)
				{
					const FakeBlockData &blockB2 = mvClipB2.GetBlock(0, i);
					int blxB2 = blockB2.GetX() * nPel + blockB2.GetMV().x;
					int blyB2 = blockB2.GetY() * nPel + blockB2.GetMV().y;
					pB2V = pPlanesB2[2]->GetPointer(blxB2>>1, blyB2/yRatioUV);
					npB2V = pPlanesB2[2]->GetPitch();
					int blockSAD = blockB2.GetSAD();
					WRefB2 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pB2V = pSrcCur[2]+ (xx>>1);
					npB2V = nSrcPitches[2];
					WRefB2 = 0;
				}
				if (isUsableF2)
				{
					const FakeBlockData &blockF2 = mvClipF2.GetBlock(0, i);
					int blxF2 = blockF2.GetX() * nPel + blockF2.GetMV().x;
					int blyF2 = blockF2.GetY() * nPel + blockF2.GetMV().y;
					pF2V = pPlanesF2[2]->GetPointer(blxF2>>1, blyF2/yRatioUV);
					npF2V = pPlanesF2[2]->GetPitch();
					int blockSAD = blockF2.GetSAD();
					WRefF2 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pF2V = pSrcCur[2]+ (xx>>1);
					npF2V = nSrcPitches[2];
					WRefF2 = 0;
				}
				if (isUsableB3)
				{
					const FakeBlockData &blockB3 = mvClipB3.GetBlock(0, i);
					int blxB3 = blockB3.GetX() * nPel + blockB3.GetMV().x;
					int blyB3 = blockB3.GetY() * nPel + blockB3.GetMV().y;
					pB3V = pPlanesB3[2]->GetPointer(blxB3>>1, blyB3/yRatioUV);
					npB3V = pPlanesB3[2]->GetPitch();
					int blockSAD = blockB3.GetSAD();
					WRefB3 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pB3V = pSrcCur[2]+ (xx>>1);
					npB3V = nSrcPitches[2];
					WRefB3 = 0;
				}
				if (isUsableF3)
				{
					const FakeBlockData &blockF3 = mvClipF3.GetBlock(0, i);
					int blxF3 = blockF3.GetX() * nPel + blockF3.GetMV().x;
					int blyF3 = blockF3.GetY() * nPel + blockF3.GetMV().y;
					pF3V = pPlanesF3[2]->GetPointer(blxF3>>1, blyF3/yRatioUV);
					npF3V = pPlanesF3[2]->GetPitch();
					int blockSAD = blockF3.GetSAD();
					WRefF3 = DegrainWeight(thSADC, blockSAD);
				}
				else
				{
					pF3V = pSrcCur[2]+ (xx>>1);
					npF3V = nSrcPitches[2];
					WRefF3 = 0;
				}
				WSrc = 256;
				int WSum = WRefB + WRefF + WSrc + WRefB2 + WRefF2 + WRefB3 + WRefF3 + 1;
				WRefB = WRefB*256/WSum; // normailize weights to 256
				WRefF = WRefF*256/WSum;
				WRefB2 = WRefB2*256/WSum;
				WRefF2 = WRefF2*256/WSum;
				WRefB3 = WRefB3*256/WSum;
				WRefF3 = WRefF3*256/WSum;
				WSrc = 256 - WRefB - WRefF - WRefB2 - WRefF2 - WRefB3 - WRefF3;
					// chroma v
					DEGRAINCHROMA(tmpBlock, tmpPitch, pSrcCur[2]+ (xx>>1), nSrcPitches[2],
						pBV, npBV, pFV, npFV, pB2V, npB2V, pF2V, npF2V, pB3V, npB3V, pF3V, npF3V,
						WSrc, WRefB, WRefF, WRefB2, WRefF2, WRefB3, WRefF3);
				OVERSCHROMA(pDstShort + (xx>>1), dstShortPitch, tmpBlock, tmpPitch, winOverUV, nBlkSizeX/2);

					xx += (nBlkSizeX - nOverlapX);

			}
               pSrcCur[2] += (nBlkSizeY - nOverlapY)/yRatioUV * (nSrcPitches[2]) ;
               pDstShort += (nBlkSizeY - nOverlapY)/yRatioUV * dstShortPitch;
		}
		Short2Bytes(pDst[2], nDstPitches[2], DstShort, dstShortPitch, nWidth_B>>1, nHeight_B/yRatioUV);
		if (nWidth_B < nWidth)
			BitBlt(pDst[2] + (nWidth_B>>1), nDstPitches[2], pSrc[2] + (nWidth_B>>1), nSrcPitches[2], (nWidth-nWidth_B)>>1, nHeight_B/yRatioUV, isse);
		if (nHeight_B < nHeight) // bottom noncovered region
			BitBlt(pDst[2] + nDstPitches[2]*nHeight_B/yRatioUV, nDstPitches[2], pSrc[2] + nSrcPitches[2]*nHeight_B/yRatioUV, nSrcPitches[2], nWidth>>1, (nHeight-nHeight_B)/yRatioUV, isse);
	  }
	  if (nLimit < 255) {
      if (isse)
        LimitChanges_mmx(pDst[2], nDstPitches[2], pSrc[2], nSrcPitches[2], nWidth/2, nHeight/yRatioUV, nLimit);
      else
        LimitChanges_c(pDst[2], nDstPitches[2], pSrc[2], nSrcPitches[2], nWidth/2, nHeight/yRatioUV, nLimit);
	  }
  }
//--------------------------------------------------------------------------------
	  __asm emms; // (we may use double-float somewhere) Fizick
         PROFILE_STOP(MOTION_PROFILE_COMPENSATION);


	if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 && !planar)
	{
		YUY2FromPlanes(pDstYUY2, nDstPitchYUY2, nWidth, nHeight,
					  pDst[0], nDstPitches[0], pDst[1], pDst[2], nDstPitches[1], isse);
	}
   return dst;
}

