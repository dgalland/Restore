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
//#include "Padding.h"
#include "MVFinest.h"

MVFlow::MVFlow(PClip _child, PClip super, PClip _mvec, int _time256, int _mode, bool _fields,
                           int nSCD1, int nSCD2, bool _isse, bool _planar, IScriptEnvironment* env) :
GenericVideoFilter(_child),
MVFilter(_mvec, "MFlow", env),
mvClip(_mvec, nSCD1, nSCD2, env)
{
   time256 = _time256;
   mode = _mode;

   isse = _isse;
   fields = _fields;
   planar = _planar;

        SuperParams64Bits params;
        memcpy(&params, &super->GetVideoInfo().num_audio_samples, 8);
        int nHeightS = params.nHeight;
        int nSuperHPad = params.nHPad;
        int nSuperVPad = params.nVPad;
        int nSuperPel = params.nPel;
        int nSuperModeYUV = params.nModeYUV;
        int nSuperLevels = params.nLevels;
        int nSuperWidth = super->GetVideoInfo().width; // really super
        int nSuperHeight = super->GetVideoInfo().height;

        if (nHeight != nHeightS || nWidth != nSuperWidth-nSuperHPad*2)
                env->ThrowError("MFlow : wrong super frame clip");

    if (nPel==1)
        finest = super; // v2.0.9.1
    else
    finest = new MVFinest(super, isse, env);
//    AVSValue cache_args[1] = { finest };
//    finest = env->Invoke("InternalCache", AVSValue(cache_args,1)).AsClip(); // add cache for speed

//   if (nWidth  != vi.width || (nWidth + nHPadding*2)*nPel != finest->GetVideoInfo().width ||
//       nHeight  != vi.height || (nHeight + nVPadding*2)*nPel != finest->GetVideoInfo().height )
//			env->ThrowError("MVFlow: wrong source of finest frame size");

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

	 int CPUF_Resize = env->GetCPUFlags();
	 if (!isse) CPUF_Resize = (CPUF_Resize & !CPUF_INTEGER_SSE) & !CPUF_SSE2;

	 upsizer = new SimpleResize(nWidthP, nHeightP, nBlkXP, nBlkYP, CPUF_Resize);
	 upsizerUV = new SimpleResize(nWidthPUV, nHeightPUV, nBlkXP, nBlkYP, CPUF_Resize);

	 LUTV = new int[256];
	Create_LUTV(time256, LUTV);

   if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 && !planar)
   {
		DstPlanes =  new YUY2Planes(nWidth, nHeight);
   }
}

MVFlow::~MVFlow()
{
   if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2  && !planar)
   {
	delete DstPlanes;
   }

	delete upsizer;
	delete upsizerUV;

	delete [] VXFullY;
	delete [] VXFullUV;
	delete [] VYFullY;
	delete [] VYFullUV;
	delete [] VXSmallY;
	delete [] VYSmallY;
	delete [] VXSmallUV;
	delete [] VYSmallUV;

//	 if (nPel>1)
//	 {
//		 delete [] pel2PlaneY;
//		 delete [] pel2PlaneU;
//		 delete [] pel2PlaneV;
//	 }

	 delete [] LUTV;
//	 delete pRefGOF;
}

void MVFlow::Create_LUTV(int time256, int *LUTV)
{
	for (int v=0; v<256; v++)
		LUTV[v] = ((v-128)*time256)/256;
}



void MVFlow::Shift(BYTE * pdst, int dst_pitch, const BYTE *pref, int ref_pitch,  BYTE *VXFull, int VXPitch,  BYTE *VYFull, int VYPitch, int width, int height, int time256)
{
	// shift mode
	if (nPel==1)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
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
	else if (nPel==2)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
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
	else if (nPel==4)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
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

void MVFlow::Fetch(BYTE * pdst, int dst_pitch, const BYTE *pref, int ref_pitch,  BYTE *VXFull, int VXPitch,  BYTE *VYFull, int VYPitch, int width, int height, int time256)
{
	// fetch mode
	if (nPel==1)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{

//				int vx = ((VXFull[w]-128)*time256)>>8; //fast but not quite correct
//				int vy = ((VYFull[w]-128)*time256)>>8; // shift of negative values result in not same as division

//				int vx = ((VXFull[w]-128)*time256)/256; //correct
//				int vy = ((VYFull[w]-128)*time256)/256;
/*
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
	else if (nPel==2)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
				// use interpolated image

//				int vx = ((VXFull[w]-128)*time256)>>8;
//				int vy = ((VYFull[w]-128)*time256)>>8;

//				int vx = ((VXFull[w]-128)*time256)/256; //correct
//				int vy = ((VYFull[w]-128)*time256)/256;

/*
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
	else if (nPel==4)
	{
		for (int h=0; h<height; h++)
		{
			for (int w=0; w<width; w++)
			{
				// use interpolated image

//				int vx = ((VXFull[w]-128)*time256)>>8;
//				int vy = ((VYFull[w]-128)*time256)>>8;

//				int vx = ((VXFull[w]-128)*time256)/256; //correct
//				int vy = ((VYFull[w]-128)*time256)/256;

/*
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
   PVideoFrame dst, ref;
   BYTE *pDst[3];
	const BYTE *pRef[3];
    int nDstPitches[3], nRefPitches[3];
	int nref;
	unsigned char *pDstYUY2;
	int nDstPitchYUY2;

   int off = mvClip.GetDeltaFrame(); // integer offset of reference frame

   if ( mvClip.IsBackward() )
   {
	   nref = n + off;
   }
   else
   {
		nref = n - off;
   }
	PVideoFrame mvn = mvClip.GetFrame(n, env);
   mvClip.Update(mvn, env);// backward from next to current
   mvn = 0;

   if ( mvClip.IsUsable())
   {
		ref = finest->GetFrame(nref, env);//  ref for  compensation
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
		  }
		  else
		  {
            // planar data packed to interleaved format (same as interleved2planar by kassandro) - v2.0.0.5
                pDst[0] = dst->GetWritePtr();
                pDst[1] = pDst[0] + dst->GetRowSize()/2;
                pDst[2] = pDst[1] + dst->GetRowSize()/4;
                nDstPitches[0] = dst->GetPitch();
                nDstPitches[1] = nDstPitches[0];
                nDstPitches[2] = nDstPitches[0];
		  }
                pRef[0] = ref->GetReadPtr();
                pRef[1] = pRef[0] + ref->GetRowSize()/2;
                pRef[2] = pRef[1] + ref->GetRowSize()/4;
                nRefPitches[0] = ref->GetPitch();
                nRefPitches[1] = nRefPitches[0];
                nRefPitches[2] = nRefPitches[0];
		}
		else
		{

		 pDst[0] = YWPLAN(dst);
         pDst[1] = UWPLAN(dst);
         pDst[2] = VWPLAN(dst);
         nDstPitches[0] = YPITCH(dst);
         nDstPitches[1] = UPITCH(dst);
         nDstPitches[2] = VPITCH(dst);

         pRef[0] = YRPLAN(ref);
         pRef[1] = URPLAN(ref);
         pRef[2] = VRPLAN(ref);
         nRefPitches[0] = YPITCH(ref);
         nRefPitches[1] = UPITCH(ref);
         nRefPitches[2] = VPITCH(ref);
		}


    int nOffsetY = nRefPitches[0] * nVPadding*nPel + nHPadding*nPel;
    int nOffsetUV = nRefPitches[1] * nVPaddingUV*nPel + nHPaddingUV*nPel;


	  // make  vector vx and vy small masks
	 // 1. ATTENTION: vectors are assumed SHORT (|vx|, |vy| < 127) !
	 // 2. they will be zeroed if not
	// 3. added 128 to all values
	MakeVectorSmallMasks(mvClip, nBlkX, nBlkY, VXSmallY, nBlkXP, VYSmallY, nBlkXP);
	if (nBlkXP > nBlkX) // fill right
	{
		for (int j=0; j<nBlkY; j++)
		{
			VXSmallY[j*nBlkXP + nBlkX] = min(VXSmallY[j*nBlkXP + nBlkX-1],128);
			VYSmallY[j*nBlkXP + nBlkX] = VYSmallY[j*nBlkXP + nBlkX-1];
		}
	}
	if (nBlkYP > nBlkY) // fill bottom
	{
		for (int i=0; i<nBlkXP; i++)
		{
			VXSmallY[nBlkXP*nBlkY +i] = VXSmallY[nBlkXP*(nBlkY-1) +i];
			VYSmallY[nBlkXP*nBlkY +i] = min(VYSmallY[nBlkXP*(nBlkY-1) +i],128);
		}
	}

		int fieldShift = 0;
		if (fields && (nPel >= 2) && (off %2 != 0))
		{
			bool paritySrc = finest->GetParity(n);
	 		bool parityRef = finest->GetParity(nref);
			fieldShift = (paritySrc && !parityRef) ? (nPel/2) : ( (parityRef && !paritySrc) ? -(nPel/2) : 0);
			// vertical shift of fields for fieldbased video at finest level pel2
		}

		for (int j=0; j<nBlkYP; j++)
		{
			for (int i=0; i<nBlkXP; i++)
			{
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
			Fetch(pDst[0], nDstPitches[0], pRef[0] + nOffsetY, nRefPitches[0], VXFullY, VPitchY, VYFullY, VPitchY, nWidth, nHeight, time256); //padded
			Fetch(pDst[1], nDstPitches[1], pRef[1] + nOffsetUV, nRefPitches[1], VXFullUV, VPitchUV, VYFullUV, VPitchUV, nWidthUV, nHeightUV, time256);
			Fetch(pDst[2], nDstPitches[2], pRef[2] + nOffsetUV, nRefPitches[2], VXFullUV, VPitchUV, VYFullUV, VPitchUV, nWidthUV, nHeightUV, time256);
		}
		else if (mode==1) // shift mode
		{
			MemZoneSet(pDst[0], 0, nWidth, nHeight, 0, 0, nDstPitches[0]);
			MemZoneSet(pDst[1], 0, nWidthUV, nHeightUV, 0, 0, nDstPitches[1]);
			MemZoneSet(pDst[2], 0, nWidthUV, nHeightUV, 0, 0, nDstPitches[2]);
			Shift(pDst[0], nDstPitches[0], pRef[0] + nOffsetY, nRefPitches[0], VXFullY, VPitchY, VYFullY, VPitchY, nWidth, nHeight, time256);
			Shift(pDst[1], nDstPitches[1], pRef[1] + nOffsetUV, nRefPitches[1], VXFullUV, VPitchUV, VYFullUV, VPitchUV, nWidthUV, nHeightUV, time256);
			Shift(pDst[2], nDstPitches[2], pRef[2] + nOffsetUV, nRefPitches[2], VXFullUV, VPitchUV, VYFullUV, VPitchUV, nWidthUV, nHeightUV, time256);
		}

		if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 && !planar)
		{
			YUY2FromPlanes(pDstYUY2, nDstPitchYUY2, nWidth, nHeight,
								  pDst[0], nDstPitches[0], pDst[1], pDst[2], nDstPitches[1], isse);
		}
		return dst;
   }
   else
   {
       	PVideoFrame	src	= child->GetFrame(n, env);

	   return src;
   }

}
