//       1         2         3         4         5         6         7         8         9        10        11        12        13   
//3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// $File: //AviSynth/Main/MVTools/Thread.cpp $
// $Revision: #1 $
// $DateTime: 2008/09/05 17:25:31 $
//
// Description: 
//     This is the implementation file for the thread class
// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Thread.hpp"

#include <memory.h>
#include <process.h>

#pragma warning (disable: 4800) // disable bool performance warning
#pragma warning (disable: 6258) // disable terminate thread warning

namespace ThreadFunctions {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static variable declaration/initilizations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SYSTEM_INFO Thread::m_SystemInfo;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define constructor/destructor functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// this creates a suspended thread with thread parameters passed to the thread and with stack size StackSize and Thread Attributes 
// ThreadAttributes which will default to Inheritable if NULL 
Thread::Thread(void* ThreadParameters, unsigned int const StackSize, SECURITY_ATTRIBUTES* ThreadAttributes) :
        m_DelaySemaphore("", 1)
{
	// get the system information
	GetSystemInfo(&m_SystemInfo);

	// populate the thread parameters structure
	m_ThreadParameters.pUserTheadParameters=ThreadParameters;
	m_ThreadParameters.pThreadClass=this;

	// set up the security attributes
	if (!ThreadAttributes) {
		// use default
		m_SecurityAttributes.nLength=sizeof(SECURITY_ATTRIBUTES);
		m_SecurityAttributes.lpSecurityDescriptor=0;
		m_SecurityAttributes.bInheritHandle=TRUE;
	}
	else {
		// use passed parameters
		memcpy(&m_SecurityAttributes, &ThreadAttributes, 
			   sizeof(SECURITY_ATTRIBUTES));
	}

    {
        CriticalSectionAuto AutoCS(m_ThreadCS);

	    // set the creation flags
        unsigned int CreationFlags=CREATE_SUSPENDED;

        // create the thread
	    if (m_ThreadHandle=reinterpret_cast<HANDLE>(_beginthreadex(reinterpret_cast<void*>(&m_SecurityAttributes),
							                                       StackSize, &ThreadProc,
												                   reinterpret_cast<void*>(&m_ThreadParameters),
												                   CreationFlags, &m_ThreadId)))	
	    {
		    // set the flags appropriately
		    m_ThreadEnabled=true;
            m_ThreadRunning=false;
	        m_SuspendCount=1;
	    }
	    else {
		    // thread not created, so set variables to safe states
		    m_ThreadHandle=0;
		    m_ThreadId=0;
		    m_ThreadParameters.pUserTheadParameters=0;
		    m_ThreadParameters.pThreadClass=0;
		    m_SuspendCount=0;
		    // set all the flags appropriately
		    m_ThreadEnabled=false;
		    m_ThreadRunning=false;
	    }
    }
}

// clean up thread handles
Thread::~Thread() 
{
	// close the thread handle
	if (m_ThreadHandle) CloseHandle(m_ThreadHandle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define member access functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// the system information
SYSTEM_INFO& Thread::SystemInfo() const
{
	return m_SystemInfo;
}

// returns the number of processors
unsigned int Thread::NumProcessors() const
{
	return static_cast<unsigned int>(m_SystemInfo.dwNumberOfProcessors);
}

// the handle to the current thread
HANDLE Thread::Handle() const
{
	return m_ThreadHandle;
}

// the thread id of the current thread
unsigned int Thread::Id() const
{
	return m_ThreadId;
}

// whether the thread is enabled or not
bool Thread::Enabled() const
{
    CriticalSectionAuto AutoCS(m_ThreadCS);
	return m_ThreadEnabled;
}

// whether the thread is running or not
bool Thread::Running() const
{
    CriticalSectionAuto AutoCS(m_ThreadCS);
	return m_ThreadRunning;
}

// returns the suspend count
unsigned int Thread::SuspendCount() const
{
    CriticalSectionAuto AutoCS(m_ThreadCS);
	return m_SuspendCount;
}

// returns the thread parameters passed during thread creation
void* Thread::UserParameters() const
{
	return m_ThreadParameters.pUserTheadParameters;
}

// this gets the thread priority
Thread::ThreadPriority Thread::Priority() const
{
	if (m_ThreadHandle) {
		int PriorityCode=GetThreadPriority(m_ThreadHandle);
		ThreadPriority Priority;

		switch(PriorityCode) {
		case THREAD_PRIORITY_TIME_CRITICAL: // time critical priority
			Priority=TimeCritical;
			break;
		case THREAD_PRIORITY_HIGHEST: // highest priority
			Priority=Highest;
			break;
		case THREAD_PRIORITY_ABOVE_NORMAL: // Above normal priority
			Priority=AboveNormal; 
			break;
		case THREAD_PRIORITY_NORMAL: // normal priority
			Priority=Normal; 
			break;
		case THREAD_PRIORITY_BELOW_NORMAL: // below normal priority
			Priority=BelowNormal; 
			break;
		case THREAD_PRIORITY_LOWEST: // lowest priority
			Priority=Lowest; 
			break;
		default:
		case THREAD_PRIORITY_IDLE: // idle priority
			Priority=Idle; 
			break;
		}
		return Priority;
	}
	else {
		// no thread
		return Normal;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define member operations 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// disable and end thread
void Thread::Disable()
{
    {
        CriticalSectionAuto AutoCS(m_ThreadCS);
	    // set thread enabled to false
	    m_ThreadEnabled=false;
    }

	// if thread is delayed then stop delay
	m_DelaySemaphore.Release();
}

// suspend the current thread, and return success or failure
bool Thread::Suspend()
{
	// if thread then try to suspend
	if (m_ThreadHandle) {
		if (DWORD OldCount=SuspendThread(m_ThreadHandle)!=-1) {
            CriticalSectionAuto AutoCS(m_ThreadCS);
			// thread suspended, increase count
			++m_SuspendCount;
			m_ThreadRunning=false;
			return true;
		}
		else {
			// error, thread not suspended so leave state as is
			return false;
		}
	}
	else {
		// no thread so suspended
		return true;
	}
}

// resume the current thread, and return success or failure
bool Thread::Resume()
{
	if (m_ThreadHandle) {
		if (DWORD OldCount=ResumeThread(m_ThreadHandle)!=-1) {
            CriticalSectionAuto AutoCS(m_ThreadCS);
			// decrement the suspend count
			if (m_SuspendCount) --m_SuspendCount;

			// see if thread was restarted
			if (OldCount<=1) {
				// thread was restarted
				m_ThreadRunning=true;
				return true;
			}
			else {
				// thread still suspended
				m_ThreadRunning=false;
				return false;
			}
		}
		else {
			// error, thread not resumed so leave state as is
			return false;
		}
	}
	else {
		// no thread handle, so can't resume
		return false;
	}
}

// delay the thread for milliseconds time
bool Thread::Delay(unsigned long const Milliseconds) 
{
	// thread then delay
	if (m_ThreadHandle) {
        if (m_DelaySemaphore.Wait(Milliseconds)==Semaphore::WaitTimeout) {
            // successfull time out
            return true;
        }
		else {
            // semaphore was signalled or error
            return false;
		}
	}
	else {
		// no thread so no delay
		return false;
	}
}

// stops the thread delay
void Thread::StopDelay() {
	// if thread is delayed then stop delay
	if (m_ThreadHandle) m_DelaySemaphore.Release();
}

// this sets the thread priority
bool Thread::SetPriority(ThreadPriority const Priority)
{
	if (m_ThreadHandle) {
		int PriorityCode;

		switch(Priority) {
		case TimeCritical: // time critical priority
			PriorityCode=THREAD_PRIORITY_TIME_CRITICAL;
			break;
		case Highest: // highest priority
			PriorityCode=THREAD_PRIORITY_HIGHEST;
			break;
		case AboveNormal: // Above normal priority
			PriorityCode=THREAD_PRIORITY_ABOVE_NORMAL; 
			break;
		case Normal: // normal priority
			PriorityCode=THREAD_PRIORITY_NORMAL; 
			break;
		case BelowNormal: // below normal priority
			PriorityCode=THREAD_PRIORITY_BELOW_NORMAL; 
			break;
		case Lowest: // lowest priority
			PriorityCode=THREAD_PRIORITY_LOWEST; 
			break;
		default:
		case Idle: // idle priority
			PriorityCode=THREAD_PRIORITY_IDLE; 
			break;
		}
		return static_cast<bool>(SetThreadPriority(m_ThreadHandle, PriorityCode));
	}
	else {
		// no thread
		return false;
	}
}

// this sets the thread to run on the specific processors
bool Thread::SetProcessors(CPUS const CPU)
{
	if (m_ThreadHandle) {
		// get the current processor affinity mask
		HANDLE CurrentProcess=GetCurrentProcess();
		DWORD ProcessAffinity, SystemAffinity;
		GetProcessAffinityMask(CurrentProcess, &ProcessAffinity, &SystemAffinity);
		
		// see if the current processors are set in the process affinity
		if ((CPU&ProcessAffinity)!=CPU) {
			// flags not set so add processor to process affinity 
			// if they are allowed by the system affinity
			ProcessAffinity|=(CPU&SystemAffinity);
			if (!SetProcessAffinityMask(CurrentProcess, ProcessAffinity)) {
				return false; // failure
			}
		}

		// set the thread affinity
		if (!SetThreadAffinityMask(m_ThreadHandle, (CPU&SystemAffinity))) {
			return false; // failure
		}
		else {
			return true; // succuess
		}
	}
	else {
		// no thread
		return false;
	}
}

// the ugly way to terminate the thread
bool Thread::Kill(unsigned int const ExitCode)
{
	// stop delay if there is any
	StopDelay();

    {
        CriticalSectionAuto AutoCS(m_ThreadCS);
        // set the thread running and enabled
	    m_ThreadRunning=false;
	    m_ThreadEnabled=false;
    }

	// terminate thread
	if (m_ThreadHandle) return static_cast<bool>(TerminateThread(m_ThreadHandle, ExitCode));
	else return true;
}

// terminate the current thread
void Thread::Terminate(unsigned int const ExitCode)
{
	// stop delay if there is any
	StopDelay();

    {
        CriticalSectionAuto AutoCS(m_ThreadCS);
	    // set the thread running and enabled
	    m_ThreadRunning=false;
	    m_ThreadEnabled=false;
    }
	
	// exit thread if thread exists
	if (m_ThreadHandle) _endthreadex(ExitCode);
}

// wait for completion the thread for milliseconds time
Thread::WaitResult Thread::WaitForCompletion(unsigned long const Milliseconds) 
{
	// thread then wait for completion
	if (m_ThreadHandle) {
        DWORD Result=WaitForSingleObject(m_ThreadHandle, Milliseconds);
        switch(Result) {
        case WAIT_OBJECT_0: // signaled
            return WaitSignaled;
        case WAIT_TIMEOUT:  // timeout
            return WaitTimeout;
        default:
            return WaitFailed;
        }
	}
	else {
		// no thread so no wait for completion
		return WaitFailed;
	}
}

// this is the function which is called when the thread is started
unsigned int Thread::Run(void *UserThreadData)
{
	// dummy routine
	while(m_ThreadEnabled) Sleep(1); 

	// exit the thread with a successful value
	return 0; // return successful
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define static functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// this is the static thread procedure which is called when the thread is started
unsigned int __stdcall Thread::ThreadProc(void* ThreadData)
{
	// call the virtual run function
	ThreadParameters* pTP=static_cast<ThreadParameters*>(ThreadData);
	unsigned int ExitCode=pTP->pThreadClass->Run(pTP->pUserTheadParameters);

    {
        CriticalSectionAuto AutoCS(pTP->pThreadClass->m_ThreadCS);
	    // set the thread running and enabled
	    pTP->pThreadClass->m_ThreadRunning=false;
	    pTP->pThreadClass->m_ThreadEnabled=false;
    }

    // return the exit code and end the thread
    return(ExitCode);
}

}; // namespace end


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012
//       1         2        3         4         5         6         7         8         9        10        11        12        13   
