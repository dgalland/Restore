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

#ifndef __MV_ANALYSE__
#define __MV_ANALYSE__

#include "MVInterface.h"
#include "GroupOfPlanes.h"
#include "commonfunctions.h"
#include "yuy2planes.h"
#include "dct.h"
#include "Critical Section.hpp"

class MVAnalyse : public GenericVideoFilter {
protected:
    MVAnalysisData analysisData;

    MVAnalysisData analysisDataDivided;

    /*! \brief Frames of blocks for which motion vectors will be computed */
    GroupOfPlanes *vectorFields;

    /*! \brief mmx optimisations enabled */
    bool mmx;

    /*! \brief isse optimisations enabled */
    bool isse;

    /*! \brief motion vecteur cost factor */
    int nLambda;

    /*! \brief search type chosen for refinement in the EPZ */
    SearchType searchType;

    /*! \brief additionnal parameter for this search */
    int nSearchParam; // usually search radius

    int nPelSearch; // search radius at finest level

    int lsad; // SAD limit for lambda using - added by Fizick
    int pnew; // penalty to cost for new canditate - added by Fizick
    int plen; // penalty factor (similar to lambda) for vector length - added by Fizick
    int plevel; // penalty factors (lambda, plen) level scaling - added by Fizick
    bool global; // use global motion predictor
    const char* outfilename;// vectors output file
    PClip pelclip; // upsized source clip with doubled frame width and heigth (used for pel=2)
    int divideExtra; // divide blocks on sublocks with median motion

    FILE *outfile;
    short * outfilebuf;

    HINSTANCE hinstFFTW3;
    DCTClass * DCTc;

    int headerSize;

public :

    MVAnalyse(PClip _child, int _blksizex, int _blksizey, int pel, int lv, int st, int stp, int _pelSearch, bool isb,
              int lambda, bool chroma, int df, int _lsad, int _pnew, int _plevel, bool _globalmotion,
              int _overlapx, int _overlapy, const char* _outfilename, int _interType, PClip _clipX2,
              int _dctmode, int _divide, int _idx, int _sadx264, bool _mmx, bool _isse, IScriptEnvironment* env);
    ~MVAnalyse();

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
    void ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
                                       int const Sharp, int const pixelType, int const nHeight, int const nWidth, 
                                       int const nPel, bool const isse);
};

#endif
