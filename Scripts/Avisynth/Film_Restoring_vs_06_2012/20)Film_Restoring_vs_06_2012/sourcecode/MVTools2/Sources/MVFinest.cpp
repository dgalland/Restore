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

#include "MVFinest.h"
#include "CopyCode.h"
#include "maskfun.h"


MVFinest::MVFinest(PClip _super, bool _isse, IScriptEnvironment* env) :
GenericVideoFilter(_super)
{

   isse = _isse;

// get parameters of prepared super clip - v2.0
	SuperParams64Bits params;
    memcpy(&params, &child->GetVideoInfo().num_audio_samples, 8);
    int nHeightS = params.nHeight;
    nSuperHPad = params.nHPad;
    nSuperVPad = params.nVPad;
    int nSuperPel = params.nPel;
    int nSuperModeYUV = params.nModeYUV;
    int nSuperLevels = params.nLevels;
    nPel = nSuperPel;
    nSuperWidth = _super->GetVideoInfo().width;
    nSuperHeight = _super->GetVideoInfo().height;
    int nWidth = nSuperWidth - 2*nSuperHPad;
    int nHeight = nHeightS;
    int yRatioUV = vi.IsYUY2() ? 1 : 2;
    pRefGOF = new MVGroupOfFrames(nSuperLevels, nWidth, nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, isse, yRatioUV);

//    if (nHeight != nHeightS || nHeight != vi.height || nWidth != nSuperWidth-nSuperHPad*2 || nWidth != vi.width)
//    		env->ThrowError("MVFinest : different frame sizes of input clips");

	 vi.width = (nWidth + 2*nSuperHPad)*nSuperPel;
	 vi.height = (nHeight + 2*nSuperVPad)*nSuperPel;

}

MVFinest::~MVFinest()
{

	 delete pRefGOF;
}

//-------------------------------------------------------------------------
PVideoFrame __stdcall MVFinest::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame	ref	= child->GetFrame(n, env);
   BYTE *pDst[3];
	const BYTE *pRef[3];
    int nDstPitches[3], nRefPitches[3];
//	int nref;
//	unsigned char *pDstYUY2;
//	int nDstPitchYUY2;

    PVideoFrame dst = env->NewVideoFrame(vi);

	if (nPel==1) // simply copy top lines
	{
	    if ((vi.pixel_type & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2)
	    {
            env->BitBlt(dst->GetWritePtr(), dst->GetPitch(),
                ref->GetReadPtr(), ref->GetPitch(), dst->GetRowSize(), dst->GetHeight());
	    }
	    else // YV12
	    {
			env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
					ref->GetReadPtr(PLANAR_Y), ref->GetPitch(PLANAR_Y), dst->GetRowSize(PLANAR_Y), dst->GetHeight(PLANAR_Y));
			env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U),
					ref->GetReadPtr(PLANAR_U), ref->GetPitch(PLANAR_U), dst->GetRowSize(PLANAR_U), dst->GetHeight(PLANAR_U));
			env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V),
					ref->GetReadPtr(PLANAR_V), ref->GetPitch(PLANAR_V), dst->GetRowSize(PLANAR_V), dst->GetHeight(PLANAR_V));
		}
	}
    else
    {
      PROFILE_START(MOTION_PROFILE_2X);

		if ( (vi.pixel_type & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
		{
            // planar data packed to interleaved format (same as interleved2planar by kassandro) - v2.0.0.5
                pRef[0] = ref->GetReadPtr();
                pRef[1] = pRef[0] + nSuperWidth;
                pRef[2] = pRef[1] + nSuperWidth/2;
                nRefPitches[0] = ref->GetPitch();
                nRefPitches[1] = nRefPitches[0];
                nRefPitches[2] = nRefPitches[0];
                pDst[0] = dst->GetWritePtr();
                pDst[1] = pDst[0] + dst->GetRowSize()/2;
                pDst[2] = pDst[1] + dst->GetRowSize()/4;
                nDstPitches[0] = dst->GetPitch();
                nDstPitches[1] = nDstPitches[0];
                nDstPitches[2] = nDstPitches[0];
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

        pRefGOF->Update(YUVPLANES, (BYTE*)pRef[0], nRefPitches[0], (BYTE*)pRef[1], nRefPitches[1], (BYTE*)pRef[2], nRefPitches[2]);// v2.0

         MVPlane *pPlanes[3];

         pPlanes[0] = pRefGOF->GetFrame(0)->GetPlane(YPLANE);
         pPlanes[1] = pRefGOF->GetFrame(0)->GetPlane(UPLANE);
         pPlanes[2] = pRefGOF->GetFrame(0)->GetPlane(VPLANE);


			if (nPel==2)
			{
			 // merge refined planes to big single plane
			 Merge4PlanesToBig(pDst[0], nDstPitches[0], pPlanes[0]->GetAbsolutePointer(0,0),
				 pPlanes[0]->GetAbsolutePointer(1,0), pPlanes[0]->GetAbsolutePointer(0,1),
				 pPlanes[0]->GetAbsolutePointer(1,1), pPlanes[0]->GetExtendedWidth(),
				 pPlanes[0]->GetExtendedHeight(), pPlanes[0]->GetPitch(), isse);
			 if (pPlanes[1]) // 0 if plane not exist
			 Merge4PlanesToBig(pDst[1], nDstPitches[1], pPlanes[1]->GetAbsolutePointer(0,0),
				 pPlanes[1]->GetAbsolutePointer(1,0), pPlanes[1]->GetAbsolutePointer(0,1),
				 pPlanes[1]->GetAbsolutePointer(1,1), pPlanes[1]->GetExtendedWidth(),
				 pPlanes[1]->GetExtendedHeight(), pPlanes[1]->GetPitch(), isse);
			 if (pPlanes[2])
			 Merge4PlanesToBig(pDst[2], nDstPitches[2], pPlanes[2]->GetAbsolutePointer(0,0),
				 pPlanes[2]->GetAbsolutePointer(1,0), pPlanes[2]->GetAbsolutePointer(0,1),
				 pPlanes[2]->GetAbsolutePointer(1,1), pPlanes[2]->GetExtendedWidth(),
				 pPlanes[2]->GetExtendedHeight(), pPlanes[2]->GetPitch(), isse);
			}
			else if (nPel==4)
			{
			 // merge refined planes to big single plane
			 Merge16PlanesToBig(pDst[0], nDstPitches[0],
                 pPlanes[0]->GetAbsolutePointer(0,0), pPlanes[0]->GetAbsolutePointer(1,0),
                 pPlanes[0]->GetAbsolutePointer(2,0), pPlanes[0]->GetAbsolutePointer(3,0),
                 pPlanes[0]->GetAbsolutePointer(0,1), pPlanes[0]->GetAbsolutePointer(1,1),
                 pPlanes[0]->GetAbsolutePointer(2,1), pPlanes[0]->GetAbsolutePointer(3,1),
                 pPlanes[0]->GetAbsolutePointer(0,2), pPlanes[0]->GetAbsolutePointer(1,2),
                 pPlanes[0]->GetAbsolutePointer(2,2), pPlanes[0]->GetAbsolutePointer(3,2),
                 pPlanes[0]->GetAbsolutePointer(0,3), pPlanes[0]->GetAbsolutePointer(1,3),
                 pPlanes[0]->GetAbsolutePointer(2,3), pPlanes[0]->GetAbsolutePointer(3,3),
                 pPlanes[0]->GetExtendedWidth(), pPlanes[0]->GetExtendedHeight(), pPlanes[0]->GetPitch(), isse);
			 if (pPlanes[1])
			 Merge16PlanesToBig(pDst[1], nDstPitches[1],
                 pPlanes[1]->GetAbsolutePointer(0,0), pPlanes[1]->GetAbsolutePointer(1,0),
                 pPlanes[1]->GetAbsolutePointer(2,0), pPlanes[1]->GetAbsolutePointer(3,0),
                 pPlanes[1]->GetAbsolutePointer(0,1), pPlanes[1]->GetAbsolutePointer(1,1),
                 pPlanes[1]->GetAbsolutePointer(2,1), pPlanes[1]->GetAbsolutePointer(3,1),
                 pPlanes[1]->GetAbsolutePointer(0,2), pPlanes[1]->GetAbsolutePointer(1,2),
                 pPlanes[1]->GetAbsolutePointer(2,2), pPlanes[1]->GetAbsolutePointer(3,2),
                 pPlanes[1]->GetAbsolutePointer(0,3), pPlanes[1]->GetAbsolutePointer(1,3),
                 pPlanes[1]->GetAbsolutePointer(2,3), pPlanes[1]->GetAbsolutePointer(3,3),
				 pPlanes[1]->GetExtendedWidth(), pPlanes[1]->GetExtendedHeight(), pPlanes[1]->GetPitch(), isse);
			 if (pPlanes[2])
			 Merge16PlanesToBig(pDst[2], nDstPitches[2],
                 pPlanes[2]->GetAbsolutePointer(0,0), pPlanes[2]->GetAbsolutePointer(1,0),
                 pPlanes[2]->GetAbsolutePointer(2,0), pPlanes[2]->GetAbsolutePointer(3,0),
                 pPlanes[2]->GetAbsolutePointer(0,1), pPlanes[2]->GetAbsolutePointer(1,1),
                 pPlanes[2]->GetAbsolutePointer(2,1), pPlanes[2]->GetAbsolutePointer(3,1),
                 pPlanes[2]->GetAbsolutePointer(0,2), pPlanes[2]->GetAbsolutePointer(1,2),
                 pPlanes[2]->GetAbsolutePointer(2,2), pPlanes[2]->GetAbsolutePointer(3,2),
                 pPlanes[2]->GetAbsolutePointer(0,3), pPlanes[2]->GetAbsolutePointer(1,3),
                 pPlanes[2]->GetAbsolutePointer(2,3), pPlanes[2]->GetAbsolutePointer(3,3),
				 pPlanes[2]->GetExtendedWidth(), pPlanes[2]->GetExtendedHeight(), pPlanes[2]->GetPitch(), isse);
			}
      PROFILE_STOP(MOTION_PROFILE_2X);


    }

	return dst;

}
