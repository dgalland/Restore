// Author: Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick - overlap, YUY2, sharp
// See legal notice in Copying.txt for more information
//
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

#include "MVInterface.h"

MVClip::MVClip(const PClip &vectors, int _nSCD1, int _nSCD2, IScriptEnvironment *env) :
GenericVideoFilter(vectors)
{
   // we fetch the handle on the analyze filter
   MVAnalysisData *pAnalyseFilter = reinterpret_cast<MVAnalysisData *>(vi.nchannels);
	if (vi.nchannels >= 0 &&  vi.nchannels < 9) // seems some normal clip instead of vectors
	     env->ThrowError("MVTools: invalid vectors stream");

   // 'magic' key, just to check :
   if ( pAnalyseFilter->GetMagicKey() != MOTION_MAGIC_KEY )
      env->ThrowError("MVTools: invalid vectors stream");

   // MVAnalysisData
   nBlkSizeX = pAnalyseFilter->GetBlkSizeX();
   nBlkSizeY = pAnalyseFilter->GetBlkSizeY();
   nPel = pAnalyseFilter->GetPel();
   isBackward = pAnalyseFilter->IsBackward();
   nLvCount = pAnalyseFilter->GetLevelCount();
   nDeltaFrame = pAnalyseFilter->GetDeltaFrame();
   nWidth = pAnalyseFilter->GetWidth();
   nHeight = pAnalyseFilter->GetHeight();
   nMagicKey = pAnalyseFilter->GetMagicKey();
//   nIdx = pAnalyseFilter->GetFramesIdx();
   nOverlapX = pAnalyseFilter->GetOverlapX();
   nOverlapY = pAnalyseFilter->GetOverlapY();
   pixelType = pAnalyseFilter->GetPixelType();
   yRatioUV = pAnalyseFilter->GetYRatioUV();
//   sharp = pAnalyseFilter->GetSharp();
//   usePelClip = pAnalyseFilter->UsePelClip();
    nVPadding = pAnalyseFilter->GetVPadding();
    nHPadding = pAnalyseFilter->GetHPadding();
    nFlags = pAnalyseFilter->GetFlags();

//   pmvCore = pAnalyseFilter->GetMVCore();

   // MVClip
//   nVPadding = nBlkSizeY;
//   nHPadding = nBlkSizeX;
	nBlkX = pAnalyseFilter->GetBlkX();
	nBlkY = pAnalyseFilter->GetBlkY();
   nBlkCount = nBlkX * nBlkY;

   // SCD thresholds
   if ( pAnalyseFilter->IsChromaMotion() )
      nSCD1 = _nSCD1 * (nBlkSizeX * nBlkSizeY) / (8 * 8) * (1 + yRatioUV) / yRatioUV;
   else
      nSCD1 = _nSCD1 * (nBlkSizeX * nBlkSizeY) / (8 * 8);

   nSCD2 = _nSCD2 * nBlkCount / 256;

   // FakeGroupOfPlane creation
   FakeGroupOfPlanes::Create(nBlkSizeX, nBlkSizeY, nLvCount, nPel, nOverlapX, nOverlapY, yRatioUV, nBlkX, nBlkY);
}

MVClip::~MVClip()
{
}
/* disabled in v1.11.4
void MVClip::SetVectorsNeed(bool srcluma, bool refluma, bool var,
                            bool compy, bool compu, bool compv) const
{
   int nFlags = 0;

   nFlags |= srcluma ? MOTION_CALC_SRC_LUMA        : 0;
   nFlags |= refluma ? MOTION_CALC_REF_LUMA        : 0;
   nFlags |= var     ? MOTION_CALC_VAR             : 0;
   nFlags |= compy   ? MOTION_COMPENSATE_LUMA      : 0;
   nFlags |= compu   ? MOTION_COMPENSATE_CHROMA_U  : 0;
   nFlags |= compv   ? MOTION_COMPENSATE_CHROMA_V  : 0;

   MVAnalysisData *pAnalyseFilter = reinterpret_cast<MVAnalysisData *>(vi.nchannels);
   pAnalyseFilter->SetFlags(nFlags);
}
*/
//void MVClip::Update(int n, IScriptEnvironment *env)
void MVClip::Update(PVideoFrame &fn, IScriptEnvironment *env)

{
//	PVideoFrame fn = child->GetFrame(n, env);
    const int *pMv = reinterpret_cast<const int*>(fn->GetReadPtr());
    int _headerSize = pMv[0];
    int nMagicKey1 = pMv[1];
    if (nMagicKey1 != MOTION_MAGIC_KEY)
      env->ThrowError("MVTools: invalid vectors stream");
    int nVersion1 = pMv[2];
    if (nVersion1 != MVANALYSIS_DATA_VERSION)
      env->ThrowError("MVTools: incompatible version of vectors stream");
    pMv += _headerSize/sizeof(int); // go to data - v1.8.1
//   FakeGroupOfPlanes::Update(reinterpret_cast<const int*>(fn->GetReadPtr()));// fixed a bug with lost frames
   FakeGroupOfPlanes::Update(pMv);// fixed a bug with lost frames
}

bool  MVClip::IsUsable(int nSCD1_, int nSCD2_) const
{
   return (!FakeGroupOfPlanes::IsSceneChange(nSCD1_, nSCD2_)) && FakeGroupOfPlanes::IsValid();
}
