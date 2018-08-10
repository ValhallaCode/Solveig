#include "Multithreading/CriticalSection.hpp"

//////////////////////////////////////////////////////
//													//
//					  Datatypes						//
//													//
//////////////////////////////////////////////////////
struct thread_pass_data
{
	thread_cb cb;
	void *arg;
};

//////////////////////////////////////////////////////
//													//
//				Class Structures					//
//													//
//////////////////////////////////////////////////////
CriticalSection::CriticalSection()
{
	InitializeCriticalSection(&m_windowsCritical);
}

CriticalSection::~CriticalSection()
{
	DeleteCriticalSection(&m_windowsCritical);
}

void CriticalSection::Lock()
{
	EnterCriticalSection(&m_windowsCritical);
}

void CriticalSection::Unlock()
{
	LeaveCriticalSection(&m_windowsCritical);
}

ScopedCriticalSection::ScopedCriticalSection(CriticalSection* cs)
{
	m_critical = cs;
	m_critical->Lock();
}

ScopedCriticalSection::~ScopedCriticalSection()
{
	m_critical->Unlock();
}

//////////////////////////////////////////////////////
//													//
//					Functions						//
//													//
//////////////////////////////////////////////////////
static DWORD WINAPI ThreadEntryPointCommon(void* arg)
{
	thread_pass_data* pass_ptr = (thread_pass_data*)arg;

	pass_ptr->cb(pass_ptr->arg);
	delete pass_ptr;
	return 0;
}

// Creates a thread with the entry point of cb, passed data
thread_handle ThreadCreate(thread_cb cb, void* data, wchar_t* thread_name)
{
	// handle is like pointer, or reference to a thread
	// thread_id is unique identifier
	thread_pass_data* pass = new thread_pass_data();
	pass->cb = cb;
	pass->arg = data;

	DWORD thread_id;
	thread_handle th = ::CreateThread(nullptr,   // SECURITY OPTIONS
		0,                         // STACK SIZE, 0 is default
		ThreadEntryPointCommon,    // "main" for this thread
		pass,                     // data to pass to it
		0,                         // initial flags
		&thread_id);              // thread_id

	#ifdef _DEBUG
		SetThreadDescription(th, thread_name);
	#endif

	return th;
}

void ThreadSleep(unsigned int ms)
{
	::Sleep(ms);
}

void ThreadYield()
{
	::SwitchToThread();
}

// Releases my hold on this thread.
void ThreadDetach(thread_handle th)
{
	::CloseHandle(th);
}

void ThreadJoin(thread_handle th)
{
	::WaitForSingleObject(th, INFINITE);
	::CloseHandle(th);
}
