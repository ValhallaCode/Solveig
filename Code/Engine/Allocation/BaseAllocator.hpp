#pragma once
#include "Core/NumberDef.hpp"
#include "Memory/AllocationTracker.hpp"

class BaseAllocator
{
protected:
	virtual void* Allocate(U64 size) = 0;
	virtual void Free(void* pointer, U64 size) = 0;

public:
	template <typename Object, typename ...ARGS>
	Object* Create(ARGS ...args)
	{
		void* pointer = Allocate((U64)sizeof(Object));
		return new (pointer) Object(args...);
	}

	template <typename Object>
	void Destroy(Object* obj)
	{
		obj->~Object();
		Free(obj, (U64)sizeof(Object));
	}
};