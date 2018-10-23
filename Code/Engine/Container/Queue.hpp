#pragma once
#include "Engine/Multithreading/CriticalSection.hpp"
#include <cstdint>

template <typename Object>
class Queue
{
public:
	void Push(const Object& data)
	{
		SCOPE_LOCK(m_lock);

		Node* node = new Node();				// create node
		node->m_item = data;					// set node pointers
		node->m_next = nullptr;

		if (nullptr == m_front)			// if queue is empty,
			m_front = node;					// place item at front
		else
			m_rear->m_next = node;				// else place at rear

		m_rear = node;							// have rear point to new node
		++m_count;						// increment count
	}

	bool IsEmpty()
	{
		SCOPE_LOCK(m_lock);

		return 0 == m_count;
	}

	bool Pop()
	{
		SCOPE_LOCK(m_lock);

		if (IsEmpty())
			return false;

		Node* temp = m_front; 				// save location of first item

		m_front = m_front->m_next; 				// reset front to next item
		delete temp;						// delete former first item

		if (nullptr == m_front)
			m_rear = nullptr;

		--m_count;							// decrement count

		return true;
	}

	Object Front()
	{
		SCOPE_LOCK(m_lock);

		if (IsEmpty())
			return (Object)0;

		return m_front->m_item;
	}

	Object Rear()
	{
		SCOPE_LOCK(m_lock);

		if (IsEmpty())
			return (Object)0;

		return m_rear->m_item;
	}

	Queue<Object>& operator=(const Queue<Object>& q) { return *this; };

	uint16_t GetCount()
	{
		return m_count;
	};

	~Queue()
	{
		Node* temp = nullptr;

		while (nullptr != m_front)
		{
			temp = m_front;
			m_front = m_front->m_next;
			delete temp;
		}

		delete m_lock;
	}

	Queue()
		: m_front(nullptr)
		, m_rear(nullptr)
		, m_count(0)
	{
		m_lock = new CriticalSection();
	}

private:
	struct Node
	{
		Object m_item;
		struct Node* m_next = nullptr;
	};

	Node* m_front;
	Node* m_rear;
	CriticalSection* m_lock;
	uint16_t m_count;
};