//       1         2         3         4         5         6         7         8         9        10        11        12        13   
//3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// $File: //AviSynth/Main/MVTools/Critical Section.cpp $
// $Revision: #1 $
// $DateTime: 2008/09/05 17:25:31 $
//
// Description: 
//     This is the implementation file for the critical section class.
// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Critical Section.hpp"

namespace ThreadFunctions {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define constructor/destructor functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// create critical section
CriticalSection::CriticalSection(unsigned long const SpinCount) :
    m_SpinCount(SpinCount)
{
    if (InitializeCriticalSectionAndSpinCount(&m_CriticalSection, static_cast<DWORD>(SpinCount))) {
        // critical section succeeded
	    m_NumLocks=0;
    }
    else {
        // critical section failed
        throw;
    }
}

// unlock and delete critical section
CriticalSection::~CriticalSection() {
	// release all outstanding locks
	while(m_NumLocks) {
		--m_NumLocks;
		LeaveCriticalSection(&m_CriticalSection);
	}
	DeleteCriticalSection(&m_CriticalSection);	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define member access functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// whether critical section is locked or not
bool CriticalSection::IsLocked() const {
	if (m_NumLocks>0) return true;
	else return false;
}

// returns the number of locks
unsigned int CriticalSection::NumLocks() const
{
	return m_NumLocks;
}

// returns the spin count of the critical section
unsigned long CriticalSection::SpinCount() const
{
    return m_SpinCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define member operations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// lock critical section
void CriticalSection::Lock() {
	// enter the critical section an increase the number of locks
	EnterCriticalSection(&m_CriticalSection); 
	++m_NumLocks;
}

void CriticalSection::Unlock() {
	// if locks on critical section then leave it and decrease lock count
	if (m_NumLocks) {
		--m_NumLocks;
		LeaveCriticalSection(&m_CriticalSection); 
	}
}

// try to lock and enter the critical section.  It returns true if successful.
bool CriticalSection::TryLock()
{
	// try to enter the critical section an increase the number of locks
    if (TryEnterCriticalSection(&m_CriticalSection)) {
        // entered critical section
	    ++m_NumLocks;
        return true;
    }
    else {
        // failed to enter critical section
        return false;
    }
}

// this function sets the spin count of the critical section and returns the old spin count
unsigned long CriticalSection::SetSpinCount(unsigned long const SpinCount)
{
    m_SpinCount=SpinCount;
	return SetCriticalSectionSpinCount(&m_CriticalSection, SpinCount); 
}

}; // end namespace


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012
//       1         2        3         4         5         6         7         8         9        10        11        12        13   