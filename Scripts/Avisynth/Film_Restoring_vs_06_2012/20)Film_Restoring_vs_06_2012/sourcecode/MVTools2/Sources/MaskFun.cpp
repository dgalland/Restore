// Create an overlay mask with the motion vectors
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

#include "MaskFun.h"
#include <math.h>
#include <memory.h>
#include "CopyCode.h"

inline void ByteOccMask(BYTE *occMask, int occlusion, double occnorm, double fGamma)
{
	if (fGamma == 1.0)
		*occMask = max(*occMask, min((int)(255 * occlusion*occnorm),255));
	else
		*occMask= max(*occMask, min((int)(255 * pow(occlusion*occnorm, fGamma) ), 255));
}

void MakeVectorOcclusionMaskTime(MVClip &mvClip, int nBlkX, int nBlkY, double dMaskNormFactor, double fGamma, int nPel, BYTE * occMask, int occMaskPitch, int time256, int blkSizeX, int blkSizeY)
{ // analyse vectors field to detect occlusion
	MemZoneSet(occMask, 0, nBlkX, nBlkY, 0, 0, nBlkX);
	int time4096X = time256*16/blkSizeX;
	int time4096Y = time256*16/blkSizeY;
	_asm emms;
	double occnorm = 10 / dMaskNormFactor/nPel;
	  int occlusion;

			  for (int by=0; by<nBlkY; by++)
			  {
				  for (int bx=0; bx<nBlkX; bx++)
				  {
					  int i = bx + by*nBlkX; // current block
						const FakeBlockData &block = mvClip.GetBlock(0, i);
						int vx = block.GetMV().x;
						int vy = block.GetMV().y;
						if (bx < nBlkX-1) // right neighbor
						{
							int i1 = i+1;
							const FakeBlockData &block1 = mvClip.GetBlock(0, i1);
							int vx1 = block1.GetMV().x;
							//int vy1 = block1.GetMV().y;
							if (vx1<vx) {
							    occlusion = vx-vx1;
                                for (int bxi=bx+vx1*time4096X/4096; bxi<=bx+vx*time4096X/4096+1 && bxi>=0 && bxi<nBlkX; bxi++)
                                    ByteOccMask(&occMask[bxi+by*occMaskPitch], occlusion, occnorm, fGamma);
							}
						}
						if (by < nBlkY-1) // bottom neighbor
						{
							int i1 = i + nBlkX;
							const FakeBlockData &block1 = mvClip.GetBlock(0, i1);
							//int vx1 = block1.GetMV().x;
							int vy1 = block1.GetMV().y;
							if (vy1<vy) {
							    occlusion = vy-vy1;
                                for (int byi=by+vy1*time4096Y/4096; byi<=by+vy*time4096Y/4096+1 && byi>=0 && byi<nBlkY; byi++)
                                     ByteOccMask(&occMask[bx+byi*occMaskPitch], occlusion, occnorm, fGamma);
							}
						}
				  }
			  }
}

void VectorMasksToOcclusionMaskTime(BYTE *VXMask, BYTE *VYMask, int nBlkX, int nBlkY, double dMaskNormFactor, double fGamma, int nPel, BYTE * occMask, int occMaskPitch, int time256, int blkSizeX, int blkSizeY)
{ // analyse vectors field to detect occlusion
	MemZoneSet(occMask, 0, nBlkX, nBlkY, 0, 0, nBlkX);
	int time4096X = time256*16/blkSizeX;
	int time4096Y = time256*16/blkSizeY;
	_asm emms;
	double occnorm = 10 / dMaskNormFactor/nPel;
	  int occlusion;
			  for (int by=0; by<nBlkY; by++)
			  {
				  for (int bx=0; bx<nBlkX; bx++)
				  {
					  int i = bx + by*nBlkX; // current block
						int vx = VXMask[i]-128;
						int vy = VYMask[i]-128;
						int bxv = bx+vx*time4096X/4096;
						int byv = by+vy*time4096Y/4096;
						if (bx < nBlkX-1) // right neighbor
						{
							int i1 = i+1;
							int vx1 = VXMask[i1]-128;
							if (vx1<vx) {
							    occlusion = vx-vx1;
                                for (int bxi=bx+vx1*time4096X/4096; bxi<=bx+vx*time4096X/4096+1 && bxi>=0 && bxi<nBlkX; bxi++)
                                    ByteOccMask(&occMask[bxi+byv*occMaskPitch], occlusion, occnorm, fGamma);
							}
						}
						if (by < nBlkY-1) // bottom neighbor
						{
							int i1 = i + nBlkX;
							int vy1 = VYMask[i1]-128;
							if (vy1<vy) {
							    occlusion = vy-vy1;
                                for (int byi=by+vy1*time4096Y/4096; byi<=by+vy*time4096Y/4096+1 && byi>=0 && byi<nBlkY; byi++)
                                     ByteOccMask(&occMask[bxv+byi*occMaskPitch], occlusion, occnorm, fGamma);
							}
						}
				  }
			  }
}



// it is old pre 1.8 version
void MakeVectorOcclusionMask(MVClip &mvClip, int nBlkX, int nBlkY, double dMaskNormFactor, double fGamma, int nPel, BYTE * occMask, int occMaskPitch)
{ // analyse vectors field to detect occlusion
	_asm emms;
	double occnorm = 10 / dMaskNormFactor/nPel; // empirical
			  for (int by=0; by<nBlkY; by++)
			  {
				  for (int bx=0; bx<nBlkX; bx++)
				  {
					  int occlusion = 0;
					  int i = bx + by*nBlkX; // current block
						const FakeBlockData &block = mvClip.GetBlock(0, i);
						int vx = block.GetMV().x;
						int vy = block.GetMV().y;
						if (bx > 0) // left neighbor
						{
							int i1 = i-1;
							const FakeBlockData &block1 = mvClip.GetBlock(0, i1);
							int vx1 = block1.GetMV().x;
							//int vy1 = block1.GetMV().y;
							if (vx1>vx) occlusion += (vx1-vx); // only positive (abs)
						}
						if (bx < nBlkX-1) // right neighbor
						{
							int i1 = i+1;
							const FakeBlockData &block1 = mvClip.GetBlock(0, i1);
							int vx1 = block1.GetMV().x;
							//int vy1 = block1.GetMV().y;
							if (vx1<vx) occlusion += vx-vx1;
						}
						if (by > 0) // top neighbor
						{
							int i1 = i - nBlkX;
							const FakeBlockData &block1 = mvClip.GetBlock(0, i1);
							//int vx1 = block1.GetMV().x;
							int vy1 = block1.GetMV().y;
							if (vy1>vy) occlusion += vy1-vy;
						}
						if (by < nBlkY-1) // bottom neighbor
						{
							int i1 = i + nBlkX;
							const FakeBlockData &block1 = mvClip.GetBlock(0, i1);
							//int vx1 = block1.GetMV().x;
							int vy1 = block1.GetMV().y;
							if (vy1<vy) occlusion += vy-vy1;
						}
						if (fGamma == 1.0)
							occMask[bx + by*occMaskPitch] = min((int)(255 * occlusion*occnorm),255);
						else
							occMask[bx + by*occMaskPitch]= min((int)(255 * pow(occlusion*occnorm, fGamma) ), 255);
				  }
			  }
}


// it is olr pre 1.8 mask
void VectorMasksToOcclusionMask(BYTE *VXMask, BYTE *VYMask, int nBlkX, int nBlkY, double dMaskNormFactor, double fGamma, int nPel, BYTE * occMask)
{ // analyse vectors field to detect occlusion
	// note: dMaskNormFactor was = 1/ml, now is = ml
	_asm emms;
	double occnorm = 10 / dMaskNormFactor/nPel; // empirical
			  for (int by=0; by<nBlkY; by++)
			  {
				  for (int bx=0; bx<nBlkX; bx++)
				  {
					  int occlusion = 0;
					  int i = bx + by*nBlkX; // current block
						int vx = VXMask[i];
						int vy = VYMask[i];
						if (bx > 0) // left neighbor
						{
							int i1 = i-1;
							int vx1 = VXMask[i1];
//							int vy1 = VYMask[i1];
							if (vx1>vx) occlusion += (vx1-vx); // only positive (abs)
						}
						if (bx < nBlkX-1) // right neighbor
						{
							int i1 = i+1;
							int vx1 = VXMask[i1];
//							int vy1 = VYMask[i1];
							if (vx1<vx) occlusion += vx-vx1;
						}
						if (by > 0) // top neighbor
						{
							int i1 = i - nBlkX;
//							int vx1 = VXMask[i1];
							int vy1 = VYMask[i1];
							if (vy1>vy) occlusion += vy1-vy;
						}
						if (by < nBlkY-1) // bottom neighbor
						{
							int i1 = i + nBlkX;
//							int vx1 = VXMask[i1];
							int vy1 = VYMask[i1];
							if (vy1<vy) occlusion += vy-vy1;
						}
						if (fGamma == 1.0)
							occMask[i] = min((int)(255 * occlusion*occnorm),255);
						else
							occMask[i]= min((int)(255 * pow(occlusion*occnorm, fGamma) ), 255);
				  }
			  }
}

void MakeVectorSmallMasks(MVClip &mvClip, int nBlkX, int nBlkY, BYTE *VXSmallY, int pitchVXSmallY, BYTE*VYSmallY, int pitchVYSmallY)
{
	  // make  vector vx and vy small masks
	 // 1. ATTENTION: vectors are assumed SHORT (|vx|, |vy| < 127) !
	 // 2. they will be zeroed if not
	// 3. added 128 to all values
	  for (int by=0; by<nBlkY; by++)
	  {
		  for (int bx=0; bx<nBlkX; bx++)
		  {
			  int i = bx + by*nBlkX;
				const FakeBlockData &block = mvClip.GetBlock(0, i);
				int vx = block.GetMV().x;
				int vy = block.GetMV().y;
				if (vx>127) vx= 127;
				else if (vx<-127) vx= -127;
				if (vy>127) vy = 127;
				else if (vy<-127) vy = -127;
				VXSmallY[bx+by*pitchVXSmallY] = vx + 128; // luma
				VYSmallY[bx+by*pitchVYSmallY] = vy + 128; // luma
		  }
	  }
}

void VectorSmallMaskYToHalfUV(BYTE * VSmallY, int nBlkX, int nBlkY, BYTE *VSmallUV, int ratioUV)
{
	if (ratioUV==2)
	{
	// YV12 colorformat
	  for (int by=0; by<nBlkY; by++)
	  {
		  for (int bx=0; bx<nBlkX; bx++)
		  {
				VSmallUV[bx] = ((VSmallY[bx]-128)>>1) + 128; // chroma
		  }
		  VSmallY += nBlkX;
		  VSmallUV += nBlkX;
	  }
	}
	else // ratioUV==1
	{
	// Height YUY2 colorformat
	  for (int by=0; by<nBlkY; by++)
	  {
		  for (int bx=0; bx<nBlkX; bx++)
		  {
				VSmallUV[bx] = VSmallY[bx]; // chroma
		  }
		  VSmallY += nBlkX;
		  VSmallUV += nBlkX;
	  }
	}

}


void Merge4PlanesToBig(BYTE *pel2Plane, int pel2Pitch, const BYTE *pPlane0, const BYTE *pPlane1,
					  const BYTE *pPlane2, const BYTE * pPlane3, int width, int height, int pitch, bool isse)
{
	// copy refined planes to big one plane
	if (!isse)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
				pel2Plane[w<<1] = pPlane0[w];
				pel2Plane[(w<<1) +1] = pPlane1[w];
			}
			pel2Plane += pel2Pitch;
			for (int w=0; w<width; w++)
			{
				pel2Plane[w<<1] = pPlane2[w];
				pel2Plane[(w<<1) +1] = pPlane3[w];
			}
			pel2Plane += pel2Pitch;
			pPlane0 += pitch;
			pPlane1 += pitch;
			pPlane2 += pitch;
			pPlane3 += pitch;
		}
	}
	else // isse - not very optimized
	{
		_asm
		{
			push ebx;
			push esi;
			push edi;
			mov edi, pel2Plane;
			mov esi, pPlane0;
			mov edx, pPlane1;

			mov ebx, height;
looph1:
			mov ecx, width;
			mov eax, 0;
align 16
loopw1:
			movd mm0, [esi+eax];
			movd mm1, [edx+eax];
			punpcklbw mm0, mm1;
			shl eax, 1;
			movq [edi+eax], mm0;
			shr eax, 1;
			add eax, 4;
			cmp eax, ecx;
			jl loopw1;

			mov eax, pel2Pitch;
			add edi, eax;
			add edi, eax;
			mov eax, pitch;
			add esi, eax;
			add edx, eax;
			dec ebx;
			cmp ebx, 0;
			jg looph1;

			mov edi, pel2Plane;
			mov esi, pPlane2;
			mov edx, pPlane3;

			mov eax, pel2Pitch;
			add edi, eax;

			mov ebx, height;
looph2:
			mov ecx, width;
			mov eax, 0;
align 16
loopw2:
			movd mm0, [esi+eax];
			movd mm1, [edx+eax];
			punpcklbw mm0, mm1;
			shl eax, 1;
			movq [edi+eax], mm0;
			shr eax, 1;
			add eax, 4;
			cmp eax, ecx;
			jl loopw2;

			mov eax, pel2Pitch;
			add edi, eax;
			add edi, eax;
			mov eax, pitch;
			add esi, eax;
			add edx, eax;
			dec ebx;
			cmp ebx, 0;
			jg looph2;

			pop edi;
			pop esi;
			pop ebx;
			emms;

		}
	}
}


void Merge16PlanesToBig(BYTE *pel4Plane, int pel4Pitch,
                    const BYTE *pPlane0, const BYTE *pPlane1, const BYTE *pPlane2, const BYTE * pPlane3,
                    const BYTE *pPlane4, const BYTE *pPlane5, const BYTE *pPlane6, const BYTE * pPlane7,
                    const BYTE *pPlane8, const BYTE *pPlane9, const BYTE *pPlane10, const BYTE * pPlane11,
                    const BYTE *pPlane12, const BYTE * pPlane13, const BYTE *pPlane14, const BYTE * pPlane15,
					int width, int height, int pitch, bool isse)
{
	// copy refined planes to big one plane
//	if (!isse)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
				pel4Plane[w<<2] = pPlane0[w];
				pel4Plane[(w<<2) +1] = pPlane1[w];
				pel4Plane[(w<<2) +2] = pPlane2[w];
				pel4Plane[(w<<2) +3] = pPlane3[w];
			}
			pel4Plane += pel4Pitch;
			for (int w=0; w<width; w++)
			{
				pel4Plane[w<<2] = pPlane4[w];
				pel4Plane[(w<<2) +1] = pPlane5[w];
				pel4Plane[(w<<2) +2] = pPlane6[w];
				pel4Plane[(w<<2) +3] = pPlane7[w];
			}
			pel4Plane += pel4Pitch;
			for (int w=0; w<width; w++)
			{
				pel4Plane[w<<2] = pPlane8[w];
				pel4Plane[(w<<2) +1] = pPlane9[w];
				pel4Plane[(w<<2) +2] = pPlane10[w];
				pel4Plane[(w<<2) +3] = pPlane11[w];
			}
			pel4Plane += pel4Pitch;
			for (int w=0; w<width; w++)
			{
				pel4Plane[w<<2] = pPlane12[w];
				pel4Plane[(w<<2) +1] = pPlane13[w];
				pel4Plane[(w<<2) +2] = pPlane14[w];
				pel4Plane[(w<<2) +3] = pPlane15[w];
			}
			pel4Plane += pel4Pitch;
			pPlane0 += pitch;
			pPlane1 += pitch;
			pPlane2 += pitch;
			pPlane3 += pitch;
			pPlane4 += pitch;
			pPlane5 += pitch;
			pPlane6 += pitch;
			pPlane7 += pitch;
			pPlane8 += pitch;
			pPlane9 += pitch;
			pPlane10 += pitch;
			pPlane11 += pitch;
			pPlane12 += pitch;
			pPlane13+= pitch;
			pPlane14 += pitch;
			pPlane15 += pitch;
		}
	}
}

//-----------------------------------------------------------
unsigned char SADToMask(unsigned int sad, unsigned int sadnorm1024)
{
	// sadnorm1024 = 255 * (4*1024)/(mlSAD*nBlkSize*nBlkSize*chromablockfactor)
	unsigned int l = sadnorm1024*sad/1024;
	return (unsigned char)((l > 255) ? 255 : l);
}


// time-weihted blend src with ref frames (used for interpolation for poor motion estimation)
void Blend(BYTE * pdst, const BYTE * psrc, const BYTE * pref, int height, int width, int dst_pitch, int src_pitch, int ref_pitch, int time256, bool isse)
{
    // add isse
    int h, w;
        for (h=0; h<height; h++)
        {
            for (w=0; w<width; w++)
            {
                pdst[w] = (psrc[w]*(256 - time256) + pref[w]*time256)>>8;
            }
            pdst += dst_pitch;
            psrc += src_pitch;
            pref += ref_pitch;
        }
}

void Create_LUTV(int time256, int *LUTVB, int *LUTVF)
{
	for (int v=0; v<256; v++)
	{
		LUTVB[v] = ((v-128)*(256-time256))/256;
		LUTVF[v] = ((v-128)*time256)/256;
	}
}

void FlowInter(BYTE * pdst, int dst_pitch, const BYTE *prefB, const BYTE *prefF, int ref_pitch,
			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
			   int VPitch, int width, int height, int time256, int nPel, int *LUTVB, int *LUTVF)
{
	if (nPel==1)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
//				int vxF = ((VXFullF[w]-128)*time256)/256;
//				int vyF = ((VYFullF[w]-128)*time256)/256;
				int vxF = LUTVF[VXFullF[w]];
				int vyF = LUTVF[VYFullF[w]];
				int dstF = prefF[vyF*ref_pitch + vxF + w];
				int dstF0 = prefF[w]; // zero
//				int vxB = ((VXFullB[w]-128)*(256-time256))/256;
//				int vyB = ((VYFullB[w]-128)*(256-time256))/256;
				int vxB = LUTVB[VXFullB[w]];
				int vyB = LUTVB[VYFullB[w]];
				int dstB = prefB[vyB*ref_pitch + vxB + w];
				int dstB0 = prefB[w]; // zero
				pdst[w] = ( ( (dstF*(255-MaskF[w]) + ((MaskF[w]*(dstB*(255-MaskB[w])+MaskB[w]*dstF0)+255)>>8) + 255)>>8 )*(256-time256) +
				            ( (dstB*(255-MaskB[w]) + ((MaskB[w]*(dstF*(255-MaskF[w])+MaskF[w]*dstB0)+255)>>8) + 255)>>8 )*time256 )>>8;
//				pdst[w] = ( ( (dstF*(255-MaskF[w]) + dstB*MaskF[w] + 255)>>8 )*(256-time256) +
//				            ( (dstB*(255-MaskB[w]) + dstF*MaskB[w] + 255)>>8 )*time256 )>>8;
			}
			pdst += dst_pitch;
			prefB += ref_pitch;
			prefF += ref_pitch;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
		}
	}
	else if (nPel==2)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
//				int vxF = ((VXFullF[w]-128)*time256)/256;
//				int vyF = ((VYFullF[w]-128)*time256)/256;
				int vxF = LUTVF[VXFullF[w]];
				int vyF = LUTVF[VYFullF[w]];
				int dstF = prefF[vyF*ref_pitch + vxF + (w<<1)];
				int dstF0 = prefF[(w<<1)]; // zero
//				int vxB = ((VXFullB[w]-128)*(256-time256))/256;
//				int vyB = ((VYFullB[w]-128)*(256-time256))/256;
				int vxB = LUTVB[VXFullB[w]];
				int vyB = LUTVB[VYFullB[w]];
				int dstB = prefB[vyB*ref_pitch + vxB + (w<<1)];
				int dstB0 = prefB[(w<<1)]; // zero
				pdst[w] = ( ( (dstF*(255-MaskF[w]) + ((MaskF[w]*(dstB*(255-MaskB[w])+MaskB[w]*dstF0)+255)>>8) + 255)>>8 )*(256-time256) +
				            ( (dstB*(255-MaskB[w]) + ((MaskB[w]*(dstF*(255-MaskF[w])+MaskF[w]*dstB0)+255)>>8) + 255)>>8 )*time256 )>>8;
//				pdst[w] = ( ( (dstF*(255-MaskF[w]) + dstB*MaskF[w] + 255)>>8 )*(256-time256) +
//				            ( (dstB*(255-MaskB[w]) + dstF*MaskB[w] + 255)>>8 )*time256 )>>8;
			}
			pdst += dst_pitch;
			prefB += ref_pitch<<1;
			prefF += ref_pitch<<1;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
		}
	}
	else if (nPel==4)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
//				int vxF = ((VXFullF[w]-128)*time256)/256;
//				int vyF = ((VYFullF[w]-128)*time256)/256;
				int vxF = LUTVF[VXFullF[w]];
				int vyF = LUTVF[VYFullF[w]];
				int dstF = prefF[vyF*ref_pitch + vxF + (w<<2)];
				int dstF0 = prefF[(w<<2)]; // zero
//				int vxB = ((VXFullB[w]-128)*(256-time256))/256;
//				int vyB = ((VYFullB[w]-128)*(256-time256))/256;
				int vxB = LUTVB[VXFullB[w]];
				int vyB = LUTVB[VYFullB[w]];
				int dstB = prefB[vyB*ref_pitch + vxB + (w<<2)];
				int dstB0 = prefB[(w<<2)]; // zero
				pdst[w] = ( ( (dstF*(255-MaskF[w]) + ((MaskF[w]*(dstB*(255-MaskB[w])+MaskB[w]*dstF0)+255)>>8) + 255)>>8 )*(256-time256) +
				            ( (dstB*(255-MaskB[w]) + ((MaskB[w]*(dstF*(255-MaskF[w])+MaskF[w]*dstB0)+255)>>8) + 255)>>8 )*time256 )>>8;
//				pdst[w] = ( ( (dstF*(255-MaskF[w]) + dstB*MaskF[w] + 255)>>8 )*(256-time256) +
//				            ( (dstB*(255-MaskB[w]) + dstF*MaskB[w] + 255)>>8 )*time256 )>>8;
			}
			pdst += dst_pitch;
			prefB += ref_pitch<<2;
			prefF += ref_pitch<<2;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
		}
	}
}

// FIND MEDIAN OF 3 ELEMENTS
//
inline int Median3 (int a, int b, int c)
{
	// b a c || c a b
	if (((b <= a) && (a <= c)) || ((c <= a) && (a <= b))) return a;

	// a b c || c b a
	else if (((a <= b) && (b <= c)) || ((c <= b) && (b <= a))) return b;

	// b c a || a c b
	else return c;

}

inline int Median3r (int a, int b, int c)
{
    // reduced median - if it is known that a <= c (more fast)

	// b a c
	if (b <= a) return a;
	// a c b
	else  if (c <= b) return c;
	// a b c
	else return b;

}

void FlowInterExtra(BYTE * pdst, int dst_pitch, const BYTE *prefB, const BYTE *prefF, int ref_pitch,
			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
			   int VPitch, int width, int height, int time256, int nPel, int *LUTVB, int * LUTVF,
			   BYTE *VXFullBB, BYTE *VXFullFF, BYTE *VYFullBB, BYTE *VYFullFF)
{
 	if (nPel==1)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
				int vxF = LUTVF[VXFullF[w]];
				int vyF = LUTVF[VYFullF[w]];
				int adrF = vyF*ref_pitch + vxF + w;
				int dstF = prefF[adrF];
//				int dstF0 = prefF[w]; // zero vector

				int vxFF = LUTVF[VXFullFF[w]];
				int vyFF = LUTVF[VYFullFF[w]];
				int adrFF = vyFF*ref_pitch + vxFF + w;
				int dstFF = prefF[adrFF];

				int vxB = LUTVB[VXFullB[w]];
				int vyB = LUTVB[VYFullB[w]];
				int adrB = vyB*ref_pitch + vxB + w;
				int dstB = prefB[adrB];
//				int dstB0 = prefB[w]; // zero vector

				int vxBB = LUTVB[VXFullBB[w]];
				int vyBB = LUTVB[VYFullBB[w]];
				int adrBB = vyBB*ref_pitch + vxBB + w;
				int dstBB = prefB[adrBB];

                // use median, firstly get min max of compensations
                int minfb;
                int maxfb;
                if (dstF > dstB) {
                    minfb = dstB;
                    maxfb = dstF;
                } else {
                    maxfb = dstB;
                    minfb = dstF;
                }

                pdst[w] = (((Median3r(minfb,dstBB,maxfb)*MaskF[w] + dstF*(255-MaskF[w])+255)>>8)*(256-time256) +
                           ((Median3r(minfb,dstFF,maxfb)*MaskB[w] + dstB*(255-MaskB[w])+255)>>8)*time256)>>8;
  			}
			pdst += dst_pitch;
			prefB += ref_pitch;
			prefF += ref_pitch;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
			VXFullBB += VPitch;
			VYFullBB += VPitch;
			VXFullFF += VPitch;
			VYFullFF += VPitch;
		}
	}
	else if (nPel==2)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
				int vxF = LUTVF[VXFullF[w]];
				int vyF = LUTVF[VYFullF[w]];
				int adrF = vyF*ref_pitch + vxF + (w<<1);
				int dstF = prefF[adrF];
				int vxFF = LUTVF[VXFullFF[w]];
				int vyFF = LUTVF[VYFullFF[w]];
				int adrFF = vyFF*ref_pitch + vxFF + (w<<1);
				int dstFF = prefF[adrFF];
				int vxB = LUTVB[VXFullB[w]];
				int vyB = LUTVB[VYFullB[w]];
				int adrB = vyB*ref_pitch + vxB + (w<<1);
				int dstB = prefB[adrB];
				int vxBB = LUTVB[VXFullBB[w]];
				int vyBB = LUTVB[VYFullBB[w]];
				int adrBB = vyBB*ref_pitch + vxBB + (w<<1);
				int dstBB = prefB[adrBB];
                // use median, firsly get min max of compensations
                int minfb;
                int maxfb;
                if (dstF > dstB) {
                    minfb = dstB;
                    maxfb = dstF;
                } else {
                    maxfb = dstB;
                    minfb = dstF;
                }

                pdst[w] = (((Median3r(minfb, dstBB, maxfb)*MaskF[w] + dstF*(255-MaskF[w])+255)>>8)*(256-time256) +
                           ((Median3r(minfb, dstFF, maxfb)*MaskB[w] + dstB*(255-MaskB[w])+255)>>8)*time256)>>8;

/*				dstF = prefF[adrF+2]; // approximation for speed
				dstFF = prefF[adrFF+2];
				dstB = prefB[adrB+2];
				dstBB = prefB[adrBB+2];
                // use median, firsly get min max of compensations
                if (dstF > dstB) {
                    minfb = dstB;
                    maxfb = dstF;
                } else {
                    maxfb = dstB;
                    minfb = dstF;
                }

                pdst[w+1] = (((Median3r(minfb, dstBB, maxfb)*MaskF[w] + dstF*(255-MaskF[w])+255)>>8)*(256-time256) +
                           ((Median3r(minfb, dstFF, maxfb)*MaskB[w] + dstB*(255-MaskB[w])+255)>>8)*time256)>>8;
*/			}
			pdst += dst_pitch;
			prefB += ref_pitch<<1;
			prefF += ref_pitch<<1;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
			VXFullBB += VPitch;
			VYFullBB += VPitch;
			VXFullFF += VPitch;
			VYFullFF += VPitch;
		}
	}
	else if (nPel==4)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
				int vxF = LUTVF[VXFullF[w]];
				int vyF = LUTVF[VYFullF[w]];
				int dstF = prefF[vyF*ref_pitch + vxF + (w<<2)];
//				int dstF0 = prefF[(w<<2)]; // zero
				int vxFF = LUTVF[VXFullFF[w]];
				int vyFF = LUTVF[VYFullFF[w]];
				int dstFF = prefF[vyFF*ref_pitch + vxFF + (w<<2)];
				int vxB = LUTVB[VXFullB[w]];
				int vyB = LUTVB[VYFullB[w]];
				int dstB = prefB[vyB*ref_pitch + vxB + (w<<2)];
//				int dstB0 = prefB[(w<<2)]; // zero
				int vxBB = LUTVB[VXFullBB[w]];
				int vyBB = LUTVB[VYFullBB[w]];
				int dstBB = prefB[vyBB*ref_pitch + vxBB + (w<<2)];
//				pdst[w] = ( ( (dstF*(255-MaskF[w]) + ((MaskF[w]*(dstB*(255-MaskB[w])+MaskB[w]*dstF0)+255)>>8) + 255)>>8 )*(256-time256) +
//				            ( (dstB*(255-MaskB[w]) + ((MaskB[w]*(dstF*(255-MaskF[w])+MaskF[w]*dstB0)+255)>>8) + 255)>>8 )*time256 )>>8;
                // use median, firsly get min max of compensations
                int minfb;
                int maxfb;
                if (dstF > dstB) {
                    minfb = dstB;
                    maxfb = dstF;
                } else {
                    maxfb = dstB;
                    minfb = dstF;
                }

                pdst[w] = (((Median3r(minfb, dstBB, maxfb)*MaskF[w] + dstF*(255-MaskF[w])+255)>>8)*(256-time256) +
                           ((Median3r(minfb, dstFF, maxfb)*MaskB[w] + dstB*(255-MaskB[w])+255)>>8)*time256)>>8;
			}
			pdst += dst_pitch;
			prefB += ref_pitch<<2;
			prefF += ref_pitch<<2;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
			VXFullBB += VPitch;
			VYFullBB += VPitch;
			VXFullFF += VPitch;
			VYFullFF += VPitch;
		}
	}
}

void FlowInterSimple(BYTE * pdst, int dst_pitch, const BYTE *prefB, const BYTE *prefF, int ref_pitch,
			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
			   int VPitch, int width, int height, int time256, int nPel, int *LUTVB, int *LUTVF)
{

    if (time256==128) // special case double fps - fastest
    {
        if (nPel==1)
        {
            for (int h=0; h<height; h++)
            {
                for (int w=0; w<width; w+=2) // paired for speed
                {
    				int vxF = (VXFullF[w]-128)>>1;
    				int vyF = (VYFullF[w]-128)>>1;
                    int addrF = vyF*ref_pitch + vxF + w;
                    int dstF = prefF[addrF];
                    int dstF1 = prefF[addrF+1]; // approximation for speed
    				int vxB = (VXFullB[w]-128)>>1;
    				int vyB = (VYFullB[w]-128)>>1;
                    int addrB = vyB*ref_pitch + vxB + w;
                    int dstB = prefB[addrB];
                    int dstB1 = prefB[addrB+1];
                    pdst[w] = ( ((dstF + dstB)<<8) + (dstB - dstF)*(MaskF[w] - MaskB[w]) )>>9;
                    pdst[w+1] = ( ((dstF1 + dstB1)<<8) + (dstB1 - dstF1)*(MaskF[w+1] - MaskB[w+1]) )>>9;
                }
                pdst += dst_pitch;
                prefB += ref_pitch;
                prefF += ref_pitch;
                VXFullB += VPitch;
                VYFullB += VPitch;
                VXFullF += VPitch;
                VYFullF += VPitch;
                MaskB += VPitch;
                MaskF += VPitch;
            }
        }
        else if (nPel==2)
        {
            for (int h=0; h<height; h++)
            {
                for (int w=0; w<width; w+=1)
                {
    				int vxF = (VXFullF[w]-128)>>1;
    				int vyF = (VYFullF[w]-128)>>1;
//                    int vxF = LUTVF[VXFullF[w]];
//                    int vyF = LUTVF[VYFullF[w]];
                    int dstF = prefF[vyF*ref_pitch + vxF + (w<<1)];
    				int vxB = (VXFullB[w]-128)>>1;
    				int vyB = (VYFullB[w]-128)>>1;
//                    int vxB = LUTVB[VXFullB[w]];
//                    int vyB = LUTVB[VYFullB[w]];
                    int dstB = prefB[vyB*ref_pitch + vxB + (w<<1)];
                    pdst[w] = ( ((dstF + dstB)<<8) + (dstB - dstF)*(MaskF[w] - MaskB[w]) )>>9;
                }
                pdst += dst_pitch;
                prefB += ref_pitch<<1;
                prefF += ref_pitch<<1;
                VXFullB += VPitch;
                VYFullB += VPitch;
                VXFullF += VPitch;
                VYFullF += VPitch;
                MaskB += VPitch;
                MaskF += VPitch;
            }
        }
        else if (nPel==4)
        {
            for (int h=0; h<height; h++)
            {
                for (int w=0; w<width; w+=1)
                {
    				int vxF = (VXFullF[w]-128)>>1;
    				int vyF = (VYFullF[w]-128)>>1;
//                    int vxF = LUTVF[VXFullF[w]];
//                    int vyF = LUTVF[VYFullF[w]];
                    int dstF = prefF[vyF*ref_pitch + vxF + (w<<2)];
    				int vxB = (VXFullB[w]-128)>>1;
    				int vyB = (VYFullB[w]-128)>>1;
//                    int vxB = LUTVB[VXFullB[w]];
//                    int vyB = LUTVB[VYFullB[w]];
                    int dstB = prefB[vyB*ref_pitch + vxB + (w<<2)];
                    pdst[w] = ( ((dstF + dstB)<<8) + (dstB - dstF)*(MaskF[w] - MaskB[w]) )>>9;
                }
                pdst += dst_pitch;
                prefB += ref_pitch<<2;
                prefF += ref_pitch<<2;
                VXFullB += VPitch;
                VYFullB += VPitch;
                VXFullF += VPitch;
                VYFullF += VPitch;
                MaskB += VPitch;
                MaskF += VPitch;
            }
        }
    }
    else // general case
    {
        if (nPel==1)
        {
            for (int h=0; h<height; h++)
            {
                for (int w=0; w<width; w+=2) // paired for speed
                {
                    int vxF = LUTVF[VXFullF[w]];
                    int vyF = LUTVF[VYFullF[w]];
                    int addrF = vyF*ref_pitch + vxF + w;
                    int dstF = prefF[addrF];
                    int dstF1 = prefF[addrF+1]; // approximation for speed
                    int vxB = LUTVB[VXFullB[w]];
                    int vyB = LUTVB[VYFullB[w]];
                    int addrB = vyB*ref_pitch + vxB + w;
                    int dstB = prefB[addrB];
                    int dstB1 = prefB[addrB+1];
                    pdst[w] = ( ( (dstF*255 + (dstB-dstF)*MaskF[w] + 255) )*(256-time256) +
                                ( (dstB*255 - (dstB-dstF)*MaskB[w] + 255) )*time256 )>>16;
                    pdst[w+1] = ( ( (dstF1*255 + (dstB1-dstF1)*MaskF[w+1] + 255) )*(256-time256) +
                                ( (dstB1*255 - (dstB1-dstF1)*MaskB[w+1] + 255) )*time256 )>>16;
                }
                pdst += dst_pitch;
                prefB += ref_pitch;
                prefF += ref_pitch;
                VXFullB += VPitch;
                VYFullB += VPitch;
                VXFullF += VPitch;
                VYFullF += VPitch;
                MaskB += VPitch;
                MaskF += VPitch;
            }
        }
        else if (nPel==2)
        {
            for (int h=0; h<height; h++)
            {
                for (int w=0; w<width; w+=1)
                {
    //				int vxF = ((VXFullF[w]-128)*time256)/256;
    //				int vyF = ((VYFullF[w]-128)*time256)/256;
                    int vxF = LUTVF[VXFullF[w]];
                    int vyF = LUTVF[VYFullF[w]];
                    int dstF = prefF[vyF*ref_pitch + vxF + (w<<1)];
                    int vxB = LUTVB[VXFullB[w]];
                    int vyB = LUTVB[VYFullB[w]];
                    int dstB = prefB[vyB*ref_pitch + vxB + (w<<1)];
                    pdst[w] = ( ( (dstF*(255-MaskF[w]) + dstB*MaskF[w] + 255)>>8 )*(256-time256) +
                                ( (dstB*(255-MaskB[w]) + dstF*MaskB[w] + 255)>>8 )*time256 )>>8;
                }
                pdst += dst_pitch;
                prefB += ref_pitch<<1;
                prefF += ref_pitch<<1;
                VXFullB += VPitch;
                VYFullB += VPitch;
                VXFullF += VPitch;
                VYFullF += VPitch;
                MaskB += VPitch;
                MaskF += VPitch;
            }
        }
        else if (nPel==4)
        {
            for (int h=0; h<height; h++)
            {
                for (int w=0; w<width; w+=1)
                {
    //				int vxF = ((VXFullF[w]-128)*time256)/256;
    //				int vyF = ((VYFullF[w]-128)*time256)/256;
                    int vxF = LUTVF[VXFullF[w]];
                    int vyF = LUTVF[VYFullF[w]];
                    int dstF = prefF[vyF*ref_pitch + vxF + (w<<2)];
                    int vxB = LUTVB[VXFullB[w]];
                    int vyB = LUTVB[VYFullB[w]];
                    int dstB = prefB[vyB*ref_pitch + vxB + (w<<2)];
                    pdst[w] = ( ( (dstF*(255-MaskF[w]) + dstB*MaskF[w] + 255)>>8 )*(256-time256) +
                                ( (dstB*(255-MaskB[w]) + dstF*MaskB[w] + 255)>>8 )*time256 )>>8;
                }
                pdst += dst_pitch;
                prefB += ref_pitch<<2;
                prefF += ref_pitch<<2;
                VXFullB += VPitch;
                VYFullB += VPitch;
                VXFullF += VPitch;
                VYFullF += VPitch;
                MaskB += VPitch;
                MaskF += VPitch;
            }
        }
    }
}

void FlowInterPel(BYTE * pdst, int dst_pitch, MVPlane *prefB, MVPlane *prefF, int ref_pitch,
			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
			   int VPitch, int width, int height, int time256, int nPel, int *LUTVB, int *LUTVF)
{
	if (nPel==1)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
//				int vxF = ((VXFullF[w]-128)*time256)/256;
//				int vyF = ((VYFullF[w]-128)*time256)/256;
				int vxF = LUTVF[VXFullF[w]];
				int vyF = LUTVF[VYFullF[w]];
//				int dstF = prefF[vyF*ref_pitch + vxF + w];
				int dstF = *prefF->GetPointer(vxF + w, vyF + h); // v2.0.0.6
//				int dstF0 = prefF[w]; // zero
				int dstF0 = *prefF->GetPointer(w, h); // v2.0.0.6
//				int vxB = ((VXFullB[w]-128)*(256-time256))/256;
//				int vyB = ((VYFullB[w]-128)*(256-time256))/256;
				int vxB = LUTVB[VXFullB[w]];
				int vyB = LUTVB[VYFullB[w]];
//				int dstB = prefB[vyB*ref_pitch + vxB + w];
				int dstB = *prefB->GetPointer(vxB + w, vyB + h); // v2.0.0.6
//				int dstB0 = prefB[w]; // zero
				int dstB0 = *prefF->GetPointer(w, h); // v2.0.0.6
				pdst[w] = ( ( (dstF*(255-MaskF[w]) + ((MaskF[w]*(dstB*(255-MaskB[w])+MaskB[w]*dstF0)+255)>>8) + 255)>>8 )*(256-time256) +
				            ( (dstB*(255-MaskB[w]) + ((MaskB[w]*(dstF*(255-MaskF[w])+MaskF[w]*dstB0)+255)>>8) + 255)>>8 )*time256 )>>8;
//				pdst[w] = ( ( (dstF*(255-MaskF[w]) + dstB*MaskF[w] + 255)>>8 )*(256-time256) +
//				            ( (dstB*(255-MaskB[w]) + dstF*MaskB[w] + 255)>>8 )*time256 )>>8;
			}
			pdst += dst_pitch;
//			prefB += ref_pitch;
//			prefF += ref_pitch;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
		}
	}
	else if (nPel==2)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
//				int vxF = ((VXFullF[w]-128)*time256)/256;
//				int vyF = ((VYFullF[w]-128)*time256)/256;
				int vxF = LUTVF[VXFullF[w]];
				int vyF = LUTVF[VYFullF[w]];
//				int dstF = prefF[vyF*ref_pitch + vxF + (w<<1)];
				int dstF = *prefF->GetPointer(vxF + w, vyF + h); // v2.0.0.6
//				int dstF0 = prefF[(w<<1)]; // zero
				int dstF0 = *prefF->GetPointer(w, h); // v2.0.0.6
//				int vxB = ((VXFullB[w]-128)*(256-time256))/256;
//				int vyB = ((VYFullB[w]-128)*(256-time256))/256;
				int vxB = LUTVB[VXFullB[w]];
				int vyB = LUTVB[VYFullB[w]];
//				int dstB = prefB[vyB*ref_pitch + vxB + (w<<1)];
				int dstB = *prefB->GetPointer(vxB + w, vyB + h); // v2.0.0.6
//				int dstB0 = prefB[(w<<1)]; // zero
				int dstB0 = *prefB->GetPointer(w, h); // v2.0.0.6
				pdst[w] = ( ( (dstF*(255-MaskF[w]) + ((MaskF[w]*(dstB*(255-MaskB[w])+MaskB[w]*dstF0)+255)>>8) + 255)>>8 )*(256-time256) +
				            ( (dstB*(255-MaskB[w]) + ((MaskB[w]*(dstF*(255-MaskF[w])+MaskF[w]*dstB0)+255)>>8) + 255)>>8 )*time256 )>>8;
//				pdst[w] = ( ( (dstF*(255-MaskF[w]) + dstB*MaskF[w] + 255)>>8 )*(256-time256) +
//				            ( (dstB*(255-MaskB[w]) + dstF*MaskB[w] + 255)>>8 )*time256 )>>8;
			}
			pdst += dst_pitch;
//			prefB += ref_pitch<<1;
//			prefF += ref_pitch<<1;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
		}
	}
	else if (nPel==4)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
//				int vxF = ((VXFullF[w]-128)*time256)/256;
//				int vyF = ((VYFullF[w]-128)*time256)/256;
				int vxF = LUTVF[VXFullF[w]];
				int vyF = LUTVF[VYFullF[w]];
//				int dstF = prefF[vyF*ref_pitch + vxF + (w<<2)];
				int dstF = *prefF->GetPointer(vxF + w, vyF + h); // v2.0.0.6
//				int dstF0 = prefF[(w<<2)]; // zero
				int dstF0 = *prefF->GetPointer(w, h); // v2.0.0.6
//				int vxB = ((VXFullB[w]-128)*(256-time256))/256;
//				int vyB = ((VYFullB[w]-128)*(256-time256))/256;
				int vxB = LUTVB[VXFullB[w]];
				int vyB = LUTVB[VYFullB[w]];
//				int dstB = prefB[vyB*ref_pitch + vxB + (w<<2)];
				int dstB = *prefB->GetPointer(vxB + w, vyB + h); // v2.0.0.6
//				int dstB0 = prefB[(w<<2)]; // zero
				int dstB0 = *prefB->GetPointer(w, h); // v2.0.0.6
				pdst[w] = ( ( (dstF*(255-MaskF[w]) + ((MaskF[w]*(dstB*(255-MaskB[w])+MaskB[w]*dstF0)+255)>>8) + 255)>>8 )*(256-time256) +
				            ( (dstB*(255-MaskB[w]) + ((MaskB[w]*(dstF*(255-MaskF[w])+MaskF[w]*dstB0)+255)>>8) + 255)>>8 )*time256 )>>8;
//				pdst[w] = ( ( (dstF*(255-MaskF[w]) + dstB*MaskF[w] + 255)>>8 )*(256-time256) +
//				            ( (dstB*(255-MaskB[w]) + dstF*MaskB[w] + 255)>>8 )*time256 )>>8;
			}
			pdst += dst_pitch;
//			prefB += ref_pitch<<2;
//			prefF += ref_pitch<<2;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
		}
	}
}

void FlowInterExtraPel(BYTE * pdst, int dst_pitch, MVPlane *prefB, MVPlane *prefF, int ref_pitch,
			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
			   int VPitch, int width, int height, int time256, int nPel, int *LUTVB, int * LUTVF,
			   BYTE *VXFullBB, BYTE *VXFullFF, BYTE *VYFullBB, BYTE *VYFullFF)
{
 	if (nPel==1)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w+=1)
			{
				int vxF = LUTVF[VXFullF[w]];
				int vyF = LUTVF[VYFullF[w]];
				const BYTE * adrF = prefF->GetPointerPel1(vxF + w, vyF + h); // v2.0.0.8
				int dstF = *adrF;

				int vxFF = LUTVF[VXFullFF[w]];
				int vyFF = LUTVF[VYFullFF[w]];
				const BYTE *  adrFF = prefF->GetPointerPel1(vxFF + w, vyFF + h); // v2.0.0.8
				int dstFF = *adrFF;

				int vxB = LUTVB[VXFullB[w]];
				int vyB = LUTVB[VYFullB[w]];
				const BYTE * adrB = prefB->GetPointerPel1(vxB + w, vyB + h); // v2.0.0.8
				int dstB = *adrB;

				int vxBB = LUTVB[VXFullBB[w]];
				int vyBB = LUTVB[VYFullBB[w]];
				const BYTE * adrBB = prefB->GetPointerPel1(vxBB + w, vyBB + h); // v2.0.0.8;
				int dstBB = * adrBB;

                // use median, firstly get min max of compensations
                int minfb;
                int maxfb;
                if (dstF > dstB) {
                    minfb = dstB;
                    maxfb = dstF;
                } else {
                    maxfb = dstB;
                    minfb = dstF;
                }

                pdst[w] = (((Median3r(minfb,dstBB,maxfb)*MaskF[w] + dstF*(255-MaskF[w])+255)>>8)*(256-time256) +
                           ((Median3r(minfb,dstFF,maxfb)*MaskB[w] + dstB*(255-MaskB[w])+255)>>8)*time256)>>8;
  			}
			pdst += dst_pitch;
//			prefB += ref_pitch;
//			prefF += ref_pitch;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
			VXFullBB += VPitch;
			VYFullBB += VPitch;
			VXFullFF += VPitch;
			VYFullFF += VPitch;
		}
	}
	else if (nPel==2)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
				int vxF = LUTVF[VXFullF[w]];
				int vyF = LUTVF[VYFullF[w]];
				const BYTE * adrF = prefF->GetPointerPel2(vxF + (w<<1), vyF + (h<<1)); // v2.0.0.8
				int dstF = *adrF;

				int vxFF = LUTVF[VXFullFF[w]];
				int vyFF = LUTVF[VYFullFF[w]];
				const BYTE *  adrFF = prefF->GetPointerPel2(vxFF + (w<<1), vyFF + (h<<1)); // v2.0.0.8
				int dstFF = *adrFF;

				int vxB = LUTVB[VXFullB[w]];
				int vyB = LUTVB[VYFullB[w]];
				const BYTE * adrB = prefB->GetPointerPel2(vxB + (w<<1), vyB + (h<<1)); // v2.0.0.8
				int dstB = *adrB;

				int vxBB = LUTVB[VXFullBB[w]];
				int vyBB = LUTVB[VYFullBB[w]];
				const BYTE * adrBB = prefB->GetPointerPel2(vxBB + (w<<1), vyBB + (h<<1)); // v2.0.0.8;
				int dstBB = * adrBB;

                // use median, firsly get min max of compensations
                int minfb;
                int maxfb;
                if (dstF > dstB) {
                    minfb = dstB;
                    maxfb = dstF;
                } else {
                    maxfb = dstB;
                    minfb = dstF;
                }

                pdst[w] = (((Median3r(minfb, dstBB, maxfb)*MaskF[w] + dstF*(255-MaskF[w])+255)>>8)*(256-time256) +
                           ((Median3r(minfb, dstFF, maxfb)*MaskB[w] + dstB*(255-MaskB[w])+255)>>8)*time256)>>8;
/*
				dstF = *(adrF+1);
				dstFF = *(adrFF+1);
				dstB = *(adrB+1);
				dstBB = *(adrBB+1);

                if (dstF > dstB) {
                    minfb = dstB;
                    maxfb = dstF;
                } else {
                    maxfb = dstB;
                    minfb = dstF;
                }

                pdst[w+1] = (((Median3r(minfb, dstBB, maxfb)*MaskF[w] + dstF*(255-MaskF[w])+255)>>8)*(256-time256) +
                           ((Median3r(minfb, dstFF, maxfb)*MaskB[w] + dstB*(255-MaskB[w])+255)>>8)*time256)>>8;
*/			}
			pdst += dst_pitch;
//			prefB += ref_pitch<<1;
//			prefF += ref_pitch<<1;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
			VXFullBB += VPitch;
			VYFullBB += VPitch;
			VXFullFF += VPitch;
			VYFullFF += VPitch;
		}
	}
	else if (nPel==4)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
				int vxF = LUTVF[VXFullF[w]];
				int vyF = LUTVF[VYFullF[w]];
//				int dstF = prefF[vyF*ref_pitch + vxF + (w<<2)];
				int dstF = *prefF->GetPointerPel4(vxF + (w<<2), vyF + (h<<2)); // v2.0.0.6
//				int dstF0 = prefF[(w<<2)]; // zero
				int vxFF = LUTVF[VXFullFF[w]];
				int vyFF = LUTVF[VYFullFF[w]];
//				int dstFF = prefF[vyFF*ref_pitch + vxFF + (w<<2)];
				int dstFF = *prefF->GetPointerPel4(vxFF + (w<<2), vyFF + (h<<2)); // v2.0.0.6
				int vxB = LUTVB[VXFullB[w]];
				int vyB = LUTVB[VYFullB[w]];
//				int dstB = prefB[vyB*ref_pitch + vxB + (w<<2)];
				int dstB = *prefB->GetPointerPel4(vxB + (w<<2), vyB + (h<<2)); // v2.0.0.6
//				int dstB0 = prefB[(w<<2)]; // zero
				int vxBB = LUTVB[VXFullBB[w]];
				int vyBB = LUTVB[VYFullBB[w]];
//				int dstBB = prefB[vyBB*ref_pitch + vxBB + (w<<2)];
				int dstBB = *prefB->GetPointerPel4(vxBB + (w<<2), vyBB + (h<<2)); // v2.0.0.6
//				pdst[w] = ( ( (dstF*(255-MaskF[w]) + ((MaskF[w]*(dstB*(255-MaskB[w])+MaskB[w]*dstF0)+255)>>8) + 255)>>8 )*(256-time256) +
//				            ( (dstB*(255-MaskB[w]) + ((MaskB[w]*(dstF*(255-MaskF[w])+MaskF[w]*dstB0)+255)>>8) + 255)>>8 )*time256 )>>8;
                // use median, firsly get min max of compensations
                int minfb;
                int maxfb;
                if (dstF > dstB) {
                    minfb = dstB;
                    maxfb = dstF;
                } else {
                    maxfb = dstB;
                    minfb = dstF;
                }

                pdst[w] = (((Median3r(minfb, dstBB, maxfb)*MaskF[w] + dstF*(255-MaskF[w])+255)>>8)*(256-time256) +
                           ((Median3r(minfb, dstFF, maxfb)*MaskB[w] + dstB*(255-MaskB[w])+255)>>8)*time256)>>8;
			}
			pdst += dst_pitch;
//			prefB += ref_pitch<<2;
//			prefF += ref_pitch<<2;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
			VXFullBB += VPitch;
			VYFullBB += VPitch;
			VXFullFF += VPitch;
			VYFullFF += VPitch;
		}
	}

}

void FlowInterSimplePel(BYTE * pdst, int dst_pitch, MVPlane *prefB, MVPlane *prefF, int ref_pitch,
			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
			   int VPitch, int width, int height, int time256, int nPel, int *LUTVB, int *LUTVF)
{

    if (time256==128) // special case double fps - fastest
    {
        if (nPel==1)
        {
            for (int h=0; h<height; h++)
            {
                for (int w=0; w<width; w+=2) // paired for speed
                {
    				int vxF = (VXFullF[w]-128)>>1;
    				int vyF = (VYFullF[w]-128)>>1;
//                    int addrF = vyF*ref_pitch + vxF + w;
//                    int dstF = prefF[addrF];
//                    int dstF1 = prefF[addrF+1]; // approximation for speed
                    const BYTE * addrF = prefF->GetPointerPel1(vxF + w, vyF + h); // v2.0.0.6
                    int dstF = *addrF;
                    int dstF1 = *(addrF+1); // v2.0.0.6
    				int vxB = (VXFullB[w]-128)>>1;
    				int vyB = (VYFullB[w]-128)>>1;
//                    int addrB = vyB*ref_pitch + vxB + w;
                    const BYTE * addrB = prefB->GetPointerPel1(vxB + w, vyB + h); // v2.0.0.6
                    int dstB = *addrB;
                    int dstB1 = *(addrB+1);
                    pdst[w] = ( ((dstF + dstB)<<8) + (dstB - dstF)*(MaskF[w] - MaskB[w]) )>>9;
                    pdst[w+1] = ( ((dstF1 + dstB1)<<8) + (dstB1 - dstF1)*(MaskF[w+1] - MaskB[w+1]) )>>9;
                }
                pdst += dst_pitch;
//                prefB += ref_pitch;
//                prefF += ref_pitch;
                VXFullB += VPitch;
                VYFullB += VPitch;
                VXFullF += VPitch;
                VYFullF += VPitch;
                MaskB += VPitch;
                MaskF += VPitch;
            }
        }
        else if (nPel==2)
        {
            for (int h=0; h<height; h++)
            {
                for (int w=0; w<width; w+=1)
                {
    				int vxF = (VXFullF[w]-128)>>1;
    				int vyF = (VYFullF[w]-128)>>1;
//                    int vxF = LUTVF[VXFullF[w]];
//                    int vyF = LUTVF[VYFullF[w]];
//                    int dstF = prefF[vyF*ref_pitch + vxF + (w<<1)];
                    int dstF  = *prefF->GetPointerPel2(vxF + (w<<1), vyF + (h<<1)); // v2.0.0.6
    				int vxB = (VXFullB[w]-128)>>1;
    				int vyB = (VYFullB[w]-128)>>1;
//                    int vxB = LUTVB[VXFullB[w]];
//                    int vyB = LUTVB[VYFullB[w]];
//                    int dstB = prefB[vyB*ref_pitch + vxB + (w<<1)];
                    int dstB = *prefB->GetPointerPel2(vxB + (w<<1), vyB + (h<<1)); // v2.0.0.6
                    pdst[w] = ( ((dstF + dstB)<<8) + (dstB - dstF)*(MaskF[w] - MaskB[w]) )>>9;
                }
                pdst += dst_pitch;
//                prefB += ref_pitch<<1;
//                prefF += ref_pitch<<1;
                VXFullB += VPitch;
                VYFullB += VPitch;
                VXFullF += VPitch;
                VYFullF += VPitch;
                MaskB += VPitch;
                MaskF += VPitch;
            }
        }
        else if (nPel==4)
        {
            for (int h=0; h<height; h++)
            {
                for (int w=0; w<width; w+=1)
                {
    				int vxF = (VXFullF[w]-128)>>1;
    				int vyF = (VYFullF[w]-128)>>1;
//                    int vxF = LUTVF[VXFullF[w]];
//                    int vyF = LUTVF[VYFullF[w]];
//                    int dstF = prefF[vyF*ref_pitch + vxF + (w<<2)];
                    int dstF = *prefF->GetPointerPel4(vxF + (w<<2), vyF + (h<<2)); // v2.0.0.6
    				int vxB = (VXFullB[w]-128)>>1;
    				int vyB = (VYFullB[w]-128)>>1;
//                    int vxB = LUTVB[VXFullB[w]];
//                    int vyB = LUTVB[VYFullB[w]];
//                    int dstB = prefB[vyB*ref_pitch + vxB + (w<<2)];
                    int dstB = *prefB->GetPointerPel4(vxB + (w<<2), vyB + (h<<2)); // v2.0.0.6
                    pdst[w] = ( ((dstF + dstB)<<8) + (dstB - dstF)*(MaskF[w] - MaskB[w]) )>>9;
                }
                pdst += dst_pitch;
//                prefB += ref_pitch<<2;
//                prefF += ref_pitch<<2;
                VXFullB += VPitch;
                VYFullB += VPitch;
                VXFullF += VPitch;
                VYFullF += VPitch;
                MaskB += VPitch;
                MaskF += VPitch;
            }
        }
    }
    else // general case
    {
        if (nPel==1)
        {
            for (int h=0; h<height; h++)
            {
                for (int w=0; w<width; w+=2) // paired for speed
                {
                    int vxF = LUTVF[VXFullF[w]];
                    int vyF = LUTVF[VYFullF[w]];
//                    int addrF = vyF*ref_pitch + vxF + w;
//                    int dstF = prefF[addrF];
//                    int dstF1 = prefF[addrF+1]; // approximation for speed
                    const BYTE * addrF = prefF->GetPointerPel1(vxF + w, vyF + h); // v2.0.0.6
                    int dstF = *addrF;
                    int dstF1 = *(addrF+1); // v2.0.0.6
                    int vxB = LUTVB[VXFullB[w]];
                    int vyB = LUTVB[VYFullB[w]];
//                    int addrB = vyB*ref_pitch + vxB + w;
//                    int dstB = prefB[addrB];
//                    int dstB1 = prefB[addrB+1];
                    const BYTE * addrB = prefB->GetPointerPel1(vxB + w, vyB + h); // v2.0.0.6
                    int dstB = *addrB;
                    int dstB1 = *(addrB+1);
                    pdst[w] = ( ( (dstF*255 + (dstB-dstF)*MaskF[w] + 255) )*(256-time256) +
                                ( (dstB*255 - (dstB-dstF)*MaskB[w] + 255) )*time256 )>>16;
                    pdst[w+1] = ( ( (dstF1*255 + (dstB1-dstF1)*MaskF[w+1] + 255) )*(256-time256) +
                                ( (dstB1*255 - (dstB1-dstF1)*MaskB[w+1] + 255) )*time256 )>>16;
                }
                pdst += dst_pitch;
//                prefB += ref_pitch;
//                prefF += ref_pitch;
                VXFullB += VPitch;
                VYFullB += VPitch;
                VXFullF += VPitch;
                VYFullF += VPitch;
                MaskB += VPitch;
                MaskF += VPitch;
            }
        }
        else if (nPel==2)
        {
            for (int h=0; h<height; h++)
            {
                for (int w=0; w<width; w+=1)
                {
    //				int vxF = ((VXFullF[w]-128)*time256)/256;
    //				int vyF = ((VYFullF[w]-128)*time256)/256;
                    int vxF = LUTVF[VXFullF[w]];
                    int vyF = LUTVF[VYFullF[w]];
//                    int dstF = prefF[vyF*ref_pitch + vxF + (w<<1)];
                    int dstF  = *prefF->GetPointerPel2(vxF + (w<<1), vyF + (h<<1)); // v2.0.0.6
                    int vxB = LUTVB[VXFullB[w]];
                    int vyB = LUTVB[VYFullB[w]];
//                    int dstB = prefB[vyB*ref_pitch + vxB + (w<<1)];
                    int dstB = *prefB->GetPointerPel2(vxB + (w<<1), vyB + (h<<1)); // v2.0.0.6
                    pdst[w] = ( ( (dstF*(255-MaskF[w]) + dstB*MaskF[w] + 255)>>8 )*(256-time256) +
                                ( (dstB*(255-MaskB[w]) + dstF*MaskB[w] + 255)>>8 )*time256 )>>8;
                }
                pdst += dst_pitch;
//                prefB += ref_pitch<<1;
//                prefF += ref_pitch<<1;
                VXFullB += VPitch;
                VYFullB += VPitch;
                VXFullF += VPitch;
                VYFullF += VPitch;
                MaskB += VPitch;
                MaskF += VPitch;
            }
        }
        else if (nPel==4)
        {
            for (int h=0; h<height; h++)
            {
                for (int w=0; w<width; w+=1)
                {
    //				int vxF = ((VXFullF[w]-128)*time256)/256;
    //				int vyF = ((VYFullF[w]-128)*time256)/256;
                    int vxF = LUTVF[VXFullF[w]];
                    int vyF = LUTVF[VYFullF[w]];
//                    int dstF = prefF[vyF*ref_pitch + vxF + (w<<2)];
                    int dstF  = *prefF->GetPointerPel4(vxF + (w<<2), vyF + (h<<2)); // v2.0.0.6
                    int vxB = LUTVB[VXFullB[w]];
                    int vyB = LUTVB[VYFullB[w]];
//                    int dstB = prefB[vyB*ref_pitch + vxB + (w<<2)];
                    int dstB = *prefB->GetPointerPel4(vxB + (w<<2), vyB + (h<<2)); // v2.0.0.6
                    pdst[w] = ( ( (dstF*(255-MaskF[w]) + dstB*MaskF[w] + 255)>>8 )*(256-time256) +
                                ( (dstB*(255-MaskB[w]) + dstF*MaskB[w] + 255)>>8 )*time256 )>>8;
                }
                pdst += dst_pitch;
//                prefB += ref_pitch<<2;
//                prefF += ref_pitch<<2;
                VXFullB += VPitch;
                VYFullB += VPitch;
                VXFullF += VPitch;
                VYFullF += VPitch;
                MaskB += VPitch;
                MaskF += VPitch;
            }
        }
    }
}

