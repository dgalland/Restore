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


#include "MVDegrainBaseThread.h"
#include "MVDegrainBase.h"
#include "CopyCode.h"

#include <algorithm>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define MVDegrainBase Threads
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// this creates a suspended thread with thread parameters passed to the thread and with stack size StackSize and Thread Attributes 
// ThreadAttributes which will default to Inheritable if NULL
MVDegrainBaseProcessThread::MVDegrainBaseProcessThread(void* ThreadParameters, unsigned int const StackSize, 
                                                       SECURITY_ATTRIBUTES* ThreadAttributes) :
                            Thread(ThreadParameters, StackSize, ThreadAttributes), pTS(0), m_ProcessSemaphore("", 1), 
                            m_DoneSemaphore("", 1)
{
}

// kick of thread process
void MVDegrainBaseProcessThread::ProcessFrame()
{
    m_ProcessSemaphore.Release();
}

// get the done handle for waiting
HANDLE MVDegrainBaseProcessThread::DoneHandle()
{
    return m_DoneSemaphore.Handle();
}

unsigned int MVDegrainBaseProcessThread::Run(void *UserThreadData)
{
    pTS=reinterpret_cast<ThreadStruct*>(UserThreadData);

    ProcessStruct PS; // create process struct

    while(Enabled()) {
        // wait for process sempahore to be enabled
        m_ProcessSemaphore.Wait();

        if (!Enabled()) break; // skip processing

        // call the appropriate template function
        switch(pTS->PlaneType) {
        case LUMA:
            if (pTS->bProcess)   
                Process<true,  false, false>(&PS);
            else {
                // no luma so copy original
                PlaneCopy(pTS->pDst[0], pTS->nDstPitches[0], pTS->pSrc[0], pTS->nSrcPitches[0], 
                          pTS->pMVDegrainBase->nWidth, pTS->pMVDegrainBase->nHeight, pTS->pMVDegrainBase->isse);
            }
            break;
        case CHROMAU:
            if (pTS->bProcess)   
                Process<false, true,  false>(&PS);
            else {
                // no chroma u so copy original
                PlaneCopy(pTS->pDst[1], pTS->nDstPitches[1], pTS->pSrc[1], pTS->nSrcPitches[1], 
                          pTS->pMVDegrainBase->nWidth/2, pTS->pMVDegrainBase->nHeight/pTS->pMVDegrainBase->yRatioUV, 
                          pTS->pMVDegrainBase->isse);
            }
            break;
        case CHROMAV:
            if (pTS->bProcess)   
                Process<false, false, true>(&PS);
            else {
                // no chroma v so copy original
                PlaneCopy(pTS->pDst[2], pTS->nDstPitches[2], pTS->pSrc[2], pTS->nSrcPitches[2],  
                          pTS->pMVDegrainBase->nWidth/2, pTS->pMVDegrainBase->nHeight/pTS->pMVDegrainBase->yRatioUV,
                          pTS->pMVDegrainBase->isse);

            }
            break;
        }

        // indicate done
        m_DoneSemaphore.Release();
    }

    return 0; // end thread
}

// this is the template function instiated for processing the variants
template<bool const bLumaY, bool const bChromaU, bool const bChromaV> 
void MVDegrainBaseProcessThread::Process(ProcessStruct *pPS)
{
    for(unsigned int i=0; i<pTS->pMVDegrainBase->NumRefFrames; ++i) {
        if (pTS->bIsUsableB[i]) {
            if (bLumaY)   pPS->pPlanesB[0][i] = pTS->pRefBGOF[i]->GetFrame(0)->GetPlane(YPLANE);
            if (bChromaU) pPS->pPlanesB[1][i] = pTS->pRefBGOF[i]->GetFrame(0)->GetPlane(UPLANE);
            if (bChromaV) pPS->pPlanesB[2][i] = pTS->pRefBGOF[i]->GetFrame(0)->GetPlane(VPLANE);
        }
        if (pTS->bIsUsableF[i]) {
            if (bLumaY)   pPS->pPlanesF[0][i] = pTS->pRefFGOF[i]->GetFrame(0)->GetPlane(YPLANE);
            if (bChromaU) pPS->pPlanesF[1][i] = pTS->pRefFGOF[i]->GetFrame(0)->GetPlane(UPLANE);
            if (bChromaV) pPS->pPlanesF[2][i] = pTS->pRefFGOF[i]->GetFrame(0)->GetPlane(VPLANE);
        }
    }

    BYTE *pDstCur[3];
    const BYTE *pSrcCur[3];
    for (unsigned int i=0; i<3; ++i) {
        pDstCur[i] = pTS->pDst[i];
        pSrcCur[i] = pTS->pSrc[i];
    }
    // -----------------------------------------------------------------------------
    // -----------------------------------------------------------------------------

    // LUMA plane Y, Chroma U, Chroma V
    bool  bOverlap=!(pTS->pMVDegrainBase->nOverlapX==0 && pTS->pMVDegrainBase->nOverlapY==0);

    int pDstCur_Offset[3], pSrcCur_Offset[3];
    unsigned short *pDstShort[3];
    int pDstShort_Offset[3];
    int xx_Offset;

    int nWidth_B  = pTS->pMVDegrainBase->nBlkX*(pTS->pMVDegrainBase->nBlkSizeX - pTS->pMVDegrainBase->nOverlapX) + 
                    pTS->pMVDegrainBase->nOverlapX;
    int nHeight_B = pTS->pMVDegrainBase->nBlkY*(pTS->pMVDegrainBase->nBlkSizeY - pTS->pMVDegrainBase->nOverlapY) + 
                    pTS->pMVDegrainBase->nOverlapY;
    if (bOverlap) {
        // set destination pointers and clear buffer
        if (bLumaY) {
            pDstShort[0] = pTS->pMVDegrainBase->DstShort[pTS->PreFetchNum];
            MemZoneSet(reinterpret_cast<unsigned char*>(pDstShort[0]), 0, nWidth_B*2, 
                       nHeight_B, 0, 0, pTS->pMVDegrainBase->dstShortPitch*2);
        }
        else {
            pDstShort[0]=0;
        }
        if (bChromaU) {
            pDstShort[1] = pTS->pMVDegrainBase->DstShort[pTS->PreFetchNum]+pTS->pMVDegrainBase->DstShortSize;
            MemZoneSet(reinterpret_cast<unsigned char*>(pDstShort[1]), 0, nWidth_B, 
                       nHeight_B/pTS->pMVDegrainBase->yRatioUV, 0, 0, pTS->pMVDegrainBase->dstShortPitch*2);
        }
        else {
            pDstShort[1]=0;
        }
        if (bChromaV) {
            pDstShort[2] = pTS->pMVDegrainBase->DstShort[pTS->PreFetchNum]+2*pTS->pMVDegrainBase->DstShortSize;
            MemZoneSet(reinterpret_cast<unsigned char*>(pDstShort[2]), 0, nWidth_B, 
                       nHeight_B/pTS->pMVDegrainBase->yRatioUV, 0, 0, pTS->pMVDegrainBase->dstShortPitch*2);
        }
        else {
            pDstShort[2]=0;
        }

        // calculate offsets
        xx_Offset=(pTS->pMVDegrainBase->nBlkSizeX - pTS->pMVDegrainBase->nOverlapX);
        int Offset=(pTS->pMVDegrainBase->nBlkSizeY - pTS->pMVDegrainBase->nOverlapY);
        int Offset2=Offset/pTS->pMVDegrainBase->yRatioUV;
        pSrcCur_Offset[0]=Offset*pTS->nSrcPitches[0];
        pSrcCur_Offset[1]=Offset2*pTS->nSrcPitches[1];
        pSrcCur_Offset[2]=Offset2*pTS->nSrcPitches[2];
        pDstShort_Offset[0]=Offset*pTS->pMVDegrainBase->dstShortPitch;
        pDstShort_Offset[1]=Offset2*pTS->pMVDegrainBase->dstShortPitch;
        pDstShort_Offset[2]=Offset2*pTS->pMVDegrainBase->dstShortPitch;
    }
    else {
        // calculate offsets
        xx_Offset=pTS->pMVDegrainBase->nBlkSizeX;

        int nBlkSizeY_2=pTS->pMVDegrainBase->nBlkSizeY/pTS->pMVDegrainBase->yRatioUV;
        pDstCur_Offset[0]=pTS->pMVDegrainBase->nBlkSizeY*pTS->nDstPitches[0];
        pDstCur_Offset[1]=nBlkSizeY_2*pTS->nDstPitches[1];
        pDstCur_Offset[2]=nBlkSizeY_2*pTS->nDstPitches[2];
        pSrcCur_Offset[0]=pTS->pMVDegrainBase->nBlkSizeY*pTS->nSrcPitches[0];
        pSrcCur_Offset[1]=nBlkSizeY_2*pTS->nSrcPitches[1];
        pSrcCur_Offset[2]=nBlkSizeY_2*pTS->nSrcPitches[2];
    }

    short *winOverY, *winOverU, *winOverV;             // one overlap pointer for Y, U, V
    for (int by=0; by<pTS->pMVDegrainBase->nBlkY; ++by) {
        int xx = 0, xx2 = 0;
        for (int bx=0; bx<pTS->pMVDegrainBase->nBlkX; ++bx) {
            if (bOverlap) {
                // select window
                if ((bx==0 || bx==pTS->pMVDegrainBase->nBlkX-1) ||
                    (by==0 || by==pTS->pMVDegrainBase->nBlkY-1)) {
                    if (by==0 && bx==0) {				               
                        if (bLumaY)   winOverY = pTS->pMVDegrainBase->OverWinsY[pTS->PreFetchNum]->GetWindow(OW_TL); 
                        if (bChromaU) winOverU = pTS->pMVDegrainBase->OverWinsU[pTS->PreFetchNum]->GetWindow(OW_TL);
                        if (bChromaV) winOverV = pTS->pMVDegrainBase->OverWinsV[pTS->PreFetchNum]->GetWindow(OW_TL);
                    }
                    else if (by==0 && bx==pTS->pMVDegrainBase->nBlkX-1) { 
                        if (bLumaY)   winOverY = pTS->pMVDegrainBase->OverWinsY[pTS->PreFetchNum]->GetWindow(OW_TR); 
                        if (bChromaU) winOverU = pTS->pMVDegrainBase->OverWinsU[pTS->PreFetchNum]->GetWindow(OW_TR);
                        if (bChromaV) winOverV = pTS->pMVDegrainBase->OverWinsV[pTS->PreFetchNum]->GetWindow(OW_TR);}
                    else if (by==0) {	            				   
                        if (bLumaY)   winOverY = pTS->pMVDegrainBase->OverWinsY[pTS->PreFetchNum]->GetWindow(OW_TM); 
                        if (bChromaU) winOverU = pTS->pMVDegrainBase->OverWinsU[pTS->PreFetchNum]->GetWindow(OW_TM);
                        if (bChromaV) winOverV = pTS->pMVDegrainBase->OverWinsV[pTS->PreFetchNum]->GetWindow(OW_TM);
                    }
                    else if (by==pTS->pMVDegrainBase->nBlkY-1 && bx==0) { 
                        if (bLumaY)   winOverY = pTS->pMVDegrainBase->OverWinsY[pTS->PreFetchNum]->GetWindow(OW_BL); 
                        if (bChromaU) winOverU = pTS->pMVDegrainBase->OverWinsU[pTS->PreFetchNum]->GetWindow(OW_BL);
                        if (bChromaV) winOverV = pTS->pMVDegrainBase->OverWinsV[pTS->PreFetchNum]->GetWindow(OW_BL);
                    }
                    else if (by==pTS->pMVDegrainBase->nBlkY-1 && bx==pTS->pMVDegrainBase->nBlkX-1) {
                        if (bLumaY)   winOverY = pTS->pMVDegrainBase->OverWinsY[pTS->PreFetchNum]->GetWindow(OW_BR); 
                        if (bChromaU) winOverU = pTS->pMVDegrainBase->OverWinsU[pTS->PreFetchNum]->GetWindow(OW_BR);
                        if (bChromaV) winOverV = pTS->pMVDegrainBase->OverWinsV[pTS->PreFetchNum]->GetWindow(OW_BR);
                    }
                    else if (by==pTS->pMVDegrainBase->nBlkY-1) {		   
                        if (bLumaY)   winOverY = pTS->pMVDegrainBase->OverWinsY[pTS->PreFetchNum]->GetWindow(OW_BM); 
                        if (bChromaU) winOverU = pTS->pMVDegrainBase->OverWinsU[pTS->PreFetchNum]->GetWindow(OW_BM); 
                        if (bChromaV) winOverV = pTS->pMVDegrainBase->OverWinsV[pTS->PreFetchNum]->GetWindow(OW_BM); 
                    }
                    else if (bx==0) {					               
                        if (bLumaY)   winOverY = pTS->pMVDegrainBase->OverWinsY[pTS->PreFetchNum]->GetWindow(OW_ML); 
                        if (bChromaU) winOverU = pTS->pMVDegrainBase->OverWinsU[pTS->PreFetchNum]->GetWindow(OW_ML);
                        if (bChromaV) winOverV = pTS->pMVDegrainBase->OverWinsV[pTS->PreFetchNum]->GetWindow(OW_ML);
                    }
                    else if (bx==pTS->pMVDegrainBase->nBlkX-1) {		   
                        if (bLumaY)   winOverY = pTS->pMVDegrainBase->OverWinsY[pTS->PreFetchNum]->GetWindow(OW_MR); 
                        if (bChromaU) winOverU = pTS->pMVDegrainBase->OverWinsU[pTS->PreFetchNum]->GetWindow(OW_MR);
                        if (bChromaV) winOverV = pTS->pMVDegrainBase->OverWinsV[pTS->PreFetchNum]->GetWindow(OW_MR); 
                    }
                }
                else {    
                    if (bLumaY)   winOverY = pTS->pMVDegrainBase->OverWinsY[pTS->PreFetchNum]->GetWindow(OW_MM); 
                    if (bChromaU) winOverU = pTS->pMVDegrainBase->OverWinsU[pTS->PreFetchNum]->GetWindow(OW_MM); 
                    if (bChromaV) winOverV = pTS->pMVDegrainBase->OverWinsV[pTS->PreFetchNum]->GetWindow(OW_MM); 
                }
            }

            int block = by*pTS->pMVDegrainBase->nBlkX + bx;

            const BYTE* pSrcCurPtr[3];
            pSrcCurPtr[0]=pSrcCur[0] + xx;
            pSrcCurPtr[1]=pSrcCur[1] + xx2;
            pSrcCurPtr[2]=pSrcCur[2] + xx2;
            for(unsigned int i=0; i<pTS->pMVDegrainBase->NumRefFrames; ++i) {
                if (pTS->bIsUsableB[i]) {
                    const FakeBlockData &blockB = pTS->pmvClipB[i]->GetBlock(0, block);
                    int blxB = blockB.GetX() * pTS->pMVDegrainBase->nPel + blockB.GetMV().x;
                    int blyB = blockB.GetY() * pTS->pMVDegrainBase->nPel + blockB.GetMV().y;
                    int blxB_2=blxB>>1;
                    int blyB_2=blyB/pTS->pMVDegrainBase->yRatioUV;
                    switch(pTS->pMVDegrainBase->SadMode) {
                    case 0: // total SAD
                        if (bLumaY)   pPS->WRefB_Y[i] = std::max<int>(0, pTS->pMVDegrainBase->thSAD  - blockB.GetSAD());
                        if (bChromaU) pPS->WRefB_U[i] = std::max<int>(0, pTS->pMVDegrainBase->thSADC - blockB.GetSAD());
                        if (bChromaV) pPS->WRefB_V[i] = std::max<int>(0, pTS->pMVDegrainBase->thSADC - blockB.GetSAD());
                        break;
                    case 1: // individual SAD's
                        if (bLumaY)   pPS->WRefB_Y[i] = std::max<int>(0, pTS->pMVDegrainBase->thSAD  - blockB.GetSADLuma());
                        if (bChromaU) pPS->WRefB_U[i] = std::max<int>(0, pTS->pMVDegrainBase->thSADC - blockB.GetSADChromaU());
                        if (bChromaV) pPS->WRefB_V[i] = std::max<int>(0, pTS->pMVDegrainBase->thSADC - blockB.GetSADChromaV());
                        break;
                    }
                    if (bLumaY) {
                        pPS->pBLY[i]  = pPS->pPlanesB[0][i]->GetPointer(blxB, blyB);
                        pPS->npBLY[i] = pPS->pPlanesB[0][i]->GetPitch();
                    }
                    if (bChromaU) {
                        pPS->pBCU[i]  = pPS->pPlanesB[1][i]->GetPointer(blxB_2, blyB_2);
                        pPS->npBCU[i] = pPS->pPlanesB[1][i]->GetPitch();
                    }
                    if (bChromaV) {
                        pPS->pBCV[i]  = pPS->pPlanesB[2][i]->GetPointer(blxB_2, blyB_2);
                        pPS->npBCV[i] = pPS->pPlanesB[2][i]->GetPitch();
                    }
                }
                else  {
                    if (bLumaY)   pPS->WRefB_Y[i] = 0;
                    if (bChromaU) pPS->WRefB_U[i] = 0;
                    if (bChromaV) pPS->WRefB_V[i] = 0;
                    if (bLumaY) {
                        pPS->pBLY[i]    = pSrcCurPtr[0];
                        pPS->npBLY[i]   = pTS->nSrcPitches[0];
                    }
                    if (bChromaU) {
                        pPS->pBCU[i]    = pSrcCurPtr[1];
                        pPS->npBCU[i]   = pTS->nSrcPitches[1];
                    }
                    if (bChromaV) {
                        pPS->pBCV[i]    = pSrcCurPtr[2];
                        pPS->npBCV[i]   = pTS->nSrcPitches[2];
                    }
                }
                if (pTS->bIsUsableF[i]) {
                    const FakeBlockData &blockF = pTS->pmvClipF[i]->GetBlock(0, block);
                    int blxF = blockF.GetX() * pTS->pMVDegrainBase->nPel + blockF.GetMV().x;
                    int blyF = blockF.GetY() * pTS->pMVDegrainBase->nPel + blockF.GetMV().y;
                    int blxF_2 = blxF>>1;
                    int blyF_2 = blyF/pTS->pMVDegrainBase->yRatioUV;
                    switch(pTS->pMVDegrainBase->SadMode) {
                    case 0: // total SAD
                        if (bLumaY)   pPS->WRefF_Y[i] = std::max<int>(0, pTS->pMVDegrainBase->thSAD  - blockF.GetSAD());
                        if (bChromaU) pPS->WRefF_U[i] = std::max<int>(0, pTS->pMVDegrainBase->thSADC - blockF.GetSAD());
                        if (bChromaV) pPS->WRefF_V[i] = std::max<int>(0, pTS->pMVDegrainBase->thSADC - blockF.GetSAD());
                        break;
                    case 1: // Individual SAD's
                        if (bLumaY)   pPS->WRefF_Y[i] = std::max<int>(0, pTS->pMVDegrainBase->thSAD  - blockF.GetSADLuma());
                        if (bChromaU) pPS->WRefF_U[i] = std::max<int>(0, pTS->pMVDegrainBase->thSADC - blockF.GetSADChromaU());
                        if (bChromaV) pPS->WRefF_V[i] = std::max<int>(0, pTS->pMVDegrainBase->thSADC - blockF.GetSADChromaV());
                        break;
                    }
                    if (bLumaY) {
                        pPS->pFLY[i]  = pPS->pPlanesF[0][i]->GetPointer(blxF, blyF);
                        pPS->npFLY[i] = pPS->pPlanesF[0][i]->GetPitch();
                    }
                    if (bChromaU) {
                        pPS->pFCU[i]  = pPS->pPlanesF[1][i]->GetPointer(blxF_2, blyF_2);
                        pPS->npFCU[i] = pPS->pPlanesF[1][i]->GetPitch();
                    }
                    if (bChromaV) {
                        pPS->pFCV[i]  = pPS->pPlanesF[2][i]->GetPointer(blxF_2, blyF_2);
                        pPS->npFCV[i] = pPS->pPlanesF[2][i]->GetPitch();
                    }
                }
                else {
                    if (bLumaY)   pPS->WRefF_Y[i] = 0;
                    if (bChromaU) pPS->WRefF_U[i] = 0;
                    if (bChromaV) pPS->WRefF_V[i] = 0;
                    if (bLumaY) {
                        pPS->pFLY[i]    = pSrcCurPtr[0];
                        pPS->npFLY[i]   = pTS->nSrcPitches[0];
                    }
                    if (bChromaU) {
                        pPS->pFCU[i]    = pSrcCurPtr[1];
                        pPS->npFCU[i]   = pTS->nSrcPitches[1];
                    }
                    if (bChromaV) {
                        pPS->pFCV[i]    = pSrcCurPtr[2];
                        pPS->npFCV[i]   = pTS->nSrcPitches[2];
                    }
                }
            }

            int WSrc_Y=256;
            if (bLumaY) {
                int WDivisor = pTS->pMVDegrainBase->thSAD + 1; // 2 is empirical denominator :)
                for(unsigned int i=0; i<pTS->pMVDegrainBase->NumRefFrames; ++i) 
                    WDivisor += (pPS->WRefB_Y[i] + pPS->WRefF_Y[i]);
                for(unsigned int i=0; i<pTS->pMVDegrainBase->NumRefFrames; ++i) {
                    pPS->WRefB_Y[i] = (pPS->WRefB_Y[i]<<8)/WDivisor; // normailize weights to 256
                    pPS->WRefF_Y[i] = (pPS->WRefF_Y[i]<<8)/WDivisor;	
                    WSrc_Y -= (pPS->WRefB_Y[i]+pPS->WRefF_Y[i]);
                }
            }

            int WSrc_U=256;
            if (bChromaU) {
                int WDivisor_U = pTS->pMVDegrainBase->thSADC + 1; // 2 is empirical denominator :)
                for(unsigned int i=0; i<pTS->pMVDegrainBase->NumRefFrames; ++i) 
                    WDivisor_U += (pPS->WRefB_U[i] + pPS->WRefF_U[i]);
                for(unsigned int i=0; i<pTS->pMVDegrainBase->NumRefFrames; ++i) {
                    pPS->WRefB_U[i] = (pPS->WRefB_U[i]<<8)/WDivisor_U; // normailize weights to 256
                    pPS->WRefF_U[i] = (pPS->WRefF_U[i]<<8)/WDivisor_U;	
                    WSrc_U -= (pPS->WRefB_U[i]+pPS->WRefF_U[i]);
                }
            }

            int WSrc_V=256;
            if (bChromaV) {
                int WDivisor_V = pTS->pMVDegrainBase->thSADC + 1; // 2 is empirical denominator :)
                for(unsigned int i=0; i<pTS->pMVDegrainBase->NumRefFrames; ++i) 
                    WDivisor_V += (pPS->WRefB_V[i] + pPS->WRefF_V[i]);
                for(unsigned int i=0; i<pTS->pMVDegrainBase->NumRefFrames; ++i) {
                    pPS->WRefB_V[i] = (pPS->WRefB_V[i]<<8)/WDivisor_V; // normailize weights to 256
                    pPS->WRefF_V[i] = (pPS->WRefF_V[i]<<8)/WDivisor_V;	
                    WSrc_V -= (pPS->WRefB_V[i]+pPS->WRefF_V[i]);
                }
            }

            if (bOverlap) {
                if (bLumaY) {
                    // luma
                    pTS->pMVDegrainBase->DEGRAINLUMA(&pPS->tmpBlock[0], pPS->tmpPitch, pSrcCurPtr[0], pTS->nSrcPitches[0],
                                                     pPS->pBLY, pPS->npBLY,  pPS->pFLY, pPS->npFLY, WSrc_Y, 
                                                     pPS->WRefB_Y, pPS->WRefF_Y, pTS->pMVDegrainBase->NumRefFrames);
                    pTS->pMVDegrainBase->OVERSLUMA(pDstShort[0] + xx, pTS->pMVDegrainBase->dstShortPitch, &pPS->tmpBlock[0], 
                                                   pPS->tmpPitch, winOverY, pTS->pMVDegrainBase->nBlkSizeX);
                }
                if (bChromaU) {
                    // chroma u
                    pTS->pMVDegrainBase->DEGRAINCHROMA(&pPS->tmpBlock[256], pPS->tmpPitch, pSrcCurPtr[1], pTS->nSrcPitches[1],
                                                       pPS->pBCU, pPS->npBCU, pPS->pFCU, pPS->npFCU, WSrc_U, pPS->WRefB_U,
                                                       pPS->WRefF_U, pTS->pMVDegrainBase->NumRefFrames);
                    pTS->pMVDegrainBase->OVERSCHROMA(pDstShort[1] + xx2, pTS->pMVDegrainBase->dstShortPitch, &pPS->tmpBlock[256], 
                                                     pPS->tmpPitch, winOverU, pTS->pMVDegrainBase->nBlkSizeX>>1);
                }
                if (bChromaV) {
                    // chroma v
                    pTS->pMVDegrainBase->DEGRAINCHROMA(&pPS->tmpBlock[512], pPS->tmpPitch, pSrcCurPtr[2], pTS->nSrcPitches[2],
                                                       pPS->pBCV, pPS->npBCV, pPS->pFCV, pPS->npFCV, WSrc_V, pPS->WRefB_V,
                                                       pPS->WRefF_V, pTS->pMVDegrainBase->NumRefFrames);
                    pTS->pMVDegrainBase->OVERSCHROMA(pDstShort[2] + xx2, pTS->pMVDegrainBase->dstShortPitch, &pPS->tmpBlock[512],
                                                     pPS->tmpPitch, winOverV, pTS->pMVDegrainBase->nBlkSizeX>>1);
                }
            }
            else {
                // not overlapped
                if (bLumaY) {
                    pTS->pMVDegrainBase->DEGRAINLUMA(pDstCur[0] + xx, pTS->nDstPitches[0], pSrcCurPtr[0], pTS->nSrcPitches[0],
                                                     pPS->pBLY, pPS->npBLY, pPS->pFLY, pPS->npFLY, WSrc_Y, pPS->WRefB_Y, 
                                                     pPS->WRefF_Y, pTS->pMVDegrainBase->NumRefFrames);
                }
                if (bChromaU) {
                    pTS->pMVDegrainBase->DEGRAINCHROMA(pDstCur[1] + xx2, pTS->nDstPitches[1], pSrcCurPtr[1], pTS->nSrcPitches[1],
                                                       pPS->pBCU, pPS->npBCU, pPS->pFCU, pPS->npFCU, WSrc_U, pPS->WRefB_U,
                                                       pPS->WRefF_U, pTS->pMVDegrainBase->NumRefFrames);
                }
                if (bChromaV) {
                    pTS->pMVDegrainBase->DEGRAINCHROMA(pDstCur[2] + xx2, pTS->nDstPitches[2], pSrcCurPtr[2], pTS->nSrcPitches[2],
                                                       pPS->pBCV, pPS->npBCV, pPS->pFCV, pPS->npFCV, WSrc_V, pPS->WRefB_V,
                                                       pPS->WRefF_V, pTS->pMVDegrainBase->NumRefFrames);
                }

                if (bx == pTS->pMVDegrainBase->nBlkX-1 && nWidth_B < pTS->pMVDegrainBase->nWidth) // right non-covered region
                {
                    if (bLumaY) {
                        // luma
                        PlaneCopy(pDstCur[0] + nWidth_B, pTS->nDstPitches[0], pSrcCur[0] + nWidth_B, pTS->nSrcPitches[0], 
                                  pTS->pMVDegrainBase->nWidth-nWidth_B, pTS->pMVDegrainBase->nBlkSizeY, pTS->pMVDegrainBase->isse);
                    }
                    if (bChromaU) {
                        // chroma u
                        PlaneCopy(pDstCur[1] + (nWidth_B>>1), pTS->nDstPitches[1], pSrcCur[1] + (nWidth_B>>1), pTS->nSrcPitches[1], 
                                  (pTS->pMVDegrainBase->nWidth-nWidth_B)>>1, (pTS->pMVDegrainBase->nBlkSizeY)/pTS->pMVDegrainBase->yRatioUV,
                                  pTS->pMVDegrainBase->isse);
                    }
                    if (bChromaV) {
                        // chroma u
                        PlaneCopy(pDstCur[2] + (nWidth_B>>1), pTS->nDstPitches[2], pSrcCur[2] + (nWidth_B>>1), pTS->nSrcPitches[2],
                                  (pTS->pMVDegrainBase->nWidth-nWidth_B)>>1, (pTS->pMVDegrainBase->nBlkSizeY)/pTS->pMVDegrainBase->yRatioUV,
                                  pTS->pMVDegrainBase->isse);
                    }
                }
            }
            xx += xx_Offset;
            xx2=xx>>1;
        }

        if (bOverlap) {
            // overlapped
            for(unsigned int i=0; i<3; ++i) {
                pSrcCur[i]   += pSrcCur_Offset[i];
                pDstShort[i] += pDstShort_Offset[i];
            }
        }
        else {
            // not overlapped
            for(unsigned int i=0; i<3; ++i) {
                pDstCur[i] += pDstCur_Offset[i];
                pSrcCur[i] += pSrcCur_Offset[i];
            }

            if (by == pTS->pMVDegrainBase->nBlkY-1 && nHeight_B < pTS->pMVDegrainBase->nHeight) // bottom uncovered region
            {
                if (bLumaY) {
                    // luma
                    PlaneCopy(pDstCur[0], pTS->nDstPitches[0], pSrcCur[0], pTS->nSrcPitches[0], 
                              pTS->pMVDegrainBase->nWidth, pTS->pMVDegrainBase->nHeight-nHeight_B, pTS->pMVDegrainBase->isse);
                }
                if (bChromaU) {
                    // chroma u
                    PlaneCopy(pDstCur[1], pTS->nDstPitches[1], pSrcCur[1], pTS->nSrcPitches[1], 
                              pTS->pMVDegrainBase->nWidth>>1, (pTS->pMVDegrainBase->nHeight-nHeight_B)/pTS->pMVDegrainBase->yRatioUV,
                              pTS->pMVDegrainBase->isse);
                }
                if (bChromaV) {
                    // chroma u
                    PlaneCopy(pDstCur[2], pTS->nDstPitches[2], pSrcCur[2], pTS->nSrcPitches[2], 
                              pTS->pMVDegrainBase->nWidth>>1, (pTS->pMVDegrainBase->nHeight-nHeight_B)/pTS->pMVDegrainBase->yRatioUV,
                              pTS->pMVDegrainBase->isse);
                }
            }
        }
        Sleep(0); // be friendly
    }

    if (bOverlap) {
        if (bLumaY) {
            Short2Bytes(pTS->pDst[0], pTS->nDstPitches[0], pTS->pMVDegrainBase->DstShort[pTS->PreFetchNum],
                        pTS->pMVDegrainBase->dstShortPitch, nWidth_B, nHeight_B);
            if (nWidth_B < pTS->pMVDegrainBase->nWidth)
                PlaneCopy(pTS->pDst[0] + nWidth_B, pTS->nDstPitches[0], pTS->pSrc[0] + nWidth_B, 
                          pTS->nSrcPitches[0], pTS->pMVDegrainBase->nWidth-nWidth_B, nHeight_B, pTS->pMVDegrainBase->isse);
            if (nHeight_B < pTS->pMVDegrainBase->nHeight) // bottom noncovered region
                PlaneCopy(pTS->pDst[0] + nHeight_B*pTS->nDstPitches[0], pTS->nDstPitches[0], 
                          pTS->pSrc[0] + nHeight_B*pTS->nSrcPitches[0], 
                          pTS->nSrcPitches[0], pTS->pMVDegrainBase->nWidth, pTS->pMVDegrainBase->nHeight-nHeight_B,
                          pTS->pMVDegrainBase->isse);
        }
        if (bChromaU) {
            Short2Bytes(pTS->pDst[1], pTS->nDstPitches[1], 
                        &pTS->pMVDegrainBase->DstShort[pTS->PreFetchNum][pTS->pMVDegrainBase->DstShortSize],
                        pTS->pMVDegrainBase->dstShortPitch, nWidth_B>>1, nHeight_B/pTS->pMVDegrainBase->yRatioUV);
            if (nWidth_B < pTS->pMVDegrainBase->nWidth)
                PlaneCopy(pTS->pDst[1] + (nWidth_B>>1), pTS->nDstPitches[1], pTS->pSrc[1] + (nWidth_B>>1), 
                          pTS->nSrcPitches[1], (pTS->pMVDegrainBase->nWidth-nWidth_B)>>1, nHeight_B/pTS->pMVDegrainBase->yRatioUV,
                          pTS->pMVDegrainBase->isse);
            if (nHeight_B < pTS->pMVDegrainBase->nHeight) // bottom noncovered region
                PlaneCopy(pTS->pDst[1] + pTS->nDstPitches[1]*nHeight_B/pTS->pMVDegrainBase->yRatioUV, pTS->nDstPitches[1], 
                          pTS->pSrc[1] + pTS->nSrcPitches[1]*nHeight_B/pTS->pMVDegrainBase->yRatioUV, pTS->nSrcPitches[1], 
                          pTS->pMVDegrainBase->nWidth>>1, (pTS->pMVDegrainBase->nHeight-nHeight_B)/pTS->pMVDegrainBase->yRatioUV,
                          pTS->pMVDegrainBase->isse);
        }
        if (bChromaV) {
            Short2Bytes(pTS->pDst[2], pTS->nDstPitches[2], 
                        &pTS->pMVDegrainBase->DstShort[pTS->PreFetchNum][2*pTS->pMVDegrainBase->DstShortSize], 
                        pTS->pMVDegrainBase->dstShortPitch, nWidth_B>>1, nHeight_B/pTS->pMVDegrainBase->yRatioUV);
            if (nWidth_B < pTS->pMVDegrainBase->nWidth)
                PlaneCopy(pTS->pDst[2] + (nWidth_B>>1), pTS->nDstPitches[2], pTS->pSrc[2] + (nWidth_B>>1), 
                          pTS->nSrcPitches[2], (pTS->pMVDegrainBase->nWidth-nWidth_B)>>1, nHeight_B/pTS->pMVDegrainBase->yRatioUV, 
                          pTS->pMVDegrainBase->isse);
            if (nHeight_B < pTS->pMVDegrainBase->nHeight) // bottom noncovered region
                PlaneCopy(pTS->pDst[2] + pTS->nDstPitches[2]*nHeight_B/pTS->pMVDegrainBase->yRatioUV, pTS->nDstPitches[2], 
                          pTS->pSrc[2] + pTS->nSrcPitches[2]*nHeight_B/pTS->pMVDegrainBase->yRatioUV, pTS->nSrcPitches[2], 
                          pTS->pMVDegrainBase->nWidth>>1, (pTS->pMVDegrainBase->nHeight-nHeight_B)/pTS->pMVDegrainBase->yRatioUV,
                          pTS->pMVDegrainBase->isse);	
        }
    }

    // -----------------------------------------------------------------

    if (pTS->pMVDegrainBase->nLimit < 255) {
        if (pTS->pMVDegrainBase->isse) {
            if (bLumaY)   LimitChanges_mmx(pTS->pDst[0], pTS->nDstPitches[0], pTS->pSrc[0], pTS->nSrcPitches[0], 
                                           pTS->pMVDegrainBase->nWidth, pTS->pMVDegrainBase->nHeight, pTS->pMVDegrainBase->nLimit);
            if (bChromaU) LimitChanges_mmx(pTS->pDst[1], pTS->nDstPitches[1], pTS->pSrc[1], pTS->nSrcPitches[1], 
                                           pTS->pMVDegrainBase->nWidth/2, pTS->pMVDegrainBase->nHeight/pTS->pMVDegrainBase->yRatioUV, 
                                           pTS->pMVDegrainBase->nLimit);
            if (bChromaV) LimitChanges_mmx(pTS->pDst[2], pTS->nDstPitches[2], pTS->pSrc[2], pTS->nSrcPitches[2], 
                                           pTS->pMVDegrainBase->nWidth/2, pTS->pMVDegrainBase->nHeight/pTS->pMVDegrainBase->yRatioUV, 
                                           pTS->pMVDegrainBase->nLimit);
        }
        else {
            if (bLumaY)   LimitChanges_c(pTS->pDst[0], pTS->nDstPitches[0], pTS->pSrc[0], pTS->nSrcPitches[0], 
                                         pTS->pMVDegrainBase->nWidth, pTS->pMVDegrainBase->nHeight, pTS->pMVDegrainBase->nLimit);
            if (bChromaU) LimitChanges_c(pTS->pDst[1], pTS->nDstPitches[1], pTS->pSrc[1], pTS->nSrcPitches[1], 
                                         pTS->pMVDegrainBase->nWidth/2, pTS->pMVDegrainBase->nHeight/pTS->pMVDegrainBase->yRatioUV, 
                                         pTS->pMVDegrainBase->nLimit);
            if (bChromaV) LimitChanges_c(pTS->pDst[2], pTS->nDstPitches[2], pTS->pSrc[2], pTS->nSrcPitches[2], 
                                         pTS->pMVDegrainBase->nWidth/2, pTS->pMVDegrainBase->nHeight/pTS->pMVDegrainBase->yRatioUV, 
                                         pTS->pMVDegrainBase->nLimit);
        }
    }

    //--------------------------------------------------------------------------------
    __asm emms; // (we may use double-float somewhere) Fizick
}

