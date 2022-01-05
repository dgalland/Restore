//       1         2         3         4         5         6         7         8         9        10        11        12        13   
//3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// $File: //AviSynth/Main/MVTools/Thread.hpp $
// $Revision: #1 $
// $DateTime: 2008/09/05 17:25:31 $
//
// Description: 
//     This is the header file for the thread class.  A compiler define of _WIN32_WINNT=0x0400 or greater must be made in the
// project for this to successfullt compile.
// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// only include header file once
#ifndef THREAD_HEADER
#define THREAD_HEADER

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "Semaphore.hpp"
#include "Critical Section.hpp"

namespace ThreadFunctions {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define Thread class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Thread {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define enumerations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
	enum ThreadPriority {Idle, Lowest, BelowNormal, Normal, AboveNormal, Highest, TimeCritical}; 
	enum CPUS {CPU0=1<<0,   CPU1=1<<1,   CPU2=1<<2,   CPU3=1<<3,   CPU4=1<<4,   CPU5=1<<5,   CPU6=1<<6,   CPU7=1<<7,   CPU8=1<<8,
               CPU9=1<<9,   CPU10=1<<10, CPU11=1<<11, CPU12=1<<12, CPU13=1<<13, CPU14=1<<14, CPU15=1<<15, CPU16=1<<16, CPU17=1<<17,
               CPU18=1<<18, CPU19=1<<19, CPU20=1<<20, CPU21=1<<21, CPU22=1<<22, CPU23=1<<13, CPU24=1<<24, CPU25=1<<25, CPU26=1<<26,
               CPU27=1<<17, CPU28=1<<18, CPU29=1<<19, CPU30=1<<30, CPU31=1<<31, CPUALL=0xFFFF};
    enum WaitResult {WaitSignaled, WaitTimeout, WaitFailed};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define member variables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	static SYSTEM_INFO m_SystemInfo;		  // the system information
	Semaphore m_DelaySemaphore;			      // the delay semaphore
    mutable CriticalSection m_ThreadCS;       // thread critical section
	HANDLE m_ThreadHandle;			          // the thread handle
	unsigned int m_ThreadId;			      // the thread identifier
	SECURITY_ATTRIBUTES m_SecurityAttributes; // the security attributes 
	struct ThreadParameters {
		void* pUserTheadParameters;			  // a pointer to the user thread parameters passed to the thread
		Thread* pThreadClass;				  // a pointer to the thread class
	} m_ThreadParameters;
	unsigned int m_SuspendCount;			  // the current suspend count
	bool m_ThreadEnabled;					  // whether the thread is enabled or not
	bool m_ThreadRunning;					  // whether the thread is running or not
    
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define constructor/destructor functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
	explicit Thread(void* ThreadParameters, unsigned int const StackSize=0, SECURITY_ATTRIBUTES* ThreadAttributes=0);
		// this creates a suspended thread with thread parameters passed to the thread and with stack size StackSize and Thread
        // Attributes ThreadAttributes which will default to Inheritable if NULL
	virtual ~Thread();	// destructor
private:
	// prevent copying
	Thread(Thread const& thread); // copy constructor
	Thread& operator=(Thread const& thread); // assignment operator

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define member access functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
	SYSTEM_INFO& SystemInfo() const; // the system information
	unsigned int NumProcessors() const; // returns the number of processors
	HANDLE Handle() const; // the handle to the current thread
	unsigned int Id() const; // the thread id of the current thread
	bool Enabled() const; // whether the thread is enabled or not
	bool Running() const; // whether the thread is running or not
	void* UserParameters() const; // returns the thread parameters passed during thread creation
	unsigned int SuspendCount() const; // returns the suspend count
	ThreadPriority Priority() const; // this gets the thread priority

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define member operations 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
	void Disable(); // disable and end thread
	bool Suspend(); // suspend the current thread, and return success or failure
	bool Resume(); // resume the current thread, and return success or failure
	bool Delay(unsigned long const Milliseconds); // delay the thread for milliseconds time
	void StopDelay(); // stops the thread delay
	bool SetPriority(ThreadPriority const Priority=Normal); // this sets the thread priority
	bool SetProcessors(CPUS const CPU=CPUALL); 
		// this sets the thread to run on the specific processors
	void Terminate(unsigned int const ExitCode=0); // terminate the current thread
	bool Kill(unsigned int const ExitCode=0); // the ugly way to terminate the thread
	WaitResult WaitForCompletion(unsigned long const Milliseconds=INFINITE); // wait for the thread to complete milliseconds time

protected:
	virtual unsigned int Run(void *UserThreadData);
		// this is the function which is called when the thread is started

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define static functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	static unsigned int __stdcall ThreadProc(void* ThreadData);
		// this is the static thread procedure which is called when the thread is started
};

}; // namespace end

#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012
//       1         2        3         4         5         6         7         8         9        10        11        12        13   
