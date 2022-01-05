// Make a motion compensate temporal denoiser

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

#include "MVChangeCompensate.h"
#include "CopyCode.h"

MVChangeCompensate::MVChangeCompensate(PClip _child, PClip _video, bool _isse, IScriptEnvironment* env) :
                    GenericVideoFilter(_child), newCompensation(_video)
{
    MVAnalysisData *pAnalyseFilter = reinterpret_cast<MVAnalysisData *>(vi.nchannels);

    nLevelCount = pAnalyseFilter->GetLevelCount();
    nWidth = newCompensation->GetVideoInfo().width;
    nHeight = newCompensation->GetVideoInfo().height;
    pixelType = pAnalyseFilter->GetPixelType();
    yRatioUV = pAnalyseFilter->GetYRatioUV();
    isse = _isse;

    NewPlanes =  new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);
}

MVChangeCompensate::~MVChangeCompensate()
{
    if (NewPlanes) delete NewPlanes;
}

PVideoFrame __stdcall MVChangeCompensate::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src	= child->GetFrame(n, env);
    const BYTE *pSrc = src->GetReadPtr(); // more clear
    const int *s = reinterpret_cast<const int *>(pSrc) + 2;
    int nMvSize = 2;
    for ( int i = 0; i < nLevelCount; i++ ) {
        nMvSize += s[0];
        s += s[0];
    }
    nMvSize++;

    nMvSize *= 4;

    int nNewPitches[3];
    unsigned char *pNew[3];
    PVideoFrame newframe = newCompensation->GetFrame(n, env);
    NewPlanes->ConvertVideoFrameToPlanes(&newframe, pNew, nNewPitches);

    PVideoFrame dst = env->NewVideoFrame(vi);
    BYTE * pDst = dst->GetWritePtr();
    PlaneCopy(pDst, nMvSize, pSrc, nMvSize, nMvSize, 1, isse);
    PlaneCopy(pDst + nMvSize, nWidth, pNew[0], nNewPitches[0], nWidth, nHeight, isse);
    PlaneCopy(pDst + nMvSize + nWidth * nHeight, nWidth / 2, pNew[1], nNewPitches[1], nWidth / 2, nHeight / yRatioUV, isse);
    PlaneCopy(pDst + nMvSize + nWidth * nHeight + nWidth * nHeight / (2*yRatioUV), nWidth / 2, pNew[2], nNewPitches[2], nWidth / 2, nHeight / yRatioUV, isse); // fixed bug in v.1.4.4

    return dst;
}