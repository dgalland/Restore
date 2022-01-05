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

MVClipArray::MVClipArray(const AVSValue &vectors, int nSCD1, int nSCD2, IScriptEnvironment *env)
{
   DebugPrintf("Constructing mvClipArray... : %i", vectors.ArraySize());
   size_ = vectors.ArraySize();
   pmvClips = new MVClip*[size_];
   for ( int i = 0; i < size_; i++ )
      pmvClips[i] = new MVClip(vectors[i].AsClip(), nSCD1, nSCD2, env);
}

MVClipArray::~MVClipArray()
{
   for ( int i = 0; i < size_; i++ )
      delete pmvClips[i];
   delete[] pmvClips;
}

// excluded to prevent bug with lost frames
//void MVClipArray::Update(int n, IScriptEnvironment *env) 
//{
//   for ( int i = 0; i < size_; i++ )
//      pmvClips[i]->Update(n, env);
//}
