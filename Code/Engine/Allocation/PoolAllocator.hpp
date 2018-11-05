#pragma once
#include "Allocation/BaseAllocator.hpp"
#include "Multithreading/CriticalSection.hpp"
#include "Math/Utils.hpp"


//////////////////////////////////////////////////////////////////////////////////////
//																					
//	An allocator that makes one allocated block of memory based on a desired count	
//	given to the constructor and the size of the objects being accessed.  It is		
//	important to remember that the memory is only freed on destruction, and not on	
//	calling Free(void*) for every pointer.											
//																					
//////////////////////////////////////////////////////////////////////////////////////
template<typename Block>
class PoolAllocator : public BaseAllocator
{
public:
	explicit PoolAllocator(U64 obj_count)
		: m_freeList(nullptr)
		, m_memory(nullptr)
		, m_allocCount(0)
	{
		m_blockSize = obj_count * (U64)Max(sizeof(Block), sizeof(Node));
		m_lock = new CriticalSection();
	};

	~PoolAllocator()
	{
		delete m_lock;
		::free(m_memory);
	};

	inline U32 GetAllocationCount() { return m_allocCount; };
	inline U64 GetFreeAllocation() { return m_blockSize - ((U64)m_allocCount * (U64)Max(sizeof(Block), sizeof(Node))); };
	inline void* GetBuffer() { return m_memory; };

protected:
	void* Allocate(U64)
	{
		U64 size = (U64)Max(sizeof(Block), sizeof(Node));

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
					if(size == (U64)sizeof(Block))
						ptr = (Block*)m_memory + m_allocCount;
					else
						ptr = (Node*)m_memory + m_allocCount;

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

	void Free(void* ptr, U64)
	{
		if (nullptr != ptr) 
		{
			SCOPE_LOCK(m_lock);
			Node* block = (Node*)ptr;
			block->next = m_freeList;
			m_freeList = block;
			--m_allocCount;
		}
	};

private:
	struct Node
	{
		Node* next;
	};

private:
	void* m_memory;
	Node* m_freeList;
	CriticalSection* m_lock;
	U64 m_blockSize;
	U64 m_allocCount;
};