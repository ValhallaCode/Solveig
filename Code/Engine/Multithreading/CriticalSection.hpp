#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
// #TODO: Remove Tuple
#include <tuple>
// #TODO: Remove Utility
#include <utility>

// Datatypes
typedef void* thread_handle;
typedef void(*thread_cb)(void*);

class CriticalSection
{
public:
	CriticalSection();
	~CriticalSection();
	void Lock();
	void Unlock();
public:
	CRITICAL_SECTION m_windowsCritical;
};

class ScopedCriticalSection
{
public:
	ScopedCriticalSection(CriticalSection* cs);
	~ScopedCriticalSection();
public:
	CriticalSection* m_critical;
};

// Defines
#define INVALID_THREAD_HANDLE 0
#define COMBINE_1(X,Y) X##Y
#define COMBINE(X,Y) COMBINE_1(X,Y)
#define SCOPE_LOCK( csp ) ScopedCriticalSection COMBINE(__scs_,__LINE__)(csp)

// Functions
thread_handle ThreadCreate(thread_cb cb, void *data, wchar_t* thread_name);
void ThreadSleep(unsigned int ms);
void ThreadDetach(thread_handle th);
void ThreadJoin(thread_handle th);
void ThreadYield();

// Templates
template <typename CB, typename ...ARGS>
struct pass_data
{
	CB cb;
	// Uses Tuple
	std::tuple<ARGS...> args;

	pass_data(CB cb, ARGS ...args)
		: cb(cb)
		, args(args...) {}
};

template <typename CB, typename TUPLE, size_t ...INDICES>
void ForwardArgumentsWithIndices(CB cb, TUPLE &args, std::integer_sequence<size_t, INDICES...>&)
{
	// Uses Utility
	cb(std::get<INDICES>(args)...);
}

template <typename CB, typename ...ARGS>
void ForwardArgumentsThread(void *ptr)
{
	pass_data<CB, ARGS...> *args = (pass_data<CB, ARGS...>*) ptr;
	// Uses Utility
	auto seq_args = std::make_index_sequence<sizeof...(ARGS)>();
	ForwardArgumentsWithIndices(args->cb, args->args, seq_args);
	delete args;
}

template <typename CB, typename ...ARGS>
thread_handle ThreadCreate(wchar_t* thread_name, CB entry_point, ARGS ...args)
{
	pass_data<CB, ARGS...> *pass = new pass_data<CB, ARGS...>(entry_point, args...);
	return ThreadCreate(ForwardArgumentsThread<CB, ARGS...>, (void*)pass, thread_name);
}