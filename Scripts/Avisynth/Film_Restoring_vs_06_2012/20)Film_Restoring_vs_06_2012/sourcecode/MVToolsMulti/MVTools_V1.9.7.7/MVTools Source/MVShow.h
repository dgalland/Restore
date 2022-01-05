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

#ifndef __MVSHOW__
#define __MVSHOW__

#include "MVInterface.h"
#include "yuy2planes.h"

/*! \brief Filter which show the motion vectors
 */
class MVShow : public GenericVideoFilter, public MVFilter {
private:
    MVClip mvClip;

    /*! \brief Scalar factor by which we multiply the motion vectors before drawing them */
    int nScale;

    /*! \brief choose the plane to show */
    int nPlane;

    /*! \brief threshold over which block's mv isn't shown */
    int nTolerance;

    /*! \brief show sad mode : mean sad is shown instead of mvs */
    bool showSad;

    inline static double BDABS(double x) { return ( x < 0 ) ? -x : x; }

    YUY2Planes * SrcPlanes;
    YUY2Planes * DstPlanes;
    bool isse;

public:
    MVShow(PClip _child, PClip vectors, int _scale, int _plane, int _tol, bool _showsad,
           int nSCD1, int nSCD2, bool mmx, bool isse, IScriptEnvironment* env);
    ~MVShow();
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

    void DrawMVs(unsigned char *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch);
    void DrawMV(unsigned char *pDst, int nDstPitch, int scale, int x, int y, int sizex, int sizey,
                int w, int h, VECTOR vector, int pel);
    void DrawPixel(unsigned char *pDst, int nDstPitch, int x, int y, int w, int h);
};

#endif
