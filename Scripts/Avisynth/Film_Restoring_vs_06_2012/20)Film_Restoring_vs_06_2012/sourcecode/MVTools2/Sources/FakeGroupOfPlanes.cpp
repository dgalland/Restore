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

void FakeGroupOfPlanes::Create(int nBlkSizeX, int nBlkSizeY, int nLevelCount, int nPel, int nOverlapX, int nOverlapY, int _yRatioUV, int _nBlkX, int _nBlkY)
{
   nLvCount_ = nLevelCount;
//   nOverlap = 2;//_nOverlap;
   int nBlkX1 = _nBlkX;//(nWidth - nOverlapX) / (nBlkSizeX - nOverlapX);
//   if ((nWidth - nOverlap) > (nBlkSize - nOverlap)*nBlkX )
//	   nBlkX1++;
   int nBlkY1 = _nBlkY;//(nHeight - nOverlapY)/ (nBlkSizeY - nOverlapY);
//   if ((nHeight - nOverlap) > (nBlkSize - nOverlap)*nBlkY )
//	   nBlkY1++;
   nWidth_B = (nBlkSizeX - nOverlapX)*nBlkX1 + nOverlapX;
   nHeight_B = (nBlkSizeY - nOverlapY)*nBlkY1 + nOverlapY;
   yRatioUV_B = _yRatioUV;

//   nWidth_ = nWidth;
//   nHeight_ = nHeight;

   planes = new FakePlaneOfBlocks*[nLevelCount];
   planes[0] = new FakePlaneOfBlocks(nBlkSizeX, nBlkSizeY, 0, nPel, nOverlapX, nOverlapY, nBlkX1, nBlkY1);
	for (int i = 1; i < nLevelCount; i++ )
		planes[i] = new FakePlaneOfBlocks(nBlkSizeX, nBlkSizeY, i, 1, nOverlapX, nOverlapY, nBlkX1>>i, nBlkY1>>i); // fixed bug with nOverlapX in v1.10.2
   InitializeCriticalSection(&cs);
}

FakeGroupOfPlanes::FakeGroupOfPlanes()
{
   planes = 0;
}

FakeGroupOfPlanes::~FakeGroupOfPlanes()
{
   if ( planes )
   {
      for ( int i = 0; i < nLvCount_; i++ )
		   delete planes[i];
	   delete[] planes;
	   planes = 0; //v1.2.1
   }
   DeleteCriticalSection(&cs);
}

void FakeGroupOfPlanes::Update(const int *array)
{
	EnterCriticalSection(&cs);
   const int *pA = array;
	validity = GetValidity(array);

   pA += 2;
	for ( int i = nLvCount_ - 1; i >= 0; i-- )
		pA += pA[0];

   pA++;

//   compensatedPlane = reinterpret_cast<const unsigned char *>(pA);
//   compensatedPlaneU = compensatedPlane + nWidth_B * nHeight_B;
//   compensatedPlaneV = compensatedPlaneU + nWidth_B * nHeight_B /(2*yRatioUV_B);

   pA = array;
   pA += 2;
	for ( int i = nLvCount_ - 1; i >= 0; i-- )
	{
      planes[i]->Update(pA + 1);
		pA += pA[0];
	}
	LeaveCriticalSection(&cs);
}

bool FakeGroupOfPlanes::IsSceneChange(int nThSCD1, int nThSCD2) const
{
	return planes[0]->IsSceneChange(nThSCD1, nThSCD2);
}
