// Show the motion vectors of a clip

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

#ifndef __MVSPEED__
#define __MVSPEED__

#include "MVInterface.h"

/*! \brief Filter which show the motion vectors
 */
class MVSpeed : public GenericVideoFilter {
private:
    MVClip mvClip;

public:
    MVSpeed(PClip _child, PClip mvs, bool srcluma, bool refluma, bool var,
            bool compy, bool compu, bool compv, int nSCD1, int nSCD2, bool mmx, bool isse, IScriptEnvironment* env);
    ~MVSpeed();
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

#endif