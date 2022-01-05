// MVTools v2

// Copyright(c)2008 A.G.Balakhnin aka Fizick
// Prepare super clip (hierachical levels data) for MVAnalyse

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

#include "MVSuper.h"
#include <math.h>

// move profile init here from MVCore.cpp (it is not quite correct for several mvsuper)
#ifdef MOTION_PROFILE
int ProfTimeTable[MOTION_PROFILE_COUNT];
int ProfResults[MOTION_PROFILE_COUNT * 2];
__int64 ProfCumulatedResults[MOTION_PROFILE_COUNT * 2];
#endif


MVSuper::MVSuper(PClip _child, int _hPad, int _vPad, int _pel, int _levels, bool _chroma, int _sharp, int _rfilter, PClip _pelclip,
					 bool _isse, bool _planar, IScriptEnvironment* env) :
GenericVideoFilter(_child),
pelclip(_pelclip)
{
    planar = _planar;

	nWidth = vi.width;

	nHeight = vi.height;

	if (!vi.IsYV12() && !vi.IsYUY2())
		env->ThrowError("MSuper: Clip must be YV12 or YUY2");

   nPel = _pel;
	if (( nPel != 1 ) && ( nPel != 2 ) && ( nPel != 4 ))
		env->ThrowError("MSuper: pel has to be 1 or 2 or 4");

   nHPad = _hPad;
   nVPad = _vPad;
   rfilter = _rfilter;
	sharp = _sharp; // pel2 interpolation type
	isse = _isse;

	chroma = _chroma;
    nModeYUV = chroma ? YUVPLANES : YPLANE;

    pixelType = vi.pixel_type;
	yRatioUV = (vi.IsYV12()) ? 2 : 1;
	xRatioUV = 2; // for YV12 and YUY2, really do not used and assumed to 2

    nLevels = _levels;
	int minsize = (nWidth/xRatioUV < nHeight/yRatioUV) ? nWidth/xRatioUV : nHeight/yRatioUV;
	int nLevelsMax = ilog2(minsize);
	if (nLevels<=0 || nLevels> nLevelsMax) nLevels = nLevelsMax;

	usePelClip = false;
   if (pelclip && (nPel >= 2))
   {
		if (pelclip->GetVideoInfo().width == vi.width*nPel &&
		    pelclip->GetVideoInfo().height == vi.height*nPel)
		{
			usePelClip = true;
			isPelClipPadded = false;
		}
		else if (pelclip->GetVideoInfo().width == (vi.width + nHPad*2)*nPel &&
                 pelclip->GetVideoInfo().height == (vi.height+ nVPad*2)*nPel)
		{
			usePelClip = true;
			isPelClipPadded = true;
		}
		else
			env->ThrowError("MSuper: pelclip frame size must be Pel of source!");
   }

    nSuperWidth = nWidth + 2*nHPad;
    nSuperHeight = PlaneSuperOffset(nHeight, nLevels, nPel, nVPad, nSuperWidth)/nSuperWidth;
    if (yRatioUV==2 && nSuperHeight&1) nSuperHeight++; // even
    vi.width = nSuperWidth;
    vi.height = nSuperHeight;

   if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 && !planar)
   {
		SrcPlanes =  new YUY2Planes(nWidth, nHeight);
//		DstPlanes =  new YUY2Planes(nSuperWidth, nSuperHeight); // other size!
		if (usePelClip)
		{
			SrcPelPlanes =  new YUY2Planes(pelclip->GetVideoInfo().width, pelclip->GetVideoInfo().height);
		}
   }

    SuperParams64Bits params;

    params.nHeight = nHeight;
    params.nHPad = nHPad;
    params.nVPad = nVPad;
    params.nPel = nPel;
    params.nModeYUV = nModeYUV;
    params.nLevels = nLevels;


    // pack parameters to fake audio properties
    memcpy(&vi.num_audio_samples, &params, 8); //nHeight + (nHPad<<16) + (nVPad<<24) + ((_int64)(nPel)<<32) + ((_int64)nModeYUV<<40) + ((_int64)nLevels<<48);
    vi.audio_samples_per_second = 0; // kill audio

//    pSrcGOF = new MVGroupOfFrames(nLevels, nWidth, nHeight, nPel, nHPad, nVPad, nModeYUV, isse, yRatioUV);
    pSrcGOF = new MVGroupOfFrames(nLevels, nWidth, nHeight, nPel, nHPad, nVPad, YUVPLANES, isse, yRatioUV);

   PROFILE_INIT();

}

MVSuper::~MVSuper()
{
   if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2  && !planar)
   {
	delete SrcPlanes;
//	delete DstPlanes;
	if (usePelClip)
	{
		delete SrcPelPlanes;
	}
   }
    delete pSrcGOF;

   PROFILE_SHOW();
}

PVideoFrame __stdcall MVSuper::GetFrame(int n, IScriptEnvironment* env)
{
	const unsigned char *pSrcY, *pSrcU, *pSrcV;
	unsigned char *pDstY, *pDstU, *pDstV;
	const unsigned char *pSrcPelY, *pSrcPelU, *pSrcPelV;
//	unsigned char *pDstYUY2;
	int nSrcPitchY, nSrcPitchUV;
	int nDstPitchY, nDstPitchUV;
	int nSrcPelPitchY, nSrcPelPitchUV;
//    int nDstPitchYUY2;

	DebugPrintf("MSuper: Get src frame %d clip %d",n,child);

	PVideoFrame src = child->GetFrame(n, env);
	PVideoFrame srcPel;
	if (usePelClip)
		srcPel = pelclip->GetFrame(n, env);

	PVideoFrame	dst = env->NewVideoFrame(vi);

      PROFILE_START(MOTION_PROFILE_YUY2CONVERT);
	if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
	{
      if (!planar)
      {
		pSrcY = SrcPlanes->GetPtr();
		pSrcU = SrcPlanes->GetPtrU();
		pSrcV = SrcPlanes->GetPtrV();
		nSrcPitchY  = SrcPlanes->GetPitch();
		nSrcPitchUV  = SrcPlanes->GetPitchUV();
		YUY2ToPlanes(src->GetReadPtr(), src->GetPitch(), nWidth, nHeight,
			pSrcY, nSrcPitchY, pSrcU, pSrcV, nSrcPitchUV, isse);
		if (usePelClip)
		{
			pSrcPelY = SrcPelPlanes->GetPtr();
			pSrcPelU = SrcPelPlanes->GetPtrU();
			pSrcPelV = SrcPelPlanes->GetPtrV();
			nSrcPelPitchY  = SrcPelPlanes->GetPitch();
			nSrcPelPitchUV  = SrcPelPlanes->GetPitchUV();
			YUY2ToPlanes(srcPel->GetReadPtr(), srcPel->GetPitch(), srcPel->GetRowSize()/2, srcPel->GetHeight(),
				pSrcPelY, nSrcPelPitchY, pSrcPelU, pSrcPelV, nSrcPelPitchUV, isse);
		}
      }
      else
      {
			pSrcY = src->GetReadPtr();
			pSrcU = pSrcY + src->GetRowSize()/2;
			pSrcV = pSrcU + src->GetRowSize()/4;
			nSrcPitchY  = src->GetPitch();
			nSrcPitchUV  = nSrcPitchY;
        if (usePelClip)
		{
			pSrcPelY = srcPel->GetReadPtr();
			pSrcPelU = pSrcPelY + srcPel->GetRowSize()/2;
			pSrcPelV = pSrcPelU + srcPel->GetRowSize()/4;
			nSrcPelPitchY  = srcPel->GetPitch();
			nSrcPelPitchUV  = nSrcPelPitchY;
		}
      }
//		pDstY = DstPlanes->GetPtr();
//		pDstU = DstPlanes->GetPtrU();
//		pDstV = DstPlanes->GetPtrV();
//		nDstPitchY  = DstPlanes->GetPitch();
//		nDstPitchUV  = DstPlanes->GetPitchUV();
        // planer data packed to interleaved format (same as interleved2planar by kassandro) - v2.0.0.5
		pDstY = dst->GetWritePtr();
		pDstU = pDstY + nSuperWidth;
		pDstV = pDstU + nSuperWidth/2;
		nDstPitchY = dst->GetPitch();
		nDstPitchUV = nDstPitchY;
	}
	else
	{
		pSrcY = src->GetReadPtr(PLANAR_Y);
	    pSrcU = src->GetReadPtr(PLANAR_U);
	    pSrcV = src->GetReadPtr(PLANAR_V);
		nSrcPitchY = src->GetPitch(PLANAR_Y);
	    nSrcPitchUV = src->GetPitch(PLANAR_U);
		if (usePelClip)
		{
			pSrcPelY = srcPel->GetReadPtr(PLANAR_Y);
			pSrcPelU = srcPel->GetReadPtr(PLANAR_U);
			pSrcPelV = srcPel->GetReadPtr(PLANAR_V);
			nSrcPelPitchY = srcPel->GetPitch(PLANAR_Y);
			nSrcPelPitchUV = srcPel->GetPitch(PLANAR_U);
		}
		pDstY = dst->GetWritePtr(PLANAR_Y);
	    pDstU = dst->GetWritePtr(PLANAR_U);
	    pDstV = dst->GetWritePtr(PLANAR_V);
		nDstPitchY  = dst->GetPitch(PLANAR_Y);
		nDstPitchUV  = dst->GetPitch(PLANAR_U);
	}
      PROFILE_STOP(MOTION_PROFILE_YUY2CONVERT);

      PROFILE_START(MOTION_PROFILE_INTERPOLATION);

      pSrcGOF->Update(YUVPLANES, pDstY, nDstPitchY, pDstU, nDstPitchUV, pDstV, nDstPitchUV);

      pSrcGOF->SetPlane(pSrcY, nSrcPitchY, YPLANE);
      pSrcGOF->SetPlane(pSrcU, nSrcPitchUV, UPLANE);
      pSrcGOF->SetPlane(pSrcV, nSrcPitchUV, VPLANE);

      pSrcGOF->Reduce(nModeYUV, rfilter);
      pSrcGOF->Pad(nModeYUV);

	  if (usePelClip)
	  {
		  MVFrame *srcFrames = pSrcGOF->GetFrame(0);
		  MVPlane *srcPlaneY = srcFrames->GetPlane(YPLANE);
		  if (nModeYUV & YPLANE) srcPlaneY->RefineExt(pSrcPelY, nSrcPelPitchY, isPelClipPadded);
		  MVPlane *srcPlaneU = srcFrames->GetPlane(UPLANE);
		  if (nModeYUV & UPLANE) srcPlaneU->RefineExt(pSrcPelU, nSrcPelPitchUV, isPelClipPadded);
		  MVPlane *srcPlaneV = srcFrames->GetPlane(VPLANE);
		  if (nModeYUV & VPLANE) srcPlaneV->RefineExt(pSrcPelV, nSrcPelPitchUV, isPelClipPadded);
	  }
	  else
		  pSrcGOF->Refine(nModeYUV, sharp);

      PROFILE_STOP(MOTION_PROFILE_INTERPOLATION);


/*
      PROFILE_START(MOTION_PROFILE_YUY2CONVERT);

	if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
	{
		pDstYUY2 = dst->GetWritePtr();
		nDstPitchYUY2 = dst->GetPitch();
		YUY2FromPlanes(pDstYUY2, nDstPitchYUY2, nSuperWidth, nSuperHeight,
					  pDstY, nDstPitchY, pDstU, pDstV, nDstPitchUV, isse);
	}
      PROFILE_STOP(MOTION_PROFILE_YUY2CONVERT);
*/

      PROFILE_CUMULATE();

	return dst;
}
