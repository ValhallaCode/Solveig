#pragma once
#include "Allocation/BaseAllocator.hpp"
#include "Multithreading/CriticalSection.hpp"
#include "Math/Utils.hpp"


template<typename Object>
class PoolAllocator : public BaseAllocator
{
public:
	PoolAllocator(U64 obj_count)
		: m_freeList(nullptr)
		, m_memory(nullptr)
		, m_allocCount(0)
	{
		m_blockSize = obj_count * (U64)Max(sizeof(Object), sizeof(Block));
		m_lock = new CriticalSection();
	};

	~PoolAllocator()
	{
		delete m_lock;
		::free(m_memory);
	};

	inline U32 GetAllocationCount() { return m_allocCount; };
	inline U64 GetFreeAllocation() { return m_blockSize - ((U64)m_allocCount * (U64)Max(sizeof(Object), sizeof(Block))); };
	inline void* GetBuffer() { return m_memory; };

protected:
	void* Allocate()
	{
		U64 size = (U64)Max(sizeof(Object), sizeof(Block));

		if (size > GetFreeAllocation())
		{
			return nullptr;
		}

		void* ptr = nullptr;

		// For the Scope Lock
		{
			SCOPE_LOCK(m_lock);
			if (nullptr == m_freeList) 
			{ 
				if (nullptr == m_memory)
				{
					++m_allocCount;
					m_memory = ::malloc(m_blockSize);
					return m_memory;
				}
				else
				{
					if(size == (U64)sizeof(Object))
						ptr = (Object*)m_memory + m_allocCount;
					else
						ptr = (Block*)m_memory + m_allocCount;

					++m_allocCount;
					return ptr;
				}
			}
			else 
			{
				ptr = m_freeList;
				m_freeList = m_freeList->next;
				++m_allocCount;
				return ptr;
			}
		}
	};

	void Free(void* ptr)
	{
		if (nullptr != ptr) 
		{
			SCOPE_LOCK(m_lock);
			Block* block = (Block*)ptr;
			block->next = m_freeList;
			m_freeList = block;
			--m_allocCount;
		}
	};

private:
	struct Block
	{
		Block* next;
	};

private:
	void* m_memory;
	Block* m_freeList;
	CriticalSection* m_lock;
	U64 m_blockSize;
	U64 m_allocCount;
};