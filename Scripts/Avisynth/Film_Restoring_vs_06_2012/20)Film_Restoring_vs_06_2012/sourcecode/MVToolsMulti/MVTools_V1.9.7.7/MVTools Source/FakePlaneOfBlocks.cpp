// Author: Manao
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
#include "PlaneOfBlocks.h"
#include "commonfunctions.h"

FakePlaneOfBlocks::FakePlaneOfBlocks(int sizeX, int sizeY, int lv, int pel, int _nOverlapX, int _nOverlapY, int _nBlkX, int _nBlkY)
{
    nBlkSizeX = sizeX;
    nBlkSizeY = sizeY;
    nOverlapX = _nOverlapX;
    nOverlapY = _nOverlapY;
    nBlkX = _nBlkX;
    nBlkY = _nBlkY;
    nWidth_Bi = nOverlapX + nBlkX*(nBlkSizeX - nOverlapX);//w;
    nHeight_Bi = nOverlapY + nBlkY*(nBlkSizeY - nOverlapY);//h;
    //   nBlkX = (nWidth_Bi - nOverlapX) / (nBlkSizeX - nOverlapX); // without remainder
    //   nBlkY = (nHeight_Bi - nOverlapY) / (nBlkSizeY - nOverlapY); //
    nBlkCount = nBlkX * nBlkY;
    nPel = pel;

    nLogPel = ilog2(nPel);
    nLogScale = lv;
    nScale = iexp2(nLogScale);

    blocks = new FakeBlockData[nBlkCount];
    int blkYSpace=(nBlkSizeY - nOverlapY);
    int blkXSpace=(nBlkSizeX - nOverlapX);
    int blkIdx=0;
    for ( int j = 0; j < nBlkY; ++j ) {
        int j1 = j * blkYSpace;
        for ( int i = 0; i < nBlkX; ++i)
            blocks[blkIdx++].Init(i * blkXSpace, j1);
    }
}

FakePlaneOfBlocks::~FakePlaneOfBlocks()
{
    if (blocks) delete [] blocks;
}

void FakePlaneOfBlocks::Update(const int *array, const unsigned char *pRef, int nPitch)
{
    array += 0;
    for ( int i = 0; i < nBlkCount; ++i ) {
        blocks[i].Update(array, pRef, nPitch);
        array += PlaneOfBlocks::ARRAY_ELEMENTS;
    }
}

bool FakePlaneOfBlocks::IsSceneChange(int nTh1, int nTh2) const
{
    int sum = 0;
    for ( int i = 0; i < nBlkCount; ++i )
        sum += ( blocks[i].GetSAD() > nTh1 ) ? 1 : 0;

    return ( sum > nTh2 );
}
