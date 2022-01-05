// MVTools
// 2004 Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick - true motion, overlap, YUY2, pelclip, divide
// General classe for motion based filters

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

#include "MVMultiExtract.h"

MVMultiExtract::MVMultiExtract(PClip _child, AVSValue _indicies, IScriptEnvironment* env) :
                GenericVideoFilter(_child), NumIndices(_indicies.ArraySize())
{
    if ( NumIndices<1 || NumIndices>=vi.height)
        env->ThrowError("MVMultiExtract : Number of indicies must be between 0 and number of elements in array minus 1");

    // extract the indicies
    for(int IndexNum=0; IndexNum<NumIndices; ++IndexNum) {
        Index[IndexNum]=_indicies[IndexNum].AsInt();
    }

    // check indicies
    for(int IndexNum=0; IndexNum<NumIndices; ++IndexNum) {
        if (Index[IndexNum]<0 || Index[IndexNum]>=vi.height)
            env->ThrowError("MVMultiExtract: Index must be between 0 and number of elements in array minus 1");

        if (IndexNum>0) {
            if (Index[IndexNum]<=Index[IndexNum-1])
                env->ThrowError("MVMultiExtract: Indicies must be in ascending order");
        }
    }

    // adjust pointer to the appropriate index
    MVAnalysisData *pAnalyseFilterArray = reinterpret_cast<MVAnalysisData*>(vi.nchannels);
    if (vi.nchannels >= 0 &&  vi.nchannels < 9) // seems some normal clip instead of vectors
        env->ThrowError("MVMultiExtract: invalid vectors stream");

    // create array to hold indicies
    pAnalysisData=new MVAnalysisData[NumIndices];

    // update video information for number of rows and pointer to analysis data
    vi.height=NumIndices;
    vi.nchannels = reinterpret_cast<int>(pAnalysisData);

    // copy analysis data rows
    for(int IndexNum=0; IndexNum<NumIndices; ++IndexNum) {
        memcpy(reinterpret_cast<void*>(&pAnalysisData[IndexNum]), 
               reinterpret_cast<void const*>(&pAnalyseFilterArray[Index[IndexNum]]),
               sizeof(MVAnalysisData));
    }
}

MVMultiExtract::~MVMultiExtract()
{
    if (pAnalysisData) delete [] pAnalysisData;
}

PVideoFrame __stdcall MVMultiExtract::GetFrame(int n, IScriptEnvironment* env)
{
    //	DebugPrintf("MVAnalyse: Get src frame %d",n);
    PVideoFrame src = child->GetFrame(n, env);
    const BYTE* srcPtr=src->GetReadPtr();
    int SrcPitch=src->GetPitch();
    
    PVideoFrame dst = env->NewVideoFrame(vi);
    unsigned char *pDst = dst->GetWritePtr();
    int DstPitch = dst->GetPitch();

    // extract the appropriate rows out of the frame RGB32 is 4 bytes per pixel
    for(int IndexNum=0; IndexNum<NumIndices; ++IndexNum) {
        memcpy(reinterpret_cast<void*>(pDst+DstPitch*IndexNum), 
               reinterpret_cast<void const*>(srcPtr+SrcPitch*Index[IndexNum]), vi.width*4);
    }

    return dst;
}
