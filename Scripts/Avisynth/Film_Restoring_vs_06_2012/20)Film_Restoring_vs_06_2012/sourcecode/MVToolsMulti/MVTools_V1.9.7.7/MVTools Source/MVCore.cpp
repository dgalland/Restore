// Author: Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick - yRatioUV
// See legal notice in Copying.txt for more information
//
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

#include "MVInterface.h"

MVCore::MVCore() : LastEmptyIndex(0)
{
}

MVCore::~MVCore()
{
    // loop through all index entries deleting all MVFrames
    FrameIndexMap::iterator FrameIndexIT=MVFramesMap.begin();
    while(FrameIndexIT!=MVFramesMap.end()) {
        // delete all MVFrames associated with index
        FrameNumMap& FrameNums=FrameIndexIT->second;
        FrameNumMap::iterator FrameNumIT=FrameNums.begin();
        while(FrameNumIT!=FrameNums.end()) {
            PMVGroupOfFrames *pPMVGroupOfFrames=FrameNumIT->second;
            // delete the MVGroupOfFrames
            (*pPMVGroupOfFrames)->~MVGroupOfFrames();
            // delete the PMVGroupOfFrames
            delete pPMVGroupOfFrames;
            // erase framenum entry
            FrameNumIT=FrameNums.erase(FrameNumIT);
        }
        // erase index entry
        FrameIndexIT=MVFramesMap.erase(FrameIndexIT);
    }
}

void MVCore::AddFrames(int nIdx, int nNbFrames, int nLevelCount, int nWidth,
                       int nHeight, int nPel, int nHPad, int nVPad, int nMode, bool isse, int yRatioUV)
{
    DebugPrintf("Add frames list, with %i as idx\n", nIdx);

    FrameIndexMap::iterator FrameIndexIT=MVFramesMap.find(nIdx);
    if (FrameIndexIT==MVFramesMap.end())
    {
        DebugPrintf("Idx not found, creating a new list of frames...\n");

        // create group of frames from -1 to -nNbFrames
        FrameNumMap TempFrameNumMap;
        for(int FrameNum=-1; FrameNum>=-nNbFrames; --FrameNum) {
            // create a PMVGroupOfFrames
            PMVGroupOfFrames *pPMVGroupOfFrames=new PMVGroupOfFrames(new MVGroupOfFrames(nLevelCount, nWidth, nHeight, nPel, nHPad, 
                                                                                         nVPad, nMode, isse, yRatioUV));
            pPMVGroupOfFrames->ResetRefCount();
            // store in map
            TempFrameNumMap[FrameNum]=pPMVGroupOfFrames;
        }

        // insert the group into the index map
        MVFramesMap[nIdx]=TempFrameNumMap;
    }
    else
    {
        DebugPrintf("Idx already found, no need to add a list of frames\n");

        // grow the map entry if not enough frames
        FrameNumMap& FrameNums=FrameIndexIT->second;
        size_t CurrentSize=FrameNums.size();
        if (CurrentSize<static_cast<size_t>(nNbFrames)) {
            // get the frame num of the first item
            FrameNumMap::iterator FrameNumIT=FrameNums.begin();
            int LowestFrameNum=FrameNumIT->first;
             // create the required group of frames decreasing from LowestFrameNum-1
            for(int FrameNum=LowestFrameNum-1; FrameNum>=LowestFrameNum-(nNbFrames-static_cast<int>(CurrentSize)); --FrameNum) {
                // create a PMVGroupOfFrames
                PMVGroupOfFrames *pPMVGroupOfFrames=new PMVGroupOfFrames(new MVGroupOfFrames(nLevelCount, nWidth, nHeight, nPel, nHPad, 
                                                                                             nVPad, nMode, isse, yRatioUV));
                pPMVGroupOfFrames->ResetRefCount();
                // store in map
                FrameNums[FrameNum]=pPMVGroupOfFrames;
            }               
        }
    }
}

PMVGroupOfFrames MVCore::GetFrame(int const nIdx, int const FrameNum)
{
    // find the index entry
    FrameIndexMap::iterator FrameIndexIT=MVFramesMap.find(nIdx);
    // get the map to the group of frames
    FrameNumMap& FrameNums=FrameIndexIT->second;
    // find the frame num
    FrameNumMap::iterator FrameNumIT=FrameNums.find(FrameNum);
    if (FrameNumIT==FrameNums.end()) {
        // loop from lowest FrameNum to highest to find unused one
        for(FrameNumIT=FrameNums.begin(); FrameNumIT!=FrameNums.end(); ++FrameNumIT) {
            PMVGroupOfFrames *pPMVGroupOfFrames=FrameNumIT->second;
            if ((*pPMVGroupOfFrames)->GetRefCount()==0) {
                // unused so reuse it by erasing old entry and assigning new framenum
                FrameNums.erase(FrameNumIT);
                FrameNums[FrameNum]=pPMVGroupOfFrames;
                // reset the state and return it
                (*pPMVGroupOfFrames)->ResetState();
                return *pPMVGroupOfFrames;
            }
        }
        // else no unused, so create one based on old settings
        FrameNumIT=FrameNums.begin();
        PMVGroupOfFrames *pPMVGroupOfFramesOld=FrameNumIT->second;
        PMVGroupOfFrames *pPMVGroupOfFrames=new PMVGroupOfFrames(new MVGroupOfFrames((*pPMVGroupOfFramesOld)->LevelCount(), 
                                                                                     (*pPMVGroupOfFramesOld)->Width(), 
                                                                                     (*pPMVGroupOfFramesOld)->Height(), 
                                                                                     (*pPMVGroupOfFramesOld)->Pel(),
                                                                                     (*pPMVGroupOfFramesOld)->HPad(),
                                                                                     (*pPMVGroupOfFramesOld)->VPad(),
                                                                                     (*pPMVGroupOfFramesOld)->Mode(),
                                                                                     (*pPMVGroupOfFramesOld)->ISSE(),
                                                                                     (*pPMVGroupOfFramesOld)->yRatioUV()));
        pPMVGroupOfFrames->ResetRefCount();
        // store in map
        FrameNums[FrameNum]=pPMVGroupOfFrames;
        return *pPMVGroupOfFrames;
    }
    else {
        // frame num found, so reset the state and return it
        PMVGroupOfFrames *pPMVGroupOfFrames=FrameNumIT->second;
        (*pPMVGroupOfFrames)->ResetState();
        return *pPMVGroupOfFrames;
    }
}
