// Create an overlay mask with the motion vectors

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

#ifndef __MV_MASK__
#define __MV_MASK__

#include "MVInterface.h"
//#include "Resizer.h"
#include "SimpleResize.h"
#include "yuy2planes.h"

/*! \brief Filter which shows the motion mask
 */
class MVMask : public GenericVideoFilter, public MVFilter {
private:
	MVClip mvClip;

	double fHalfGamma;
	double fGamma;
	double fMaskNormFactor;
	double fMaskNormFactor2;
//	bool showsad;
	int kind; // new param instead of showsad - Fizick
   unsigned char nSceneChangeValue;

//	unsigned char *LUTBilinear;
	unsigned char *smallMask;
	unsigned char *smallMaskV;
	unsigned char **destinations;

//	Upsizer *upsizer; // old slow bugged resizer replaced by fast SimpleResize (Fizick)
   SimpleResize *upsizer;
   SimpleResize *upsizerUV;

   int nWidthB;
   int nHeightB;
    int nWidthBUV;
   int nHeightBUV;

    int nWidthUV;
   int nHeightUV;

//	bool isInitialized;

//	void ComputeLUTBilinear();
	unsigned char Length(VECTOR v, unsigned char pel);
	unsigned char SAD(unsigned int);
//	void EnlargeMaskBilinear(unsigned char *derp, const int pitch);
//	void Reorganize(unsigned char *d, int dp);

	YUY2Planes * DstPlanes;
	YUY2Planes * SrcPlanes;
	bool isse;
	bool planar;

public:
	MVMask(PClip _child, PClip mvs, double _maxlength, double _gamma, int _kind,
          int _scvalue, int nSCD1, int nSCD2, bool isse, bool _planar, IScriptEnvironment* env);
	~MVMask();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

#endif
