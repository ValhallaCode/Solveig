#pragma once
#include "Allocation/BaseAllocator.hpp"
#include "Multithreading/CriticalSection.hpp"
#include "Math/Utils.hpp"


//////////////////////////////////////////////////////////////////////////////////////
//
//	Uses template datatype as smallest block size, and will allocate blocks based
//	on a power of 2 line up.  Can allocate for any datatype using Create<>, but
//	will suffer from internal fragmentation if datatypes do not fall on power of 2
//	sizes.
//
//////////////////////////////////////////////////////////////////////////////////////
template<typename SmallestBlock>
class BuddyAllocator : public BaseAllocator
{
private:
	BuddyAllocator(U64 obj_count)
		:m_allocCount(0)
	{
		obj_count = UpperPowerOfTwo(obj_count);
		block_size = UpperPowerOfTwo((U64)sizeof(SmallestBlock));

		m_memory = malloc(block_size * obj_count);
		m_lastAddr = (SmallestBlock*)m_memory + (obj_count - 1);

		int index = log2(obj_count);
		index = ClampWithin(index, 63, 0);

		// Create array obj
		InitFreeList(index, m_memory);

		for (int loop = 0; loop < 64; ++loop)
		{
			if (loop == index)
				continue;

			m_freeList[loop] = nullptr;
		}

		m_lock = new CriticalSection();
	};

	~BuddyAllocator()
	{
		free(m_memory);

		for (int index = 0; index < 64; index++)
		{
			if (!m_freeList[index])
				continue;

			if (!m_freeList[index]->m_head)
			{
				free(m_freeList[index]);
				continue;
			}

			Node* iterate = m_freeList[index]->m_head;

			while (iterate != nullptr)
			{
				Node* next = iterate->m_next;
				free(iterate);
				iterate = next;
			}

			free(m_freeList[index]);
		}

		delete m_lock;
	};

	struct Node
	{
		Node* m_prev = nullptr;
		Node* m_next = nullptr;
		void* m_data = nullptr;
	};

	struct MemList // size of 16
	{
		Node* m_head = nullptr;
		Node* m_last = nullptr;

		void Add(void* data)
		{
			Node* node_to_add = malloc(sizeof(Node));
			node_to_add->m_data = data;
			node_to_add->m_next = nullptr;
			node_to_add->m_prev = m_last;
			m_last->m_next = node_to_add;
			m_last = node_to_add;
		};

		void Remove(Node** node)
		{
			Node* prev = *node->m_prev;
			prev->m_next = *node->m_next;
			*node->m_next->m_prev = prev;

			free(*node);
			*node = prev;
		}
	};

	void InitFreeList(U64 index, void* data)
	{
		m_freeList[index] = malloc(sizeof(MemList));
		// Create list node
		m_freeList[index]->m_head = malloc(sizeof(Node));
		// Set init data members
		m_freeList[index]->m_head->m_prev = nullptr;
		m_freeList[index]->m_head->m_next = nullptr;
		m_freeList[index]->m_head->m_data = data;
		// Set end of list
		m_freeList[index]->m_last = m_freeList[index]->m_head;
	};

	void* SearchAndAddPointer(U64 size, U64 index, void* addr)
	{
		if ((U64)pow(2, index) == size)
			return addr;

		{
			SCOPE_LOCK(m_lock);
			if (!m_freeList[index - 1])
			{
				InitFreeList(index - 1, addr);
			}

			// Add address to indexed list
			m_freeList[index - 1]->Add(addr);
		}

		SearchAndAddPointer(size, index - 1, ((char*)addr + (U64)pow((U64)2, index) / (U64)2));
	};

public:
	inline U64 GetAllocationCount() { return m_allocCount; };
	inline void* GetBuffer() { return m_memory; };
protected:
	void* Allocate(U64 request_length)
	{
		request_length = UpperPowerOfTwo(request_length);
		U64 request_index = log2(request_length);

		while (!m_freeList[request_index])
		{
			if (request_index >= 64)
				return NULL;
			request_index++;
		}

		Node* end_of_list = m_freeList[request_index]->m_last;
		Node* head_of_list = m_freeList[request_index]->m_head;
		void* last_alloc = end_of_list->m_data;
		{
			SCOPE_LOCK(m_lock);
			// Pop last
			if (head_of_list == end_of_list)
			{
				//only 1 address in list
				free(end_of_list);
				free(m_freeList[request_index]);
				m_freeList[request_index] = nullptr;
			}
			else
			{
				// multiple in list
				Node* to_free = end_of_list;
				end_of_list->m_prev->m_next = nullptr;
				end_of_list = end_of_list->m_prev;
				free(to_free);
			}
		}

		++m_allocCount;
		return SearchAndAddPointer(request_length, request_index, last_alloc);
	};

	void Free(void* addr, U64 size)
	{
		// get index into free list
		U64 index = (U64)log2(size);

		void* buddy;
		// used to look for higher and lower buddy block
		int find_buddy = (((char*)m_memory - (char*)addr) / size);

		if (find_buddy % 2 != 0)
			buddy = (char*)addr - size;
		else
			buddy = (char*)addr + size;

		// Pointer is not valid if no buddy found
		if (buddy < m_memory || buddy > m_lastAddr)
			return;

		--m_allocCount;

		{
			SCOPE_LOCK(m_lock);

			if (m_freeList[index])
			{
				for (Node* iterate = m_freeList[index]->m_head; iterate != nullptr; iterate = iterate->m_next)
				{
					if (iterate->m_data == buddy)
					{
						// Remove iterate from list
						m_freeList[index]->Remove(&iterate);

						if (find_buddy % 2 != 0)
							addr = buddy;

						// Now removes buddy
						break;
					}
					else
					{
						// Add addr to free list
						m_freeList[index]->Add(addr);
						return;
					}
				}
				free(addr, size * 2);
			}
			// Creates Free list to use if non-existant
			InitFreeList(index, addr);
		}
	};

private:
	void* m_memory;
	void* m_lastAddr;
	MemList* m_freeList[64];
	CriticalSection* m_lock;
	U64 m_allocCount;
};
