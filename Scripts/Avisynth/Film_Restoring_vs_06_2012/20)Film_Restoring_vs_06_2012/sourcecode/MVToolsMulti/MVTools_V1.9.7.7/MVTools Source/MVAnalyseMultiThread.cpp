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


#include "MVAnalyseMultiThread.h"
#include "MVAnalyseMulti.h"

using namespace ThreadFunctions;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define MVAnalyseMulti Threads
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// this creates a suspended thread with thread parameters passed to the thread and with stack size StackSize and Thread Attributes 
// ThreadAttributes which will default to Inheritable if NULL
MVAnalyseMultiProcessThread::MVAnalyseMultiProcessThread(void* ThreadParameters, unsigned int const StackSize, 
                                                         SECURITY_ATTRIBUTES* ThreadAttributes) :
                             Thread(ThreadParameters, StackSize, ThreadAttributes), pTS(0), m_ProcessSemaphore("", 1), 
                             m_DoneSemaphore("", 1)
{
}

// kick of thread process
void MVAnalyseMultiProcessThread::ProcessFrame()
{
    m_ProcessSemaphore.Release();
}

// get the done handle for waiting
HANDLE MVAnalyseMultiProcessThread::DoneHandle()
{
    return m_DoneSemaphore.Handle();
}

unsigned int MVAnalyseMultiProcessThread::Run(void *UserThreadData)
{
    pTS=reinterpret_cast<ThreadStruct*>(UserThreadData);

    while(Enabled()) {
        // wait for process sempahore to be enabled
        m_ProcessSemaphore.Wait();

        if (!Enabled()) break; // skip processing

        // set write point to appropriate line
        unsigned char *pDst=pTS->pDestination+pTS->DstPitch*(pTS->PreFetchNum*(2*pTS->pMVAnalyseMulti->RefFrames)+pTS->RefNum);

        // write analysis parameters as a header to frame
        memcpy(pDst, &pTS->pMVAnalyseMulti->headerSize, sizeof(int));
        if (pTS->pMVAnalyseMulti->divideExtra)
            memcpy(pDst+sizeof(int), &pTS->pMVAnalyseMulti->analysisDataDivided[pTS->RefNum], sizeof(MVAnalysisData));
        else
            memcpy(pDst+sizeof(int), &pTS->pMVAnalyseMulti->analysisData[pTS->RefNum],        sizeof(MVAnalysisData));
        pDst += pTS->pMVAnalyseMulti->headerSize;

        //    DebugPrintf("MVAnalyse: Get ref frame %d",n + offset);

        unsigned int minframe=(pTS->pMVAnalyseMulti->analysisData[pTS->RefNum].isBackward) ? 
                              0 : pTS->pMVAnalyseMulti->analysisData[pTS->RefNum].nDeltaFrame;
        unsigned int maxframe=(pTS->pMVAnalyseMulti->analysisData[pTS->RefNum].isBackward) ? 
                              pTS->pMVAnalyseMulti->vi.num_frames - pTS->pMVAnalyseMulti->analysisData[pTS->RefNum].nDeltaFrame :
                              pTS->pMVAnalyseMulti->vi.num_frames;
       
        if (( pTS->n + pTS->PreFetchNum < maxframe ) && ( pTS->n + pTS->PreFetchNum >= minframe ))
        {
            int fieldShift = 0;
            if (pTS->pMVAnalyseMulti->vi.IsFieldBased() && pTS->pMVAnalyseMulti->analysisData[pTS->RefNum].nPel > 1 && 
                (pTS->pMVAnalyseMulti->analysisData[pTS->RefNum].nDeltaFrame %2 != 0))
            {
                fieldShift = ((*pTS->pSrcGOF)->Parity() && !(*pTS->pRefGOF)->Parity()) ? pTS->pMVAnalyseMulti->analysisData[pTS->RefNum].nPel/2 : 
                             (((*pTS->pRefGOF)->Parity() && !(*pTS->pSrcGOF)->Parity()) ? -(pTS->pMVAnalyseMulti->analysisData[pTS->RefNum].nPel/2) : 0);
                // vertical shift of fields for fieldbased video at finest level pel2
            }

            // note this has outfile buf so look for cirtical section
            pTS->pMVAnalyseMulti->vectorFields[pTS->PreFetchNum][pTS->RefNum]->
                                    SearchMVs((*pTS->pSrcGOF), (*pTS->pRefGOF), pTS->pMVAnalyseMulti->searchType, 
                                              pTS->pMVAnalyseMulti->nSearchParam, pTS->pMVAnalyseMulti->nPelSearch, 
                                              pTS->pMVAnalyseMulti->nLambda, pTS->pMVAnalyseMulti->lsad, 
                                              pTS->pMVAnalyseMulti->pnew, pTS->pMVAnalyseMulti->plevel,
                                              pTS->pMVAnalyseMulti->global, 
                                              pTS->pMVAnalyseMulti->analysisData[pTS->RefNum].nFlags, 
                                              reinterpret_cast<int*>(pDst), pTS->pMVAnalyseMulti->outfilebuf, 
                                              fieldShift, pTS->pMVAnalyseMulti->DCTc[pTS->PreFetchNum][pTS->RefNum]);

            if (pTS->pMVAnalyseMulti->divideExtra) {
                // make extra level with divided sublocks with median (not estimated) motion
                pTS->pMVAnalyseMulti->vectorFields[pTS->PreFetchNum][pTS->RefNum]->
                                        ExtraDivide(reinterpret_cast<int*>(pDst), 
                                                    pTS->pMVAnalyseMulti->analysisData[pTS->RefNum].nFlags);
            }

            PROFILE_CUMULATE();
            if (pTS->pMVAnalyseMulti->outfile != NULL) {
                CriticalSectionAuto AutoCS(pTS->pMVAnalyseMulti->MVAnalyseMultiCS);
                fwrite(pTS->pMVAnalyseMulti->outfilebuf, sizeof(short)*pTS->pMVAnalyseMulti->analysisData[pTS->RefNum].nBlkX*
                                                         pTS->pMVAnalyseMulti->analysisData[pTS->RefNum].nBlkY*4, 1, pTS->pMVAnalyseMulti->outfile);
            }

        }
        else
        {
            pTS->pMVAnalyseMulti->vectorFields[pTS->PreFetchNum][pTS->RefNum]->WriteDefaultToArray(reinterpret_cast<int*>(pDst));
        }

        // indicate done
        m_DoneSemaphore.Release();
    }

	return 0; // end thread
}


