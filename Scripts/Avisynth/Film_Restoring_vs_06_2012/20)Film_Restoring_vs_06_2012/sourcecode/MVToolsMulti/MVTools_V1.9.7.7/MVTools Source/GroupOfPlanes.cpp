// Author: Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick - overlap, global MV, divide
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

#include "GroupOfPlanes.h"

GroupOfPlanes::GroupOfPlanes(int nWidth, int nHeight, int _nBlkSizeX, int _nBlkSizeY, int _nLevelCount, 
                             int _nPel, int _nFlags, int _nOverlapX, int _nOverlapY, int _nBlkX, int _nBlkY, 
                             int _yRatioUV, int _divideExtra)
{
    nBlkSizeX = _nBlkSizeX;
    nBlkSizeY = _nBlkSizeY;
    nLevelCount = _nLevelCount;
    nPel = _nPel;
    nFlags = _nFlags;
    nOverlapX = _nOverlapX;
    nOverlapY = _nOverlapY;
    yRatioUV = _yRatioUV;
    divideExtra = _divideExtra;

    planes = new PlaneOfBlocks*[nLevelCount];

    int nBlkX = _nBlkX;
    int nBlkY = _nBlkY;

    int nPelCurrent = nPel;
    int nFlagsCurrent = nFlags;

    for ( int i = 0; i < nLevelCount; ++i )
    {
        if (i == nLevelCount-1)
            nFlagsCurrent |= MOTION_SMALLEST_PLANE;
        planes[i] = new PlaneOfBlocks(nBlkX, nBlkY, nBlkSizeX, nBlkSizeY, nPelCurrent, i, nFlagsCurrent, nOverlapX, nOverlapY, yRatioUV);
        nBlkX /= 2;
        nBlkY /= 2;
        nPelCurrent = 1;
    }
}

GroupOfPlanes::~GroupOfPlanes()
{
    if (planes) {
        for ( int i = 0; i < nLevelCount; ++i )
            if (planes[i]) delete planes[i];
        delete [] planes;
    }
}

void GroupOfPlanes::SearchMVs(PMVGroupOfFrames pSrcGOF, PMVGroupOfFrames pRefGOF,
                              SearchType searchType, int nSearchParam, int nPelSearch, int nLambda,
                              int lsad, int pnew, int plevel, bool global, int flags,
                              int *out, short *outfilebuf, int fieldShift, DCTClass * _DCT)
{
    int i;

    nFlags |= flags;

    // write group's size
    out[0] = GetArraySize();

    // write validity : 1 in that case
    out[1] = 1;

    out += 2;

    int fieldShiftCur = (nLevelCount - 1 == 0) ? fieldShift : 0; // may be non zero for finest level only

    VECTOR globalMV; // create and init global motion vector as zero
    globalMV.x = zeroMV.x;
    globalMV.y = zeroMV.y;
    globalMV.sadLuma    = zeroMV.sadLuma;
    globalMV.sadChromaU = zeroMV.sadChromaU;
    globalMV.sadChromaV = zeroMV.sadChromaV;

    int meanLumaChange = 0;

    // Search the motion vectors, for the low details interpolations first
    // Refining the search until we reach the highest detail interpolation.
 //         DebugPrintf("SearchMV level %i", nLevelCount-1);
    planes[nLevelCount - 1]->SearchMVs(pSrcGOF->GetFrame(nLevelCount-1), pRefGOF->GetFrame(nLevelCount-1),
                                       searchType, nSearchParam, nLambda, lsad, pnew, plevel, flags,
                                       out, &globalMV, outfilebuf, fieldShiftCur, _DCT, &meanLumaChange, divideExtra);

    out += planes[nLevelCount - 1]->GetArraySize(divideExtra);

    for ( i = nLevelCount - 2; i >= 0; --i ) {
        int nSearchParamLevel = (i==0) ? nPelSearch : nSearchParam; // special case for finest level
        PROFILE_START(MOTION_PROFILE_PREDICTION);
        if (global) {
            planes[i+1]->EstimateGlobalMVDoubled(&globalMV); // get updated global MV (doubled)
            DebugPrintf("SearchMV globalMV %i, %i", globalMV.x, globalMV.y);
        }
        planes[i]->InterpolatePrediction(*(planes[i+1]));
        PROFILE_STOP(MOTION_PROFILE_PREDICTION);
        fieldShiftCur = (i == 0) ? fieldShift : 0; // may be non zero for finest level only
        //        DebugPrintf("SearchMV level %i", i);
        planes[i]->SearchMVs(pSrcGOF->GetFrame(i), pRefGOF->GetFrame(i),
                             searchType, nSearchParamLevel, nLambda, lsad, pnew, plevel, flags,
                             out, &globalMV, outfilebuf, fieldShiftCur, _DCT, &meanLumaChange, divideExtra);
                             out += planes[i]->GetArraySize(divideExtra);
    }
}

void GroupOfPlanes::WriteDefaultToArray(int *array)
{
    // write group's size
    array[0] = GetArraySize();

    // write validity : unvalid in that case
    array[1] = 0;

    array += 2;

    // write planes
    for (int i = nLevelCount - 1; i >= 0; --i ) {
        array += planes[i]->WriteDefaultToArray(array, divideExtra);
    }
}

int GroupOfPlanes::GetArraySize()
{
    int size = 2; // size, validity
    //	for ( int i = 0; i < nLevelCount; i++ )
    for (int i = nLevelCount - 1; i >= 0; --i )
        size += planes[i]->GetArraySize(divideExtra);

    return size;
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

void GetMedian(int *vx, int *vy, int vx1, int vy1, int vx2, int vy2, int vx3, int vy3)
{ // existant median vector (not mixed)
    *vx = Median3(vx1, vx2, vx3);
    *vy = Median3(vy1, vy2, vy3);
    if ( (*vx == vx1 && *vy == vy1) || (*vx == vx2 && *vy == vy2) ||(*vx == vx3 && *vy == vy3))
        return;
    else {
        *vx = vx1;
        *vy = vy1;
    }
}

void GroupOfPlanes::ExtraDivide(int *out, int flags)
{
    out += 2; // skip full size and validity
    for (int i = nLevelCount - 1; i >= 1; --i ) // skip all levels up to finest estimated
        out += planes[i]->GetArraySize(0);

    int * inp = out + 1; // finest estimated plane
    out += out[0] + 1; // position for divided sublocks data

    int nBlkY = planes[0]->GetnBlkY();
    int const ArrayElem=PlaneOfBlocks::ARRAY_ELEMENTS;
    int nBlkXAE = PlaneOfBlocks::ARRAY_ELEMENTS*planes[0]->GetnBlkX();
   // ARRAY_ELEMENTS stored variables

    int by = 0; // top blocks
    for (int bx = 0; bx<nBlkXAE; bx+=ArrayElem) {
        for (int i=0; i<2; ++i) // vx, vy
        {
            out[bx*2+i] = inp[bx+i]; // top left subblock
            out[bx*2+ArrayElem+i] = inp[bx+i]; // top right subblock
            out[bx*2 + nBlkXAE*2+i] = inp[bx+i]; // bottom left subblock
            out[bx*2+ArrayElem + nBlkXAE*2+i] = inp[bx+i]; // bottom right subblock
        }
        for (int i=2; i<ArrayElem; ++i) // sads, var, luma, ref
        {
            out[bx*2+i] = (inp[bx+i]+2)>>2; // top left subblock
            out[bx*2+ArrayElem+i] = (inp[bx+i]+2)>>2; // top right subblock
            out[bx*2 + nBlkXAE*2+i] = (inp[bx+i]+2)>>2; // bottom left subblock
            out[bx*2+ArrayElem + nBlkXAE*2+i] = (inp[bx+i]+2)>>2; // bottom right subblock
        }
    }
    out += nBlkXAE*4;
    inp += nBlkXAE;
    for (by = 1; by<nBlkY-1; ++by) // middle blocks
    {
        int bx = 0;
        for (int i=0; i<2; ++i) {
            out[bx*2+i] = inp[bx+i]; // top left subblock
            out[bx*2+ArrayElem+i] = inp[bx+i]; // top right subblock
            out[bx*2 + nBlkXAE*2+i] = inp[bx+i]; // bottom left subblock
            out[bx*2+ArrayElem + nBlkXAE*2+i] = inp[bx+i]; // bottom right subblock
        }
        for (int i=2; i<ArrayElem; ++i) { // sads, var, luma, ref
            out[bx*2+i] = (inp[bx+i]+2)>>2; // top left subblock
            out[bx*2+ArrayElem+i] = (inp[bx+i]+2)>>2; // top right subblock
            out[bx*2 + nBlkXAE*2+i] = (inp[bx+i]+2)>>2; // bottom left subblock
            out[bx*2+ArrayElem + nBlkXAE*2+i] = (inp[bx+i]+2)>>2; // bottom right subblock
        }
        for (bx = ArrayElem; bx<nBlkXAE-ArrayElem; bx+=ArrayElem) {
            if (divideExtra==1) {
                out[bx*2] = inp[bx]; // top left subblock
                out[bx*2+ArrayElem] = inp[bx]; // top right subblock
                out[bx*2 + nBlkXAE*2] = inp[bx]; // bottom left subblock
                out[bx*2+ArrayElem + nBlkXAE*2] = inp[bx]; // bottom right subblock
                out[bx*2+1] = inp[bx+1]; // top left subblock
                out[bx*2+ArrayElem+1] = inp[bx+1]; // top right subblock
                out[bx*2 + nBlkXAE*2+1] = inp[bx+1]; // bottom left subblock
                out[bx*2+ArrayElem + nBlkXAE*2+1] = inp[bx+1]; // bottom right subblock
            }
            else // divideExtra=2
            {
                int vx;
                int vy;
                GetMedian(&vx, &vy, inp[bx], inp[bx+1], inp[bx-ArrayElem], inp[bx+1-ArrayElem], inp[bx-nBlkXAE], inp[bx+1-nBlkXAE]);// top left subblock
                out[bx*2] = vx;
                out[bx*2+1] = vy;
                GetMedian(&vx, &vy, inp[bx], inp[bx+1], inp[bx+ArrayElem], inp[bx+1+ArrayElem], inp[bx-nBlkXAE], inp[bx+1-nBlkXAE]);// top right subblock
                out[bx*2+ArrayElem] = vx;
                out[bx*2+ArrayElem+1] = vy;
                GetMedian(&vx, &vy, inp[bx], inp[bx+1], inp[bx-ArrayElem], inp[bx+1-ArrayElem], inp[bx+nBlkXAE], inp[bx+1+nBlkXAE]);// bottom left subblock
                out[bx*2 + nBlkXAE*2] = vx;
                out[bx*2 + nBlkXAE*2+1] = vy;
                GetMedian(&vx, &vy, inp[bx], inp[bx+1], inp[bx+ArrayElem], inp[bx+1+ArrayElem], inp[bx+nBlkXAE], inp[bx+1+nBlkXAE]);// bottom right subblock
                out[bx*2+ArrayElem + nBlkXAE*2] = vx;
                out[bx*2+ArrayElem + nBlkXAE*2+1] = vy;
            }
            for (int i=2; i<ArrayElem; ++i) { // sads, var, luma, ref
                out[bx*2+i] = (inp[bx+i]+2)>>2; // top left subblock
                out[bx*2+ArrayElem+i] = (inp[bx+i]+2)>>2; // top right subblock
                out[bx*2 + nBlkXAE*2+i] = (inp[bx+i]+2)>>2; // bottom left subblock
                out[bx*2+ArrayElem + nBlkXAE*2+i] = (inp[bx+i]+2)>>2; // bottom right subblock
            }
        }
        bx = nBlkXAE - ArrayElem;
        for (int i=0; i<2; ++i) {
            out[bx*2+i] = inp[bx+i]; // top left subblock
            out[bx*2+ArrayElem+i] = inp[bx+i]; // top right subblock
            out[bx*2 + nBlkXAE*2+i] = inp[bx+i]; // bottom left subblock
            out[bx*2+ArrayElem + nBlkXAE*2+i] = inp[bx+i]; // bottom right subblock
        }
        for (int i=2; i<ArrayElem; ++i) { // sads, var, luma, ref
            out[bx*2+i] = (inp[bx+i]+2)>>2; // top left subblock
            out[bx*2+ArrayElem+i] = (inp[bx+i]+2)>>2; // top right subblock
            out[bx*2 + nBlkXAE*2+i] = (inp[bx+i]+2)>>2; // bottom left subblock
            out[bx*2+ArrayElem + nBlkXAE*2+i] = (inp[bx+i]+2)>>2; // bottom right subblock
        }
        out += nBlkXAE*4;
        inp += nBlkXAE;
    }
    by = nBlkY - 1; // bottom blocks
    for (int bx = 0; bx<nBlkXAE; bx+=ArrayElem) {
        for (int i=0; i<2; ++i) {
            out[bx*2+i] = inp[bx+i]; // top left subblock
            out[bx*2+ArrayElem+i] = inp[bx+i]; // top right subblock
            out[bx*2 + nBlkXAE*2+i] = inp[bx+i]; // bottom left subblock
            out[bx*2+ArrayElem + nBlkXAE*2+i] = inp[bx+i]; // bottom right subblock
        }
        for (int i=2; i<ArrayElem; ++i) { // sads, var, luma, ref
            out[bx*2+i] = (inp[bx+i]+2)>>2; // top left subblock
            out[bx*2+ArrayElem+i] = (inp[bx+i]+2)>>2; // top right subblock
            out[bx*2 + nBlkXAE*2+i] = (inp[bx+i]+2)>>2; // bottom left subblock
            out[bx*2+ArrayElem + nBlkXAE*2+i] = (inp[bx+i]+2)>>2; // bottom right subblock
        }
    }
}
