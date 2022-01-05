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

#include "MVDegrainMulti.h"

MVDegrainMulti::MVDegrainMulti(PClip _child, PClip mvMulti, int _RefFrames, int _thSAD, int _thSADC, int _YUVplanes, int _nLimit,
					          PClip _pelclip, int _nIdx, int _nSCD1, int _nSCD2, bool _mmx, bool _isse, int _MaxThreads,
                              int _PreFetch, int _SadMode, IScriptEnvironment* env) :
			    MVDegrainBase(_child, _RefFrames, _YUVplanes, _nLimit, _pelclip, _nIdx, _mmx, _isse, env, mvMulti, 
                              "MVDegrainMulti", 0, _MaxThreads, _PreFetch, _SadMode), RefFrames(_RefFrames)
{
    if (RefFrames<1 || RefFrames>32) env->ThrowError("MVDegrainMulti: refframes must be >=1 and <=32");

    // get the true number of reference frames
    VideoInfo mvMultivi=mvMulti->GetVideoInfo();
    unsigned int RefFramesAvailable=mvMultivi.height/2;

    // if refframes is greater than MVAnalyseMulti height then limit to height
    if (RefFramesAvailable<RefFrames) {
        RefFrames=RefFramesAvailable;
        UpdateNumRefFrames(RefFrames, env);
    }

    // PreFetch max 21 since 21*3=63 and 64 is max threads at one time
    if (_PreFetch<1 || _PreFetch>21) env->ThrowError("MVDegrainMulti: PreFetch must be >=1 and <=21");

    if (_PreFetch*RefFrames>32) env->ThrowError("MVDegrainMulti: PreFetch*RefFrames<=32");

    // initialize MVClip's which are in order BX, ..., B3, B2, B1, F1, F2, F3, ..., FX in mvMulti
    for (unsigned int PreFetchNum=0; PreFetchNum<static_cast<unsigned int>(_PreFetch); ++PreFetchNum) {
        if (RefFrames<RefFramesAvailable) {
            // we are taking a subset of the mvMulti clip
            for(unsigned int RefNum=0; RefNum<RefFrames; ++RefNum) {
                pmvClipF[PreFetchNum][RefNum]=new MVClip(mvMulti, _nSCD1, _nSCD2, env, true, RefFramesAvailable+RefNum);  
                pmvClipB[PreFetchNum][RefNum]=new MVClip(mvMulti, _nSCD1, _nSCD2, env, true, RefFramesAvailable-RefNum-1);   
            }               
        }
        else {
            // we are taking the full mvMulti clip
            for(unsigned int RefNum=0; RefNum<RefFrames; ++RefNum) {
                pmvClipF[PreFetchNum][RefNum]=new MVClip(mvMulti, _nSCD1, _nSCD2, env, true, RefFrames+RefNum);  
                pmvClipB[PreFetchNum][RefNum]=new MVClip(mvMulti, _nSCD1, _nSCD2, env, true, RefFrames-RefNum-1);   
            }
        }
    }

    // check simularities
    CheckSimilarity(*pmvClipF[0][0], "mvMulti", env); // only need to check one since they are grouped together

    // normalize thSAD
    thSAD  = _thSAD*pmvClipB[0][0]->GetThSCD1()/_nSCD1; // normalize to block SAD
    thSADC = _thSADC*pmvClipB[0][0]->GetThSCD1()/_nSCD1; // chroma

    // find the maximum extent
    unsigned int MaxDelta=static_cast<unsigned int>(pmvClipF[0][RefFrames-1]->GetDeltaFrame());
    if (static_cast<unsigned int>(pmvClipB[0][RefFrames-1]->GetDeltaFrame())>MaxDelta)
        MaxDelta=static_cast<unsigned int>(pmvClipB[0][RefFrames-1]->GetDeltaFrame());

    // numframes 2*MaxDelta+1, i.e. to cover all possible frames in sliding window
    mvCore->AddFrames(nIdx, (2*MaxDelta)*_PreFetch+1, pmvClipB[0][0]->GetLevelCount(), nWidth, nHeight, nPel, nHPadding, nVPadding, 
                      YUVPLANES, _isse, yRatioUV);
}

MVDegrainMulti::~MVDegrainMulti()
{
}
