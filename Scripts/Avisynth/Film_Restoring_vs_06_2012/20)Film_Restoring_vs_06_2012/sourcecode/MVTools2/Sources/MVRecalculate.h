// Make a motion compensate temporal denoiser

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

#ifndef __MV_RECALCULATE__
#define __MV_RECALCULATE__

#include "MVInterface.h"
#include "GroupOfPlanes.h"
#include "commonfunctions.h"
#include "yuy2planes.h"
#include "dct.h"

class MVRecalculate : public GenericVideoFilter {
protected:
   MVClip mvClip;
//   PClip pelclip;

/*    MVCore * mvCore; */

   MVAnalysisData analysisData;

   MVAnalysisData analysisDataDivided;

	/*! \brief Frames of blocks for which motion vectors will be computed */
	GroupOfPlanes *vectorFields;

   /*! \brief isse optimisations enabled */
	bool isse;

   /*! \brief motion vecteur cost factor */
   int nLambda;

   /*! \brief search type chosen for refinement in the EPZ */
   SearchType searchType;

   /*! \brief additionnal parameter for this search */
	int nSearchParam; // usually search radius

//	int nPelSearch; // search radius at finest level

	int lsad; // SAD limit for lambda using - added by Fizick
	int pnew; // penalty to cost for new canditate - added by Fizick
	int plen; // penalty factor (similar to lambda) for vector length - added by Fizick
	int plevel; // penalty factors (lambda, plen) level scaling - added by Fizick
//	bool global; // use global motion predictor
	const char* outfilename;// vectors output file
//	PClip pelclip; // upsized source clip with doubled frame width and heigth (used for pel=2)
    int divideExtra; // divide blocks on sublocks with median motion
    int smooth; // smooth vector interpolation or by nearest neighbors

	FILE *outfile;
	short * outfilebuf;

	YUY2Planes * SrcPlanes;
	YUY2Planes * RefPlanes;
	YUY2Planes * Src2xPlanes;
	YUY2Planes * Ref2xPlanes;

	HINSTANCE hinstFFTW3;
	DCTClass * DCTc;

    int headerSize;

    int thSAD;

    MVGroupOfFrames *pSrcGOF, *pRefGOF; //v2.0
    int nModeYUV;

public :

	MVRecalculate(PClip _super, PClip _vectors, int thSAD, int smooth, int _blksizex, int _blksizey,
            int st, int stp, int lambda, bool chroma,
			 int _pnew, int _overlapx, int _overlapy,
			 const char* _outfilename, int _dctmode, int _divide,
			 int _sadx264, bool _isse, IScriptEnvironment* env);
	~MVRecalculate();

   PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

};

#endif
