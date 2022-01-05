//       1         2         3         4         5         6         7         8         9        10        11        12        13   
//3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// $File: //AviSynth/Main/MVTools/Semaphore.cpp $
// $Revision: #1 $
// $DateTime: 2008/09/05 17:25:31 $
//
// Description: 
//     This is the implementation file for the semaphore class.
// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Semaphore.hpp"

#pragma warning (disable: 4800) // disable bool performance warning

namespace ThreadFunctions {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define constructor/destructor functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// create semaphore
Semaphore::Semaphore(const char* const Name, long const MaximumCount, long const InitialCount, 
                     SECURITY_ATTRIBUTES* pSemaphoreAttributes)
{
	m_Semaphore=CreateSemaphore(pSemaphoreAttributes, InitialCount,	MaximumCount, Name);
}

// delete semaphore object
Semaphore::~Semaphore() {
	if (m_Semaphore) CloseHandle(m_Semaphore);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define member access functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// returns a handle to the semaphore
HANDLE Semaphore::Handle() const
{
	return m_Semaphore;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define member operations 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// release the semaphore
bool Semaphore::Release(long const Count) const {
	if (m_Semaphore) 
		return static_cast<bool>(ReleaseSemaphore(m_Semaphore, Count, 0));
	else 
		return false;
}

// wait for semaphore to be signaled for milliseconds time.  It returnes true if semaphore was signaled or false if timeout or
// error.
Semaphore::WaitResult Semaphore::Wait(unsigned long const Milliseconds) 
{
	// thread then wait for completion
    if (m_Semaphore) {
        DWORD Result=WaitForSingleObject(m_Semaphore, Milliseconds);
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
        // no semaphore        
        return WaitFailed;
    }
};

}; // end namespace

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012
//       1         2        3         4         5         6         7         8         9        10        11        12        13   