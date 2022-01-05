#ifndef __MV_DEGRAINBASE__
#define __MV_DEGRAINBASE__

#include "MVInterface.h"
#include "yuy2planes.h"
#include "overlap.h"
#include "MVDegrainBaseThread.h"

/*! \brief Filter that denoise the picture
 */
class MVDegrainBase : public GenericVideoFilter, public MVFilter {

friend class MVDegrainBaseGetFramesThread;
friend class MVDegrainBaseProcessThread;

typedef void (DenoiseFunction)(BYTE *pDst, int nDstPitch, const BYTE *pSrc, int nSrcPitch,
                               const BYTE **pRefB, int *BPitch, const BYTE **pRefF, int *FPitch,
                               int WSrc, int *WRefB, int *WRefF, unsigned int const NumRefFrames);

protected:
    static unsigned int const MAX_PREFETCH=21;
    static unsigned int const MAX_REFFRAMES=32;

    MVClip *pmvClipB[MAX_PREFETCH][MAX_REFFRAMES]; // 0=B, 1=B2, 2=B3 
    MVClip *pmvClipF[MAX_PREFETCH][MAX_REFFRAMES]; // 0=F, 1=F2, 2=F3
    int thSAD;
    int thSADC;

private:
    int YUVplanes;
    int nLimit;
    PClip pelclip;
    bool isse;
    bool mmx;
    unsigned int NumRefFrames;
    unsigned int const MaxThreads;
    unsigned int const PreFetch;
    unsigned int NumThreads;
    unsigned int const SadMode;

    bool usePelClipHere;

    YUY2Planes *DstPlanes[MAX_PREFETCH];
    YUY2Planes *SrcPlanes[MAX_PREFETCH];

    OverlapWindows *OverWinsY[MAX_PREFETCH], *OverWinsU[MAX_PREFETCH], *OverWinsV[MAX_PREFETCH]; // one overlap pointer for Y, U, V

    OverlapsFunction *OVERSLUMA;
    OverlapsFunction *OVERSCHROMA;
    DenoiseFunction *DEGRAINLUMA;
    DenoiseFunction *DEGRAINCHROMA;

    unsigned int DstShortSize;
    int dstShortPitch;
    unsigned short *DstShort[MAX_PREFETCH];

    // variables for prefetching
    PVideoFrame CachedVideoFrames[MAX_PREFETCH-1]; 
    int CacheFrameNum; // the start of the cache frame

    // pointers to thread structures and threads
    MVDegrainBaseProcessThread::ThreadStruct *m_pProcessThreadTS[64];   
    MVDegrainBaseProcessThread  *m_pProcessThread[64]; 
	HANDLE ProcessThreadWaitHandles[64];

    // variables used in get frame which are declared here to avoid reinstation on each call to get frames
    typedef struct {
        PVideoFrame *src, *dst;
        unsigned char const *pSrc[MAX_PREFETCH][3];
        unsigned char *pDst[MAX_PREFETCH][3];
        int nDstPitches[MAX_PREFETCH][3], nSrcPitches[MAX_PREFETCH][3];
        PMVGroupOfFrames *pRefBGOF[MAX_PREFETCH], *pRefFGOF[MAX_PREFETCH];
        bool *isUsableB[MAX_PREFETCH], *isUsableF[MAX_PREFETCH];
    } GetFrameVariables;

    GetFrameVariables GFV;

public:
    MVDegrainBase(PClip _child, unsigned int _NumRefFrames, int _YUVplanes, int _nLimit, PClip _pelclip, int _nIdx, bool mmx, bool isse, 
                  IScriptEnvironment* env, const PClip &MVFiltervector, const char *filterName, int const MultiIndex,  
                  unsigned int const _MaxThreads, unsigned int const _PreFetch, unsigned int const _SadMode);
    ~MVDegrainBase();
    void UpdateNumRefFrames(unsigned int const _NumRefFrames, IScriptEnvironment *env);
    virtual PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
    void ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
                                       int const Sharp, int const pixelType, int const nHeight, int const nWidth, 
                                       int const nPel, bool const isse);
    template <int const width, int const height> static DenoiseFunction Degrain_C;
};

#endif
