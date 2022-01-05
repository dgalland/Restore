// Make a motion compensate temporal denoiser
// Copyright(c)2006 A.G.Balakhnin aka Fizick
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

#include "MVDegrainBase.h"
#include "Padding.h"
#include "Interpolation.h"
#include "CopyCode.h"

#include <algorithm>

MVDegrainBase::MVDegrainBase(PClip _child, unsigned int _NumRefFrames, int _YUVplanes, int _nLimit, 
                             PClip _pelclip, int _nIdx, bool _mmx, bool _isse, IScriptEnvironment* env,
                             const PClip &MVFiltervector, const char *filterName, int const MultiIndex, 
                             unsigned int const _MaxThreads, unsigned int const _PreFetch, unsigned int const _SadMode) :
               YUVplanes(_YUVplanes), nLimit(_nLimit), isse(_isse), mmx(_mmx), NumRefFrames(_NumRefFrames),
               pelclip(_pelclip), GenericVideoFilter(_child), MVFilter(MVFiltervector, filterName, env, MultiIndex),
               MaxThreads(_MaxThreads), PreFetch(_PreFetch), SadMode(_SadMode)
{	
    // populate mvfilter
    nIdx=_nIdx;

    if (NumRefFrames<1 || NumRefFrames>32) env->ThrowError("MVDegrainBase: NumRefFrames must be >=1 and <=32");

    if (MaxThreads<1 || MaxThreads>64) env->ThrowError("MVDegrainBase: Threads must be >=1 and <=64");

    usePelClipHere = false;
    if (pelclip && (nPel > 1)) {
        if (pelclip->GetVideoInfo().width != nWidth*nPel || pelclip->GetVideoInfo().height != nHeight*nPel)
            env->ThrowError("MVDegrainBase: pelclip frame size must be Pel of source!");
        else
            usePelClipHere = true;
    }

    dstShortPitch = ((nWidth + 15)/16)*16;
    DstShortSize=dstShortPitch*nHeight;
    for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum) {
        DstPlanes[PreFetchNum] = new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);
        SrcPlanes[PreFetchNum] = new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);

        if (nOverlapX >0 || nOverlapY>0) {
            OverWinsY[PreFetchNum] = new OverlapWindows(nBlkSizeX, nBlkSizeY, nOverlapX, nOverlapY);
            OverWinsU[PreFetchNum] = new OverlapWindows(nBlkSizeX/2, nBlkSizeY/yRatioUV, nOverlapX/2, nOverlapY/yRatioUV);
            OverWinsV[PreFetchNum] = new OverlapWindows(nBlkSizeX/2, nBlkSizeY/yRatioUV, nOverlapX/2, nOverlapY/yRatioUV);
            DstShort[PreFetchNum] = new unsigned short[3*DstShortSize];
        }
    }

    if (isse) {
        switch (nBlkSizeX) {
        case 32:
            if (nBlkSizeY==16) {
                OVERSLUMA = Overlaps32x16_mmx;
                DEGRAINLUMA = Degrain_C<32, 16>;
                if (yRatioUV==2) {
                    OVERSCHROMA = Overlaps16x8_mmx;
                    DEGRAINCHROMA = Degrain_C<16, 8>;
                }
                else {
                    OVERSCHROMA = Overlaps16x16_mmx;
                    DEGRAINCHROMA = Degrain_C<16, 16>;
                }
            }
            break;
        case 16:
            if (nBlkSizeY==16) {
                OVERSLUMA = Overlaps16x16_mmx;;
                DEGRAINLUMA = Degrain_C<16, 16>;
                if (yRatioUV==2) {
                    OVERSCHROMA = Overlaps8x8_mmx;
                    DEGRAINCHROMA = Degrain_C<8, 8>;
                }
                else {//yRatioUV==1
                    OVERSCHROMA = Overlaps8x16_mmx;
                    DEGRAINCHROMA = Degrain_C<8, 16>;
                }
            } else if (nBlkSizeY==8) {
                OVERSLUMA = Overlaps16x8_mmx;
                DEGRAINLUMA = Degrain_C<16, 8>;
                if (yRatioUV==2) {
                    OVERSCHROMA = Overlaps8x4_mmx;
                    DEGRAINCHROMA = Degrain_C<8, 4>;
                }
                else {//yRatioUV==1
                    OVERSCHROMA = Overlaps8x8_mmx;
                    DEGRAINCHROMA = Degrain_C<8, 8>;
                }
            } else if (nBlkSizeY==2) {
                OVERSLUMA = Overlaps16x2_mmx;
                DEGRAINLUMA = Degrain_C<16, 2>;
                if (yRatioUV==2) {
                    OVERSCHROMA = Overlaps8x1_mmx;
                    DEGRAINCHROMA = Degrain_C<8, 1>;
                }
                else {//yRatioUV==1
                    OVERSCHROMA = Overlaps8x2_mmx;
                    DEGRAINCHROMA = Degrain_C<8, 2>;
                }
            }
            break;
        case 4:
            OVERSLUMA = Overlaps4x4_mmx;
            DEGRAINLUMA = Degrain_C<4, 4>;
            if (yRatioUV==2) {
                OVERSCHROMA = Overlaps_C<2, 2>;
                DEGRAINCHROMA = Degrain_C<2, 2>;
            }
            else { //yRatioUV==1
                OVERSCHROMA = Overlaps_C<2, 4>;
                DEGRAINCHROMA = Degrain_C<2, 4>;
            }
            break;
        case 8:
        default:
            if (nBlkSizeX==8) {
                OVERSLUMA = Overlaps8x8_mmx;
                DEGRAINLUMA = Degrain_C<8, 8>;
                if (yRatioUV==2) {
                    OVERSCHROMA = Overlaps4x4_mmx;
                    DEGRAINCHROMA = Degrain_C<4, 4>;
                }
                else {//yRatioUV==1
                    OVERSCHROMA = Overlaps4x8_mmx;
                    DEGRAINCHROMA = Degrain_C<4, 8>;
                }
            } else if (nBlkSizeY==4) {
                OVERSLUMA = Overlaps8x4_mmx;
                DEGRAINLUMA = Degrain_C<8, 4>;
                if (yRatioUV==2) {
                    OVERSCHROMA = Overlaps4x2_mmx;
                    DEGRAINCHROMA = Degrain_C<4, 2>;
                }
                else {
                    OVERSCHROMA = Overlaps4x4_mmx;
                    DEGRAINCHROMA = Degrain_C<4, 4>;
                }
            }
            break;
        }
    }
    else {
        switch (nBlkSizeX) {
        case 32:
            if (nBlkSizeY==16) {
                OVERSLUMA = Overlaps_C<32, 16>;
                DEGRAINLUMA = Degrain_C<32, 16>;
                if (yRatioUV==2) {
                    OVERSCHROMA = Overlaps_C<16, 8>;
                    DEGRAINCHROMA = Degrain_C<16, 8>;
                }
                else {
                    OVERSCHROMA = Overlaps_C<16, 16>;
                    DEGRAINCHROMA = Degrain_C<16, 16>;
                }
            } 
            break;
        case 16:
            if (nBlkSizeY==16) {
                OVERSLUMA = Overlaps_C<16, 16>;
                DEGRAINLUMA = Degrain_C<16, 16>;
                if (yRatioUV==2) {
                    OVERSCHROMA = Overlaps_C<8, 8>;
                    DEGRAINCHROMA = Degrain_C<8, 8>;
                }
                else {//yRatioUV==1
                    OVERSCHROMA = Overlaps_C<8, 16>;
                    DEGRAINCHROMA = Degrain_C<8, 16>;
                }
            } else if (nBlkSizeY==8) {
                OVERSLUMA = Overlaps_C<16, 8>;
                DEGRAINLUMA = Degrain_C<16, 8>;
                if (yRatioUV==2) {
                    OVERSCHROMA = Overlaps_C<8, 4>;
                    DEGRAINCHROMA = Degrain_C<8, 4>;
                }
                else {//yRatioUV==1
                    OVERSCHROMA = Overlaps_C<8, 8>;
                    DEGRAINCHROMA = Degrain_C<8, 8>;
                }
            } else if (nBlkSizeY==2) {
                OVERSLUMA = Overlaps_C<16, 2>;
                DEGRAINLUMA = Degrain_C<16, 2>;
                if (yRatioUV==2) {
                    OVERSCHROMA = Overlaps_C<8, 1>;
                    DEGRAINCHROMA = Degrain_C<8, 1>;
                }
                else {//yRatioUV==1
                    OVERSCHROMA = Overlaps_C<8, 2>;
                    DEGRAINCHROMA = Degrain_C<8, 2>;
                }
            }
            break;
        case 4:
            OVERSLUMA = Overlaps_C<4, 4>;
            DEGRAINLUMA = Degrain_C<4, 4>;
            if (yRatioUV==2) {
                OVERSCHROMA = Overlaps_C<2, 2>;
                DEGRAINCHROMA = Degrain_C<2, 2>;
            }
            else { //yRatioUV==1
                OVERSCHROMA = Overlaps_C<2, 4>;
                DEGRAINCHROMA = Degrain_C<2, 4>;
            }
            break;
        case 8:
            default:
            if (nBlkSizeX==8) {
                OVERSLUMA = Overlaps_C<8, 8>;
                DEGRAINLUMA = Degrain_C<8, 8>;
                if (yRatioUV==2) {
                    OVERSCHROMA = Overlaps_C<4, 4>;
                    DEGRAINCHROMA = Degrain_C<4, 4>;
                }
                else {//yRatioUV==1
                    OVERSCHROMA = Overlaps_C<4, 8>;
                    DEGRAINCHROMA = Degrain_C<4, 8>;
                }
            } else if (nBlkSizeY==4) {
                OVERSLUMA = Overlaps_C<8, 4>;
                DEGRAINLUMA = Degrain_C<8, 4>;
                if (yRatioUV==2) {
                    OVERSCHROMA = Overlaps_C<4, 2>;
                    DEGRAINCHROMA = Degrain_C<4, 2>;
                }
                else {
                    OVERSCHROMA = Overlaps_C<4, 4>;
                    DEGRAINCHROMA = Degrain_C<4, 4>;
                }
            }
            break;
        }
    } 

    // create get frame class varibales
    GFV.src=new PVideoFrame[PreFetch];
    GFV.dst=new PVideoFrame[PreFetch];
    for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum) {
        GFV.pRefBGOF[PreFetchNum]=new PMVGroupOfFrames[NumRefFrames];
        GFV.pRefFGOF[PreFetchNum]=new PMVGroupOfFrames[NumRefFrames];
        GFV.isUsableB[PreFetchNum]=new bool[NumRefFrames];
        GFV.isUsableF[PreFetchNum]=new bool[NumRefFrames];
    }

    // create the thread structures and threads
    NumThreads=std::min<unsigned int>(3*PreFetch, MaxThreads); // minimum of maximum and number that can be run
    try {

        // create multiple threads
        for(unsigned int ThreadNum=0; ThreadNum<NumThreads; ++ThreadNum) {        
            m_pProcessThreadTS[ThreadNum]=new MVDegrainBaseProcessThread::ThreadStruct(this);
            m_pProcessThread[ThreadNum]  =new MVDegrainBaseProcessThread(reinterpret_cast<void*>(m_pProcessThreadTS[ThreadNum]));
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

MVDegrainBase::~MVDegrainBase()
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
        // delete movie clips
        for (unsigned int i=0; i<NumRefFrames; ++i) {
            if (pmvClipB[PreFetchNum][i]) delete pmvClipB[PreFetchNum][i];
            if (pmvClipF[PreFetchNum][i]) delete pmvClipF[PreFetchNum][i];
        }

        if (DstPlanes[PreFetchNum]) delete DstPlanes[PreFetchNum];
        if (SrcPlanes[PreFetchNum]) delete SrcPlanes[PreFetchNum];

        if (nOverlapX >0 || nOverlapY>0) {
            if (OverWinsY[PreFetchNum]) delete OverWinsY[PreFetchNum];
            if (OverWinsU[PreFetchNum]) delete OverWinsU[PreFetchNum];
            if (OverWinsV[PreFetchNum]) delete OverWinsV[PreFetchNum];
            if (DstShort[PreFetchNum])  delete [] DstShort[PreFetchNum];
        }
    }

    // delete get frame class variables
    if (GFV.src) delete [] GFV.src;
    if (GFV.dst) delete [] GFV.dst;
    for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum) {
        if (GFV.pRefBGOF[PreFetchNum])  delete [] GFV.pRefBGOF[PreFetchNum];
        if (GFV.pRefFGOF[PreFetchNum])  delete [] GFV.pRefFGOF[PreFetchNum];
        if (GFV.isUsableB[PreFetchNum]) delete [] GFV.isUsableB[PreFetchNum];
        if (GFV.isUsableF[PreFetchNum]) delete [] GFV.isUsableF[PreFetchNum];
    }
}

void MVDegrainBase::UpdateNumRefFrames(unsigned int const _NumRefFrames, IScriptEnvironment *env)
{
    if (_NumRefFrames<1 || _NumRefFrames>32) env->ThrowError("MVDegrainBase: NumRefFrames must be >=1 and <=32");

    // update refframes
    NumRefFrames=_NumRefFrames;
}

PVideoFrame __stdcall MVDegrainBase::GetFrame(int n, IScriptEnvironment* env)
{
    // if not cached or n outside last cache range
    if ((PreFetch==1) || (n<=CacheFrameNum) || (n>=CacheFrameNum+static_cast<int>(PreFetch))) {
        // calculate all frames and cache results not needed yet
        // get source and destination frames
        for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum) {
            int FrameNum=n+PreFetchNum;
            if (n<vi.num_frames) {
                GFV.src[PreFetchNum] = child->GetFrame(FrameNum, env);
                GFV.dst[PreFetchNum] = env->NewVideoFrame(vi);

                SrcPlanes[PreFetchNum]->ConvertVideoFrameToPlanes(&GFV.src[PreFetchNum], GFV.pSrc[PreFetchNum], 
                                                                  GFV.nSrcPitches[PreFetchNum]);
                DstPlanes[PreFetchNum]->ConvertVideoFrameToPlanes(&GFV.dst[PreFetchNum], GFV.pDst[PreFetchNum], 
                                                                  GFV.nDstPitches[PreFetchNum]);

                // get Reference frames
                for(unsigned int i=0; i<NumRefFrames; ++i) {
                    PVideoFrame pVF_B = pmvClipB[PreFetchNum][i]->GetFrame(n + PreFetchNum, env);
                    pmvClipB[PreFetchNum][i]->Update(pVF_B, env);
                    GFV.isUsableB[PreFetchNum][i] = pmvClipB[PreFetchNum][i]->IsUsable();
                    if (GFV.isUsableB[PreFetchNum][i]) {
                        int offB = (pmvClipB[PreFetchNum][i]->IsBackward() ) ? 1 : -1;
                        offB *= pmvClipB[PreFetchNum][i]->GetDeltaFrame();
                        GFV.pRefBGOF[PreFetchNum][i] = mvCore->GetFrame(nIdx, n + offB + PreFetchNum); 

                        // get the current reference frame information
                        if (!GFV.pRefBGOF[PreFetchNum][i]->IsProcessed()) {
                            PVideoFrame RefB, RefB2x;
                            RefB=child->GetFrame(n + offB + PreFetchNum, env);
                            GFV.pRefBGOF[PreFetchNum][i]->SetParity(child->GetParity(n + offB + PreFetchNum));
                            if (usePelClipHere)
                                RefB2x=pelclip->GetFrame(n + offB + PreFetchNum, env);
                            ProcessFrameIntoGroupOfFrames(&GFV.pRefBGOF[PreFetchNum][i], &RefB, &RefB2x, 
                                                          pmvClipB[PreFetchNum][i]->GetSharp(), pixelType,
                                                          nHeight, nWidth, nPel, isse);
                        }
                    }
                    PVideoFrame pVF_F = pmvClipF[PreFetchNum][i]->GetFrame(n + PreFetchNum, env);
                    pmvClipF[PreFetchNum][i]->Update(pVF_F, env);
                    GFV.isUsableF[PreFetchNum][i] = pmvClipF[PreFetchNum][i]->IsUsable();
                    if (GFV.isUsableF[PreFetchNum][i]) {
                        int offB = (pmvClipF[PreFetchNum][i]->IsBackward() ) ? 1 : -1;
                        offB *= pmvClipF[PreFetchNum][i]->GetDeltaFrame();
                        GFV.pRefFGOF[PreFetchNum][i] = mvCore->GetFrame(nIdx, n + offB + PreFetchNum); 

                        // get the current reference frame information
                        if (!GFV.pRefFGOF[PreFetchNum][i]->IsProcessed()) {
                            PVideoFrame RefF, RefF2x;
                            RefF=child->GetFrame(n + offB + PreFetchNum, env);
                            GFV.pRefFGOF[PreFetchNum][i]->SetParity(child->GetParity(n + offB + PreFetchNum));
                            if (usePelClipHere)
                                RefF2x=pelclip->GetFrame(n + offB + PreFetchNum, env);
                            ProcessFrameIntoGroupOfFrames(&GFV.pRefFGOF[PreFetchNum][i], &RefF, &RefF2x, 
                                                          pmvClipF[PreFetchNum][i]->GetSharp(), pixelType,
                                                          nHeight, nWidth, nPel, isse);
                        }
                    }
                }
            }
        }

        // run threads in groups of max threads
        unsigned int ActiveThreads=0;
        bool bFirstTime=true;
        DWORD ThreadCompleted;
        for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum) {
            if (GFV.src[PreFetchNum]) {
                for(unsigned int PlaneNum=0; PlaneNum<3; ++PlaneNum) {
                    // pick the plane to process
                    MVDegrainBaseProcessThread::PlaneTypes PlaneType;
                    bool bProcess;
                    switch(PlaneNum) {
                    case 0: // luma
                        PlaneType=MVDegrainBaseProcessThread::LUMA;
                        bProcess=YUVplanes & YPLANE;
                        break;
                    case 1: // chroma U
                        PlaneType=MVDegrainBaseProcessThread::CHROMAU;
                        bProcess=YUVplanes & UPLANE;
                        break;
                    case 2: // chroma V
                        PlaneType=MVDegrainBaseProcessThread::CHROMAV;
                        bProcess=YUVplanes & VPLANE;
                        break;
                    }
                    
                    // populate the thread structure and start it
                    unsigned int ThreadNum;
                    if (bFirstTime)ThreadNum=ActiveThreads;
                    else ThreadNum=ThreadCompleted;
                    m_pProcessThreadTS[ThreadNum]->Populate(PreFetchNum, n, env, GFV.pSrc[PreFetchNum], GFV.pDst[PreFetchNum], 
                                                            GFV.nSrcPitches[PreFetchNum], GFV.nDstPitches[PreFetchNum], 
                                                            pmvClipB[PreFetchNum], pmvClipF[PreFetchNum],
                                                            GFV.pRefBGOF[PreFetchNum], GFV.pRefFGOF[PreFetchNum],
                                                            GFV.isUsableB[PreFetchNum], GFV.isUsableF[PreFetchNum],
                                                            PlaneType, bProcess);
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

        // give up references to groups of frames since static
        for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum) {
            for(unsigned int i=0; i<NumRefFrames; ++i) {
                GFV.pRefBGOF[PreFetchNum][i]=0;
                GFV.pRefFGOF[PreFetchNum][i]=0;
            }
        }

        // convert back from planes
        for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum) {
            unsigned char *pDstYUY2 = GFV.dst[PreFetchNum]->GetWritePtr();
            int nDstPitchYUY2 = GFV.dst[PreFetchNum]->GetPitch();
            DstPlanes[PreFetchNum]->YUY2FromPlanes(pDstYUY2, nDstPitchYUY2);
        }

        // store frames to be cached
        for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch-1; ++PreFetchNum) {
            CachedVideoFrames[PreFetchNum]=GFV.dst[PreFetchNum+1];
        }

        // set cached frame base
        CacheFrameNum=n;

        // give up references to video frames since static
        for(unsigned int PreFetchNum=0; PreFetchNum<PreFetch; ++PreFetchNum) {
                GFV.src[PreFetchNum]=0;
                if (PreFetchNum) GFV.dst[PreFetchNum]=0; // 1 up since returning 0
        }

        // return the first frame
        return GFV.dst[0];
    }
    else {
        // return cached data
        if (n<vi.num_frames) {
            return CachedVideoFrames[n-CacheFrameNum-1];
        }
        else 
            return 0;
    }
}

void MVDegrainBase::ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
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

//-------------------------------------------------------------

template <int const width, int const height>
void MVDegrainBase::Degrain_C(BYTE *pDst, int nDstPitch, const BYTE *pSrc, int nSrcPitch,
						      const BYTE **pRefB, int *BPitch, const BYTE **pRefF, int *FPitch,
						      int WSrc, int *WRefB, int *WRefF, unsigned int const NumRefFrames)
{
    for(int h=0; h<height; ++h) {
        for(int x=0; x<width; ++x) {
            int DegrainValue=(*(pSrc++))*WSrc;
            for(unsigned int i=0; i<NumRefFrames; ++i) {
                DegrainValue+=((*(pRefF[i]++))*WRefF[i] + (*(pRefB[i]++))*WRefB[i]);
            }
            *(pDst++) = static_cast<BYTE>((DegrainValue+128)>>8);
        }

        pDst += (nDstPitch-width);
        pSrc += (nSrcPitch-width);
        for(unsigned int i=0; i<NumRefFrames; ++i) {
            pRefB[i] += (BPitch[i]-width);
            pRefF[i] += (FPitch[i]-width);
        }
    }
}

