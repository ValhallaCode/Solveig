#pragma once
#include "Core/NumberDef.hpp"
#include "Memory/AllocationTracker.hpp"
#include "Multithreading/CriticalSection.hpp"
#include "Math/Utils.hpp"

template<typename Object>
Object DefaultError()
{
	printf("ERROR: Tried to retrieve from empty buffer!");
	return Object();
};

//////////////////////////////////////////////////////////////////////////////////////
//
//	Essentially a max sized queue that can either override on write once full,
//	or can stop and ignore additional inputs.  It will wrap and override by default.
//
//////////////////////////////////////////////////////////////////////////////////////
template <class Object>
class RingBuffer
{
public:
	// An error handling function that returns an Object Type
	typedef Object(*error_callback)();

	explicit RingBuffer(U64 obj_count) 
		:m_canWrap(true)
	{
		m_lock = new CriticalSection();
		m_memory = malloc(sizeof(Object) * obj_count);
		m_endOfBuffer = (Object*)m_memory + obj_count;
		m_head = m_memory;
		m_tail = m_head;
		m_emptyError = DefaultError<Object>;
	};

	~RingBuffer()
	{
		delete m_lock;
		free(m_memory);
	};

	void Push(Object item)
	{
		SCOPE_LOCK(m_lock);

		*(Object*)m_head = item;

		bool is_full = Full();
		if (is_full && m_canWrap)
		{
			if ((Object*)m_tail + 1 > m_endOfBuffer)
				m_tail = m_memory;
			else
				m_tail = (Object*)m_tail + 1;
		}
		else if (is_full && !m_canWrap)
			return;

		if ((Object*)m_head + 1 > m_endOfBuffer)
			m_head = m_memory;
		else
			m_head = (Object*)m_head + 1;
	};
	
	Object Pop()
	{
		SCOPE_LOCK(m_lock);

		// throw error?
		if (Empty())
		{
			return m_emptyError();
		}

		Object obj = *(Object*)m_tail;

		if ((Object*)m_tail + 1 > m_endOfBuffer)
			m_tail = m_memory;
		else
			m_tail = (Object*)m_tail + 1;

		return obj;
	};

	void Reset()
	{
		SCOPE_LOCK(m_lock);
		m_head = m_tail;
	};

	inline bool Empty() const
	{
		//if head and tail are equal, we are empty
		return m_head == m_tail;
	};

	bool Full() const
	{
		//If tail is ahead the head by 1, we are full
		Object* head_adv = (Object*)m_head + 1;

		if (head_adv > m_endOfBuffer)
			head_adv = (Object*)m_memory;

		return m_tail == head_adv;
	};

	inline U64 Capacity() const
	{
		return (U64)( ((Object*)m_endOfBuffer - (Object*)m_memory) );
	};

	U64 Size() const
	{
		U64 size = Capacity();

		if (!Full())
		{
			if (m_head >= m_tail)
			{
				size = (U64)( (Object*)m_head - (Object*)m_tail );
			}
			else
			{
				size = size - (U64)( (Object*)m_tail - (Object*)m_head );
			}
		}

		return size;
	};

	inline void SetEmptyErrorCallback(const error_callback& cb) { m_emptyError = cb; };
	inline void SetCanWrap(const bool& can_wrap) { m_canWrap = can_wrap; };
	inline void* GetBuffer() const { return m_memory; };
	inline bool GetWrap() const { return m_canWrap; };
	inline U64 GetHeadIndex() const { return (U64)( (Object*)m_head - (Object*)m_memory ); };
	inline U64 GetTailIndex() const { return (U64)( (Object*)m_tail - (Object*)m_memory ); };


	void operator=(const RingBuffer<Object>& buffer)
	{
		void* buff_mem = buffer.GetBuffer();
		U64 my_cap = Capacity();
		U64 buff_cap = buffer.Capacity();
		U64 mem_size = Min(my_cap, buff_cap);

		memcpy(m_memory, buff_mem, mem_size * sizeof(Object));

		U64 buff_head_idx = buffer.GetHeadIndex();
		U64 buff_tail_idx = buffer.GetTailIndex();

		U64 my_head_idx = 0;
		if (buff_head_idx > my_cap)
			my_head_idx = my_cap;
		else if(my_cap > buff_cap)
			my_head_idx = my_cap - 1;
		else
			my_head_idx = buff_head_idx;

		U64 my_tail_idx = 0;
		if (buff_tail_idx < my_cap && my_cap < buff_cap)
			my_tail_idx = buff_tail_idx;

		m_head = (Object*)m_memory + my_head_idx;
		m_tail = (Object*)m_memory + my_tail_idx;
		m_endOfBuffer = (Object*)m_memory + my_cap;
		m_canWrap = buffer.m_canWrap;
	};

private:
	CriticalSection* m_lock;
	void* m_memory;
	void* m_head;
	void* m_tail;
	void* m_endOfBuffer;
	error_callback m_emptyError;
	bool m_canWrap;
};