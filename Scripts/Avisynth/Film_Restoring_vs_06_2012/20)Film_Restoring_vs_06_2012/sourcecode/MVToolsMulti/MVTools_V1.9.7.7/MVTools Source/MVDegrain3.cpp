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

#include "MVDegrain3.h"

MVDegrain3::MVDegrain3(PClip _child, PClip mvbw, PClip mvfw, PClip mvbw2, PClip mvfw2, PClip mvbw3, PClip mvfw3, 
					   int _thSAD, int _thSADC, int _YUVplanes, int _nLimit, PClip _pelclip, int _nIdx, int _nSCD1,
					   int _nSCD2, bool _mmx, bool _isse, int _MaxThreads, int _PreFetch, int _SadMode, IScriptEnvironment* env) :
			MVDegrainBase(_child, 3, _YUVplanes, _nLimit, _pelclip, _nIdx, _mmx, _isse, env, mvfw3, 
                          "MVDegrain3", -1, _MaxThreads, _PreFetch, _SadMode)
{
    // PreFetch max 16 since 10*3*2=60 and 64 is max threads at one time
    if (_PreFetch<1 || _PreFetch>10) env->ThrowError("MVDegrain3: PreFetch must be >=1 and <=10");

   // initialize MVClip's
   for (unsigned int PreFetchNum=0; PreFetchNum<static_cast<unsigned int>(_PreFetch); ++PreFetchNum) {
       pmvClipF[PreFetchNum][2]=new MVClip(mvfw3, _nSCD1, _nSCD2, env, true);
       pmvClipF[PreFetchNum][1]=new MVClip(mvfw2, _nSCD1, _nSCD2, env, true);
       pmvClipF[PreFetchNum][0]=new MVClip(mvfw,  _nSCD1, _nSCD2, env, true);
       pmvClipB[PreFetchNum][0]=new MVClip(mvbw,  _nSCD1, _nSCD2, env, true);
       pmvClipB[PreFetchNum][1]=new MVClip(mvbw2, _nSCD1, _nSCD2, env, true);
       pmvClipB[PreFetchNum][2]=new MVClip(mvbw3, _nSCD1, _nSCD2, env, true);
   }

   CheckSimilarity(*pmvClipF[0][0], "mvfw", env);
   CheckSimilarity(*pmvClipB[0][0], "mvbw", env);
   CheckSimilarity(*pmvClipF[0][1], "mvfw2", env);
   CheckSimilarity(*pmvClipB[0][1], "mvbw2", env);
   CheckSimilarity(*pmvClipF[0][2], "mvfw3", env);
   CheckSimilarity(*pmvClipB[0][2], "mvbw3", env);

   // normalize thSAD
   thSAD  = _thSAD*pmvClipB[0][0]->GetThSCD1()/_nSCD1; // normalize to block SAD
   thSADC = _thSADC*pmvClipB[0][0]->GetThSCD1()/_nSCD1; // chroma

    // find the maximum extent
    unsigned int MaxDelta=static_cast<unsigned int>(pmvClipF[0][2]->GetDeltaFrame());
    if (static_cast<unsigned int>(pmvClipB[0][2]->GetDeltaFrame())>MaxDelta) 
        MaxDelta=static_cast<unsigned int>(pmvClipB[0][2]->GetDeltaFrame());

   // numframes 2*MaxDelta+1, i.e. to cover all possible frames in sliding window
   mvCore->AddFrames(nIdx, (2*MaxDelta)*_PreFetch+1, pmvClipB[0][0]->GetLevelCount(), nWidth, nHeight, nPel, nHPadding, nVPadding,
                     YUVPLANES, _isse, yRatioUV);
}

MVDegrain3::~MVDegrain3()
{

}
