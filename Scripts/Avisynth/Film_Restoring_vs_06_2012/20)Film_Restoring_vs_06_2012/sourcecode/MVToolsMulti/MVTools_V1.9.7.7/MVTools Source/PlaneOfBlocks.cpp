// Author: Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick - global motion, overlap,  mode
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

#include "PlaneOfBlocks.h"
#include "Padding.h"

#include <algorithm>

PlaneOfBlocks::PlaneOfBlocks(int _nBlkX, int _nBlkY, int _nBlkSizeX, int _nBlkSizeY, int _nPel, int _nLevel, int _nFlags, int _nOverlapX, int _nOverlapY, int _yRatioUV)
{

/* constant fields */

    nPel = _nPel;
    nLogPel = ilog2(nPel);
    // nLogPel=0 for nPel=1, 1 for nPel=2, 2 for nPel=4, i.e. (x*nPel) = (x<<nLogPel)
    nLogScale = _nLevel;
    nScale = iexp2(nLogScale);

    nBlkSizeX = _nBlkSizeX;
    nBlkSizeY = _nBlkSizeY;
    nOverlapX = _nOverlapX;
    nOverlapY = _nOverlapY;

    nBlkX = _nBlkX;
    nBlkY = _nBlkY;
    nBlkCount = nBlkX * nBlkY;

    nFlags = _nFlags;
    yRatioUV = _yRatioUV;

    smallestPlane = (bool)(nFlags & MOTION_SMALLEST_PLANE);
    mmx = (bool)(nFlags & MOTION_USE_MMX);
    isse = (bool)(nFlags & MOTION_USE_ISSE);
    chroma = (bool)(nFlags & MOTION_USE_CHROMA_MOTION);

    bool mmxext = (bool)(nFlags & CPU_MMXEXT);
    bool cache32 = (bool)(nFlags & CPU_CACHELINE_32);
    bool cache64 = (bool)(nFlags & CPU_CACHELINE_64);
    bool sse2 = (bool)(nFlags & CPU_SSE2_IS_FAST);
    bool sse3 = (bool)(nFlags & CPU_SSE3);
    bool ssse3 = (bool)(nFlags & CPU_SSSE3);
    bool ssse3pha = (bool)(nFlags & CPU_PHADD_IS_FAST);
    bool ssd = (bool)(nFlags & MOTION_USE_SSD);
    bool satd = (bool)(nFlags & MOTION_USE_SATD);
   
//	ssd=false;
//	satd=false;

    globalMVPredictor.x = zeroMV.x;
    globalMVPredictor.y = zeroMV.y;
    globalMVPredictor.sadLuma    = zeroMV.sadLuma;
    globalMVPredictor.sadChromaU = zeroMV.sadChromaU;
    globalMVPredictor.sadChromaV = zeroMV.sadChromaV;

/* arrays memory allocation */

    vectors = new VECTOR[nBlkCount];

/* function's pointers initialization */

#define SET_FUNCPTR(blksizex, blksizey, blksizex2, blksizey2) \
        SAD = Sad##blksizex##x##blksizey##_iSSE; \
        VAR = Var##blksizex##x##blksizey##_iSSE; \
        LUMA = Luma##blksizex##x##blksizey##_iSSE; \
        BLITLUMA = Copy_C<blksizex , blksizey>; \
        if (yRatioUV==2) { \
            BLITCHROMA = Copy_C<blksizex2 , blksizey2>;  \
            SADCHROMA = Sad##blksizex2##x##blksizey2##_iSSE; \
        } \
        else { \
            BLITCHROMA = Copy_C<blksizex2 , blksizey>; \
            SADCHROMA = Sad##blksizex2##x##blksizey##_iSSE; \
        }

#define SET_FUNCPTR_C(blksizex, blksizey, blksizex2, blksizey2) \
        SAD = Sad_C<blksizex , blksizey>; \
        VAR = Var_C<blksizex , blksizey>; \
        LUMA = Luma_C<blksizex , blksizey>; \
        BLITLUMA = Copy_C<blksizex , blksizey>; \
        if (yRatioUV==2) { \
            BLITCHROMA = Copy_C<blksizex2 , blksizey2>; \
            SADCHROMA = Sad_C<blksizex2 , blksizey2>; \
        } \
        else { \
            BLITCHROMA = Copy_C<blksizex2 , blksizey>; \
            SADCHROMA = Sad_C<blksizex2  , blksizey>; \
        }

//#define NEWBLIT

#ifdef NEWBLIT
//I suppose it is faster to use MMX than use SSE on a cache line split affected platform if that occurs
#define SETBLIT16(blksizex, blksizey, type) \
	type = copy_mc_##blksizex##x##blksizey##_mmx; \
	if (sse2&&(((!cache32)&&(!cache64))||(!ssse3))) type = copy_mc_##blksizex##x##blksizey##_sse2; \
	if (sse3&&(((!cache32)&&(!cache64))||(!ssse3))) type = copy_mc_##blksizex##x##blksizey##_sse3
	if (sse2&&(nOverlapX %16 == 0)) type = copy_mc_##blksizex##x##blksizey##_aligned_sse2; \
#define SETBLITX(blksizex, blksizey, type) \
	type = copy_mc_##blksizex##x##blksizey##_mmx
#else
#define SETBLIT16(blksizex, blksizey, type)
#define SETBLITX(blksizex, blksizey, type)
#endif


#define SET_FUNCPTR_x264(blksizex, blksizey, type) \
        type = x264_pixel_sad_##blksizex##x##blksizey##_mmxext; \
        if (cache32) type = x264_pixel_sad_##blksizex##x##blksizey##_cache32_mmxext; \
        if (cache64) type = x264_pixel_sad_##blksizex##x##blksizey##_cache64_mmxext; \
        if (sse2) type = x264_pixel_sad_##blksizex##x##blksizey##_sse2; \
        if (sse3) type = x264_pixel_sad_##blksizex##x##blksizey##_sse3; \
        if (cache32&&cache64) type = x264_pixel_sad_##blksizex##x##blksizey##_cache64_sse2; \
        if (ssse3&&cache64) type = x264_pixel_sad_##blksizex##x##blksizey##_cache64_ssse3; \
        if (ssd) type = x264_pixel_ssd_##blksizex##x##blksizey##_mmx; \
        if (satd) type = x264_pixel_satd_##blksizex##x##blksizey##_mmxext; \
        if (satd&&sse2) type = x264_pixel_satd_##blksizex##x##blksizey##_sse2; \
        if (satd&&ssse3) type = x264_pixel_satd_##blksizex##x##blksizey##_ssse3; \
        if (satd&&ssse3pha) type = x264_pixel_satd_##blksizex##x##blksizey##_ssse3_phadd
		
  
#define SET_FUNCPTR_x264_mmx(blksizex, blksizey, type) \
        type = x264_pixel_sad_##blksizex##x##blksizey##_mmxext; \
        if (cache32) type = x264_pixel_sad_##blksizex##x##blksizey##_cache32_mmxext; \
        if (cache64) type = x264_pixel_sad_##blksizex##x##blksizey##_cache64_mmxext; \
        if (ssd) type = x264_pixel_ssd_##blksizex##x##blksizey##_mmx; \
        if (satd) type = x264_pixel_satd_##blksizex##x##blksizey##_mmxext; \
        if (satd&&sse2) type = x264_pixel_satd_##blksizex##x##blksizey##_sse2; \
        if (satd&&ssse3) type = x264_pixel_satd_##blksizex##x##blksizey##_ssse3; \
        if (satd&&ssse3pha) type = x264_pixel_satd_##blksizex##x##blksizey##_ssse3_phadd

#define SET_FUNCPTR_x264_mmx_4x(blksizey, type) \
        type = x264_pixel_sad_4x##blksizey##_mmxext; \
        if (ssd) type = x264_pixel_ssd_4x##blksizey##_mmx; \
        if (satd) type = x264_pixel_satd_4x##blksizey##_mmxext

#define SET_FUNCPTR_x264_SATD(blksizex,blksizey) \
        SATD = x264_pixel_satd_##blksizex##x##blksizey##_mmxext; \
        if (sse2) SATD = x264_pixel_satd_##blksizex##x##blksizey##_sse2; \
        if (ssse3) SATD = x264_pixel_satd_##blksizex##x##blksizey##_ssse3


	SATD = Sad0_C; //for now disable SATD if default functions are used
	if ( isse )
	{
		switch (nBlkSizeX)
		{
		case 16:
			if (nBlkSizeY==16) {
				SET_FUNCPTR(16,16,8,8)
			} else if (nBlkSizeY==8) {
			    SET_FUNCPTR(16,8,8,4)
			} else if (nBlkSizeY==2) {
				SET_FUNCPTR(16,2,8,1)
				} else if (nBlkSizeY==1){
					SAD = Sad16x1_iSSE;
					VAR = Var_C<16,1>;;
					LUMA = Luma_C<16,1>;
					BLITLUMA = Copy_C<16,1>;
					if (yRatioUV==2) {
						//error
					}
					else { //yRatioUV==1
						BLITCHROMA = Copy_C<8, 1>;
						SADCHROMA = Sad8x1_iSSE;
					}
				}
			break;
		case 4:
			SET_FUNCPTR(4,4,2,2)
				if (yRatioUV==2) {
					//SADCHROMA = Sad2x2_iSSE_T;
				}
				else {	
					//SADCHROMA = Sad2x4_iSSE_T;
				}
			break;
		case 32:
			if (nBlkSizeY==16) {
				SET_FUNCPTR(32,16,16,8)
			}
			break;
		case 8:
		default:
			if (nBlkSizeY == 8) {
				SET_FUNCPTR(8,8,4,4)
			} else if (nBlkSizeY == 4) { // 8x4
				SET_FUNCPTR(8,4,4,2)
			}
		}//end switch
	}//end isse
	else
	{
		switch (nBlkSizeX)
		{
		case 16:
			if (nBlkSizeY==16) {
				SET_FUNCPTR_C(16, 16, 8, 8)
			} else if (nBlkSizeY==8) {
				SET_FUNCPTR_C(16, 8, 8, 4)
			} else if (nBlkSizeY==2) {
				SET_FUNCPTR_C(16, 2, 8, 1)
			}
			break;
		case 4:
			SET_FUNCPTR_C(4, 4, 2, 2)
			break;
		case 32:
			if (nBlkSizeY==16) {
				SET_FUNCPTR_C(32, 16, 16, 8)
			}
		break;
		case 8:
		default:
			if (nBlkSizeY==8) {
				SET_FUNCPTR_C(8, 8, 4, 4)
			}else if (nBlkSizeY==4) {
				SET_FUNCPTR_C(8, 4, 4, 2)
			}
		}
	}

	if (mmxext) //use new functions from x264
	{	
		switch (nBlkSizeX)
		{
		case 32:
			if (nBlkSizeY==16) {
				if (yRatioUV==2) {
					SET_FUNCPTR_x264(16,8,SADCHROMA);
					SETBLIT16(16,8,BLITCHROMA);
				}
				else {
					SET_FUNCPTR_x264(16,16,SADCHROMA);
					SETBLIT16(16,16,BLITCHROMA);
				}
			}
			break;
		case 16:
			if (nBlkSizeY==16) {
				SET_FUNCPTR_x264(16,16,SAD);
				SET_FUNCPTR_x264_SATD(16,16);
				SETBLIT16(16,16,BLITLUMA);
				if (yRatioUV==2) {
					SET_FUNCPTR_x264_mmx(8,8, SADCHROMA);
					SETBLITX(8,8,BLITCHROMA);
				}
				else {
					SET_FUNCPTR_x264_mmx(8,16, SADCHROMA);
					SETBLITX(8,16,BLITCHROMA);
				}				
			} else if (nBlkSizeY==8) {
				SET_FUNCPTR_x264(16,8,SAD);
				SET_FUNCPTR_x264_SATD(16,8);
				SETBLIT16(16,8,BLITLUMA);
				if (yRatioUV==2) {
					SET_FUNCPTR_x264_mmx(8,4, SADCHROMA);
					SETBLITX(8,4,BLITCHROMA);
				}
				else {
					SET_FUNCPTR_x264_mmx(8,8, SADCHROMA);
					SETBLITX(8,8,BLITCHROMA);
				}
			} else if (nBlkSizeY==2) {
			}
			break;
		case 4:
				SET_FUNCPTR_x264_mmx_4x(4, SAD);
				SATD = x264_pixel_satd_4x4_mmxext;
				SETBLITX(4,4,BLITLUMA);
				if (yRatioUV==2) {
					SADCHROMA = Sad2x2_iSSE_T;
			//		if (sse3)
			//			SADCHROMA = Sad2x2_iSSE_O; //debug
				}
				else {	
					SADCHROMA = Sad2x4_iSSE_T;
				}
			break;
		case 8:
		default:
			if (nBlkSizeY == 8) {
				SET_FUNCPTR_x264_mmx(8,8, SAD);
				SET_FUNCPTR_x264_SATD(8,8);
				SETBLITX(8,8,BLITLUMA);
				if (yRatioUV==2) {
					SET_FUNCPTR_x264_mmx_4x(4,SADCHROMA);
					SETBLITX(4,4,BLITCHROMA);
				}
				else {
					SET_FUNCPTR_x264_mmx_4x(8,SADCHROMA);
					SETBLITX(4,8,BLITCHROMA);
				}
			} else if (nBlkSizeY == 4) { // 8x4
				SET_FUNCPTR_x264_mmx(8,4, SAD);
				SET_FUNCPTR_x264_SATD(8,4);
				SETBLITX(8,4,BLITLUMA);
				if (yRatioUV==2) {	//for making it obvious,that yv12 is empty	
					SETBLITX(4,2,BLITCHROMA);
				}
				else {
					SET_FUNCPTR_x264_mmx_4x(4,SADCHROMA);
					SETBLITX(4,4,BLITCHROMA);
				}
			}
		}//end switch
	}
	if ( !chroma )
		SADCHROMA = Sad0_C;

// for debug:
//         SAD = x264_pixel_sad_4x4_mmxext;
//         VAR = Var_C<8>;
//         LUMA = Luma_C<8>;
//         BLITLUMA = Copy_C<16,16>;
//		 BLITCHROMA = Copy_C<8,8>; // idem
//		 SADCHROMA = x264_pixel_sad_8x8_mmxext;

    

    // round pitches to be mod PitchAlignement
    dctpitch   = (nBlkSizeX + (PitchAlignment-1)) & (~(PitchAlignment-1));
 
    // round plane sizes to be mod PlaneAlignment
    int PlaneSize =(dctpitch*nBlkSizeY + (PlaneSizeAlignment-1)) & (~(PlaneSizeAlignment-1));
    
    // allocate planes, Y - Planesize, U & V - 1/2 Planesize
    dctSrc = (unsigned char*) _aligned_malloc(2*PlaneSize, PlaneSizeAlignment);
    if (dctSrc) {
        dctRef = dctSrc + PlaneSize ;
    }
    else {
        // no memory 
        throw "Out of memroy PlaneOfBlocks.cpp";
    }

    // allocate temporary planes to copy source data to if not properly aligned
	int PlaneSizeY =(nBlkSizeX*nBlkSizeY + (PlaneSizeAlignment-1)) & (~(PlaneSizeAlignment-1));
	int PlaneSizeUV=((nBlkSizeX>>1)*nBlkSizeY/yRatioUV + (PlaneSizeAlignment-1)) & (~(PlaneSizeAlignment-1));
    pSrc_temp[0]=(unsigned char*) _aligned_malloc(PlaneSizeY+2*PlaneSizeUV, PlaneSizeAlignment);
    if (pSrc_temp[0]) {
	    pSrc_temp[1]=pSrc_temp[0]+PlaneSizeY;
	    pSrc_temp[2]=pSrc_temp[1]+PlaneSizeUV;
    }
    else {
        // no memory 
        throw "Out of memroy PlaneOfBlocks.cpp";
    }

    freqSize = 8192*nPel*2;// half must be more than max vector length, which is (framewidth + Padding) * nPel
    freqArray = new int[freqSize];
}

PlaneOfBlocks::~PlaneOfBlocks()
{
    if (vectors)      delete [] vectors;
    if (freqArray)    delete [] freqArray;
    if (dctSrc)       _aligned_free(dctSrc);
    if (pSrc_temp[0]) _aligned_free(pSrc_temp[0]);
}

void PlaneOfBlocks::SearchMVs(MVFrame *_pSrcFrame, MVFrame *_pRefFrame, SearchType st, int stp, 
                              int lambda, int lsad, int pnew, int plevel, int flags, int *out, 
                              VECTOR * globalMVec, short *outfilebuf, int fieldShift, DCTClass *_DCT, 
                              int * pmeanLumaChange, int divideExtra)
{
    DCT = _DCT;
#ifdef ALLOW_DCT
    if (DCT==0)
        dctmode = 0;
    else
        dctmode = DCT->dctmode;
    dctweight16 = std::min<int>(16,abs(*pmeanLumaChange)/(nBlkSizeX*nBlkSizeY)); //equal dct and spatial weights for meanLumaChange=8 (empirical)
#endif
    zeroMVfieldShifted.x = 0;
    zeroMVfieldShifted.y = fieldShift;
    globalMVPredictor.x = nPel*globalMVec->x;// v1.8.2
    globalMVPredictor.y = nPel*globalMVec->y + fieldShift;
    globalMVPredictor.sadLuma    = globalMVec->sadLuma;
    globalMVPredictor.sadChromaU = globalMVec->sadChromaU;
    globalMVPredictor.sadChromaV = globalMVec->sadChromaV;
    bool calcSrcLuma = (bool)(flags & MOTION_CALC_SRC_LUMA);
    bool calcRefLuma = (bool)(flags & MOTION_CALC_REF_LUMA);
    bool calcSrcVar = (bool)(flags & MOTION_CALC_VAR);
    int nOutPitchY = nBlkX * (nBlkSizeX - nOverlapX) + nOverlapX;
    int nOutPitchUV = (nBlkX * (nBlkSizeX - nOverlapX) + nOverlapX) / 2; // xRatioUV=2
//  char debugbuf[128];
//   wsprintf(debugbuf,"MVCOMP1: nOutPitchUV=%d, nOverlap=%d, nBlkX=%d, nBlkSize=%d",nOutPitchUV, nOverlap, nBlkX, nBlkSize);
//   OutputDebugString(debugbuf);

   // write the plane's header
    WriteHeaderToArray(out);

    int *pBlkData = out + 1;

    Uint8 *pMoCompDataY=NULL, *pMoCompDataU=NULL, *pMoCompDataV=NULL;
    if ( nLogScale == 0 ) {
        if (divideExtra) out += out[0]; // skip space for extra divided subblocks
        int sizeY = nOutPitchY * (nBlkY * (nBlkSizeY - nOverlapY) + nOverlapY);
        pMoCompDataY = reinterpret_cast<Uint8 *>(out + out[0] + 1);
        pMoCompDataU = pMoCompDataY + sizeY;
        pMoCompDataV = pMoCompDataU + sizeY/(2*yRatioUV);
        out += out[0];
        int size = (sizeY + sizeY/(2*yRatioUV) + sizeY/(2*yRatioUV) +3)/4;
        out[0] = size + 1; // int4
    }

    nFlags |= flags;

    pSrcFrame = _pSrcFrame;
    pRefFrame = _pRefFrame;

    x[0] = pSrcFrame->GetPlane(YPLANE)->GetHPadding();
    x[1] = pSrcFrame->GetPlane(UPLANE)->GetHPadding();
    x[2] = pSrcFrame->GetPlane(VPLANE)->GetHPadding();
    y[0] = pSrcFrame->GetPlane(YPLANE)->GetVPadding();
    y[1] = pSrcFrame->GetPlane(UPLANE)->GetVPadding();
    y[2] = pSrcFrame->GetPlane(VPLANE)->GetVPadding();

    blkx = 0;
    blky = 0;

    nRefPitch[0] = pRefFrame->GetPlane(YPLANE)->GetPitch();
    nRefPitch[1] = pRefFrame->GetPlane(UPLANE)->GetPitch();
    nRefPitch[2] = pRefFrame->GetPlane(VPLANE)->GetPitch();

    searchType = st;
    nSearchParam = stp;//*nPel; // v1.8.2 - redesigned in v1.8.5

    int nLambdaLevel = lambda / (nPel * nPel);
    if (plevel==1)
    nLambdaLevel = nLambdaLevel*nScale;// scale lambda - Fizick
    else if (plevel==2)
    nLambdaLevel = nLambdaLevel*nScale*nScale;

    // Functions using float must not be used here
    for ( blkIdx = 0; blkIdx < nBlkCount; ++blkIdx ) {
        //		DebugPrintf("BlkIdx = %d \n", blkIdx);
        PROFILE_START(MOTION_PROFILE_ME);
        
        pSrc[0] = pSrcFrame->GetPlane(YPLANE)->GetAbsolutePelPointer(x[0], y[0]);
        pSrc[1] = pSrcFrame->GetPlane(UPLANE)->GetAbsolutePelPointer(x[1], y[1]);
        pSrc[2] = pSrcFrame->GetPlane(VPLANE)->GetAbsolutePelPointer(x[2], y[2]);
        nSrcPitch[0] = pSrcFrame->GetPlane(YPLANE)->GetPitch();
        nSrcPitch[1] = pSrcFrame->GetPlane(UPLANE)->GetPitch();
        nSrcPitch[2] = pSrcFrame->GetPlane(VPLANE)->GetPitch();        

        // if source data is not pitch aligned then align it
        if (reinterpret_cast<int>(pSrc[0]) & (PitchAlignment-1)) {
            //create aligned copy 
            BLITLUMA  (pSrc_temp[0], nBlkSizeX,    pSrc[0], nSrcPitch[0], isse);

            //set the to the aligned copy
            pSrc[0] = pSrc_temp[0];
	        nSrcPitch[0] = nBlkSizeX;
        }
        // if source data is not pitch aligned then align it
        if (reinterpret_cast<int>(pSrc[1]) & (PitchAlignment-1)) {
            //create aligned copy 
            BLITCHROMA(pSrc_temp[1], nBlkSizeX>>1, pSrc[1], nSrcPitch[1], isse);

            //set the to the aligned copy
            pSrc[1] = pSrc_temp[1];
	        nSrcPitch[1] = nBlkSizeX/2;
        }
        // if source data is not pitch aligned then align it
        if (reinterpret_cast<int>(pSrc[2]) & (PitchAlignment-1)) {
            //create aligned copy 
            BLITCHROMA(pSrc_temp[2], nBlkSizeX>>1, pSrc[2], nSrcPitch[2], isse);

            //set the to the aligned copy
            pSrc[2] = pSrc_temp[2];
	        nSrcPitch[2] = nBlkSizeX/2;
        }

        if ( blky == 0 )
            nLambda = 0;
        else
            nLambda = nLambdaLevel;

        penaltyNew = pnew; // penalty for new vector
        LSAD = lsad;    // SAD limit for lambda using
        // may be they must be scaled by nPel ?

        /* computes search boundaries */
        //      nDxMax = nPel * (pSrcFrame->GetPlane(YPLANE)->GetExtendedWidth() - x[0] - nBlkSize);
        nDxMax = nPel * (pSrcFrame->GetPlane(YPLANE)->GetExtendedWidth() - x[0] - pSrcFrame->GetPlane(YPLANE)->GetHPadding());
        //      nDyMax = nPel * (pSrcFrame->GetPlane(YPLANE)->GetExtendedHeight()  - y[0] - nBlkSize);
        nDyMax = nPel * (pSrcFrame->GetPlane(YPLANE)->GetExtendedHeight()  - y[0] - pSrcFrame->GetPlane(YPLANE)->GetVPadding() );
        nDxMin = -nPel * x[0];
        nDyMin = -nPel * y[0];

        /* search the mv */
        predictor = ClipMV(vectors[blkIdx]);

        PseudoEPZSearch();
    //		bestMV = zeroMV; // debug

        if (outfilebuf != NULL) // write vector to outfile
        {
            outfilebuf[0] = bestMV.x;
            outfilebuf[1] = bestMV.y;
            outfilebuf[2] = (bestMV.sadLuma & 0x0000ffff); // low word
            outfilebuf[3] = (bestMV.sadLuma >> 16);     // high word, usually null
            outfilebuf[4] = (bestMV.sadChromaU & 0x0000ffff); // low word
            outfilebuf[5] = (bestMV.sadChromaU >> 16);     // high word, usually null
            outfilebuf[6] = (bestMV.sadChromaV & 0x0000ffff); // low word
            outfilebuf[7] = (bestMV.sadChromaV >> 16);     // high word, usually null
            outfilebuf += 8;// 8 bytes per block
        }

        /* write the results */
        pBlkData[0] = bestMV.x;
        pBlkData[1] = bestMV.y;
        pBlkData[2] = bestMV.sadLuma;
        pBlkData[3] = bestMV.sadChromaU;
        pBlkData[4] = bestMV.sadChromaV;

        PROFILE_STOP(MOTION_PROFILE_ME);

        PROFILE_START(MOTION_PROFILE_LUMA_VAR);
        if ( calcSrcVar )
            pBlkData[5] = VAR(pSrc[0], nSrcPitch[0], pBlkData + 4);
        else if ( calcSrcLuma ) {
            pBlkData[5] = 0;
            pBlkData[6] = LUMA(pSrc[0], nSrcPitch[0]);
        }
        else
            pBlkData[5] = pBlkData[6] = 0;

        if ( calcRefLuma )
            pBlkData[7] = LUMA(GetRefBlock(bestMV.x, bestMV.y), nRefPitch[0]);
        else
            pBlkData[7] = 0;
        pBlkData += ARRAY_ELEMENTS;
        PROFILE_STOP(MOTION_PROFILE_LUMA_VAR);

        if (smallestPlane)
            sumLumaChange += LUMA(GetRefBlock(0,0), nRefPitch[0]) - LUMA(pSrc[0], nSrcPitch[0]);

        PROFILE_START(MOTION_PROFILE_COMPENSATION);
        if ( nLogScale == 0 ) {
            if ( nFlags & MOTION_COMPENSATE_LUMA )
                BLITLUMA(pMoCompDataY, nOutPitchY, GetRefBlock(bestMV.x, bestMV.y), nRefPitch[0], isse);
            if ( nFlags & MOTION_COMPENSATE_CHROMA_U )
                BLITCHROMA(pMoCompDataU, nOutPitchUV, GetRefBlockU(bestMV.x, bestMV.y), nRefPitch[1], isse);
            if ( nFlags & MOTION_COMPENSATE_CHROMA_V )
                BLITCHROMA(pMoCompDataV, nOutPitchUV, GetRefBlockV(bestMV.x, bestMV.y), nRefPitch[2], isse);

            pMoCompDataY += (nBlkSizeX - nOverlapX);
            pMoCompDataU += (nBlkSizeX - nOverlapX) /2;
            pMoCompDataV += (nBlkSizeX - nOverlapX) /2;
            if ( blkx == nBlkX-1 )
            {
                pMoCompDataY += (nBlkSizeY - nOverlapY) * nOutPitchY - (nBlkSizeX - nOverlapX)*nBlkX;
                pMoCompDataU += ((nBlkSizeY - nOverlapY) / yRatioUV) * nOutPitchUV - ((nBlkSizeX - nOverlapX)/2) * nBlkX;
                pMoCompDataV += ((nBlkSizeY - nOverlapY) / yRatioUV) * nOutPitchUV - ((nBlkSizeX - nOverlapX)/2) * nBlkX;
            }
        }
        PROFILE_STOP(MOTION_PROFILE_COMPENSATION);

        /* increment indexes & pointers */
        ++blkx;
        x[0] += (nBlkSizeX - nOverlapX);
        x[1] += ((nBlkSizeX - nOverlapX) /2);
        x[2] += ((nBlkSizeX - nOverlapX) /2);
        if ( blkx == nBlkX ) {
            blkx = 0;
            x[0] = pSrcFrame->GetPlane(YPLANE)->GetHPadding();
            x[1] = pSrcFrame->GetPlane(UPLANE)->GetHPadding();
            x[2] = pSrcFrame->GetPlane(VPLANE)->GetHPadding();
            ++blky;
            y[0] += (nBlkSizeY - nOverlapY);
            y[1] += ((nBlkSizeY - nOverlapY) /yRatioUV );
            y[2] += ((nBlkSizeY - nOverlapY) /yRatioUV );
        }
	}
    if (smallestPlane)
	    *pmeanLumaChange = sumLumaChange/nBlkCount; // for all finer planes

    __asm { emms }
}

void PlaneOfBlocks::InterpolatePrediction(const PlaneOfBlocks &pob)
{
    int normFactor = 3 - nLogPel + pob.nLogPel;
    int mulFactor = (normFactor < 0) ? -normFactor : 0;
    normFactor = (normFactor < 0) ? 0 : normFactor;
    int normov = (nBlkSizeX - nOverlapX)*(nBlkSizeY - nOverlapY);
    int aoddx= (nBlkSizeX*3 - nOverlapX*2);
    int aevenx = (nBlkSizeX*3 - nOverlapX*4);
    int aoddy= (nBlkSizeY*3 - nOverlapY*2);
    int aeveny = (nBlkSizeY*3 - nOverlapY*4);

    for ( int l = 0, index = 0; l < nBlkY; ++l ) {
        for ( int k = 0; k < nBlkX; ++k, ++index ) {
            VECTOR v1, v2, v3, v4;
            int i = k;
            int j = l;
            if ( i == 2 * pob.nBlkX ) --i;
            if ( j == 2 * pob.nBlkY ) --j;
            int offy = -1 + 2 * ( j % 2);
            int offx = -1 + 2 * ( i % 2);

            if (( i == 0 ) || (i == 2 * pob.nBlkX - 1)) {
                if (( j == 0 ) || ( j == 2 * pob.nBlkY - 1)) {
                    v1 = v2 = v3 = v4 = pob.vectors[i / 2 + (j / 2) * pob.nBlkX];
                }
                else {
                    v1 = v2 = pob.vectors[i / 2 + (j / 2) * pob.nBlkX];
                    v3 = v4 = pob.vectors[i / 2 + (j / 2 + offy) * pob.nBlkX];
                }
            }
            else if (( j == 0 ) || ( j >= 2 * pob.nBlkY - 1)) {
                v1 = v2 = pob.vectors[i / 2 + (j / 2) * pob.nBlkX];
                v3 = v4 = pob.vectors[i / 2 + offx + (j / 2) * pob.nBlkX];
            }
            else {
                v1 = pob.vectors[i / 2 + (j / 2) * pob.nBlkX];
                v2 = pob.vectors[i / 2 + offx + (j / 2) * pob.nBlkX];
                v3 = pob.vectors[i / 2 + (j / 2 + offy) * pob.nBlkX];
                v4 = pob.vectors[i / 2 + offx + (j / 2 + offy) * pob.nBlkX];
            }

            if (nOverlapX == 0 && nOverlapY == 0) {
                vectors[index].x = 9 * v1.x + 3 * v2.x + 3 * v3.x + v4.x;
                vectors[index].y = 9 * v1.y + 3 * v2.y + 3 * v3.y + v4.y;
                vectors[index].sadLuma    = 9 * v1.sadLuma    + 3 * v2.sadLuma    + 3 * v3.sadLuma    + v4.sadLuma;
                vectors[index].sadChromaU = 9 * v1.sadChromaU + 3 * v2.sadChromaU + 3 * v3.sadChromaU + v4.sadChromaU;
                vectors[index].sadChromaV = 9 * v1.sadChromaV + 3 * v2.sadChromaV + 3 * v3.sadChromaV + v4.sadChromaV;
            }
            else if (nOverlapX <= (nBlkSizeX>>1) && nOverlapY <= (nBlkSizeY>>1)) // corrected in v1.4.11
            {
                int	ax1 = (offx > 0) ? aoddx : aevenx;
                int ax2 = (nBlkSizeX - nOverlapX)*4 - ax1;
                int ay1 = (offy > 0) ? aoddy : aeveny;
                int ay2 = (nBlkSizeY - nOverlapY)*4 - ay1;
                vectors[index].x = (ax1*ay1*v1.x + ax2*ay1*v2.x + ax1*ay2*v3.x + ax2*ay2*v4.x) / normov;
                vectors[index].y = (ax1*ay1*v1.y + ax2*ay1*v2.y + ax1*ay2*v3.y + ax2*ay2*v4.y) / normov;
                vectors[index].sadLuma    = (ax1*ay1*v1.sadLuma    + ax2*ay1*v2.sadLuma    + ax1*ay2*v3.sadLuma + 
                                             ax2*ay2*v4.sadLuma + (normov>>1)) / normov;
                vectors[index].sadChromaU = (ax1*ay1*v1.sadChromaU + ax2*ay1*v2.sadChromaU + ax1*ay2*v3.sadChromaU + 
                                             ax2*ay2*v4.sadChromaU + (normov>>1)) / normov;
                vectors[index].sadChromaV = (ax1*ay1*v1.sadChromaV + ax2*ay1*v2.sadChromaV + ax1*ay2*v3.sadChromaV + 
                                             ax2*ay2*v4.sadChromaV + (normov>>1)) / normov;
            }
            else // large overlap. Weights are not quite correct but let it be
            {
                vectors[index].x = (v1.x + v2.x + v3.x + v4.x) <<2;
                vectors[index].y = (v1.y + v2.y + v3.y + v4.y) <<2;
                vectors[index].sadLuma    = (v1.sadLuma    + v2.sadLuma    + v3.sadLuma    + v4.sadLuma) << 2;
                vectors[index].sadChromaU = (v1.sadChromaU + v2.sadChromaU + v3.sadChromaU + v4.sadChromaU) << 2;
                vectors[index].sadChromaV = (v1.sadChromaV + v2.sadChromaV + v3.sadChromaV + v4.sadChromaV) << 2;
            }

            vectors[index].x = (vectors[index].x >> normFactor) << mulFactor;
            vectors[index].y = (vectors[index].y >> normFactor) << mulFactor;
            vectors[index].sadLuma    = (vectors[index].sadLuma    + 8) >> 4;
            vectors[index].sadChromaU = (vectors[index].sadChromaU + 8) >> 4;
            vectors[index].sadChromaV = (vectors[index].sadChromaV + 8) >> 4;
        }
    }
}

void PlaneOfBlocks::WriteHeaderToArray(int *array)
{
    array[0] = nBlkCount * ARRAY_ELEMENTS + 1;
}

int PlaneOfBlocks::WriteDefaultToArray(int *array, int divideMode)
{
    array[0] = nBlkCount * ARRAY_ELEMENTS + 1;
    memset(array + 1, 0, nBlkCount * sizeof(int) * ARRAY_ELEMENTS);

    if ( nLogScale == 0 ) {
        array += array[0];
        if (divideMode) { // reserve space for divided subblocks extra level
            array[0] = nBlkCount * ARRAY_ELEMENTS * 4 + 1; // 4 subblocks
            memset(array + 1, 0, nBlkCount * sizeof(int) * ARRAY_ELEMENTS *4);
            array += array[0];
        }
        int nn = (nBlkX * (nBlkSizeX - nOverlapX) + nOverlapX) * (nBlkY  * (nBlkSizeY - nOverlapY) + nOverlapY);
        //	   int size = (nn + nn/4 + nn/4 +3)/4;
        int size = (nn + nn/(2*yRatioUV) + nn/(2*yRatioUV) +3)/4;
        array[0] = size + 1;
        memset(array + 1, 0, size*sizeof(int));
    }
    return GetArraySize(divideMode);
}

int PlaneOfBlocks::GetArraySize(int divideMode)
{
    int size = 0;
    size += 1;              // mb data size storage
    size += nBlkCount * ARRAY_ELEMENTS;  // vectors, sad, luma src, luma ref, var

    if ( nLogScale == 0) {
        if (divideMode)
            size += 1 + nBlkCount * ARRAY_ELEMENTS * 4; // reserve space for divided subblocks extra level
        size += 1;           // compensation size storage
        int nn = (nBlkX * (nBlkSizeX - nOverlapX) + nOverlapX) * (nBlkY  * (nBlkSizeY - nOverlapY) + nOverlapY);
        size += (nn + nn/(2*yRatioUV) + nn/(2*yRatioUV) +3)/4; // luma comp, chroma u comp, chroma v comp
    }

    return size;
}

void PlaneOfBlocks::FetchPredictors()
{
    // Left predictor
    if ( blkx > 0 ) predictors[1] = ClipMV(vectors[blkIdx - 1]);
    else predictors[1] = zeroMVfieldShifted;

    // Up predictor
    if ( blky > 0 ) predictors[2] = ClipMV(vectors[blkIdx - nBlkX]);
    else predictors[2] = zeroMVfieldShifted;

    // Up-right predictor
    if (( blky > 0 ) && ( blkx < nBlkX - 1 ))
    predictors[3] = ClipMV(vectors[blkIdx - nBlkX + 1]);
    else predictors[3] = zeroMVfieldShifted;

    // Median predictor
    if ( blky > 0 ) // replaced 1 by 0 - Fizick
    {
        // we really do not know SAD, here is more safe estimation especially for phaseshift method - v1.6.0
        int sad1 = predictors[1].sadLuma + predictors[1].sadChromaU + predictors[1].sadChromaV;
        int sad2 = predictors[2].sadLuma + predictors[2].sadChromaU + predictors[2].sadChromaV;
        int sad3 = predictors[3].sadLuma + predictors[3].sadChromaU + predictors[3].sadChromaV;
        // int sad = std::max<int>(sad1, std::max<int>(sad2, sad3));
        int sad = Median(sad1, sad2, sad3);
        int Source=1;
        if (sad==sad2)
            Source=2;
        else if (sad==sad3)
            Source=3;
        predictors[0].x = predictors[Source].x;
        predictors[0].y = predictors[Source].y;
        predictors[0].sadLuma    = predictors[Source].sadLuma;
        predictors[0].sadChromaU = predictors[Source].sadChromaU;
        predictors[0].sadChromaV = predictors[Source].sadChromaV;
    }
    else {
        //		predictors[0].x = (predictors[1].x + predictors[2].x + predictors[3].x);
        //		predictors[0].y = (predictors[1].y + predictors[2].y + predictors[3].y);
        //      predictors[0].sad = (predictors[1].sad + predictors[2].sad + predictors[3].sad);
        // but for top line we have only left predictor[1] - v1.6.0
        predictors[0].x = predictors[1].x;
        predictors[0].y = predictors[1].y;
        predictors[0].sadLuma    = predictors[1].sadLuma;
        predictors[0].sadChromaU = predictors[1].sadChromaU;
        predictors[0].sadChromaV = predictors[1].sadChromaV;
    }

    // if there are no other planes, predictor is the median
    if ( smallestPlane ) predictor = predictors[0];
    /*   else
    {
        if ( predictors[0].sad < predictor.sad )// disabled by Fizick (hierarhy only!)
        {
            predictors[4] = predictor;
            predictor = predictors[0];
            predictors[0] = predictors[4];
        }
    }
    */
   
    // if ( predictor.sad > LSAD ) nLambda = 0; // generalized (was LSAD=400) by Fizick
    int sad = predictor.sadLuma + predictor.sadChromaU + predictor.sadChromaV;
    nLambda = (nLambda*(LSAD*LSAD))/((LSAD + (sad>>1))*(LSAD + (sad>>1)));
    // replaced hard threshold by soft in v1.10.2 by Fizick (a liitle complex expression to avoid overflow)
}

void PlaneOfBlocks::PseudoEPZSearch()
{
    FetchPredictors();

#ifdef ALLOW_DCT
    if ( dctmode != 0 ) // DCT method (luma only - currently use normal spatial SAD chroma)
    {
        // make dct of source block
        if (dctmode <= 4) //don't do the slow dct conversion if SATD used
            DCT->DCTBytes2D(pSrc[0], nSrcPitch[0], dctSrc, dctpitch);
    }
    if (dctmode >= 3) // most use it and it should be fast anyway //if (dctmode == 3 || dctmode == 4) // check it
        srcLuma = LUMA(pSrc[0], nSrcPitch[0]);
#endif

    {
        // We treat zero alone
        // Do we bias zero with not taking into account distorsion ?
        bestMV.x = zeroMVfieldShifted.x;
        bestMV.y = zeroMVfieldShifted.y;
        int sadChromaU = SADCHROMA(pSrc[1], nSrcPitch[1], GetRefBlockU(0, 0), nRefPitch[1]);
        int sadChromaV = SADCHROMA(pSrc[2], nSrcPitch[2], GetRefBlockV(0, 0), nRefPitch[2]);
        int sadLuma = LumaSAD(GetRefBlock(0, zeroMVfieldShifted.y));
        int sad = sadLuma + sadChromaU + sadChromaV;
        nMinCost = sad;
        bestMV.sadLuma    = sadLuma;
        bestMV.sadChromaU = sadChromaU;
        bestMV.sadChromaV = sadChromaV;
    }

    // Global MV predictor  - added by Fizick
    if ( (( globalMVPredictor.x != zeroMVfieldShifted.x ) || ( globalMVPredictor.y != zeroMVfieldShifted.y )) &&
         IsVectorOK(globalMVPredictor.x, globalMVPredictor.y ) )
    {
        int sadChromaU = SADCHROMA(pSrc[1], nSrcPitch[1], GetRefBlockU(globalMVPredictor.x, globalMVPredictor.y), nRefPitch[1]);
        int sadChromaV = SADCHROMA(pSrc[2], nSrcPitch[2], GetRefBlockV(globalMVPredictor.x, globalMVPredictor.y), nRefPitch[2]);
        int sadLuma = LumaSAD(GetRefBlock(globalMVPredictor.x, globalMVPredictor.y));
        int sad = sadLuma + sadChromaU + sadChromaV;
        int cost = sad;
        if ( cost  < nMinCost ) {
            nMinCost = cost;
            bestMV.x = globalMVPredictor.x;
            bestMV.y = globalMVPredictor.y;
            bestMV.sadLuma    = sadLuma;
            bestMV.sadChromaU = sadChromaU;
            bestMV.sadChromaV = sadChromaV; 
        }
    }

	// Then, the predictor :
    if ( (( predictor.x != zeroMVfieldShifted.x ) || ( predictor.y != zeroMVfieldShifted.y )) &&
         (( predictor.x != globalMVPredictor.x ) || ( predictor.y != globalMVPredictor.y )) &&
         IsVectorOK(predictor.x, predictor.y) )
    {
        int sadChromaU = SADCHROMA(pSrc[1], nSrcPitch[1], GetRefBlockU(predictor.x, predictor.y), nRefPitch[1]);
        int sadChromaV = SADCHROMA(pSrc[2], nSrcPitch[2], GetRefBlockV(predictor.x, predictor.y), nRefPitch[2]);
        int sadLuma = LumaSAD(GetRefBlock(predictor.x, predictor.y));
        int sad = sadLuma + sadChromaU + sadChromaV;
        int cost = sad;
        if ( cost  < nMinCost ) {
            nMinCost = cost;
            bestMV.x = predictor.x;
            bestMV.y = predictor.y;
            bestMV.sadLuma    = sadLuma;
            bestMV.sadChromaU = sadChromaU;
            bestMV.sadChromaV = sadChromaV; 
        }
    }

    // then all the other predictors
    for ( int i = 0; i < 4; ++i )
    {
        CheckMV(predictors[i].x, predictors[i].y);
    }

//	if (nMinCost > penaltyNew) // only do search if no good predictors - disabled in v1.5.8


    // then, we refine, according to the search type
    if ( searchType & ONETIME )
        for ( int i = nSearchParam; i > 0; i /= 2 )
            OneTimeSearch(i);

    if ( searchType & NSTEP )
        NStepSearch(nSearchParam);

    if ( searchType & LOGARITHMIC )
        for ( int i = nSearchParam; i > 0; i /= 2 )
            DiamondSearch(i);

    if ( searchType & EXHAUSTIVE )
    //       ExhaustiveSearch(nSearchParam);
        for ( int i = 1; i <= nSearchParam; i++ )// region is same as enhausted, but ordered by radius (from near to far)
            ExpandingSearch(i);

//	if ( searchType & SQUARE )
//		SquareSearch();

    // we store the result
    vectors[blkIdx].x = bestMV.x;
    vectors[blkIdx].y = bestMV.y;
    vectors[blkIdx].sadLuma    = bestMV.sadLuma;
    vectors[blkIdx].sadChromaU = bestMV.sadChromaU;
    vectors[blkIdx].sadChromaV = bestMV.sadChromaV;
}

void PlaneOfBlocks::DiamondSearch(int length)
{
    // The meaning of the directions are the following :
    //		* 1 means right
    //		* 2 means left
    //		* 4 means down
    //		* 8 means up
    // So 1 + 4 means down right, and so on...

    int dx;
    int dy;

    // We begin by making no assumption on which direction to search.
    int direction = 15;

    int lastDirection;

    while ( direction > 0 ) {
        dx = bestMV.x;
        dy = bestMV.y;
        lastDirection = direction;
        direction = 0;

        // First, we look the directions that were hinted by the previous step
        // of the algorithm. If we find one, we add it to the set of directions
        // we'll test next
        int dxp=dx+length, dxm=dx-length;
        int dyp=dy+length, dym=dy-length;
        if ( lastDirection & 1 ) CheckMV2(dxp, dy, &direction, 1);
        if ( lastDirection & 2 ) CheckMV2(dxm, dy, &direction, 2);
        if ( lastDirection & 4 ) CheckMV2(dx,  dyp, &direction, 4);
        if ( lastDirection & 8 ) CheckMV2(dx,  dym, &direction, 8);

        // If one of the directions improves the SAD, we make further tests
        // on the diagonals
        if ( direction ) {
            lastDirection = direction;
            dx = bestMV.x;
            dy = bestMV.y;
            dxp=dx+length, dxm=dx-length;
            dyp=dy+length, dym=dy-length;

            if ( lastDirection & 3 ) {
                CheckMV2(dx, dyp, &direction, 4);
                CheckMV2(dx, dym, &direction, 8);
            }
            else {
                CheckMV2(dxp, dy, &direction, 1);
                CheckMV2(dxm, dy, &direction, 2);
            }
        }

        // If not, we do not stop here. We infer from the last direction the
        // diagonals to be checked, because we might be lucky.
        else {
            switch ( lastDirection ) {
            case 1 :
                CheckMV2(dxp, dyp, &direction, 1 + 4);
                CheckMV2(dxp, dym, &direction, 1 + 8);
                break;
            case 2 :
                CheckMV2(dxm, dyp, &direction, 2 + 4);
                CheckMV2(dxm, dym, &direction, 2 + 8);
                break;
            case 4 :
                CheckMV2(dxp, dyp, &direction, 1 + 4);
                CheckMV2(dxm, dyp, &direction, 2 + 4);
                break;
            case 8 :
                CheckMV2(dxp, dym, &direction, 1 + 8);
                CheckMV2(dxm, dym, &direction, 2 + 8);
                break;
            case 1 + 4 :
                CheckMV2(dxp, dyp, &direction, 1 + 4);
                CheckMV2(dxm, dyp, &direction, 2 + 4);
                CheckMV2(dxp, dym, &direction, 1 + 8);
                break;
            case 2 + 4 :
                CheckMV2(dxp, dyp, &direction, 1 + 4);
                CheckMV2(dxm, dyp, &direction, 2 + 4);
                CheckMV2(dxm, dym, &direction, 2 + 8);
                break;
            case 1 + 8 :
                CheckMV2(dxp, dyp, &direction, 1 + 4);
                CheckMV2(dxm, dym, &direction, 2 + 8);
                CheckMV2(dxp, dym, &direction, 1 + 8);
                break;
            case 2 + 8 :
                CheckMV2(dxm, dym, &direction, 2 + 8);
                CheckMV2(dxm, dyp, &direction, 2 + 4);
                CheckMV2(dxp, dym, &direction, 1 + 8);
                break;
            default :
                // Even the default case may happen, in the first step of the
                // algorithm for example.
                CheckMV2(dxp, dyp, &direction, 1 + 4);
                CheckMV2(dxm, dyp, &direction, 2 + 4);
                CheckMV2(dxp, dym, &direction, 1 + 8);
                CheckMV2(dxm, dym, &direction, 2 + 8);
                break;
            }
        }
    }
}

void PlaneOfBlocks::SquareSearch()
{
    ExhaustiveSearch(1);
}

void PlaneOfBlocks::ExhaustiveSearch(int s)// diameter = 2*s - 1
{
    int i, j;
    VECTOR mv = bestMV;

    for ( i = -s + 1; i < 0; ++i )
        for ( j = -s + 1; j < s; ++j )
            CheckMV(mv.x + i, mv.y + j);

    for ( i = 1; i < s; ++i )
        for ( j = -s + 1; j < s; ++j )
            CheckMV(mv.x + i, mv.y + j);

    for ( j = -s + 1; j < 0; ++j )
        CheckMV(mv.x, mv.y + j);

    for ( j = 1; j < s; ++j )
        CheckMV(mv.x, mv.y + j);
}

void PlaneOfBlocks::NStepSearch(int stp)
{
    int dx, dy;
    int length = stp;
    while ( length > 0 ) {
        dx = bestMV.x;
        dy = bestMV.y;

        int dxp=dx+length, dxm=dx-length;
        int dyp=dy+length, dym=dy-length;
        CheckMV(dxp, dyp);
        CheckMV(dxp, dy);
        CheckMV(dxp, dym);
        CheckMV(dx, dym);
        CheckMV(dx, dyp);
        CheckMV(dxm, dyp);
        CheckMV(dxm, dy);
        CheckMV(dxm, dym);

        --length;
    }
}

void PlaneOfBlocks::OneTimeSearch(int length)
{
    int direction = 0;
    int dx = bestMV.x;
    int dy = bestMV.y;

    CheckMV2(dx - length, dy, &direction, 2);
    CheckMV2(dx + length, dy, &direction, 1);

    if ( direction == 1 ) {
        while ( direction ) {
            direction = 0;
            dx += length;
            CheckMV2(dx + length, dy, &direction, 1);
        }
    }
    else if ( direction == 2 ) {
        while ( direction ) {
            direction = 0;
            dx -= length;
            CheckMV2(dx - length, dy, &direction, 1);
        }
    }

    CheckMV2(dx, dy - length, &direction, 2);
    CheckMV2(dx, dy + length, &direction, 1);

    if ( direction == 1 ) {
        while ( direction ) {
            direction = 0;
            dy += length;
            CheckMV2(dx, dy + length, &direction, 1);
        }
    }
    else if ( direction == 2 ) {
        while ( direction ) {
            direction = 0;
            dy -= length;
            CheckMV2(dx, dy - length, &direction, 1);
        }
    }
}

void PlaneOfBlocks::ExpandingSearch(int s) // diameter = 2*s + 1
{
    int i, j;
    VECTOR mv = bestMV;

    int mvxp=mv.x+s, mvxm=mv.x-s;
    int mvyp=mv.y+s, mvym=mv.y-s;

    // sides of square without corners
    for ( i = -s; i <= s; ++i ) {
        CheckMV(mv.x + i, mvym);
        CheckMV(mv.x + i, mvyp);
    }

    for ( j = -s; j <= s; ++j ) {
        CheckMV(mvxm, mv.y + j);
        CheckMV(mvxp, mv.y + j);
    }

    // then corners - they are more far from cenrer
    CheckMV(mvxm, mvym);
    CheckMV(mvxm, mvyp);
    CheckMV(mvxp, mvym);
    CheckMV(mvxp, mvyp);
}

//----------------------------------------------------------------

void PlaneOfBlocks::EstimateGlobalMVDoubled(VECTOR *globalMVec)
{
    // estimate global motion from current plane vectors data for using on next plane - added by Fizick
    // on input globalMVec is prev estimation
    // on output globalMVec is doubled for next scale plane using

    // use very simple but robust method
    // more advanced method (like MVDepan) can be implemented later

    // find most frequent x
    memset(&freqArray[0], 0, freqSize*sizeof(int)); // reset
    int indmin = freqSize-1;
    int indmax = 0;
    VECTOR *pvectors=vectors;
    int freqSize2=freqSize>>1;
    for (int i=0; i<nBlkCount; ++i, ++pvectors) {
        int ind = freqSize2+pvectors->x;
        if (ind>=0 && ind<freqSize) {
            ++freqArray[ind];
            if (ind > indmax) indmax = ind;
            if (ind < indmin) indmin = ind;
        }
    }
    int count = freqArray[indmin];
    int index = indmin;
    for (int i=indmin+1; i<=indmax; ++i) {
        if (freqArray[i] > count) {
            count = freqArray[i];
            index = i;
        }
    }
    int medianx = (index-freqSize2); // most frequent value

    // find most frequent y
    memset(&freqArray[0], 0, freqSize*sizeof(int)); // reset
    indmin = freqSize-1;
    indmax = 0;
    pvectors=vectors;
    for (int i=0; i<nBlkCount; ++i, ++pvectors) {
        int ind = freqSize2+pvectors->y;
        if (ind>=0 && ind<freqSize) {
            ++freqArray[ind];
            if (ind > indmax) indmax = ind;
            if (ind < indmin) indmin = ind;
        }
    }
    count = freqArray[indmin];
    index = indmin;
    for (int i=indmin+1; i<=indmax; ++i) {
        if (freqArray[i] > count) {
            count = freqArray[i];
            index = i;
        }
    }
    int mediany = (index-freqSize2); // most frequent value


    // iteration to increase precision
    int meanvx = 0;
    int meanvy = 0;
    int num = 0;
    pvectors=vectors;
    for ( int i=0; i < nBlkCount; ++i, ++pvectors ) {
        if (abs(pvectors->x - medianx) < 6 && abs(pvectors->y - mediany) < 6 ) {
            meanvx += pvectors->x;
            meanvy += pvectors->y;
            ++num;
        }
    }

    // output vectors must be doubled for next (finer) scale level
    if (num >0) {
        globalMVec->x = 2*meanvx / num;
        globalMVec->y = 2*meanvy / num;
    }
    else {
        globalMVec->x = 2*medianx;
        globalMVec->y = 2*mediany;
    }

// char debugbuf[100];
// sprintf(debugbuf,"MVAnalyse: nx=%d ny=%d next global vx=%d vy=%d", nBlkX, nBlkY, globalMVec->x, globalMVec->y);
// OutputDebugString(debugbuf);

}
