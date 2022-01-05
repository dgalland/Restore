#ifndef __MV_MVANALYSEMULTITHREAD__
#define __MV_MVANALYSEMULTITHREAD__

#include "Thread.hpp"
#include "Semaphore.hpp"
#include "avisynth.h"
#include "MVInterface.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MVAnalyseMulti;
class PVideoFrame;
class MVFrames;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define MVAnalyseMulti threads class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MVAnalyseMultiProcessThread :public ThreadFunctions::Thread {
public:
    struct ThreadStruct{
        MVAnalyseMulti* pMVAnalyseMulti;
        unsigned int PreFetchNum, RefNum;
        int n;
        IScriptEnvironment* env;
        unsigned char *pDestination;
        int DstPitch;
        PMVGroupOfFrames *pSrcGOF, *pRefGOF;
        ThreadStruct::ThreadStruct(MVAnalyseMulti* _pMVAnalyseMulti): pMVAnalyseMulti(_pMVAnalyseMulti) {};
        void Populate(unsigned int _PreFetchNum, int _RefNum, int _n, IScriptEnvironment* _env, unsigned char *_pDestination, int _DstPitch, 
                      PMVGroupOfFrames *_pSrcGOF, PMVGroupOfFrames *_pRefGOF) 
        {
            PreFetchNum=_PreFetchNum;   RefNum=_RefNum;
            n=_n;                       env=_env;
            pDestination=_pDestination; DstPitch=_DstPitch;       
            pSrcGOF=_pSrcGOF;           pRefGOF=_pRefGOF;
        };
    };
private:
	ThreadStruct* pTS;
    ThreadFunctions::Semaphore m_ProcessSemaphore; // semaphore to indicate start processing
    ThreadFunctions::Semaphore m_DoneSemaphore;    // semphore to indicate done processing

public:
	// define constructors
	explicit MVAnalyseMultiProcessThread(void* ThreadParameters, unsigned int const StackSize=0,SECURITY_ATTRIBUTES* ThreadAttributes=0);
		// this creates a suspended thread with thread parameters passed to the thread and with stack size StackSize and Thread
        // Attributes ThreadAttributes which will default to Inheritable if NULL
	virtual ~MVAnalyseMultiProcessThread() {}; // default destructor
private:
	// prevent copying
	MVAnalyseMultiProcessThread(MVAnalyseMultiProcessThread const& thread); // copy constructor
	MVAnalyseMultiProcessThread& operator=(MVAnalyseMultiProcessThread const& thread); // assignment 

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define member operations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
    void ProcessFrame(); // process the frame
    HANDLE DoneHandle(); // get the done handle for waiting

protected:
	virtual unsigned int Run(void *UserThreadData);
		// this is the function which is called when the thread is started
};

#endif
