#ifndef __MV_DEGRAINBASETHREAD__
#define __MV_DEGRAINBASETHREAD__

#include "Thread.hpp"
#include "Semaphore.hpp"
#include "avisynth.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MVDegrainBase;
class MVClip;
class MVFrames;
class PMVGroupOfFrames;
class MVPlane;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define MVDegrainBase threads class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MVDegrainBaseProcessThread :public ThreadFunctions::Thread {
public:
    typedef enum {LUMA, CHROMAU, CHROMAV} PlaneTypes;

    struct ThreadStruct{
        MVDegrainBase* pMVDegrainBase;
        MVClip** pmvClipB, **pmvClipF;
        unsigned int PreFetchNum;
        int n;
        IScriptEnvironment* env;
        const BYTE** pSrc;
        BYTE** pDst;
        int* nDstPitches, *nSrcPitches;
        bool *bIsUsableB, *bIsUsableF;
        PlaneTypes PlaneType;
        bool bProcess;
        PMVGroupOfFrames *pRefBGOF, *pRefFGOF;
        ThreadStruct::ThreadStruct(MVDegrainBase* _pMVDegrainBase): pMVDegrainBase(_pMVDegrainBase) {};
        void Populate(unsigned int _PreFetchNum, int _n, IScriptEnvironment* _env, const BYTE** _pSrc, BYTE** _pDst,
                      int* _nSrcPitches, int* _nDstPitches, MVClip** _pmvClipB, MVClip** _pmvClipF,
                      PMVGroupOfFrames* _pRefBGOF, PMVGroupOfFrames* _pRefFGOF,
                      bool* _bIsUsableB, bool* _bIsUsableF, PlaneTypes _PlaneType, bool _bProcess) 
        {
            PreFetchNum=_PreFetchNum;
            n=_n;                     env=_env;
            pSrc=_pSrc;               pDst=_pDst;
            nSrcPitches=_nSrcPitches; nDstPitches=_nDstPitches; 
            pmvClipB=_pmvClipB;       pmvClipF=_pmvClipF;
            pRefBGOF=_pRefBGOF;       pRefFGOF=_pRefFGOF;
            bIsUsableB=_bIsUsableB;   bIsUsableF=_bIsUsableF;
            PlaneType=_PlaneType;     bProcess=_bProcess;
        }
    };

private:
    ThreadStruct* pTS;
    ThreadFunctions::Semaphore m_ProcessSemaphore; // semaphore to indicate start processing
    ThreadFunctions::Semaphore m_DoneSemaphore;    // semphore to indicate done processing

    // this is the process structure used to hold big variables so as not to incur creation penalty on function call
    typedef struct {
        MVPlane *pPlanesB[3][32];
        MVPlane *pPlanesF[3][32];
        static int const tmpPitch = 16;
        unsigned char tmpBlock[(tmpPitch*tmpPitch)*3];
        const BYTE    *pBLY[32],   *pFLY[32];
        int           npBLY[32],   npFLY[32];
        const BYTE    *pBCU[32],   *pFCU[32];
        int           npBCU[32],   npFCU[32];
        const BYTE    *pBCV[32],   *pFCV[32];
        int           npBCV[32],   npFCV[32];
        int           WRefB_Y[32], WRefF_Y[32];
        int           WRefB_U[32], WRefF_U[32];
        int           WRefB_V[32], WRefF_V[32];
    } ProcessStruct;

public:
    // define constructors
    explicit MVDegrainBaseProcessThread(void* ThreadParameters, unsigned int const StackSize=0,SECURITY_ATTRIBUTES* ThreadAttributes=0);
        // this creates a suspended thread with thread parameters passed to the thread and with stack size StackSize and Thread
        // Attributes ThreadAttributes which will default to Inheritable if NULL
    virtual ~MVDegrainBaseProcessThread() {}; // default destructor
private:
    // prevent copying
    MVDegrainBaseProcessThread(MVDegrainBaseProcessThread const& thread); // copy constructor
    MVDegrainBaseProcessThread& operator=(MVDegrainBaseProcessThread const& thread); // assignment 

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define member operations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
    void ProcessFrame(); // process the frame
    HANDLE DoneHandle(); // get the done handle for waiting

protected:
    virtual unsigned int Run(void *UserThreadData);
        // this is the function which is called when the thread is started

private:
    template <bool const bLumaY, bool const bChromaU, bool const bChromaV> void Process(ProcessStruct *pPS);
        // this is the template function instiated for processing the variants
};

#endif
