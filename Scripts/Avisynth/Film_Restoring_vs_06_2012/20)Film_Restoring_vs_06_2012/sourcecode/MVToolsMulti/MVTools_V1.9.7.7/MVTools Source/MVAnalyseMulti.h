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

#ifndef __MV_ANALYSEMULTI__
#define __MV_ANALYSEMULTI__

#include "MVAnalyseMultiThread.h"
#include "MVInterface.h"
#include "GroupOfPlanes.h"
#include "commonfunctions.h"
#include "yuy2planes.h"
#include "dct.h"
#include "Critical Section.hpp"

class MVAnalyseMulti : public GenericVideoFilter {

friend class MVAnalyseMultiProcessThread;

protected:
    static unsigned int const MAX_PREFETCH=32;
    static unsigned int const MAX_REFFRAMES=32;
 
    MVAnalysisData analysisData[2*MAX_REFFRAMES];
    MVAnalysisData analysisDataDivided[2*MAX_REFFRAMES];

    /*! \brief Frames of blocks for which motion vectors will be computed */
    GroupOfPlanes *vectorFields[MAX_PREFETCH][2*MAX_REFFRAMES];

    /*! the number of reference frames, i.e. if 1 then 1 backward and 1 forward frame */
    unsigned int const RefFrames;

    // variables for prefetching
    unsigned int const PreFetch;
    unsigned char *pCachedData;
    int CacheFrameNum; // the start of the cache frame

    unsigned int const DeltaMult; // the delta multiplier

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
    bool usePelClip;

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
    DCTClass * DCTc[MAX_PREFETCH][2*MAX_REFFRAMES];

    int headerSize;

    static ThreadFunctions::CriticalSection MVAnalyseMultiCS;

    unsigned int MaxThreads, NumThreads; // max threads and num threads

    // pointers to thread structures and threads, and wait handles
    MVAnalyseMultiProcessThread::ThreadStruct *m_pProcessThreadTS[2*MAX_REFFRAMES];   
	MVAnalyseMultiProcessThread               *m_pProcessThread[2*MAX_REFFRAMES]; 
	HANDLE ProcessThreadWaitHandles[2*MAX_REFFRAMES];

    // variables used in get frame which are declared here to avoid reinstation on each call to get frames
    typedef struct {
        PMVGroupOfFrames *pSrcGOF, *pRefGOF[MAX_PREFETCH];
    } GetFrameVariables;

    GetFrameVariables GFV;

public :

    MVAnalyseMulti(PClip _child, int _blksizex, int _blksizey, int pel, int lv, int st, int stp, int _pelSearch, int _refframes,
                   int lambda, bool chroma, int _lsad, int _pnew, int _plevel, bool _globalmotion, int _overlapx, int _overlapy,
                   const char* _outfilename, int _interType, PClip _clipX2, int _dctmode, int _divide,
                   int _idx, int _sadx264, bool _mmx, bool _isse, int _deltamult, int _MaxThreads, int _PreFetch,
                   IScriptEnvironment* env);
    ~MVAnalyseMulti();

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

    void ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
                                       int const Sharp, int const pixelType, int const nHeight, int const nWidth, 
                                       int const nPel, bool const isse);

};

#endif
