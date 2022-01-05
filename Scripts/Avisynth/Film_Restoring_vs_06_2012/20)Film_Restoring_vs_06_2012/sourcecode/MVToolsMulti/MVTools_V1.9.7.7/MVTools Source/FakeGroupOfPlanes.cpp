// Author: Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick - overlap
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

FakeGroupOfPlanes::FakeGroupOfPlanes(int _nWidth, int _nHeight, int _nBlkSizeX, int _nBlkSizeY, int _nLevelCount, int _nPel, int _nOverlapX, int _nOverlapY, int _yRatioUV, int _nBlkX, int _nBlkY)
{
    nLvCount = _nLevelCount;
    // nOverlap = 2;//_nOverlap;
    int nBlkX1 = _nBlkX;//(nWidth - nOverlapX) / (nBlkSizeX - nOverlapX);
    // if ((nWidth - nOverlap) > (nBlkSize - nOverlap)*nBlkX ) nBlkX1++;   
    int nBlkY1 = _nBlkY;//(nHeight - nOverlapY)/ (nBlkSizeY - nOverlapY);
    // if ((nHeight - nOverlap) > (nBlkSize - nOverlap)*nBlkY ) nBlkY1++;  
    nWidth_B = (_nBlkSizeX - _nOverlapX)*nBlkX1 + _nOverlapX;
    nHeight_B = (_nBlkSizeY - _nOverlapY)*nBlkY1 + _nOverlapY;
    yRatioUV_B = _yRatioUV;

    // nWidth_ = nWidth;
    // nHeight_ = nHeight;

    planes = new FakePlaneOfBlocks*[nLvCount];
    planes[0] = new FakePlaneOfBlocks(_nBlkSizeX, _nBlkSizeY, 0, _nPel, _nOverlapX, _nOverlapY, nBlkX1, nBlkY1);
    for (int i = 1; i < nLvCount; ++i )
        planes[i] = new FakePlaneOfBlocks(_nBlkSizeX, _nBlkSizeY, i, 1, _nOverlapX, _nOverlapY, nBlkX1>>i, nBlkY1>>i);
}

FakeGroupOfPlanes::~FakeGroupOfPlanes()
{
    if (planes)
    {
        for ( int i = 0; i < nLvCount; ++i )
            if (planes[i]) delete planes[i];
        delete [] planes;
    }
}

void FakeGroupOfPlanes::Update(const int *array)
{
    const int *pA = array;
    validity = GetValidity(array);

    pA += 2;
    for ( int i = nLvCount - 1; i >= 0; --i )
        pA += pA[0];

    pA++;

    compensatedPlane = reinterpret_cast<const unsigned char *>(pA);
    compensatedPlaneU = compensatedPlane + nWidth_B * nHeight_B;
    compensatedPlaneV = compensatedPlaneU + nWidth_B * nHeight_B /(2*yRatioUV_B);

    pA = array;
    pA += 2;
    for ( int i = nLvCount - 1; i >= 0; --i )
    {
        planes[i]->Update(pA + 1, compensatedPlane, nWidth_B);
        pA += pA[0];
    }
}

bool FakeGroupOfPlanes::IsSceneChange(int nThSCD1, int nThSCD2) const
{
    return planes[0]->IsSceneChange(nThSCD1, nThSCD2);
}
