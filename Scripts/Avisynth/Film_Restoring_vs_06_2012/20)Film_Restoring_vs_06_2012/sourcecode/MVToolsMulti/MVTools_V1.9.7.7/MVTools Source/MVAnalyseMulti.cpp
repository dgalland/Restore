// MVTools
// 2004 Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick - true motion, overlap, YUY2, pelclip, divide
// General classe for motion based filters

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

#include "MVAnalyseMulti.h"
#include <stdio.h>
#include <math.h>
#include "dctfftw.h"
#include "dctint.h"
#include "cpu.h"

using namespace ThreadFunctions;

extern MVCore mvCore;

CriticalSection MVAnalyseMulti::MVAnalyseMultiCS; // create static

MVAnalyseMulti::MVAnalyseMulti(PClip _child, int _blksizex, int _blksizey, int pel, int lv, int st, int stp, 
                               int _pelSearch, int _refframes, int lambda, bool chroma, int _lsad, int _pnew,
                               int _plevel, bool _global, int _overlapx, int _overlapy, const char* _outfilename,
                               int _sharp, PClip _pelclip, int _dctmode, int _divide, int _idx, int _sadx264,
                               bool _mmx, bool _isse, int _deltamult, int _MaxThreads, int _PreFetch,
                               IScriptEnvironment* env) :
                GenericVideoFilter(_child),
                //analysisData(),
                outfilename(_outfilename), pelclip(_pelclip), RefFrames(_refframes), nLambda(lambda), lsad(_lsad), 
                pnew(_pnew), plevel(_plevel), global(_global), divideExtra(_divide), DeltaMult(_deltamult),
                MaxThreads(_MaxThreads), PreFetch(_PreFetch)
{
    if (RefFrames<1 || RefFrames>32) env->ThrowError("MVAnalyseMulti: refframes must be >=1 and <=32");

    if (PreFetch<1 || PreFetch>32) env->ThrowError("MVAnalyseMulti: PreFetch must be >=1 and <=32");

    if (PreFetch*RefFrames>32) env->ThrowError("MVAnalyseMulti: PreFetch*RefFrames<=32");

    if (MaxThreads<1 || MaxThreads>64) env->ThrowError("MVAnalyseMulti: Threads must be >=1 and <=64");

    if (DeltaMult<1 || DeltaMult>32) env->ThrowError("MVAnalyseMulti: deltamult must be >=1 and <=32");

    if ((_blksizex!=4  || _blksizey!=4)  && (_blksizex!=8  || _blksizey!=4) &&
        (_blksizex!=8  || _blksizey!=8)  && (_blksizex!=16 || _blksizey!=2) &&
        (_blksizex!=16 || _blksizey!=8)  && (_blksizex!=16 || _blksizey!=16) &&
        (_blksizex!=32 || _blksizey!=16))
        env->ThrowError("MVAnalyseMulti: Block's size must be 4x4, 8x4, 8x8, 16x2, 16x8, 16x16, 32x16");

    if (!vi.IsYV12() && !vi.IsYUY2())
        env->ThrowError("MVAnalyseMulti: Clip must be YV12 or YUY2");

    if ((pel!= 1) && (pel!= 2) && (pel!= 4))
        env->ThrowError("MVAnalyseMulti: pel has to be 1 or 2 or 4");

    if (_overlapx<0 || _overlapx>=_blksizex || _overlapy<0 || _overlapy>=_blksizey)
        env->ThrowError("MVAnalyseMulti: overlap must be less than block size");

    if (_overlapx%2 || (_overlapy%2>0 && vi.IsYV12()))
        env->ThrowError("MVAnalyseMulti: overlap must be more even");

    if (_divide!=0 && (_blksizex<8 && _blksizey<8))
        env->ThrowError("MVAnalyseMulti: Block sizes must be 8 or more for divide mode");
    if (_divide!=0 && (_overlapx%4 || (_overlapy%4>0 && vi.IsYV12()) || (_overlapy%2>0 && vi.IsYUY2())))
        env->ThrowError("MVAnalyseMulti: overlap must be more even for divide mode");

    mmx = _mmx && ( env->GetCPUFlags() & CPUF_MMX );

    switch (st)
    {
    case 0 :
        searchType = ONETIME;
        nSearchParam = ( stp < 1 ) ? 1 : stp;
        break;
    case 1 :
        searchType = NSTEP;
        nSearchParam = ( stp < 0 ) ? 0 : stp;
        break;
    case 3 :
        searchType = EXHAUSTIVE;
        nSearchParam = ( stp < 1 ) ? 1 : stp;
        break;
    case 2 :
    default :
        searchType = LOGARITHMIC;
        nSearchParam = ( stp < 1 ) ? 1 : stp;
    }

    nPelSearch = ( _pelSearch < pel) ? pel : _pelSearch; // not below value of pel at finest level

    int nBlkX=(vi.width  - _overlapx)/(_blksizex - _overlapx);
    int nBlkY=(vi.height - _overlapy)/(_blksizey - _overlapy);
    int nWidth_B  =nBlkX*(_blksizex - _overlapx) + _overlapx;
    int nHeight_B =nBlkY*(_blksizey - _overlapy) + _overlapy;

    usePelClip = false;
    if (pelclip && pel>=2)
    {
        if (pelclip->GetVideoInfo().width!=vi.width*pel || pelclip->GetVideoInfo().height!=vi.height*pel)
            env->ThrowError("MVAnalyseMulti: pelclip frame size must be Pel of source!");
        else
            usePelClip = true;
    }

    if (lstrlen(outfilename) > 0) {
        fopen_s(&outfile, outfilename,"wb");
        if (outfile == NULL)
            env->ThrowError("MVAnalyse: out file can not be created!");
        else
        {
            outfilebuf = new short[nBlkX*nBlkY*4]; // short vx, short vy, int SAD = 4 words = 8 bytes per block
        }
    }
    else {
        outfile = NULL;
        outfilebuf = NULL;
    }

    headerSize = std::max<int>(4 + sizeof(MVAnalysisData), 256); // include itself, but usually equal to 256 :-)

    // store analysis data in order BX[0], ..., B3, B2, B1[RefFrames-1], F1[RefFrames], F2, F3, ..., FX[2*RefFrames-1] 
    bool IsBackward=true;
    int DeltaFrame=RefFrames*DeltaMult;
    for(unsigned int RefNum=0; RefNum<2*RefFrames; ++RefNum) {
        analysisData[RefNum].nWidth = vi.width;
        analysisData[RefNum].nHeight = vi.height;

        analysisData[RefNum].nBlkSizeX = _blksizex;
        analysisData[RefNum].nBlkSizeY = _blksizey;

        analysisData[RefNum].nPel = pel;

        analysisData[RefNum].nDeltaFrame = DeltaFrame;

        analysisData[RefNum].nOverlapX = _overlapx;
        analysisData[RefNum].nOverlapY = _overlapy;

        analysisData[RefNum].nBlkX = nBlkX;
        analysisData[RefNum].nBlkY = nBlkY;

        analysisData[RefNum].nLvCount = ilog2(	((nBlkX) > (nBlkY)) ? (nBlkY) : (nBlkX) ) - lv;
        analysisData[RefNum].nLvCount = ( analysisData[RefNum].nLvCount < 1 ) ? 1 : analysisData[RefNum].nLvCount;

        analysisData[RefNum].isBackward = IsBackward;

        analysisData[RefNum].pixelType = vi.pixel_type;
        analysisData[RefNum].yRatioUV = (vi.IsYV12()) ? 2 : 1;
        analysisData[RefNum].xRatioUV = 2; // for YV12 and YUY2, really do not used and assumed to 2

        analysisData[RefNum].nFlags = 0;
        analysisData[RefNum].nFlags |= _isse ? MOTION_USE_ISSE : 0;
        analysisData[RefNum].nFlags |= _mmx ? MOTION_USE_MMX : 0;
        analysisData[RefNum].nFlags |= analysisData[RefNum].isBackward ? MOTION_IS_BACKWARD : 0;
        analysisData[RefNum].nFlags |= chroma ? MOTION_USE_CHROMA_MOTION : 0;

        if (_sadx264 == 0)
        {
            analysisData[RefNum].nFlags |= cpu_detect();		
        }
        else
        {
            if ((_sadx264 > 0)&&(_sadx264 <= 12))
            {
                //force specific function
                analysisData[RefNum].nFlags |= CPU_MMXEXT;
                analysisData[RefNum].nFlags |= (_sadx264 == 2) ? CPU_CACHELINE_32 : 0;
                analysisData[RefNum].nFlags |= ((_sadx264 == 3)||(_sadx264 == 5)||(_sadx264 == 7)) ? CPU_CACHELINE_64 : 0;
                analysisData[RefNum].nFlags |= ((_sadx264 == 4)||(_sadx264 == 5)||(_sadx264 ==10)) ? CPU_SSE2_IS_FAST : 0;
                analysisData[RefNum].nFlags |= (_sadx264 == 6) ? CPU_SSE3 : 0;
                analysisData[RefNum].nFlags |= ((_sadx264 == 7)||(_sadx264 >=11)) ? CPU_SSSE3 : 0;
                //beta (debug)
                analysisData[RefNum].nFlags |= (_sadx264 == 8) ? MOTION_USE_SSD : 0;
                analysisData[RefNum].nFlags |= ((_sadx264 >= 9)&&(_sadx264 <= 12)) ? MOTION_USE_SATD : 0;
                analysisData[RefNum].nFlags |= (_sadx264 ==12) ? CPU_PHADD_IS_FAST : 0;			
            }
        }

        analysisData[RefNum].usePelClip = usePelClip;

        analysisData[RefNum].nIdx = _idx;

        analysisData[RefNum].nMagicKey = MOTION_MAGIC_KEY;

        analysisData[RefNum].pmvCore = &mvCore;

        analysisData[RefNum].nHPadding = analysisData[RefNum].nBlkSizeX; //v1.8.1
        analysisData[RefNum].nVPadding = analysisData[RefNum].nBlkSizeY;

        analysisData[RefNum].nVersion = MVANALYSIS_DATA_VERSION; // MVAnalysisData and outfile format version: last update v1.8.1
        //DebugPrintf(" MVAnalyseData size= %d",sizeof(analysisData));

        if (outfile != NULL) {
            fwrite(&analysisData[RefNum], sizeof(MVAnalysisData), 1, outfile);
        }
        analysisData[RefNum].sharp = _sharp; // pel2 interpolation type

        if (divideExtra) { //v1.8.1
            memcpy(&analysisDataDivided[RefNum], &analysisData[RefNum], sizeof(MVAnalysisData));
            analysisDataDivided[RefNum].nBlkX = analysisData[RefNum].nBlkX * 2;
            analysisDataDivided[RefNum].nBlkY = analysisData[RefNum].nBlkY * 2;
            analysisDataDivided[RefNum].nBlkSizeX = analysisData[RefNum].nBlkSizeX / 2;
            analysisDataDivided[RefNum].nBlkSizeY = analysisData[RefNum].nBlkSizeY / 2;
            analysisDataDivided[RefNum].nOverlapX = analysisData[RefNum].nOverlapX / 2;
            analysisDataDivided[RefNum].nOverlapY = analysisData[RefNum].nOverlapY / 2;
            analysisDataDivided[RefNum].nLvCount = analysisData[RefNum].nLvCount + 1;
            analysisDataDivided[RefNum].pmvCore = &mvCore;
        }

        // update is backward and delta frame
        if (IsBackward) {
            DeltaFrame-=DeltaMult;
            if (!(DeltaFrame)) {
                // go forward now
                DeltaFrame=DeltaMult;
                IsBackward=false;
            }
        }
        else DeltaFrame+=DeltaMult;
    }

    // create reference frames, which is 2*maximum offset + 1 for source frame
    mvCore.AddFrames(analysisData[0].nIdx, (2*RefFrames)*DeltaMult*PreFetch+1, analysisData[0].nLvCount,
      		         analysisData[0].nWidth, analysisData[0].nHeight, analysisData[0].nPel, 
                     analysisData[0].nHPadding, analysisData[0].nVPadding, YUVPLANES, _isse,
                     analysisData[0].yRatioUV);

    // create DCT storage and vectorfields
    for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum) {
        for(unsigned int RefNum=0; RefNum<2*RefFrames; ++RefNum) {
            if (_dctmode == 0)
            {
                hinstFFTW3 = NULL;
                DCTc[PreFetchNum][RefNum] = 0;
            }
            else
            {
                if (_isse && (_blksizex == 8) && _blksizey ==8 )
                    DCTc[PreFetchNum][RefNum] = new DCTINT(_blksizex, _blksizey, _dctmode);
                else
                {
                    if (!PreFetchNum && !RefNum) {
                        // load library once
                        hinstFFTW3 = LoadLibrary("fftw3.dll"); // delayed loading
                        if (hinstFFTW3==NULL) env->ThrowError("MVAnalyseMulti: Can not load FFTW3.DLL !");
                    }
                    DCTc[PreFetchNum][RefNum] = new DCTFFTW(_blksizex, _blksizey, hinstFFTW3, _dctmode); // check order x,y
                }
            }

            vectorFields[PreFetchNum][RefNum]=new GroupOfPlanes(analysisData[RefNum].nWidth, analysisData[RefNum].nHeight, 
                                                                analysisData[RefNum].nBlkSizeX, analysisData[RefNum].nBlkSizeY,
                                                                analysisData[RefNum].nLvCount, analysisData[RefNum].nPel, 
                                                                analysisData[RefNum].nFlags,
                                                                analysisData[RefNum].nOverlapX, analysisData[RefNum].nOverlapY, 
                                                                analysisData[RefNum].nBlkX, analysisData[RefNum].nBlkY, 
                                                                analysisData[RefNum].yRatioUV, divideExtra);
        }
    }

    // vector steam packed in
    vi.height = 2*RefFrames;
    vi.width = headerSize/sizeof(int) + vectorFields[0][0]->GetArraySize(); //v1.8.1
    vi.pixel_type = VideoInfo::CS_BGR32;
    vi.audio_samples_per_second = 0; //v1.8.1

    // create array to store cached data wich is row size times number of rows
    if (PreFetch>1) 
        pCachedData=new unsigned char[(vi.width*4)*(2*RefFrames)*(PreFetch-1)];
    else
        pCachedData=0;

    // create get frame class varibales
    GFV.pSrcGOF=new PMVGroupOfFrames[PreFetch];
    for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum)
        GFV.pRefGOF[PreFetchNum]=new PMVGroupOfFrames[2*RefFrames];

    if (divideExtra) { //v1.8.1
        vi.nchannels = reinterpret_cast<int>(&analysisDataDivided);
    }
    else
    {
        //  we'll transmit to the processing filters a handle
        // on the analyzing filter itself ( it's own pointer ), in order
        // to activate the right parameters.
        vi.nchannels = reinterpret_cast<int>(&analysisData);
    }

    // create the thread structures and threads
    NumThreads=std::min<unsigned int>((2*RefFrames)*PreFetch, MaxThreads); // minimum of maximum and number that can be run
    try {

        // create multiple threads
        for(unsigned int ThreadNum=0; ThreadNum<NumThreads; ++ThreadNum) {        
            m_pProcessThreadTS[ThreadNum]=new MVAnalyseMultiProcessThread::ThreadStruct(this);
            m_pProcessThread[ThreadNum]  =new MVAnalyseMultiProcessThread(reinterpret_cast<void*>(m_pProcessThreadTS[ThreadNum]));
        }
    }
    catch(...) {
        // delete the allocated threads and structures
        for(unsigned int ThreadNum=0; ThreadNum<NumThreads; ++ThreadNum) {        
            if (m_pProcessThread[ThreadNum])   delete m_pProcessThread[ThreadNum];
            if (m_pProcessThreadTS[ThreadNum]) delete m_pProcessThreadTS[ThreadNum];
        }
        throw;
    }

    // start the threads
    for(unsigned int ThreadNum=0; ThreadNum<NumThreads; ++ThreadNum) 
        if (m_pProcessThread[ThreadNum]) m_pProcessThread[ThreadNum]->Resume();

    // set cached frame num
    CacheFrameNum=-1000;
}

MVAnalyseMulti::~MVAnalyseMulti()
{
    for(unsigned int ThreadNum=0; ThreadNum<NumThreads; ++ThreadNum) {
        if (m_pProcessThread[ThreadNum])    {
                m_pProcessThread[ThreadNum]->Disable();
                m_pProcessThread[ThreadNum]->ProcessFrame(); // process if stuck waiting
                m_pProcessThread[ThreadNum]->WaitForCompletion(); // wait to complete
                delete m_pProcessThread[ThreadNum]; // delete thread
        }
    }

	// delete the thread structures
    for(unsigned int ThreadNum=0; ThreadNum<NumThreads; ++ThreadNum) {
        if (m_pProcessThreadTS[ThreadNum])  delete m_pProcessThreadTS[ThreadNum];
    }

    for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum) {
        for(unsigned int RefNum=0; RefNum<2*RefFrames; ++RefNum) {
            if (vectorFields[PreFetchNum][RefNum]) delete vectorFields[PreFetchNum][RefNum];
            if (DCTc[PreFetchNum][RefNum])         delete DCTc[PreFetchNum][RefNum];
        }
    }

    if (outfile != NULL) {
        fclose(outfile);
        if (outfilebuf) delete [] outfilebuf;
    }
    if (hinstFFTW3 != NULL) FreeLibrary(hinstFFTW3);

    if (pCachedData) delete [] pCachedData;

    // delete get frame class variables
    if (GFV.pSrcGOF) delete [] GFV.pSrcGOF;
    for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum)
        if (GFV.pRefGOF[PreFetchNum]) delete [] GFV.pRefGOF[PreFetchNum];
}

PVideoFrame __stdcall MVAnalyseMulti::GetFrame(int n, IScriptEnvironment* env)
{
    // if not cahced or n outside last cache range
    if ((PreFetch==1) || (n<=CacheFrameNum) || (n>=CacheFrameNum+static_cast<int>(PreFetch))) {
        // calculate cached data
        vi.height = PreFetch*(2*RefFrames);
        PVideoFrame dst = env->NewVideoFrame(vi);
        unsigned char * pDestination = dst->GetWritePtr();
        int DstPitch  = dst->GetPitch();

        for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum) {
            // get source frame
            int SourceFrameNum=n+PreFetchNum;
            if (SourceFrameNum<vi.num_frames) {
                GFV.pSrcGOF[PreFetchNum] = mvCore.GetFrame(analysisData[0].nIdx, SourceFrameNum);
                if (!GFV.pSrcGOF[PreFetchNum]->IsProcessed()) {
                    PVideoFrame src, src2x;
                    src = child->GetFrame(SourceFrameNum, env);
                    GFV.pSrcGOF[PreFetchNum]->SetParity(child->GetParity(SourceFrameNum));
                    if (usePelClip) 
	                    src2x = pelclip->GetFrame(SourceFrameNum, env);

                    ProcessFrameIntoGroupOfFrames(&GFV.pSrcGOF[PreFetchNum], &src, &src2x, analysisData[0].sharp, 
                                                  analysisData[0].pixelType, analysisData[0].nHeight, analysisData[0].nWidth,
                                                  analysisData[0].nPel, isse);
                }

                // get reference frames
                int offset=static_cast<int>(RefFrames*DeltaMult);  // start with backward frames which actually have a forward offset
                for(unsigned int RefNum=0; RefNum<2*RefFrames; ++RefNum) {
                    int RefFrameNum=n+PreFetchNum+offset;
                    if (RefFrameNum>=0 && RefFrameNum<vi.num_frames) {
                        GFV.pRefGOF[PreFetchNum][RefNum] = mvCore.GetFrame(analysisData[0].nIdx, RefFrameNum);
                        if (!GFV.pRefGOF[PreFetchNum][RefNum]->IsProcessed()) {
                            PVideoFrame ref, ref2x;
                            ref = child->GetFrame(RefFrameNum, env);
                            GFV.pRefGOF[PreFetchNum][RefNum]->SetParity(child->GetParity(RefFrameNum));
                            if (usePelClip)
                                ref2x = pelclip->GetFrame(RefFrameNum, env);

                            ProcessFrameIntoGroupOfFrames(&GFV.pRefGOF[PreFetchNum][RefNum], &ref, &ref2x, analysisData[RefNum].sharp,
                                                          analysisData[RefNum].pixelType, analysisData[RefNum].nHeight,
                                                          analysisData[RefNum].nWidth, analysisData[RefNum].nPel, isse);
                        }
                    }
                    offset-=DeltaMult;
                    if (!offset) offset=-static_cast<int>(DeltaMult); // skip 0 since it is the src frame
                }
            }
        }

        if (outfile != NULL) {
            fwrite(&n, sizeof(int), 1, outfile);// write frame number
        }

        // run threads in groups of max threads
        unsigned int ActiveThreads=0;
        bool bFirstTime=true;
        DWORD ThreadCompleted;
        for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum) {
            if (GFV.pSrcGOF[PreFetchNum]) {
                for(unsigned int RefNum=0; RefNum<2*RefFrames; ++RefNum) {
                    unsigned int ThreadNum;
                    if (bFirstTime)ThreadNum=ActiveThreads;
                    else ThreadNum=ThreadCompleted;

                    // populate thread and start it
                    m_pProcessThreadTS[ThreadNum]->Populate(PreFetchNum, RefNum, n, env, pDestination, DstPitch, 
                                                            &GFV.pSrcGOF[PreFetchNum], &GFV.pRefGOF[PreFetchNum][RefNum]);
                    m_pProcessThread[ThreadNum]->ProcessFrame();
                    ProcessThreadWaitHandles[ThreadNum]=m_pProcessThread[ThreadNum]->DoneHandle();
                    ++ActiveThreads;

                    if (bFirstTime) {
                        if (ActiveThreads==NumThreads) {
                            // wait for a thread to finish
                            ThreadCompleted=WaitForMultipleObjects(NumThreads, ProcessThreadWaitHandles, FALSE, INFINITE)-
                                            WAIT_OBJECT_0;
                            --ActiveThreads;
                            bFirstTime=false;
                        }
                    }
                    else {
                        // wait for a thread to finish
                        ThreadCompleted=WaitForMultipleObjects(NumThreads, ProcessThreadWaitHandles, FALSE, INFINITE)-
                                        WAIT_OBJECT_0;
                        --ActiveThreads;
                    }
                }
            }
        }
        // wait for any remaining threads
        if (ActiveThreads) {
            // wait for threads to finish
            if (bFirstTime) {
                // we have not waited yet so only wait on up to ActiveThreads
                WaitForMultipleObjects(ActiveThreads, ProcessThreadWaitHandles, TRUE, INFINITE);
            }
            else {
                // remove last completed signaled handle
                for(unsigned int ThreadNum=0; ThreadNum<NumThreads-1; ++ThreadNum) {
                    if (ThreadNum>=ThreadCompleted) 
                        ProcessThreadWaitHandles[ThreadNum]=ProcessThreadWaitHandles[ThreadNum+1];
                }
                // we have waited so wait on up to NumThreads-1
                WaitForMultipleObjects(NumThreads-1, ProcessThreadWaitHandles, TRUE, INFINITE);
            }
            ActiveThreads=0;
        }

        // give up references to groups of frames since variables are static
        for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum) {
            GFV.pSrcGOF[PreFetchNum]=0;
            for(unsigned int RefNum=0; RefNum<2*RefFrames; ++RefNum) 
                GFV.pRefGOF[PreFetchNum][RefNum]=0;
        }

        if (PreFetch>1) {
            // get return destination frame of real size
            vi.height = 2*RefFrames;
            PVideoFrame dstReturn = env->NewVideoFrame(vi);
            unsigned char * pDestinationReturn = dstReturn->GetWritePtr();
            int DstPitchReturn  = dstReturn->GetPitch();

            // copy data to return frame
            unsigned char *pDst = pDestinationReturn;
            unsigned char const *pSrc = pDestination;
            for(unsigned int RefNum=0; RefNum<2*RefFrames; ++RefNum) {
                memcpy(reinterpret_cast<void*>(pDst), reinterpret_cast<void const*>(pSrc), vi.width*4);
                pDst+=DstPitchReturn;
                pSrc+=DstPitch;
            }

            // copy data to cache which is other vector data
            pDst=pCachedData;
            pSrc=pDestination+(2*RefFrames)*DstPitch; // pointer after first result
            for(unsigned int RefNum=0; RefNum<(2*RefFrames)*(PreFetch-1); ++RefNum) {
                memcpy(reinterpret_cast<void*>(pDst), reinterpret_cast<void const*>(pSrc), vi.width*4);
                pDst+=vi.width*4;
                pSrc+=DstPitch;
            }                

            // set cached frame base
            CacheFrameNum=n;

            // return copied data
            return dstReturn;
        }
        else {
            // return original destination 
            return dst;
        }
    }
    else {
        if (n<vi.num_frames) {
            // return cached data
            vi.height = 2*RefFrames;
            PVideoFrame dst = env->NewVideoFrame(vi);
            unsigned char *pDestination = dst->GetWritePtr();
            int DstPitch = dst->GetPitch();

            // copy data from cache
            unsigned char *pDst=pDestination;
            unsigned char const *pSrc=pCachedData+(2*RefFrames)*(n-CacheFrameNum-1)*(vi.width*4);
            for(unsigned int RefNum=0; RefNum<2*RefFrames; ++RefNum) {
                memcpy(reinterpret_cast<void*>(pDst), reinterpret_cast<void const*>(pSrc), vi.width*4);
                pSrc+=vi.width*4;
                pDst+=DstPitch;
            }

            return dst;
        }
        else 
            return 0;
    }
}

void MVAnalyseMulti::ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
                                                   int const Sharp, int const pixelType, int const nHeight, int const nWidth, 
                                                   int const nPel, bool const isse)
{
    (*srcGOF)->ConvertVideoFrame(src, src2x, pixelType, nWidth, nHeight, nPel, isse);

    PROFILE_START(MOTION_PROFILE_INTERPOLATION);
    (*srcGOF)->Reduce();
    (*srcGOF)->Pad(YUVPLANES);

    if (*src2x) {
        MVFrame *srcFrames = (*srcGOF)->GetFrame(0);
        MVPlane *srcPlaneY = srcFrames->GetPlane(YPLANE);
        srcPlaneY->RefineExt((*srcGOF)->GetVF2xYPtr(), (*srcGOF)->GetVF2xYPitch());
        MVPlane *srcPlaneU = srcFrames->GetPlane(UPLANE);
        srcPlaneU->RefineExt((*srcGOF)->GetVF2xUPtr(), (*srcGOF)->GetVF2xUPitch());
        MVPlane *srcPlaneV = srcFrames->GetPlane(VPLANE);
        srcPlaneV->RefineExt((*srcGOF)->GetVF2xVPtr(), (*srcGOF)->GetVF2xVPitch());
    }
    else
        (*srcGOF)->Refine(YUVPLANES, Sharp);

    PROFILE_STOP(MOTION_PROFILE_INTERPOLATION);

    // set processed
    (*srcGOF)->SetProcessed();
}
