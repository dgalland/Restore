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

#include "MVDegrain1.h"

//-------------------------------------------------------------------------

MVDegrain1::MVDegrain1(PClip _child, PClip _super, PClip mvbw, PClip mvfw, int _thSAD, int _thSADC, int _YUVplanes, int _nLimit,
					 int _nSCD1, int _nSCD2, bool _isse, bool _planar, IScriptEnvironment* env) :
GenericVideoFilter(_child),
MVFilter(mvfw, "MDegrain1", env),
mvClipB(mvbw, _nSCD1, _nSCD2, env),
mvClipF(mvfw, _nSCD1, _nSCD2, env),
super(_super)
{
   thSAD = _thSAD*mvClipB.GetThSCD1()/_nSCD1; // normalize to block SAD
   thSADC = _thSADC*mvClipB.GetThSCD1()/_nSCD1; // chroma threshold, normalized to block SAD
   YUVplanes = _YUVplanes;
   nLimit = _nLimit;

   isse = _isse;
   planar = _planar;

   CheckSimilarity(mvClipF, "mvfw", env);
   CheckSimilarity(mvClipB, "mvbw", env);

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
    int nSuperWidth = super->GetVideoInfo().width;
    int nSuperHeight = super->GetVideoInfo().height;

    if (nHeight != nHeightS || nHeight != vi.height || nWidth != nSuperWidth-nSuperHPad*2 || nWidth != vi.width)
    		env->ThrowError("MDegrain1 : wrong source or super frame size");


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
      if (nBlkSizeY==16) {          OVERSLUMA = Overlaps32x16_mmx;  DEGRAINLUMA = Degrain1_C<32,16>;
		 if (yRatioUV==2) {         OVERSCHROMA = Overlaps16x8_mmx; DEGRAINCHROMA = Degrain1_C<16,8>;		 }
		 else {                     OVERSCHROMA = Overlaps16x16_mmx;DEGRAINCHROMA = Degrain1_C<16,16>;		 }
      } break;
      case 16:
      if (nBlkSizeY==16) {          OVERSLUMA = Overlaps16x16_mmx; DEGRAINLUMA = Degrain1_C<16,16>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps8x8_mmx; DEGRAINCHROMA = Degrain1_C<8,8>;		 }
		 else {	                    OVERSCHROMA = Overlaps8x16_mmx;DEGRAINCHROMA = Degrain1_C<8,16>;		 }
      } else if (nBlkSizeY==8) {    OVERSLUMA = Overlaps16x8_mmx;  DEGRAINLUMA = Degrain1_C<16,8>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps8x4_mmx; DEGRAINCHROMA = Degrain1_C<8,4>;		 }
		 else {	                    OVERSCHROMA = Overlaps8x8_mmx; DEGRAINCHROMA = Degrain1_C<8,8>;		 }
      } else if (nBlkSizeY==2) {    OVERSLUMA = Overlaps16x2_mmx;  DEGRAINLUMA = Degrain1_C<16,2>;
		 if (yRatioUV==2) {         OVERSCHROMA = Overlaps8x1_mmx; DEGRAINCHROMA = Degrain1_C<8,1>;		 }
		 else {	                    OVERSCHROMA = Overlaps8x2_mmx; DEGRAINCHROMA = Degrain1_C<8,2>;		 }
      }
         break;
      case 4:
                                    OVERSLUMA = Overlaps4x4_mmx;    DEGRAINLUMA = Degrain1_C<4,4>;
		 if (yRatioUV==2) {			OVERSCHROMA = Overlaps_C<2,2>;	DEGRAINCHROMA = Degrain1_C<2,2>;		 }
		 else {			            OVERSCHROMA = Overlaps_C<2,4>;    DEGRAINCHROMA = Degrain1_C<2,4>;		 }
         break;
      case 8:
      default:
      if (nBlkSizeY==8) {           OVERSLUMA = Overlaps8x8_mmx;    DEGRAINLUMA = Degrain1_C<8,8>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps4x4_mmx;  DEGRAINCHROMA = Degrain1_C<4,4>;		 }
		 else {	                    OVERSCHROMA = Overlaps4x8_mmx;  DEGRAINCHROMA = Degrain1_C<4,8>;		 }
      }else if (nBlkSizeY==4) {     OVERSLUMA = Overlaps8x4_mmx;	DEGRAINLUMA = Degrain1_C<8,4>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps4x2_mmx;	DEGRAINCHROMA = Degrain1_C<4,2>;		 }
		 else {	                    OVERSCHROMA = Overlaps4x4_mmx;  DEGRAINCHROMA = Degrain1_C<4,4>;		 }
      }
      }
   }
   else
   {
	switch (nBlkSizeX)
      {
      case 32:
      if (nBlkSizeY==16) {          OVERSLUMA = Overlaps_C<32,16>;  DEGRAINLUMA = Degrain1_C<32,16>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps_C<16,8>; DEGRAINCHROMA = Degrain1_C<16,8>;		 }
		 else {	                    OVERSCHROMA = Overlaps_C<16,16>;DEGRAINCHROMA = Degrain1_C<16,16>;		 }
      } break;
      case 16:
      if (nBlkSizeY==16) {          OVERSLUMA = Overlaps_C<16,16>;  DEGRAINLUMA = Degrain1_C<16,16>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps_C<8,8>;  DEGRAINCHROMA = Degrain1_C<8,8>;		 }
		 else {	                    OVERSCHROMA = Overlaps_C<8,16>; DEGRAINCHROMA = Degrain1_C<8,16>;		 }
      } else if (nBlkSizeY==8) {    OVERSLUMA = Overlaps_C<16,8>;   DEGRAINLUMA = Degrain1_C<16,8>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps_C<8,4>;  DEGRAINCHROMA = Degrain1_C<8,4>;		 }
		 else {	                    OVERSCHROMA = Overlaps_C<8,8>;  DEGRAINCHROMA = Degrain1_C<8,8>;		 }
      } else if (nBlkSizeY==2) {    OVERSLUMA = Overlaps_C<16,2>;   DEGRAINLUMA = Degrain1_C<16,2>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps_C<8,1>;  DEGRAINCHROMA = Degrain1_C<8,1>;		 }
		 else {	                    OVERSCHROMA = Overlaps_C<8,2>;  DEGRAINCHROMA = Degrain1_C<8,2>;		 }
      }
         break;
      case 4:
                                    OVERSLUMA = Overlaps_C<4,4>;    DEGRAINLUMA = Degrain1_C<4,4>;
		 if (yRatioUV==2) {			OVERSCHROMA = Overlaps_C<2,2>;  DEGRAINCHROMA = Degrain1_C<2,2>;		 }
		 else {			            OVERSCHROMA = Overlaps_C<2,4>;  DEGRAINCHROMA = Degrain1_C<2,4>;		 }
         break;
      case 8:
      default:
      if (nBlkSizeY==8) {           OVERSLUMA = Overlaps_C<8,8>;    DEGRAINLUMA = Degrain1_C<8,8>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps_C<4,4>;  DEGRAINCHROMA = Degrain1_C<4,4>;		 }
		 else {	                    OVERSCHROMA = Overlaps_C<4,8>;  DEGRAINCHROMA = Degrain1_C<4,8>;		 }
      }else if (nBlkSizeY==4) {     OVERSLUMA = Overlaps_C<8,4>;    DEGRAINLUMA = Degrain1_C<8,4>;
		 if (yRatioUV==2) {	        OVERSCHROMA = Overlaps_C<4,2>;  DEGRAINCHROMA = Degrain1_C<4,2>;		 }
		 else {	                    OVERSCHROMA = Overlaps_C<4,4>;  DEGRAINCHROMA = Degrain1_C<4,4>;		 }
      }
      }
   }

   	tmpBlock = new BYTE[32*32];


}


MVDegrain1::~MVDegrain1()
{
   if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 && !planar)
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
   delete pRefFGOF; // v2.0
   delete pRefBGOF;
}

PVideoFrame __stdcall MVDegrain1::GetFrame(int n, IScriptEnvironment* env)
{
	int nWidth_B = nBlkX*(nBlkSizeX - nOverlapX) + nOverlapX;
	int nHeight_B = nBlkY*(nBlkSizeY - nOverlapY) + nOverlapY;

	PVideoFrame	src	= child->GetFrame(n, env);
   PVideoFrame dst;
   BYTE *pDst[3], *pDstCur[3];
   const BYTE *pSrcCur[3];
    const BYTE *pSrc[3];
	const BYTE *pRefB[3];
	const BYTE *pRefF[3];
    int nDstPitches[3], nSrcPitches[3];
	int nRefBPitches[3], nRefFPitches[3];
	unsigned char *pDstYUY2;
	const unsigned char *pSrcYUY2;
	int nDstPitchYUY2;
	int nSrcPitchYUY2;
	bool isUsableB, isUsableF;
	int tmpPitch = nBlkSizeX;
	int WSrc, WRefB, WRefF;
	unsigned short *pDstShort;
   int nLogPel = (nPel==4) ? 2 : (nPel==2) ? 1 : 0;
   // nLogPel=0 for nPel=1, 1 for nPel=2, 2 for nPel=4, i.e. (x*nPel) = (x<<nLogPel)

	PVideoFrame mvF = mvClipF.GetFrame(n, env);
   mvClipF.Update(mvF, env);
   isUsableF = mvClipF.IsUsable();
   mvF = 0; // v2.0.9.2 -  it seems, we do not need in vectors clip anymore when we finished copiing them to fakeblockdatas
	PVideoFrame mvB = mvClipB.GetFrame(n, env);
   mvClipB.Update(mvB, env);
   isUsableB = mvClipB.IsUsable();
   mvB = 0;

//   int sharp = mvClipB.GetSharp();
   int ySubUV = (yRatioUV == 2) ? 1 : 0;
//   if ( mvClipB.IsUsable() && mvClipF.IsUsable() && mvClipB2.IsUsable() && mvClipF2.IsUsable() )
//   {
      dst = env->NewVideoFrame(vi);
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

//         MVFrames *pFrames = mvCore->GetFrames(nIdx);
		PVideoFrame refB, refF;

//		PVideoFrame refB2x, refF2x;

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

  		if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
		{
            // planar data packed to interleaved format (same as interleved2planar by kassandro) - v2.0.0.5
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
		}
		else
		{
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
		}


            pRefFGOF->Update(YUVplanes, (BYTE*)pRefF[0], nRefFPitches[0], (BYTE*)pRefF[1], nRefFPitches[1], (BYTE*)pRefF[2], nRefFPitches[2]);
            pRefBGOF->Update(YUVplanes, (BYTE*)pRefB[0], nRefBPitches[0], (BYTE*)pRefB[1], nRefBPitches[1], (BYTE*)pRefB[2], nRefBPitches[2]);// v2.0

         MVPlane *pPlanesB[3];
         MVPlane *pPlanesF[3];

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

         PROFILE_START(MOTION_PROFILE_COMPENSATION);
		 pDstCur[0] = pDst[0];
		 pDstCur[1] = pDst[1];
		 pDstCur[2] = pDst[2];
		 pSrcCur[0] = pSrc[0];
		 pSrcCur[1] = pSrc[1];
		 pSrcCur[2] = pSrc[2];
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
				const BYTE * pB, *pF;
				int npB, npF;
				int WRefB, WRefF, WSrc;

				if (isUsableB)
				{
					const FakeBlockData &blockB = mvClipB.GetBlock(0, i);
					int blxB = (blockB.GetX()<<nLogPel) + blockB.GetMV().x;
					int blyB = (blockB.GetY()<<nLogPel) + blockB.GetMV().y;
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
					int blxF = (blockF.GetX()<<nLogPel) + blockF.GetMV().x;
					int blyF = (blockF.GetY()<<nLogPel) + blockF.GetMV().y;
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
				WSrc = 256;
				int WSum = WRefB + WRefF + WSrc + 1;
				WRefB = WRefB*256/WSum; // normailize weights to 256
				WRefF = WRefF*256/WSum;
				WSrc = 256 - WRefB - WRefF;

				DEGRAINLUMA(pDstCur[0] + xx, nDstPitches[0], pSrcCur[0]+ xx, nSrcPitches[0],
					pB, npB, pF, npF, WSrc, WRefB, WRefF);

				xx += (nBlkSizeX);

				if (bx == nBlkX-1 && nWidth_B < nWidth) // right non-covered region
				{
					BitBlt(pDstCur[0] + nWidth_B, nDstPitches[0],
						pSrcCur[0] + nWidth_B, nSrcPitches[0], nWidth-nWidth_B, nBlkSizeY, isse);
				}
			}
               pDstCur[0] += ( nBlkSizeY ) * nDstPitches[0];
               pSrcCur[0] += ( nBlkSizeY ) * nSrcPitches[0];

			if (by == nBlkY-1 && nHeight_B < nHeight) // bottom uncovered region
			{
				BitBlt(pDstCur[0], nDstPitches[0], pSrcCur[0], nSrcPitches[0], nWidth, nHeight-nHeight_B, isse);
			}
		}
	  }
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
				const BYTE * pB, *pF;
				int npB, npF;

				if (isUsableB)
				{
					const FakeBlockData &blockB = mvClipB.GetBlock(0, i);
					int blxB = (blockB.GetX()<<nLogPel) + blockB.GetMV().x;
					int blyB = (blockB.GetY()<<nLogPel) + blockB.GetMV().y;
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
					int blxF = (blockF.GetX()<<nLogPel) + blockF.GetMV().x;
					int blyF = (blockF.GetY()<<nLogPel) + blockF.GetMV().y;
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
				WSrc = 256;
				int WSum = WRefB + WRefF + WSrc + 1;
				WRefB = WRefB*256/WSum; // normailize weights to 256
				WRefF = WRefF*256/WSum;
				WSrc = 256 - WRefB - WRefF;
				// luma
				DEGRAINLUMA(tmpBlock, tmpPitch, pSrcCur[0]+ xx, nSrcPitches[0], pB, npB, pF, npF, WSrc, WRefB, WRefF);
				OVERSLUMA(pDstShort + xx, dstShortPitch, tmpBlock, tmpPitch, winOver, nBlkSizeX);

					xx += (nBlkSizeX - nOverlapX);

			}
               pSrcCur[0] += (nBlkSizeY - nOverlapY) * nSrcPitches[0];
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

//-----------------------------------------------------------------------------------
 // CHROMA plane U
  if (!(YUVplanes & UPLANE & nSuperModeYUV)) // v2.0.8.1
		BitBlt(pDstCur[1], nDstPitches[1], pSrcCur[1], nSrcPitches[1], nWidth>>1, nHeight>>ySubUV, isse);
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
				const BYTE * pBU, *pFU;
				int npBU, npFU;
				int WRefB, WRefF, WSrc;

				if (isUsableB)
				{
					const FakeBlockData &blockB = mvClipB.GetBlock(0, i);
					int blxB = (blockB.GetX()<<nLogPel) + blockB.GetMV().x;
					int blyB = (blockB.GetY()<<nLogPel) + blockB.GetMV().y;
					pBU = pPlanesB[1]->GetPointer(blxB>>1, blyB>>ySubUV);
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
					int blxF = (blockF.GetX()<<nLogPel) + blockF.GetMV().x;
					int blyF = (blockF.GetY()<<nLogPel) + blockF.GetMV().y;
					pFU = pPlanesF[1]->GetPointer(blxF>>1, blyF>>ySubUV);
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
				WSrc = 256;
				int WSum = WRefB + WRefF + WSrc + 1;
				WRefB = WRefB*256/WSum; // normailize weights to 256
				WRefF = WRefF*256/WSum;
				WSrc = 256 - WRefB - WRefF;

				DEGRAINCHROMA(pDstCur[1] + (xx>>1), nDstPitches[1], pSrcCur[1]+ (xx>>1), nSrcPitches[1],
						pBU, npBU, pFU, npFU, WSrc, WRefB, WRefF);

					xx += (nBlkSizeX);

					if (bx == nBlkX-1 && nWidth_B < nWidth) // right non-covered region
					{
						BitBlt(pDstCur[1] + (nWidth_B>>1), nDstPitches[1],
								pSrcCur[1] + (nWidth_B>>1), nSrcPitches[1], (nWidth-nWidth_B)>>1, (nBlkSizeY)>>ySubUV, isse);
					}
			}
               pDstCur[1] += ( nBlkSizeY >>ySubUV) * nDstPitches[1] ;
               pSrcCur[1] += ( nBlkSizeY >>ySubUV) * nSrcPitches[1] ;

			if (by == nBlkY-1 && nHeight_B < nHeight) // bottom uncovered region
			{
					BitBlt(pDstCur[1], nDstPitches[1], pSrcCur[1], nSrcPitches[1], nWidth>>1, (nHeight-nHeight_B)>>ySubUV, isse);
			}
		}
	  }
	  else // overlap
	  {
		pDstShort = DstShort;
		MemZoneSet(reinterpret_cast<unsigned char*>(pDstShort), 0, nWidth_B, nHeight_B>>ySubUV, 0, 0, dstShortPitch*2);
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
				const BYTE * pBU, *pFU;
				int npBU, npFU;

				if (isUsableB)
				{
					const FakeBlockData &blockB = mvClipB.GetBlock(0, i);
					int blxB = (blockB.GetX()<<nLogPel) + blockB.GetMV().x;
					int blyB = (blockB.GetY()<<nLogPel) + blockB.GetMV().y;
					pBU = pPlanesB[1]->GetPointer(blxB>>1, blyB>>ySubUV);
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
					int blxF = (blockF.GetX()<<nLogPel) + blockF.GetMV().x;
					int blyF = (blockF.GetY()<<nLogPel) + blockF.GetMV().y;
					pFU = pPlanesF[1]->GetPointer(blxF>>1, blyF>>ySubUV);
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
				WSrc = 256;
				int WSum = WRefB + WRefF + WSrc + 1;
				WRefB = WRefB*256/WSum; // normailize weights to 256
				WRefF = WRefF*256/WSum;
				WSrc = 256 - WRefB - WRefF;

				DEGRAINCHROMA(tmpBlock, tmpPitch, pSrcCur[1]+ (xx>>1), nSrcPitches[1],
						pBU, npBU, pFU, npFU, WSrc, WRefB, WRefF);
				OVERSCHROMA(pDstShort + (xx>>1), dstShortPitch, tmpBlock, tmpPitch, winOverUV, nBlkSizeX/2);

					xx += (nBlkSizeX - nOverlapX);

			}
               pSrcCur[1] += ((nBlkSizeY - nOverlapY)>>ySubUV) * nSrcPitches[1] ;
               pDstShort += ((nBlkSizeY - nOverlapY)>>ySubUV) * dstShortPitch;

		}
		Short2Bytes(pDst[1], nDstPitches[1], DstShort, dstShortPitch, nWidth_B>>1, nHeight_B>>ySubUV);
		if (nWidth_B < nWidth)
			BitBlt(pDst[1] + (nWidth_B>>1), nDstPitches[1], pSrc[1] + (nWidth_B>>1), nSrcPitches[1], (nWidth-nWidth_B)>>1, nHeight_B>>ySubUV, isse);
		if (nHeight_B < nHeight) // bottom noncovered region
			BitBlt(pDst[1] + nDstPitches[1]*(nHeight_B>>ySubUV), nDstPitches[1], pSrc[1] + nSrcPitches[1]*(nHeight_B>>ySubUV), nSrcPitches[1], nWidth>>1, (nHeight-nHeight_B)>>ySubUV, isse);
	  }
	  if (nLimit < 255) {
	  if (isse)
        LimitChanges_mmx(pDst[1], nDstPitches[1], pSrc[1], nSrcPitches[1], nWidth/2, nHeight/yRatioUV, nLimit);
      else
        LimitChanges_c(pDst[1], nDstPitches[1], pSrc[1], nSrcPitches[1], nWidth/2, nHeight/yRatioUV, nLimit);
	  }
  }
//--------------------------------------------------------------------------------

 // CHROMA plane V
  if (!(YUVplanes & VPLANE & nSuperModeYUV))
		BitBlt(pDstCur[2], nDstPitches[2], pSrcCur[2], nSrcPitches[2], nWidth>>1, nHeight>>ySubUV, isse);
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
				const BYTE * pBV, *pFV;
				int npBV, npFV;
				int WRefB, WRefF, WSrc;

				if (isUsableB)
				{
					const FakeBlockData &blockB = mvClipB.GetBlock(0, i);
					int blxB = (blockB.GetX()<<nLogPel) + blockB.GetMV().x;
					int blyB = (blockB.GetY()<<nLogPel) + blockB.GetMV().y;
					pBV = pPlanesB[2]->GetPointer(blxB>>1, blyB>>ySubUV);
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
					int blxF = (blockF.GetX()<<nLogPel) + blockF.GetMV().x;
					int blyF = (blockF.GetY()<<nLogPel) + blockF.GetMV().y;
					pFV = pPlanesF[2]->GetPointer(blxF>>1, blyF>>ySubUV);
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
				WSrc = 256;
				int WSum = WRefB + WRefF + WSrc + 1;
				WRefB = WRefB*256/WSum; // normailize weights to 256
				WRefF = WRefF*256/WSum;
				WSrc = 256 - WRefB - WRefF;

					DEGRAINCHROMA(pDstCur[2] + (xx>>1), nDstPitches[2], pSrcCur[2]+ (xx>>1), nSrcPitches[2],
						pBV, npBV, pFV, npFV, WSrc, WRefB, WRefF);

					xx += (nBlkSizeX);

					if (bx == nBlkX-1 && nWidth_B < nWidth) // right non-covered region
					{
							BitBlt(pDstCur[2] + (nWidth_B>>1), nDstPitches[2],
								pSrcCur[2] + (nWidth_B>>1), nSrcPitches[2], (nWidth-nWidth_B)>>1, nBlkSizeY>>ySubUV, isse);
					}
			}
               pDstCur[2] += ( nBlkSizeY >>ySubUV) * nDstPitches[2] ;
               pSrcCur[2] += ( nBlkSizeY >>ySubUV) * nSrcPitches[2] ;

			if (by == nBlkY-1 && nHeight_B < nHeight) // bottom uncovered region
			{
					BitBlt(pDstCur[2], nDstPitches[2], pSrcCur[2], nSrcPitches[2], nWidth>>1, (nHeight-nHeight_B)>>ySubUV, isse);
			}
		}
	  }
	  else // overlap
	  {
		pDstShort = DstShort;
		MemZoneSet(reinterpret_cast<unsigned char*>(pDstShort), 0, nWidth_B, nHeight_B>>ySubUV, 0, 0, dstShortPitch*2);
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
				const BYTE * pBV, *pFV;
				int npBV, npFV;

				if (isUsableB)
				{
					const FakeBlockData &blockB = mvClipB.GetBlock(0, i);
					int blxB = (blockB.GetX()<<nLogPel) + blockB.GetMV().x;
					int blyB = (blockB.GetY()<<nLogPel) + blockB.GetMV().y;
					pBV = pPlanesB[2]->GetPointer(blxB>>1, blyB>>ySubUV);
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
					int blxF = (blockF.GetX()<<nLogPel) + blockF.GetMV().x;
					int blyF = (blockF.GetY()<<nLogPel) + blockF.GetMV().y;
					pFV = pPlanesF[2]->GetPointer(blxF>>1, blyF>>ySubUV);
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
				WSrc = 256;
				int WSum = WRefB + WRefF + WSrc + 1;
				WRefB = WRefB*256/WSum; // normailize weights to 256
				WRefF = WRefF*256/WSum;
				WSrc = 256 - WRefB - WRefF;

				DEGRAINCHROMA(tmpBlock, tmpPitch, pSrcCur[2]+ (xx>>1), nSrcPitches[2],
						pBV, npBV, pFV, npFV, WSrc, WRefB, WRefF);
				OVERSCHROMA(pDstShort + (xx>>1), dstShortPitch, tmpBlock, tmpPitch, winOverUV, nBlkSizeX/2);

					xx += (nBlkSizeX - nOverlapX);

			}
               pSrcCur[2] += ((nBlkSizeY - nOverlapY)>>ySubUV) * nSrcPitches[2] ;
               pDstShort += ((nBlkSizeY - nOverlapY)>>ySubUV) * dstShortPitch;

		}
		Short2Bytes(pDst[2], nDstPitches[2], DstShort, dstShortPitch, nWidth_B>>1, nHeight_B>>ySubUV);
		if (nWidth_B < nWidth)
			BitBlt(pDst[2] + (nWidth_B>>1), nDstPitches[2], pSrc[2] + (nWidth_B>>1), nSrcPitches[2], (nWidth-nWidth_B)>>1, nHeight_B>>ySubUV, isse);
		if (nHeight_B < nHeight) // bottom noncovered region
			BitBlt(pDst[2] + nDstPitches[2]*(nHeight_B>>ySubUV), nDstPitches[2], pSrc[2] + nSrcPitches[2]*(nHeight_B>>ySubUV), nSrcPitches[2], nWidth>>1, (nHeight-nHeight_B)>>ySubUV, isse);
	  }
	  if (nLimit < 255) {
	  if (isse)
        LimitChanges_mmx(pDst[2], nDstPitches[2], pSrc[2], nSrcPitches[2], nWidth/2, nHeight/yRatioUV, nLimit);
      else
        LimitChanges_c(pDst[2], nDstPitches[2], pSrc[2], nSrcPitches[2], nWidth/2, nHeight/yRatioUV, nLimit);
	  }
  }
//-------------------------------------------------------------------------------
	  __asm emms; // (we may use double-float somewhere) Fizick
         PROFILE_STOP(MOTION_PROFILE_COMPENSATION);


	if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 && !planar)
	{
		YUY2FromPlanes(pDstYUY2, nDstPitchYUY2, nWidth, nHeight,
					  pDst[0], nDstPitches[0], pDst[1], pDst[2], nDstPitches[1], isse);
	}
   return dst;
}
