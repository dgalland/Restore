// Author: Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick - global motion, overlap,  mode, refineMVs
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
   nLogyRatioUV = ilog2(yRatioUV);

   smallestPlane = (bool)(nFlags & MOTION_SMALLEST_PLANE);
//   mmx = (bool)(nFlags & MOTION_USE_MMX);
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
   globalMVPredictor.sad = zeroMV.sad;

/* arrays memory allocation */

   vectors = new VECTOR[nBlkCount];

/* function's pointers initialization */

#define SET_FUNCPTR(blksizex, blksizey, blksizex2, blksizey2) \
	SAD = Sad##blksizex##x##blksizey##_iSSE; \
	VAR = Var##blksizex##x##blksizey##_iSSE; \
	LUMA = Luma##blksizex##x##blksizey##_iSSE; \
	BLITLUMA = Copy##blksizex##x##blksizey##_mmx; \
	if (yRatioUV==2) { \
		BLITCHROMA = Copy##blksizex2##x##blksizey2##_mmx; \
		SADCHROMA = Sad##blksizex2##x##blksizey2##_iSSE; \
	} \
	else { \
		BLITCHROMA = Copy##blksizex2##x##blksizey##_mmx; \
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


/*
   if ( isse )
   {
      switch (nBlkSizeX)
      {
      case 16:
      if (nBlkSizeY==16) {
         SAD = Sad16x16_iSSE;
         VAR = Var16x16_iSSE;
         LUMA = Luma16x16_iSSE;
         BLITLUMA = Copy16_mmx;
		 if (yRatioUV==2) {
			 BLITCHROMA = Copy8_mmx;
			 SADCHROMA = Sad8x8_iSSE;
		 }
		 else { //yRatioUV==1
			 BLITCHROMA = Copy8x16_mmx;
			 SADCHROMA = Sad8x16_iSSE;
		 }
      } else if (nBlkSizeY==8) {
         SAD = Sad16x8_iSSE;
         VAR = Var16x8_iSSE;
         LUMA = Luma16x8_iSSE;
         BLITLUMA = Copy16x8_mmx;
		 if (yRatioUV==2) {
			 BLITCHROMA = Copy8x4_mmx;
			 SADCHROMA = Sad8x4_iSSE;
		 }
		 else { //yRatioUV==1
			 BLITCHROMA = Copy8_mmx;
			 SADCHROMA = Sad8x8_iSSE;
		 }
      } else if (nBlkSizeY==2) {
         SAD = Sad16x2_iSSE;
         VAR = Var16x2_iSSE;
         LUMA = Luma16x2_iSSE;
         BLITLUMA = Copy16x2_mmx;
 		 if (yRatioUV==2) {
			BLITCHROMA = Copy8x1_mmx;
			 SADCHROMA = Sad8x1_iSSE;
		 }
		 else {//yRatioUV==1
			BLITCHROMA = Copy8x2_mmx;
			 SADCHROMA = Sad8x2_iSSE;
		 }
      }
         break;
      case 4:
         SAD = Sad4x4_iSSE;
         VAR = Var4x4_iSSE;
         LUMA = Luma4x4_iSSE;
         BLITLUMA = Copy4_mmx;
		 if (yRatioUV==2) {
			 BLITCHROMA = Copy2_mmx;
			 SADCHROMA = Sad2x2_iSSE;
		 }
		 else { //yRatioUV==1
			 BLITCHROMA = Copy2x4_mmx;
			 SADCHROMA = Sad2x4_iSSE;
		 }
         break;
      case 8:
      default:
      if (nBlkSizeY == 8) {
         SAD = Sad8x8_iSSE;
         VAR = Var8x8_iSSE;
         LUMA = Luma8x8_iSSE;
         BLITLUMA = Copy8_mmx;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy4_mmx;
		     SADCHROMA = Sad4x4_iSSE;
		 }
		 else {//yRatioUV==1
	         BLITCHROMA = Copy4x8_mmx;
		     SADCHROMA = Sad4x8_iSSE;
		 }
      } else if (nBlkSizeY == 4) { // 8x4
         SAD = Sad8x4_iSSE;
         VAR = Var8x4_iSSE;
         LUMA = Luma8x4_iSSE;
         BLITLUMA = Copy8x4_mmx;
		 if (yRatioUV==2) {
			 BLITCHROMA = Copy4x2_mmx; // idem
			 SADCHROMA = Sad4x2_iSSE;
		 }
		 else {//yRatioUV==1
			 BLITCHROMA = Copy4_mmx; // idem
			 SADCHROMA = Sad4x4_iSSE;
		 }
      }

      }
   }
   else
   {
      switch (nBlkSizeX)
      {
      case 16:
      if (nBlkSizeY==16) {
         SAD = Sad_C<16>;
         VAR = Var_C<16>;
         LUMA = Luma_C<16>;
         BLITLUMA = Copy_C<16>; // "mmx" version could be used, but it's more like a debugging version
 		 if (yRatioUV==2) {
			BLITCHROMA = Copy_C<8>; // idem
			 SADCHROMA = Sad_C<8>;
		 }
		 else {//yRatioUV==1
			BLITCHROMA = Copy_C<8,16>; // idem
			 SADCHROMA = Sad_C<8,16>;
		 }
      } else if (nBlkSizeY==8) {
         SAD = Sad_C<16,8>;
         VAR = Var_C<16,8>;
         LUMA = Luma_C<16,8>;
         BLITLUMA = Copy_C<16,8>; // "mmx" version could be used, but it's more like a debugging version
 		 if (yRatioUV==2) {
			BLITCHROMA = Copy_C<8,4>; // idem
			 SADCHROMA = Sad_C<8,4>;
		 }
		 else {//yRatioUV==1
			BLITCHROMA = Copy_C<8>; // idem
			 SADCHROMA = Sad_C<8>;
		 }
      } else if (nBlkSizeY==2) {
         SAD = Sad_C<16,2>;
         VAR = Var_C<16,2>;
         LUMA = Luma_C<16,2>;
         BLITLUMA = Copy_C<16,2>;
 		 if (yRatioUV==2) {
			BLITCHROMA = Copy_C<8,1>;
			 SADCHROMA = Sad_C<8,1>;
		 }
		 else {//yRatioUV==1
			BLITCHROMA = Copy_C<8,2>;
			 SADCHROMA = Sad_C<8,2>;
		 }
      }
         break;
      case 4:
         SAD = Sad_C<4>;
         VAR = Var_C<4>;
         LUMA = Luma_C<4>;
         BLITLUMA = Copy_C<4>; // "mmx" version could be used, but it's more like a debugging version
 		 if (yRatioUV==2) {
			 BLITCHROMA = Copy_C<2>; // idem
			 SADCHROMA = Sad_C<2>;
		 }
		 else {//yRatioUV==1
			BLITCHROMA = Copy_C<2,4>; // idem
			SADCHROMA = Sad_C<2,4>;
		 }
         break;
      case 8:
      default:
      if (nBlkSizeY==8) {
         SAD = Sad_C<8>;
         VAR = Var_C<8>;
         LUMA = Luma_C<8>;
         BLITLUMA = Copy_C<8>;
		 if (yRatioUV==2) {
			 BLITCHROMA = Copy_C<4>; // idem
			 SADCHROMA = Sad_C<4>;
		 }
		 else {//yRatioUV==1
			 BLITCHROMA = Copy_C<4,8>; // idem
			 SADCHROMA = Sad_C<4,8>;
		 }
      }else if (nBlkSizeY==4) {
         SAD = Sad_C<8,4>;
         VAR = Var_C<8,4>;
         LUMA = Luma_C<8,4>;
         BLITLUMA = Copy_C<8,4>;
		 if (yRatioUV==2) {
			 BLITCHROMA = Copy_C<4,2>; // idem
			 SADCHROMA = Sad_C<4,2>;
		 }
		 else {//yRatioUV==1
			 BLITCHROMA = Copy_C<4>; // idem
			 SADCHROMA = Sad_C<4>;
		 }
      }

      }
   }
   if ( !chroma )
      SADCHROMA = Sad_C<0>;
*/
	SATD = SadDummy; //for now disable SATD if default functions are used
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
						BLITCHROMA = Copy8x1_mmx;
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
		SADCHROMA = SadDummy;

// for debug:
//         SAD = x264_pixel_sad_4x4_mmxext;
//         VAR = Var_C<8>;
//         LUMA = Luma_C<8>;
//         BLITLUMA = Copy_C<16,16>;
//		 BLITCHROMA = Copy_C<8,8>; // idem
//		 SADCHROMA = x264_pixel_sad_8x8_mmxext;


#ifdef ALIGN_SOURCEBLOCK
	dctpitch = max(nBlkSizeX, 16);
	dctSrc_base = new BYTE[nBlkSizeY*dctpitch+ALIGN_PLANES-1];
	dctRef_base = new BYTE[nBlkSizeY*dctpitch+ALIGN_PLANES-1];
	dctSrc = (BYTE *)(((((int)dctSrc_base) + ALIGN_PLANES - 1)&(~(ALIGN_PLANES - 1))));//aligned like this means, that it will have optimum fit in the cache
	dctRef = (BYTE *)(((((int)dctRef_base) + ALIGN_PLANES - 1)&(~(ALIGN_PLANES - 1))));

	int blocksize=nBlkSizeX*nBlkSizeY;
	int sizeAlignedBlock=blocksize+(ALIGN_SOURCEBLOCK-(blocksize%ALIGN_SOURCEBLOCK))+2*((blocksize/2)>>nLogyRatioUV)+(ALIGN_SOURCEBLOCK-(((blocksize/2)/yRatioUV)%ALIGN_SOURCEBLOCK));
	pSrc_temp_base=new Uint8[sizeAlignedBlock+ALIGN_PLANES-1];
	pSrc_temp[0]=(Uint8 *)((((int)pSrc_temp_base) + ALIGN_PLANES - 1)&(~(ALIGN_PLANES - 1)));
	pSrc_temp[1]=(Uint8 *)((((int)pSrc_temp[0]) + blocksize + ALIGN_SOURCEBLOCK - 1)&(~(ALIGN_SOURCEBLOCK - 1)));
	pSrc_temp[2]=(Uint8 *)(((((int)pSrc_temp[1]) + ((blocksize/2)>>nLogyRatioUV) + ALIGN_SOURCEBLOCK - 1)&(~(ALIGN_SOURCEBLOCK - 1))));
#else
	dctpitch = max(nBlkSizeX, 16);
	dctSrc = new BYTE[nBlkSizeY*dctpitch];
	dctRef = new BYTE[nBlkSizeY*dctpitch];
#endif

   freqSize = 8192*nPel*2;// half must be more than max vector length, which is (framewidth + Padding) * nPel
   freqArray = new int[freqSize];

}

PlaneOfBlocks::~PlaneOfBlocks()
{
   delete[] vectors;
   delete[] freqArray;

#ifdef ALIGN_SOURCEBLOCK
	delete[] dctSrc_base;
	delete[] dctRef_base;
	delete[] pSrc_temp_base;
#else
	delete[] dctSrc;
	delete[] dctRef;
#endif
}

void PlaneOfBlocks::SearchMVs(MVFrame *_pSrcFrame, MVFrame *_pRefFrame,
                              SearchType st, int stp, int lambda, int lsad, int pnew,
							  int plevel, int flags, int *out, VECTOR * globalMVec,
							  short *outfilebuf, int fieldShift, DCTClass *_DCT, int * pmeanLumaChange,
							  int divideExtra, int _pzero, int _pglobal, int _badSAD, int _badrange)
{
	DCT = _DCT;
#ifdef ALLOW_DCT
	if (DCT==0)
		dctmode = 0;
	else
		dctmode = DCT->dctmode;
	dctweight16 = min(16,abs(*pmeanLumaChange)/(nBlkSizeX*nBlkSizeY)); //equal dct and spatial weights for meanLumaChange=8 (empirical)
#endif
    badSAD = _badSAD;
    badrange = _badrange;
	zeroMVfieldShifted.x = 0;
	zeroMVfieldShifted.y = fieldShift;
	globalMVPredictor.x = nPel*globalMVec->x;// v1.8.2
	globalMVPredictor.y = nPel*globalMVec->y + fieldShift;
	globalMVPredictor.sad = globalMVec->sad;
   int nOutPitchY = nBlkX * (nBlkSizeX - nOverlapX) + nOverlapX;
   int nOutPitchUV = (nBlkX * (nBlkSizeX - nOverlapX) + nOverlapX) / 2; // xRatioUV=2
//  char debugbuf[128];
//   wsprintf(debugbuf,"MVCOMP1: nOutPitchUV=%d, nOverlap=%d, nBlkX=%d, nBlkSize=%d",nOutPitchUV, nOverlap, nBlkX, nBlkSize);
//   OutputDebugString(debugbuf);

   // write the plane's header
   WriteHeaderToArray(out);

   int *pBlkData = out + 1;

   if ( nLogScale == 0 )
   {
       if (divideExtra)
           out += out[0]; // skip space for extra divided subblocks
   }

   nFlags |= flags;

   pSrcFrame = _pSrcFrame;
   pRefFrame = _pRefFrame;


   x[0] = pSrcFrame->GetPlane(YPLANE)->GetHPadding();
   y[0] = pSrcFrame->GetPlane(YPLANE)->GetVPadding();

  if (pSrcFrame->GetMode() & UPLANE)
  {
     x[1] = pSrcFrame->GetPlane(UPLANE)->GetHPadding();
     y[1] = pSrcFrame->GetPlane(UPLANE)->GetVPadding();
  }
  if (pSrcFrame->GetMode() & VPLANE)
  {
    x[2] = pSrcFrame->GetPlane(VPLANE)->GetHPadding();
    y[2] = pSrcFrame->GetPlane(VPLANE)->GetVPadding();
  }
	blkx = 0;
	blky = 0;

#ifdef ALIGN_SOURCEBLOCK
	nSrcPitch_plane[0] = pSrcFrame->GetPlane(YPLANE)->GetPitch();
  if (chroma)
  {
    nSrcPitch_plane[1] = pSrcFrame->GetPlane(UPLANE)->GetPitch();
    nSrcPitch_plane[2] = pSrcFrame->GetPlane(VPLANE)->GetPitch();
  }
	nSrcPitch[0] = nBlkSizeX;
	nSrcPitch[1] = nBlkSizeX/2;
	nSrcPitch[2] = nBlkSizeX/2;
#else
	nSrcPitch[0] = pSrcFrame->GetPlane(YPLANE)->GetPitch();
  if (chroma)
  {
    nSrcPitch[1] = pSrcFrame->GetPlane(UPLANE)->GetPitch();
	nSrcPitch[2] = pSrcFrame->GetPlane(VPLANE)->GetPitch();
  }
#endif
   nRefPitch[0] = pRefFrame->GetPlane(YPLANE)->GetPitch();
  if (chroma)
  {
   nRefPitch[1] = pRefFrame->GetPlane(UPLANE)->GetPitch();
   nRefPitch[2] = pRefFrame->GetPlane(VPLANE)->GetPitch();
  }

   searchType = st;//( nLogScale == 0 ) ? st : EXHAUSTIVE;
	nSearchParam = stp;//*nPel; // v1.8.2 - redesigned in v1.8.5

   int nLambdaLevel = lambda / (nPel * nPel);
   if (plevel==1)
        nLambdaLevel = nLambdaLevel*nScale;// scale lambda - Fizick
    else if (plevel==2)
        nLambdaLevel = nLambdaLevel*nScale*nScale;

    penaltyZero = _pzero;
    pglobal = _pglobal;
    planeSAD = 0;
    badcount = 0;
	// Functions using float must not be used here
	for ( blkIdx = 0; blkIdx < nBlkCount; blkIdx++ )
	{
	    iter=0;
//		DebugPrintf("BlkIdx = %d \n", blkIdx);
      PROFILE_START(MOTION_PROFILE_ME);

#ifdef ALIGN_SOURCEBLOCK
		//store the pitch
		pSrc[0] = pSrcFrame->GetPlane(YPLANE)->GetAbsolutePelPointer(x[0], y[0]);
		//create aligned copy
		BLITLUMA  (pSrc_temp[0],nSrcPitch[0],pSrc[0],nSrcPitch_plane[0]);
		//set the to the aligned copy
		pSrc[0] = pSrc_temp[0];
    if (chroma)
    {
		pSrc[1] = pSrcFrame->GetPlane(UPLANE)->GetAbsolutePelPointer(x[1], y[1]);
		BLITCHROMA(pSrc_temp[1],nSrcPitch[1],pSrc[1],nSrcPitch_plane[1]);
		pSrc[1] = pSrc_temp[1];
		pSrc[2] = pSrcFrame->GetPlane(VPLANE)->GetAbsolutePelPointer(x[2], y[2]);
		BLITCHROMA(pSrc_temp[2],nSrcPitch[2],pSrc[2],nSrcPitch_plane[2]);
		pSrc[2] = pSrc_temp[2];
    }
#else
		pSrc[0] = pSrcFrame->GetPlane(YPLANE)->GetAbsolutePelPointer(x[0], y[0]);
    if (chroma)
    {
		pSrc[1] = pSrcFrame->GetPlane(UPLANE)->GetAbsolutePelPointer(x[1], y[1]);
		pSrc[2] = pSrcFrame->GetPlane(VPLANE)->GetAbsolutePelPointer(x[2], y[2]);
    }
#endif

      if ( blky == 0 )
		  nLambda = 0;
      else
        nLambda = nLambdaLevel;

	  penaltyNew = pnew; // penalty for new vector
	  LSAD = lsad;    // SAD limit for lambda using
	  // may be they must be scaled by nPel ?

      /* computes search boundaries */
      nDxMax = nPel * (pSrcFrame->GetPlane(YPLANE)->GetExtendedWidth() - x[0] - nBlkSizeX);
      nDyMax = nPel * (pSrcFrame->GetPlane(YPLANE)->GetExtendedHeight()  - y[0] - nBlkSizeY);
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
		  outfilebuf[2] = (bestMV.sad & 0x0000ffff); // low word
		  outfilebuf[3] = (bestMV.sad >> 16);     // high word, usually null
		  outfilebuf += 4;// 8 bytes per block
	  }

      /* write the results */
      pBlkData[0] = bestMV.x;
      pBlkData[1] = bestMV.y;
      pBlkData[2] = bestMV.sad;

      PROFILE_STOP(MOTION_PROFILE_ME);

      pBlkData += N_PER_BLOCK;

		if (smallestPlane)
			sumLumaChange += LUMA(GetRefBlock(0,0), nRefPitch[0]) - LUMA(pSrc[0], nSrcPitch[0]);

        /* increment indexes & pointers */
		blkx++;
      x[0] += (nBlkSizeX - nOverlapX);
      x[1] += ((nBlkSizeX - nOverlapX) /2);
      x[2] += ((nBlkSizeX - nOverlapX) /2);
 		if ( blkx == nBlkX )
		{
			blkx = 0;
         x[0] = pSrcFrame->GetPlane(YPLANE)->GetHPadding();
        if (chroma)
        {
         x[1] = pSrcFrame->GetPlane(UPLANE)->GetHPadding();
         x[2] = pSrcFrame->GetPlane(VPLANE)->GetHPadding();
        }
			blky++;
			y[0] += (nBlkSizeY - nOverlapY);
         y[1] += ((nBlkSizeY - nOverlapY) >> nLogyRatioUV );
         y[2] += ((nBlkSizeY - nOverlapY) >> nLogyRatioUV );
		}
	}
	if (smallestPlane)
		*pmeanLumaChange = sumLumaChange/nBlkCount; // for all finer planes

	__asm { emms }
}


void PlaneOfBlocks::RecalculateMVs(MVClip & mvClip, MVFrame *_pSrcFrame, MVFrame *_pRefFrame,
                              SearchType st, int stp, int lambda, int lsad, int pnew,
							  int flags, int *out,
							  short *outfilebuf, int fieldShift, int thSAD, DCTClass *_DCT, int divideExtra, int smooth)
{
	DCT = _DCT;
#ifdef ALLOW_DCT
	if (DCT==0)
		dctmode = 0;
	else
		dctmode = DCT->dctmode;
	dctweight16 = 8;//min(16,abs(*pmeanLumaChange)/(nBlkSizeX*nBlkSizeY)); //equal dct and spatial weights for meanLumaChange=8 (empirical)
#endif
	zeroMVfieldShifted.x = 0;
	zeroMVfieldShifted.y = fieldShift;
	globalMVPredictor.x = 0;//nPel*globalMVec->x;// there is no global
	globalMVPredictor.y = fieldShift;//nPel*globalMVec->y + fieldShift;
	globalMVPredictor.sad = 9999999;//globalMVec->sad;

   int nOutPitchY = nBlkX * (nBlkSizeX - nOverlapX) + nOverlapX;
   int nOutPitchUV = (nBlkX * (nBlkSizeX - nOverlapX) + nOverlapX) / 2; // xRatioUV=2
//  char debugbuf[128];
//   wsprintf(debugbuf,"MVCOMP1: nOutPitchUV=%d, nOverlap=%d, nBlkX=%d, nBlkSize=%d",nOutPitchUV, nOverlap, nBlkX, nBlkSize);
//   OutputDebugString(debugbuf);

   // write the plane's header
   WriteHeaderToArray(out);

   int *pBlkData = out + 1;

   if ( nLogScale == 0 )
   {
       if (divideExtra)
           out += out[0]; // skip space for extra divided subblocks
   }

   nFlags |= flags;

   pSrcFrame = _pSrcFrame;
   pRefFrame = _pRefFrame;

   x[0] = pSrcFrame->GetPlane(YPLANE)->GetHPadding();
   y[0] = pSrcFrame->GetPlane(YPLANE)->GetVPadding();
  if (chroma)
  {
   x[1] = pSrcFrame->GetPlane(UPLANE)->GetHPadding();
   x[2] = pSrcFrame->GetPlane(VPLANE)->GetHPadding();
   y[1] = pSrcFrame->GetPlane(UPLANE)->GetVPadding();
   y[2] = pSrcFrame->GetPlane(VPLANE)->GetVPadding();
  }

	blkx = 0;
	blky = 0;

#ifdef ALIGN_SOURCEBLOCK
	nSrcPitch_plane[0] = pSrcFrame->GetPlane(YPLANE)->GetPitch();
  if (chroma)
  {
	nSrcPitch_plane[1] = pSrcFrame->GetPlane(UPLANE)->GetPitch();
	nSrcPitch_plane[2] = pSrcFrame->GetPlane(VPLANE)->GetPitch();
  }
	nSrcPitch[0] = nBlkSizeX;
	nSrcPitch[1] = nBlkSizeX/2;
	nSrcPitch[2] = nBlkSizeX/2;
#else
	nSrcPitch[0] = pSrcFrame->GetPlane(YPLANE)->GetPitch();
  if (chroma)
  {
	nSrcPitch[1] = pSrcFrame->GetPlane(UPLANE)->GetPitch();
	nSrcPitch[2] = pSrcFrame->GetPlane(VPLANE)->GetPitch();
  }
#endif
   nRefPitch[0] = pRefFrame->GetPlane(YPLANE)->GetPitch();
  if (chroma)
  {
   nRefPitch[1] = pRefFrame->GetPlane(UPLANE)->GetPitch();
   nRefPitch[2] = pRefFrame->GetPlane(VPLANE)->GetPitch();
  }

   searchType = st;
	nSearchParam = stp;//*nPel; // v1.8.2 - redesigned in v1.8.5

   int nLambdaLevel = lambda / (nPel * nPel);
//   if (plevel==1)
//        nLambdaLevel = nLambdaLevel*nScale;// scale lambda - Fizick
//    else if (plevel==2)
//        nLambdaLevel = nLambdaLevel*nScale*nScale;

    // get old vectors plane
		const FakePlaneOfBlocks &plane = mvClip.GetPlane(0);
		int nBlkXold = plane.GetReducedWidth();
		int nBlkYold = plane.GetReducedHeight();
		int nBlkSizeXold = plane.GetBlockSizeX();
		int nBlkSizeYold = plane.GetBlockSizeY();
		int nOverlapXold = plane.GetOverlapX();
		int nOverlapYold = plane.GetOverlapY();
		int nStepXold = nBlkSizeXold - nOverlapXold;
		int nStepYold = nBlkSizeYold - nOverlapYold;

	// Functions using float must not be used here
	for ( blkIdx = 0; blkIdx < nBlkCount; blkIdx++ )
	{
//		DebugPrintf("BlkIdx = %d \n", blkIdx);
      PROFILE_START(MOTION_PROFILE_ME);

#ifdef ALIGN_SOURCEBLOCK
		//store the pitch
		pSrc[0] = pSrcFrame->GetPlane(YPLANE)->GetAbsolutePelPointer(x[0], y[0]);
		//create aligned copy
		BLITLUMA  (pSrc_temp[0],nSrcPitch[0],pSrc[0],nSrcPitch_plane[0]);
		//set the to the aligned copy
		pSrc[0] = pSrc_temp[0];
  if (chroma)
  {
		pSrc[1] = pSrcFrame->GetPlane(UPLANE)->GetAbsolutePelPointer(x[1], y[1]);
		pSrc[2] = pSrcFrame->GetPlane(VPLANE)->GetAbsolutePelPointer(x[2], y[2]);
		BLITCHROMA(pSrc_temp[1],nSrcPitch[1],pSrc[1],nSrcPitch_plane[1]);
		BLITCHROMA(pSrc_temp[2],nSrcPitch[2],pSrc[2],nSrcPitch_plane[2]);
		pSrc[1] = pSrc_temp[1];
		pSrc[2] = pSrc_temp[2];
  }
#else
		pSrc[0] = pSrcFrame->GetPlane(YPLANE)->GetAbsolutePelPointer(x[0], y[0]);
  if (chroma)
  {
		pSrc[1] = pSrcFrame->GetPlane(UPLANE)->GetAbsolutePelPointer(x[1], y[1]);
		pSrc[2] = pSrcFrame->GetPlane(VPLANE)->GetAbsolutePelPointer(x[2], y[2]);
  }
#endif

      if ( blky == 0 )
		  nLambda = 0;
      else
        nLambda = nLambdaLevel;

	  penaltyNew = pnew; // penalty for new vector
	  LSAD = lsad;    // SAD limit for lambda using
	  // may be they must be scaled by nPel ?

      /* computes search boundaries */
      nDxMax = nPel * (pSrcFrame->GetPlane(YPLANE)->GetExtendedWidth() - x[0] - nBlkSizeX);
      nDyMax = nPel * (pSrcFrame->GetPlane(YPLANE)->GetExtendedHeight()  - y[0] - nBlkSizeY);
	   nDxMin = -nPel * x[0];
		nDyMin = -nPel * y[0];

	// get and interplolate old vectors
		int centerX = nBlkSizeX/2 + (nBlkSizeX - nOverlapX)*blkx; // center of new block
		int blkxold = (centerX - nBlkSizeXold/2)/nStepXold; // centerXold less or equal to new
		int centerY = nBlkSizeY/2 + (nBlkSizeY - nOverlapY)*blky;
		int blkyold = (centerY - nBlkSizeYold/2)/nStepYold;

		int deltaX = max(0, centerX - (nBlkSizeXold/2 + nStepXold*blkxold)); // distance from old to new
		int deltaY = max(0, centerY - (nBlkSizeYold/2 + nStepYold*blkyold));

		int blkxold1 = min(nBlkXold-1, max(0, blkxold));
		int blkxold2 = min(nBlkXold-1, max(0, blkxold+1));
		int blkyold1 = min(nBlkYold-1, max(0, blkyold));
		int blkyold2 = min(nBlkYold-1, max(0, blkyold+1));

        VECTOR vectorOld; // interpolated or nearest

        if (smooth==1) // interpolate
        {

        VECTOR vectorOld1 = mvClip.GetBlock(0, blkxold1 + blkyold1*nBlkXold).GetMV(); // 4 old nearest vectors (may coinside)
        VECTOR vectorOld2 = mvClip.GetBlock(0, blkxold2 + blkyold1*nBlkXold).GetMV();
        VECTOR vectorOld3 = mvClip.GetBlock(0, blkxold1 + blkyold2*nBlkXold).GetMV();
        VECTOR vectorOld4 = mvClip.GetBlock(0, blkxold2 + blkyold2*nBlkXold).GetMV();

        // interpolate
		int vector1_x = vectorOld1.x*nStepXold + deltaX*(vectorOld2.x - vectorOld1.x); // scaled by nStepXold to skip slow division
		int vector1_y = vectorOld1.y*nStepXold + deltaX*(vectorOld2.y - vectorOld1.y);
		int vector1_sad = vectorOld1.sad*nStepXold + deltaX*(vectorOld2.sad - vectorOld1.sad);

		int vector2_x = vectorOld3.x*nStepXold + deltaX*(vectorOld4.x - vectorOld3.x);
		int vector2_y = vectorOld3.y*nStepXold + deltaX*(vectorOld4.y - vectorOld3.y);
		int vector2_sad = vectorOld3.sad*nStepXold + deltaX*(vectorOld4.sad - vectorOld3.sad);

		vectorOld.x = (vector1_x + deltaY*(vector2_x - vector1_x)/nStepYold)/nStepXold;
		vectorOld.y = (vector1_y + deltaY*(vector2_y - vector1_y)/nStepYold)/nStepXold;
		vectorOld.sad = (vector1_sad + deltaY*(vector2_sad - vector1_sad)/nStepYold)/nStepXold;

        }
        else // nearest
        {
            if (deltaX*2<nStepXold && deltaY*2<nStepYold )
                vectorOld = mvClip.GetBlock(0, blkxold1 + blkyold1*nBlkXold).GetMV();
            else if (deltaX*2>=nStepXold && deltaY*2<nStepYold )
                vectorOld = mvClip.GetBlock(0, blkxold2 + blkyold1*nBlkXold).GetMV();
            else if (deltaX*2<nStepXold && deltaY*2>=nStepYold )
                vectorOld = mvClip.GetBlock(0, blkxold1 + blkyold2*nBlkXold).GetMV();
            else //(deltaX*2>=nStepXold && deltaY*2>=nStepYold )
                vectorOld = mvClip.GetBlock(0, blkxold2 + blkyold2*nBlkXold).GetMV();
        }

        predictor = ClipMV(vectorOld); // predictor
        predictor.sad =  vectorOld.sad * (nBlkSizeX*nBlkSizeY)/(nBlkSizeXold*nBlkSizeYold); // normalized to new block size

//        bestMV = predictor; // by pointer?
        bestMV.x = predictor.x;
        bestMV.y = predictor.y;
        bestMV.sad = predictor.sad;

        // update SAD
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

                int saduv = (chroma) ? SADCHROMA(pSrc[1], nSrcPitch[1], GetRefBlockU(predictor.x, predictor.y), nRefPitch[1])
                 + SADCHROMA(pSrc[2], nSrcPitch[2], GetRefBlockV(predictor.x, predictor.y), nRefPitch[2]) : 0;
                int sad = LumaSAD(GetRefBlock(predictor.x, predictor.y));
                sad += saduv;
                bestMV.sad = sad;
                nMinCost = sad;


      if (bestMV.sad > thSAD)// if old interpolated vector is bad
      {
//          CheckMV(vectorOld1.x, vectorOld1.y);
//          CheckMV(vectorOld2.x, vectorOld2.y);
//          CheckMV(vectorOld3.x, vectorOld3.y);
//          CheckMV(vectorOld4.x, vectorOld4.y);
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
            {
        //       ExhaustiveSearch(nSearchParam);
                int mvx = bestMV.x;
                int mvy = bestMV.y;
                for ( int i = 1; i <= nSearchParam; i++ )// region is same as exhaustive, but ordered by radius (from near to far)
                    ExpandingSearch(i, 1, mvx, mvy);
            }

            if ( searchType & HEX2SEARCH )
                Hex2Search(nSearchParam);

            if ( searchType & UMHSEARCH )
                UMHSearch(nSearchParam, bestMV.x, bestMV.y);

	}

       // we store the result
        vectors[blkIdx].x = bestMV.x;
        vectors[blkIdx].y = bestMV.y;
        vectors[blkIdx].sad = bestMV.sad;


	  if (outfilebuf != NULL) // write vector to outfile
	  {
		  outfilebuf[0] = bestMV.x;
		  outfilebuf[1] = bestMV.y;
		  outfilebuf[2] = (bestMV.sad & 0x0000ffff); // low word
		  outfilebuf[3] = (bestMV.sad >> 16);     // high word, usually null
		  outfilebuf += 4;// 8 bytes per block
	  }

      /* write the results */
      pBlkData[0] = bestMV.x;
      pBlkData[1] = bestMV.y;
      pBlkData[2] = bestMV.sad;

      PROFILE_STOP(MOTION_PROFILE_ME);

      pBlkData += N_PER_BLOCK;

		if (smallestPlane)
			sumLumaChange += LUMA(GetRefBlock(0,0), nRefPitch[0]) - LUMA(pSrc[0], nSrcPitch[0]);

        /* increment indexes & pointers */
		blkx++;
      x[0] += (nBlkSizeX - nOverlapX);
      x[1] += ((nBlkSizeX - nOverlapX) /2);
      x[2] += ((nBlkSizeX - nOverlapX) /2);
 		if ( blkx == nBlkX )
		{
			blkx = 0;
         x[0] = pSrcFrame->GetPlane(YPLANE)->GetHPadding();
      if (chroma)
      {
         x[1] = pSrcFrame->GetPlane(UPLANE)->GetHPadding();
         x[2] = pSrcFrame->GetPlane(VPLANE)->GetHPadding();
      }
			blky++;
			y[0] += (nBlkSizeY - nOverlapY);
         y[1] += ((nBlkSizeY - nOverlapY)>>nLogyRatioUV );
         y[2] += ((nBlkSizeY - nOverlapY)>>nLogyRatioUV );
		}
	}
//	if (smallestPlane)
//		*pmeanLumaChange = sumLumaChange/nBlkCount; // for all finer planes

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

   for ( int l = 0, index = 0; l < nBlkY; l++ )
	{
		for ( int k = 0; k < nBlkX; k++, index++ )
		{
			VECTOR v1, v2, v3, v4;
			int i = k;
			int j = l;
			if ( i == 2 * pob.nBlkX ) i--;
			if ( j == 2 * pob.nBlkY ) j--;
			int offy = -1 + 2 * ( j % 2);
			int offx = -1 + 2 * ( i % 2);

			if (( i == 0 ) || (i == 2 * pob.nBlkX - 1))
			{
				if (( j == 0 ) || ( j == 2 * pob.nBlkY - 1))
				{
					v1 = v2 = v3 = v4 = pob.vectors[i / 2 + (j / 2) * pob.nBlkX];
				}
				else
				{
					v1 = v2 = pob.vectors[i / 2 + (j / 2) * pob.nBlkX];
					v3 = v4 = pob.vectors[i / 2 + (j / 2 + offy) * pob.nBlkX];
				}
			}
			else if (( j == 0 ) || ( j >= 2 * pob.nBlkY - 1))
			{
				v1 = v2 = pob.vectors[i / 2 + (j / 2) * pob.nBlkX];
				v3 = v4 = pob.vectors[i / 2 + offx + (j / 2) * pob.nBlkX];
			}
			else
			{
				v1 = pob.vectors[i / 2 + (j / 2) * pob.nBlkX];
				v2 = pob.vectors[i / 2 + offx + (j / 2) * pob.nBlkX];
				v3 = pob.vectors[i / 2 + (j / 2 + offy) * pob.nBlkX];
				v4 = pob.vectors[i / 2 + offx + (j / 2 + offy) * pob.nBlkX];
			}

			if (nOverlapX == 0 && nOverlapY == 0)
			{
				vectors[index].x = 9 * v1.x + 3 * v2.x + 3 * v3.x + v4.x;
				vectors[index].y = 9 * v1.y + 3 * v2.y + 3 * v3.y + v4.y;
				vectors[index].sad = 9 * v1.sad + 3 * v2.sad + 3 * v3.sad + v4.sad + 8;
			}
			else if (nOverlapX <= (nBlkSizeX>>1) && nOverlapY <= (nBlkSizeY>>1)) // corrected in v1.4.11
			{
				int	ax1 = (offx > 0) ? aoddx : aevenx;
				int ax2 = (nBlkSizeX - nOverlapX)*4 - ax1;
				int ay1 = (offy > 0) ? aoddy : aeveny;
				int ay2 = (nBlkSizeY - nOverlapY)*4 - ay1;
				vectors[index].x = (ax1*ay1*v1.x + ax2*ay1*v2.x + ax1*ay2*v3.x + ax2*ay2*v4.x) /normov;
				vectors[index].y = (ax1*ay1*v1.y + ax2*ay1*v2.y + ax1*ay2*v3.y + ax2*ay2*v4.y) /normov;
				vectors[index].sad = (ax1*ay1*v1.sad + ax2*ay1*v2.sad + ax1*ay2*v3.sad + ax2*ay2*v4.sad) /normov;
			}
			else // large overlap. Weights are not quite correct but let it be
			{
				vectors[index].x = (v1.x + v2.x + v3.x + v4.x) <<2;
				vectors[index].y = (v1.y + v2.y + v3.y + v4.y) <<2;
				vectors[index].sad = (v1.sad + v2.sad + v3.sad + v4.sad + 2) << 2;
			}

			vectors[index].x = (vectors[index].x >> normFactor) << mulFactor;
			vectors[index].y = (vectors[index].y >> normFactor) << mulFactor;
			vectors[index].sad = vectors[index].sad >> 4;
		}
	}
}

void PlaneOfBlocks::WriteHeaderToArray(int *array)
{
    array[0] = nBlkCount * N_PER_BLOCK + 1;
}

int PlaneOfBlocks::WriteDefaultToArray(int *array, int divideMode)
{
   array[0] = nBlkCount * N_PER_BLOCK + 1;
   int verybigSAD = nBlkSizeX*nBlkSizeY*256;
	for (int i=0; i<nBlkCount*N_PER_BLOCK; i+=N_PER_BLOCK)
	{
	    array[i+1] = 0;
	    array[i+2] = 0;
	    array[i+3] = verybigSAD;
	}

   if ( nLogScale == 0 )
   {
      array += array[0];
      if (divideMode) { // reserve space for divided subblocks extra level
            array[0] = nBlkCount * N_PER_BLOCK * 4 + 1; // 4 subblocks
            for (int i=0; i<nBlkCount*4*N_PER_BLOCK; i+=N_PER_BLOCK)
            {
                array[i+1] = 0;
                array[i+2] = 0;
                array[i+3] = verybigSAD;
            }
            array += array[0];
      }
   }
   return GetArraySize(divideMode);
}

int PlaneOfBlocks::GetArraySize(int divideMode)
{
	int size = 0;
	size += 1;              // mb data size storage
	size += nBlkCount * N_PER_BLOCK;  // vectors, sad, luma src, luma ref, var

   if ( nLogScale == 0)
   {
        if (divideMode)
            size += 1 + nBlkCount * N_PER_BLOCK * 4; // reserve space for divided subblocks extra level
   }

   return size;
}

void PlaneOfBlocks::FetchPredictors()
{
	// Left predictor
	if ( blkx > 0 ) predictors[1] = ClipMV(vectors[blkIdx - 1]);
	else predictors[1] = ClipMV(zeroMVfieldShifted); // v1.11.1 - values instead of pointer

	// Up predictor
	if ( blky > 0 ) predictors[2] = ClipMV(vectors[blkIdx - nBlkX]);
	else predictors[2] = ClipMV(zeroMVfieldShifted);

	// Up-right predictor
	if (( blky > 0 ) && ( blkx < nBlkX - 1 ))
		predictors[3] = ClipMV(vectors[blkIdx - nBlkX + 1]);
	else predictors[3] = ClipMV(zeroMVfieldShifted);

	// Median predictor
	if ( blky > 0 ) // replaced 1 by 0 - Fizick
	{
		predictors[0].x = Median(predictors[1].x, predictors[2].x, predictors[3].x);
		predictors[0].y = Median(predictors[1].y, predictors[2].y, predictors[3].y);
//      predictors[0].sad = Median(predictors[1].sad, predictors[2].sad, predictors[3].sad);
	  // but it is not true median vector (x and y may be mixed) and not its sad ?!
		// we really do not know SAD, here is more safe estimation especially for phaseshift method - v1.6.0
		predictors[0].sad = max(predictors[1].sad, max(predictors[2].sad, predictors[3].sad));
	}
	else {
//		predictors[0].x = (predictors[1].x + predictors[2].x + predictors[3].x);
//		predictors[0].y = (predictors[1].y + predictors[2].y + predictors[3].y);
//      predictors[0].sad = (predictors[1].sad + predictors[2].sad + predictors[3].sad);
		// but for top line we have only left predictor[1] - v1.6.0
		predictors[0].x = predictors[1].x;
		predictors[0].y = predictors[1].y;
	    predictors[0].sad = predictors[1].sad;
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
//	if ( predictor.sad > LSAD ) nLambda = 0; // generalized (was LSAD=400) by Fizick
	nLambda = nLambda*LSAD/(LSAD + (predictor.sad>>1))*LSAD/(LSAD + (predictor.sad>>1));
	// replaced hard threshold by soft in v1.10.2 by Fizick (a liitle complex expression to avoid overflow)
}

void PlaneOfBlocks::PseudoEPZSearch()
{

	FetchPredictors();

	int sad;
	int saduv;
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

	// We treat zero alone
	// Do we bias zero with not taking into account distorsion ?
	   bestMV.x = zeroMVfieldShifted.x;
	   bestMV.y = zeroMVfieldShifted.y;
		saduv = (chroma) ? SADCHROMA(pSrc[1], nSrcPitch[1], GetRefBlockU(0, 0), nRefPitch[1])
		  + SADCHROMA(pSrc[2], nSrcPitch[2], GetRefBlockV(0, 0), nRefPitch[2]) : 0;
		sad = LumaSAD(GetRefBlock(0, zeroMVfieldShifted.y));
		sad += saduv;
		bestMV.sad = sad;
		nMinCost = sad + ((penaltyZero*sad)>>8); // v.1.11.0.2

//        if (sad>LSAD/4) DebugPrintf("zero %d %d %d %d %d %d %d", blkIdx, zeroMVfieldShifted.x, zeroMVfieldShifted.y, 1000, nMinCost, nMinCost, sad);

	// Global MV predictor  - added by Fizick
	if ( IsVectorOK(globalMVPredictor.x, globalMVPredictor.y ) )
	{
        saduv = (chroma) ? SADCHROMA(pSrc[1], nSrcPitch[1], GetRefBlockU(globalMVPredictor.x, globalMVPredictor.y), nRefPitch[1])
         + SADCHROMA(pSrc[2], nSrcPitch[2], GetRefBlockV(globalMVPredictor.x, globalMVPredictor.y), nRefPitch[2]) : 0;
		sad = LumaSAD(GetRefBlock(globalMVPredictor.x, globalMVPredictor.y));
		sad += saduv;
		int cost = sad + ((pglobal*sad)>>8);

//        if (sad>LSAD/4) DebugPrintf("global %d %d %d %d %d %d %d", blkIdx, globalMVPredictor.x, globalMVPredictor.y, 2000, nMinCost, cost, sad);

		if ( cost  < nMinCost )
		{
			bestMV.x = globalMVPredictor.x;
			bestMV.y = globalMVPredictor.y;
			 bestMV.sad = sad;
			nMinCost = cost;
		}
	}

	// Then, the predictor :
//	if (
//	(( predictor.x != zeroMVfieldShifted.x ) || ( predictor.y != zeroMVfieldShifted.y )) &&
//		 (( predictor.x != globalMVPredictor.x ) || ( predictor.y != globalMVPredictor.y )) &&
	{
        saduv = (chroma) ? SADCHROMA(pSrc[1], nSrcPitch[1], GetRefBlockU(predictor.x, predictor.y), nRefPitch[1])
         + SADCHROMA(pSrc[2], nSrcPitch[2], GetRefBlockV(predictor.x, predictor.y), nRefPitch[2]) : 0;
		sad = LumaSAD(GetRefBlock(predictor.x, predictor.y));
		sad += saduv;
		int cost = sad;
//        if (sad>LSAD/4) DebugPrintf("predictor %d %d %d %d %d %d %d", blkIdx, predictor.x, predictor.y, 3000, nMinCost, cost, sad);
		if ( cost  < nMinCost )
		{
			bestMV.x = predictor.x;
			bestMV.y = predictor.y;
         bestMV.sad = sad;
			nMinCost = cost;
		}
	}

   // then all the other predictors
	for ( int i = 0; i < 4; i++ )
	{
		CheckMV0(predictors[i].x, predictors[i].y);
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
	{
//       ExhaustiveSearch(nSearchParam);
        int mvx = bestMV.x;
        int mvy = bestMV.y;
		for ( int i = 1; i <= nSearchParam; i++ )// region is same as enhausted, but ordered by radius (from near to far)
			ExpandingSearch(i, 1, mvx, mvy);
	}

//	if ( searchType & SQUARE )
//		SquareSearch();

	if ( searchType & HEX2SEARCH )
		Hex2Search(nSearchParam);

	if ( searchType & UMHSEARCH )
		UMHSearch(nSearchParam, bestMV.x, bestMV.y);


    int foundSAD = bestMV.sad;

#define BADCOUNT_LIMIT 16

   if (blkIdx>1 && foundSAD > (badSAD +  badSAD*badcount/BADCOUNT_LIMIT)) // bad vector, try wide search
   {// with some soft limit (BADCOUNT_LIMIT) of bad cured vectors (time consumed)
       badcount++;

        DebugPrintf("bad  blk=%d x=%d y=%d sad=%d mean=%d iter=%d", blkIdx, bestMV.x, bestMV.y, bestMV.sad, planeSAD/blkIdx, iter);
/*
        int mvx0 = bestMV.x; // store for comparing
        int mvy0 = bestMV.y;
        int msad0 = bestMV.sad;
        int mcost0 = nMinCost;
*/
		if (badrange > 0) // UMH
		{

//			UMHSearch(badrange*nPel, bestMV.x, bestMV.y);

//			if (bestMV.sad > foundSAD/2)
			{
                // rathe good is not found, lets try around zero
//            UMHSearch(badSADRadius, abs(mvx0)%4 - 2, abs(mvy0)%4 - 2);
				UMHSearch(badrange*nPel, 0, 0);
			}

		}
		else if (badrange<0) // ESA
		{

/*        bestMV.x = mvx0; // restore  for comparing
        bestMV.y = mvy0;
        bestMV.sad = msad0;
        nMinCost = mcost0;

        int mvx = bestMV.x; // store to not move the search center!
        int mvy = bestMV.y;
        int msad = bestMV.sad;

		for ( int i = 1; i < -badrange*nPel; i+=nPel )// at radius
		{
			ExpandingSearch(i, nPel, mvx, mvy);
            if (bestMV.sad < foundSAD/4) break; // stop search
		}

		if (bestMV.sad > foundSAD/2 && abs(mvx)+abs(mvy) > badSADRadius/2)
		{
                // rathe good is not found, lets try around zero
            mvx = 0; // store to not move the search center!
            mvy = 0;
*/
			for ( int i = 1; i < -badrange*nPel; i+=nPel )// at radius
            {
                ExpandingSearch(i, nPel, 0, 0);
                if (bestMV.sad < foundSAD/4) break; // stop search if rathe good is found
            }
		}

        int mvx = bestMV.x; // refine in small area
        int mvy = bestMV.y;
		for ( int i = 1; i < nPel; i++ )// small radius
		{
			ExpandingSearch(i, 1, mvx, mvy);
		}
    DebugPrintf("best blk=%d x=%d y=%d sad=%d iter=%d", blkIdx, bestMV.x, bestMV.y, bestMV.sad, iter);
   }


   // we store the result
	vectors[blkIdx].x = bestMV.x;
	vectors[blkIdx].y = bestMV.y;
	vectors[blkIdx].sad = bestMV.sad;

	planeSAD += bestMV.sad;

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

	while ( direction > 0 )
	{
		dx = bestMV.x;
		dy = bestMV.y;
		lastDirection = direction;
		direction = 0;

		// First, we look the directions that were hinted by the previous step
		// of the algorithm. If we find one, we add it to the set of directions
		// we'll test next
		if ( lastDirection & 1 ) CheckMV2(dx + length, dy, &direction, 1);
		if ( lastDirection & 2 ) CheckMV2(dx - length, dy, &direction, 2);
		if ( lastDirection & 4 ) CheckMV2(dx, dy + length, &direction, 4);
		if ( lastDirection & 8 ) CheckMV2(dx, dy - length, &direction, 8);

		// If one of the directions improves the SAD, we make further tests
		// on the diagonals
		if ( direction ) {
			lastDirection = direction;
			dx = bestMV.x;
			dy = bestMV.y;

			if ( lastDirection & 3 )
			{
				CheckMV2(dx, dy + length, &direction, 4);
				CheckMV2(dx, dy - length, &direction, 8);
			}
			else {
				CheckMV2(dx + length, dy, &direction, 1);
				CheckMV2(dx - length, dy, &direction, 2);
			}
		}

		// If not, we do not stop here. We infer from the last direction the
		// diagonals to be checked, because we might be lucky.
		else {
			switch ( lastDirection ) {
				case 1 :
					CheckMV2(dx + length, dy + length, &direction, 1 + 4);
					CheckMV2(dx + length, dy - length, &direction, 1 + 8);
					break;
				case 2 :
					CheckMV2(dx - length, dy + length, &direction, 2 + 4);
					CheckMV2(dx - length, dy - length, &direction, 2 + 8);
					break;
				case 4 :
					CheckMV2(dx + length, dy + length, &direction, 1 + 4);
					CheckMV2(dx - length, dy + length, &direction, 2 + 4);
					break;
				case 8 :
					CheckMV2(dx + length, dy - length, &direction, 1 + 8);
					CheckMV2(dx - length, dy - length, &direction, 2 + 8);
					break;
				case 1 + 4 :
					CheckMV2(dx + length, dy + length, &direction, 1 + 4);
					CheckMV2(dx - length, dy + length, &direction, 2 + 4);
					CheckMV2(dx + length, dy - length, &direction, 1 + 8);
					break;
				case 2 + 4 :
					CheckMV2(dx + length, dy + length, &direction, 1 + 4);
					CheckMV2(dx - length, dy + length, &direction, 2 + 4);
					CheckMV2(dx - length, dy - length, &direction, 2 + 8);
					break;
				case 1 + 8 :
					CheckMV2(dx + length, dy + length, &direction, 1 + 4);
					CheckMV2(dx - length, dy - length, &direction, 2 + 8);
					CheckMV2(dx + length, dy - length, &direction, 1 + 8);
					break;
				case 2 + 8 :
					CheckMV2(dx - length, dy - length, &direction, 2 + 8);
					CheckMV2(dx - length, dy + length, &direction, 2 + 4);
					CheckMV2(dx + length, dy - length, &direction, 1 + 8);
					break;
				default :
					// Even the default case may happen, in the first step of the
					// algorithm for example.
					CheckMV2(dx + length, dy + length, &direction, 1 + 4);
					CheckMV2(dx - length, dy + length, &direction, 2 + 4);
					CheckMV2(dx + length, dy - length, &direction, 1 + 8);
					CheckMV2(dx - length, dy - length, &direction, 2 + 8);
					break;
			}
		}
	}
}
/*
void PlaneOfBlocks::SquareSearch()
{
	ExhaustiveSearch(1);
}

void PlaneOfBlocks::ExhaustiveSearch(int s)// diameter = 2*s - 1
{
	int i, j;
	VECTOR mv = bestMV;

	for ( i = -s + 1; i < 0; i++ )
		for ( j = -s + 1; j < s; j++ )
			CheckMV(mv.x + i, mv.y + j);

	for ( i = 1; i < s; i++ )
		for ( j = -s + 1; j < s; j++ )
			CheckMV(mv.x + i, mv.y + j);

	for ( j = -s + 1; j < 0; j++ )
		CheckMV(mv.x, mv.y + j);

	for ( j = 1; j < s; j++ )
		CheckMV(mv.x, mv.y + j);

}
*/
void PlaneOfBlocks::NStepSearch(int stp)
{
	int dx, dy;
	int length = stp;
	while ( length > 0 )
	{
		dx = bestMV.x;
		dy = bestMV.y;

		CheckMV(dx + length, dy + length);
		CheckMV(dx + length, dy);
		CheckMV(dx + length, dy - length);
		CheckMV(dx, dy - length);
		CheckMV(dx, dy + length);
		CheckMV(dx - length, dy + length);
		CheckMV(dx - length, dy);
		CheckMV(dx - length, dy - length);

		length--;
	}
}

void PlaneOfBlocks::OneTimeSearch(int length)
{
	int direction = 0;
	int dx = bestMV.x;
	int dy = bestMV.y;

	CheckMV2(dx - length, dy, &direction, 2);
	CheckMV2(dx + length, dy, &direction, 1);

	if ( direction == 1 )
	{
		while ( direction )
		{
			direction = 0;
			dx += length;
			CheckMV2(dx + length, dy, &direction, 1);
		}
	}
	else if ( direction == 2 )
	{
		while ( direction )
		{
			direction = 0;
			dx -= length;
			CheckMV2(dx - length, dy, &direction, 1);
		}
	}

	CheckMV2(dx, dy - length, &direction, 2);
	CheckMV2(dx, dy + length, &direction, 1);

	if ( direction == 1 )
	{
		while ( direction )
		{
			direction = 0;
			dy += length;
			CheckMV2(dx, dy + length, &direction, 1);
		}
	}
	else if ( direction == 2 )
	{
		while ( direction )
		{
			direction = 0;
			dy -= length;
			CheckMV2(dx, dy - length, &direction, 1);
		}
	}
}

void PlaneOfBlocks::ExpandingSearch(int r, int s, int mvx, int mvy) // diameter = 2*r + 1, step=s
{ // part of true enhaustive search (thin expanding square) around mvx, mvy
	int i, j;
//	VECTOR mv = bestMV; // bug: it was pointer assignent, not values, so iterative! - v2.1

// sides of square without corners
	for ( i = -r+s; i < r; i+=s ) // without corners! - v2.1
	{
        CheckMV(mvx + i, mvy - r);
        CheckMV(mvx + i, mvy + r);
	}

	for ( j = -r+s; j < r; j+=s )
	{
		CheckMV(mvx - r, mvy + j);
		CheckMV(mvx + r, mvy + j);
	}

// then corners - they are more far from cenrer
		CheckMV(mvx - r, mvy - r);
		CheckMV(mvx - r, mvy + r);
		CheckMV(mvx + r, mvy - r);
		CheckMV(mvx + r, mvy + r);
}

/* (x-1)%6 */
static const int mod6m1[8] = {5,0,1,2,3,4,5,0};
/* radius 2 hexagon. repeated entries are to avoid having to compute mod6 every time. */
static const int hex2[8][2] = {{-1,-2}, {-2,0}, {-1,2}, {1,2}, {2,0}, {1,-2}, {-1,-2}, {-2,0}};

void PlaneOfBlocks::Hex2Search(int i_me_range)
{ //adopted from x264
	int dir = -2;
	int bmx = bestMV.x;
	int bmy = bestMV.y;

    if (i_me_range > 1)
    {
        /* hexagon */
//        COST_MV_X3_DIR( -2,0, -1, 2,  1, 2, costs   );
//        COST_MV_X3_DIR(  2,0,  1,-2, -1,-2, costs+3 );
//        COPY2_IF_LT( bcost, costs[0], dir, 0 );
//        COPY2_IF_LT( bcost, costs[1], dir, 1 );
//        COPY2_IF_LT( bcost, costs[2], dir, 2 );
//        COPY2_IF_LT( bcost, costs[3], dir, 3 );
//        COPY2_IF_LT( bcost, costs[4], dir, 4 );
//        COPY2_IF_LT( bcost, costs[5], dir, 5 );
        CheckMVdir(bmx-2, bmy, &dir, 0);
        CheckMVdir(bmx-1, bmy+2, &dir, 1);
        CheckMVdir(bmx+1, bmy+2, &dir, 2);
        CheckMVdir(bmx+2, bmy, &dir, 3);
        CheckMVdir(bmx+1, bmy-2, &dir, 4);
        CheckMVdir(bmx-1, bmy-2, &dir, 5);


        if( dir != -2 )
        {
            bmx += hex2[dir+1][0];
            bmy += hex2[dir+1][1];
            /* half hexagon, not overlapping the previous iteration */
            for( int i = 1; i < i_me_range/2 && IsVectorOK(bmx, bmy); i++ )
            {
                const int odir = mod6m1[dir+1];
//                COST_MV_X3_DIR( hex2[odir+0][0], hex2[odir+0][1],
//                                hex2[odir+1][0], hex2[odir+1][1],
//                                hex2[odir+2][0], hex2[odir+2][1],
//                                costs );

                dir = -2;
//                COPY2_IF_LT( bcost, costs[0], dir, odir-1 );
//                COPY2_IF_LT( bcost, costs[1], dir, odir   );
//                COPY2_IF_LT( bcost, costs[2], dir, odir+1 );

                CheckMVdir(bmx + hex2[odir+0][0], bmy + hex2[odir+0][1], &dir, odir-1);
                CheckMVdir(bmx + hex2[odir+1][0], bmy + hex2[odir+1][1], &dir, odir);
                CheckMVdir(bmx + hex2[odir+2][0], bmy + hex2[odir+2][1], &dir, odir+1);
                if( dir == -2 )
                    break;
                bmx += hex2[dir+1][0];
                bmy += hex2[dir+1][1];
            }
        }

        bestMV.x = bmx;
        bestMV.y = bmy;
    }
        /* square refine */
//        omx = bmx; omy = bmy;
//        COST_MV_X4(  0,-1,  0,1, -1,0, 1,0 );
//        COST_MV_X4( -1,-1, -1,1, 1,-1, 1,1 );
        ExpandingSearch(1, 1, bmx, bmy);

}


void PlaneOfBlocks::CrossSearch(int start, int x_max, int y_max, int mvx, int mvy)
{ // part of umh  search

	for ( int i = start; i < x_max; i+=2 )
	{
        CheckMV(mvx - i, mvy);
        CheckMV(mvx + i, mvy);
	}

	for ( int j = start; j < y_max; j+=2 )
	{
		CheckMV(mvx, mvy + j);
		CheckMV(mvx, mvy + j);
	}
}


void PlaneOfBlocks::UMHSearch(int i_me_range, int omx, int omy) // radius
{
// Uneven-cross Multi-Hexagon-grid Search (see x264)
            /* hexagon grid */

//            int omx = bestMV.x;
//            int omy = bestMV.y;
            // my mod: do not shift the center after Cross
            CrossSearch(1, i_me_range, i_me_range,  omx,  omy);


            int i = 1;
            do
            {
                static const int hex4[16][2] = {
                    {-4, 2}, {-4, 1}, {-4, 0}, {-4,-1}, {-4,-2},
                    { 4,-2}, { 4,-1}, { 4, 0}, { 4, 1}, { 4, 2},
                    { 2, 3}, { 0, 4}, {-2, 3},
                    {-2,-3}, { 0,-4}, { 2,-3},
                };

                    for( int j = 0; j < 16; j++ )
                    {
                        int mx = omx + hex4[j][0]*i;
                        int my = omy + hex4[j][1]*i;
                            CheckMV( mx, my );
                    }
            } while( ++i <= i_me_range/4 );

//            if( bmy <= mv_y_max )
//                goto me_hex2;
                Hex2Search(i_me_range);

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
    for (int i=0; i<nBlkCount; i++)
    {
        int ind = (freqSize>>1)+vectors[i].x;
        if (ind>=0 && ind<freqSize)
        {
            freqArray[ind] += 1 ;
            if (ind > indmax)
                indmax = ind;
            if (ind < indmin)
                indmin = ind;
        }
    }
    int count = freqArray[indmin];
    int index = indmin;
    for (int i=indmin+1; i<=indmax; i++)
    {
        if (freqArray[i] > count)
        {
            count = freqArray[i];
            index = i;
         }
    }
    int medianx = (index-(freqSize>>1)); // most frequent value

// find most frequent y
    memset(&freqArray[0], 0, freqSize*sizeof(int)); // reset
    indmin = freqSize-1;
    indmax = 0;
   for (int i=0; i<nBlkCount; i++)
    {
        int ind = (freqSize>>1)+vectors[i].y;
        if (ind>=0 && ind<freqSize)
        {
            freqArray[ind] += 1 ;
            if (ind > indmax)
                indmax = ind;
            if (ind < indmin)
                indmin = ind;
        }
    }
    count = freqArray[indmin];
    index = indmin;
    for (int i=indmin+1; i<=indmax; i++)
    {
        if (freqArray[i] > count)
        {
            count = freqArray[i];
            index = i;
         }
    }
    int mediany = (index-(freqSize>>1)); // most frequent value


    // iteration to increase precision
	int meanvx = 0;
	int meanvy = 0;
	int num = 0;
	for ( int i=0; i < nBlkCount; i++ )
	{
		if (abs(vectors[i].x - medianx) < 6 && abs(vectors[i].y - mediany) < 6 )
        {
            meanvx += vectors[i].x;
            meanvy += vectors[i].y;
            num += 1;
        }
	}

   // output vectors must be doubled for next (finer) scale level
	if (num >0)
	{
	globalMVec->x = 2*meanvx / num;
	globalMVec->y = 2*meanvy / num;
	}
	else
	{
	globalMVec->x = 2*medianx;
	globalMVec->y = 2*mediany;
	}

// char debugbuf[100];
// sprintf(debugbuf,"MVAnalyse: nx=%d ny=%d next global vx=%d vy=%d", nBlkX, nBlkY, globalMVec->x, globalMVec->y);
// OutputDebugString(debugbuf);

}
