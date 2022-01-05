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

FakeBlockData::FakeBlockData() : x(0), y(0), pRef(0)
{
}

FakeBlockData::FakeBlockData(int _x, int _y) : x(_x), y(_y)
{
}

FakeBlockData::~FakeBlockData()
{
}

void FakeBlockData::Init(int _x, int _y)
{
    x=_x;
    y=_y;
}

void FakeBlockData::Update(const int *array, const unsigned char *ref, int pitch)
{
    vector.x = array[0];
    vector.y = array[1];
    vector.sadLuma    = array[2];
    vector.sadChromaU = array[3];
    vector.sadChromaV = array[4];
    nSad = vector.sadLuma + vector.sadChromaU + vector.sadChromaV;
    nVariance = array[5];
    nLuma = array[6];
    nRefLuma = array[7];
    nLength = SquareLength(vector);
    nPitch = pitch;
    pRef = ref + x + y * pitch;
}

