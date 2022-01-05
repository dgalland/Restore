// Create an overlay mask with the motion vectors

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

#ifndef __MASKFUN__
#define __MASKFUN__

#include "MVInterface.h"

void MakeVectorOcclusionMaskTime(MVClip &mvClip, int nBlkX, int nBlkY, double dMaskNormFactor, double fGamma, int nPel, BYTE * occMask, int occMaskPitch, int time256, int blkSizeX, int blkSizeY);
void VectorMasksToOcclusionMaskTime(BYTE *VXMask, BYTE *VYMask, int nBlkX, int nBlkY, double dMaskNormFactor, double fGamma, int nPel, BYTE * occMask, int occMaskPitch, int time256, int blkSizeX, int blkSizeY);

void MakeVectorOcclusionMask(MVClip &mvClip, int nBlkX, int nBlkY, double dMaskNormFactor, double fGamma, int nPel, BYTE * occMask, int occMaskPitch);

void VectorMasksToOcclusionMask(BYTE *VX, BYTE *VY, int nBlkX, int nBlkY, double fMaskNormFactor, double fGamma, int nPel, BYTE * smallMask);

void MakeVectorSmallMasks(MVClip &mvClip, int nX, int nY, BYTE *VXSmallY, int pitchVXSmallY, BYTE *VYSmallY, int pitchVYSmallY);
void VectorSmallMaskYToHalfUV(BYTE * VSmallY, int nBlkX, int nBlkY, BYTE *VSmallUV, int ratioUV);

void Merge4PlanesToBig(BYTE *pel2Plane, int pel2Pitch, const BYTE *pPlane0, const BYTE *pPlane1,
					  const BYTE *pPlane2, const BYTE * pPlane3, int width, int height, int pitch, bool isse);

void Merge16PlanesToBig(BYTE *pel4Plane, int pel4Pitch,
                    const BYTE *pPlane0, const BYTE *pPlane1, const BYTE *pPlane2, const BYTE * pPlane3,
                    const BYTE *pPlane4, const BYTE *pPlane5, const BYTE *pPlane6, const BYTE * pPlane7,
                    const BYTE *pPlane8, const BYTE *pPlane9, const BYTE *pPlane10, const BYTE * pPlane11,
                    const BYTE *pPlane12, const BYTE * pPlane13, const BYTE *pPlane14, const BYTE * pPlane15,
					int width, int height, int pitch, bool isse);

unsigned char SADToMask(unsigned int sad, unsigned int sadnorm1024);

void Blend(BYTE * pdst, const BYTE * psrc, const BYTE * pref, int height, int width, int dst_pitch, int src_pitch, int ref_pitch, int time256, bool isse);


// lookup table size 256
void Create_LUTV(int time256, int *LUTVB, int *LUTVF);

void FlowInter(BYTE * pdst, int dst_pitch, const BYTE *prefB, const BYTE *prefF, int ref_pitch,
			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
			   int VPitch, int width, int height, int time256, int nPel, int *LUTVB, int *LUTVF);

void FlowInterSimple(BYTE * pdst, int dst_pitch, const BYTE *prefB, const BYTE *prefF, int ref_pitch,
			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
			   int VPitch, int width, int height, int time256, int nPel, int *LUTVB, int *LUTVF);

void FlowInterExtra(BYTE * pdst, int dst_pitch, const BYTE *prefB, const BYTE *prefF, int ref_pitch,
			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
			   int VPitch, int width, int height, int time256, int nPel, int *LUTVB, int * LUTVF,
			   BYTE *VXFullBB, BYTE *VXFullFF, BYTE *VYFullBB, BYTE *VYFullFF);

void FlowInterPel(BYTE * pdst, int dst_pitch, MVPlane *prefB, MVPlane *prefF, int ref_pitch,
			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
			   int VPitch, int width, int height, int time256, int nPel, int *LUTVB, int *LUTVF);

void FlowInterSimplePel(BYTE * pdst, int dst_pitch, MVPlane *prefB, MVPlane *prefF, int ref_pitch,
			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
			   int VPitch, int width, int height, int time256, int nPel, int *LUTVB, int *LUTVF);

void FlowInterExtraPel(BYTE * pdst, int dst_pitch, MVPlane *prefB, MVPlane *prefF, int ref_pitch,
			   BYTE *VXFullB, BYTE *VXFullF, BYTE *VYFullB, BYTE *VYFullF, BYTE *MaskB, BYTE *MaskF,
			   int VPitch, int width, int height, int time256, int nPel, int *LUTVB, int * LUTVF,
			   BYTE *VXFullBB, BYTE *VXFullFF, BYTE *VYFullBB, BYTE *VYFullFF);

#endif
