#ifndef __MV_ANALYSIS_DATA_H__
#define __MV_ANALYSIS_DATA_H__

#include "MVInterface.h"

class MVAnalysisData
{
protected:
    /*! \brief size of a block, in pixel */
    int nBlkSize;

    /*! \brief pixel refinement of the motion estimation */
    int nPel;

    /*! \brief number of level for the hierarchal search */
    int nLvCount;

    /*! \brief difference between the index of the reference and the index of the current frame */
    int nDeltaFrame;

    /*! \brief direction of the search ( forward / backward ) */
    bool isBackward;

    /*! \brief diverse flags to set up the search */
    int nFlags;

    /*! \brief MVFrames idx */
    int nIdx;

    int nMagicKey;

public :

    inline void SetFlags(int _nFlags) { nFlags |= _nFlags; }
    inline int GetFlags() { return nFlags; }
    inline int GetBlkSize() { return nBlkSize; }
    inline int GetPel() { return nPel; }
    inline int GetLevelCount() { return nLvCount; }
    inline int GetFramesIdx() { return nIdx; }
    inline bool GetIsBackward() { return isBackward; }
    inline int GetMagicKey() { return nMagicKey; }
    inline int GetDeltaFrame() { return nDeltaFrame; }

};

#endif
