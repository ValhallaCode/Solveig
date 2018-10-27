#pragma  once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "Core/NumberDef.hpp"


inline unsigned int AtomicAdd(volatile unsigned int* ptr, const unsigned int value)
{
	return (unsigned int) ::InterlockedAddNoFence((volatile long*)ptr, (long)value);
}

inline unsigned int AtomicIncrement(unsigned int* ptr)
{
	return (unsigned int) ::InterlockedIncrementNoFence((volatile long*)ptr);
}

inline unsigned int AtomicDecrement(unsigned int* ptr)
{
	return (unsigned int) ::InterlockedDecrementNoFence((volatile long*)ptr);
}

inline unsigned int CompareAndSet(volatile unsigned int* ptr, const unsigned int comparand, const unsigned int value)
{
	return ::InterlockedCompareExchange(ptr, value, comparand);
}

inline bool CompareAndSet64(volatile U64* data, const U64* comparand, const U64* value)
{
	return 1 == ::InterlockedCompareExchange64((long long volatile*)data, *(long long*)value, *(long long*)comparand);
}

template <typename T>
inline T* CompareAndSetPointer(T* volatile *ptr, T* comparand, T* value)
{
	return (T*)::InterlockedCompareExchangePointerNoFence((PVOID volatile*)ptr, (PVOID)value, (PVOID)comparand);
}