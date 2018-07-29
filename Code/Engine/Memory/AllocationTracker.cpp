#include "Memory\AllocationTracker.hpp"
#include "IO\Callstack.hpp"
#include "Time\Utils.hpp"
#include <stdlib.h>
#include <string>
// #TODO Remove std::string

//////////////////////////////////////////////////////
//													//
//					  Macros						//
//													//
//////////////////////////////////////////////////////
#define convertToKiB(size) size / 1024UL
#define convertToMiB(size) size / (1024UL * 1024UL)
#define convertToGiB(size) size / (1024UL * 1024UL * 1024UL)
#define convertToReadableBytes(size) ((uint32_t)size > (2UL * 1024UL) && (uint32_t)size < (2UL * 1024UL * 1024UL)) ? (float)size * (float)convertToKiB(1) : ((uint32_t)size > (2UL * 1024UL * 1024UL) && (uint32_t)size < (2UL * 1024UL * 1024UL * 1024UL)) ? (float)size * (float)convertToMiB(1) : ((uint32_t)size > (2UL * 1024UL * 1024UL * 1024UL)) ? (float)size * (float)convertToGiB(1) : (float)size


//////////////////////////////////////////////////////
//													//
//					Definitions						//
//													//
//////////////////////////////////////////////////////
struct callstack_list // Treat as Linked List
{
	CallStack* current_stack = nullptr;
	uint16_t total_allocation = 0;
	callstack_list* next = nullptr;
};

struct allocation_meta
{
	uint16_t size;
	#if defined(TRACK_MEMORY)
		#if (TRACK_MEMORY == TRACK_MEMORY_VERBOSE)
			callstack_list callstack_node;
		#endif
	#endif
};

static uint64_t g_alloc_count = 0;
static uint64_t g_frame_allocs = 0;
static uint64_t g_frame_frees = 0;
static uint32_t g_allocated_byte_count = 0;
static uint32_t g_max_allocated_byte_count = 0;
static uint64_t g_alloc_hw = 0;
static uint16_t g_byte_hw = 0;
static bool g_was_report_run = false;
static callstack_list* g_callstack_root = nullptr;
static callstack_list* g_last_node = nullptr;


//////////////////////////////////////////////////////
//													//
//					 Getters						//
//													//
//////////////////////////////////////////////////////
uint64_t GetCurrentAllocationCount() { return g_alloc_count; }
uint64_t GetCurrentFrameAllocationCount() { return g_frame_allocs; }
uint64_t GetCurrentFrameFreeCount() { return g_frame_frees; }
uint64_t GetCurrentAllocationCountHighWater() { return g_alloc_hw; }
uint32_t GetCurrentAllocationSizeInBytes() { return g_allocated_byte_count; }
uint16_t GetCurrentAllocationSizeHighWaterInBytes() { return g_byte_hw; }
uint32_t GetCurrentMaxAllocationSizeInBytes() { return g_max_allocated_byte_count; }
uint64_t GetCurrentAllocationOverflowInBytes() { return g_alloc_hw - (uint64_t)g_max_allocated_byte_count; }

//////////////////////////////////////////////////////
//													//
//					 Setters						//
//													//
//////////////////////////////////////////////////////
void SetMaxMemoryInBytes(uint32_t bytes) { g_max_allocated_byte_count = bytes; }
void SetMaxMemoryInKiB(uint32_t kib) { g_max_allocated_byte_count = (uint32_t)convertToKiB(kib); }
void SetMaxMemoryInMiB(uint32_t mib) { g_max_allocated_byte_count = (uint32_t)convertToMiB(mib); }
void SetMaxMemoryInGiB(uint32_t gib) { g_max_allocated_byte_count = (uint32_t)convertToGiB(gib); }


//////////////////////////////////////////////////////
//													//
//					Functions						//
//													//
//////////////////////////////////////////////////////
void ResetFrameMemTrack()
{
	g_frame_allocs = 0;
	g_frame_frees = 0;
}

void SplitList(callstack_list* head, callstack_list** out_front, callstack_list** out_back)
{
	callstack_list* fast_walk = head->next;
	callstack_list* slow_walk = head;

	// Advance 'fast' two nodes, and advance 'slow' one node
	while (nullptr != fast_walk)
	{
		fast_walk = fast_walk->next;
		if (nullptr != fast_walk)
		{
			slow_walk = slow_walk->next;
			fast_walk = fast_walk->next;
		}
	}

	// 'slow' is before the midpoint in the list, so split it in two at that point.
	*out_front = head;
	*out_back = slow_walk->next;
	slow_walk->next = nullptr;
}

void Merge(callstack_list* front, callstack_list* back, callstack_list** out_list)
{
	if (nullptr == front) 
	{
		*out_list = back;
		return;
	}
	else if (nullptr == back) 
	{
		*out_list = front;
		return;
	}

	callstack_list* merge_list = nullptr;
	callstack_list** current_merge = &merge_list;

	while(true) 
	{
		if (back->total_allocation > front->total_allocation)
		{  
			// Back is Greater

			*current_merge = back;
			current_merge = &back->next;
			back = *current_merge;
			if (nullptr == back)
			{
				*current_merge = front;
				break;
			}
		}
		else 
		{  
			// Front is Greater

			*current_merge = front;
			current_merge = &front->next;
			front = *current_merge;
			if (nullptr == front) 
			{
				*current_merge = back;
				break;
			}
		}
	}
	
	*out_list = merge_list;
}

void SortCallstack(callstack_list** list_to_sort)
{
	callstack_list* head = *list_to_sort;
	callstack_list* front = nullptr;
	callstack_list* back = nullptr;
	
	// Base case -- length 0 or 1
	if ((nullptr == head) || (nullptr == head->next))
		return;
	
	// Split head into 'a' and 'b' sublists
	SplitList(head, &front, &back);
	
	// Recursively sort the sublists
	SortCallstack(&front);
	SortCallstack(&back);
	
	// answer = merge the two sorted lists together
	Merge(front, back, list_to_sort);
}

void MakeCopyOfCallstackList(callstack_list** new_list)
{
	callstack_list* copy_head = nullptr;
	callstack_list* current_copy = nullptr;
	callstack_list* current_true = g_callstack_root;

	while (nullptr != current_true)
	{
		callstack_list* temp = (callstack_list*) ::calloc(1, sizeof(callstack_list));

		if (nullptr == copy_head)
		{
			copy_head = temp;
			current_copy = copy_head;
		}
		else
		{
			current_copy->next = temp;
			current_copy = current_copy->next;
		}

		current_copy->current_stack = current_true->current_stack;
		current_copy->total_allocation = current_true->total_allocation;
		current_copy->next = nullptr;

		current_true = current_true->next;
	}

	*new_list = copy_head;
}

void FreeCopyOfCallstackList(callstack_list** new_list)
{
	callstack_list* current_copy = *new_list;

	while (nullptr != current_copy)
	{
		callstack_list* temp = current_copy->next;
		::free(current_copy);
		current_copy = temp;
	}
}

void ReportVerboseCallStacks(const char* start_time_str /*= ""*/, const char* end_time_str /*= ""*/, bool print_long_report /*= false*/)
{
	#ifndef TRACK_MEMORY
		#if TRACK_MEMORY != TRACK_MEMORY_VERBOSE
			return;
		#endif
	#endif

	g_was_report_run = true;

	double startTime = 0.0;
	if (start_time_str[0] != '\0')
	{
		startTime = std::stod(start_time_str);
	}

	double endTime = GetCurrentTimeSeconds();
	if (end_time_str[0] != '\0')
	{
		endTime = std::stod(end_time_str);
	}

	callstack_list* sorted_list = nullptr;
	MakeCopyOfCallstackList(&sorted_list);

	SortCallstack(&sorted_list);

	uint totalSimiliarAllocs = 0;
	uint32_t totalSimiliarSize = 0;

	float reportedTotalBytes = convertToReadableBytes(g_allocated_byte_count);
	char sizeTotal[4] = { 'B', 'y', 't', NULL };

	if (g_allocated_byte_count > (2 * 1024) && g_allocated_byte_count < (2 * 1024 * 1024))
	{
		sizeTotal[0] = 'K'; 
		sizeTotal[1] = 'i'; 
		sizeTotal[2] = 'B';
	}
	else if ((unsigned long)g_allocated_byte_count >(2UL * 1024UL * 1024UL) && (unsigned long)g_allocated_byte_count < (2UL * 1024UL * 1024UL * 1024UL))
	{
		//MB
		sizeTotal[0] = 'M';
		sizeTotal[1] = 'i';
		sizeTotal[2] = 'B';
	}
	else if ((unsigned long)g_allocated_byte_count >(2UL * 1024UL * 1024UL * 1024UL))
	{
		//GB
		sizeTotal[0] = 'G';
		sizeTotal[1] = 'i';
		sizeTotal[2] = 'B';
	}

	char init_buffer[64];
	sprintf_s(init_buffer, 64, "\n%llu leaked allocation(s).  Total: %0.3f %s\n", g_alloc_count, reportedTotalBytes, sizeTotal);
	//OutputDebugStringA(init_buffer);
	printf(init_buffer);
	//LogTaggedPrintf("leaks", init_buffer);

	callstack_list* currentNode = sorted_list;

	while(currentNode != nullptr && print_long_report)
	{
		callstack_list* nextNode = currentNode->next;

		uint32_t& currentHash = currentNode->current_stack->m_hash;
		uint32_t nextHash;
		if (nextNode == nullptr)
			nextHash = currentHash + 1;
		else
			nextHash = nextNode->current_stack->m_hash;

		if (nextHash == currentHash)
		{
			totalSimiliarSize += currentNode->total_allocation;
			totalSimiliarAllocs++;
		}

		if (nextHash != currentHash)
		{
			// Want to include the last node
			totalSimiliarSize += currentNode->total_allocation;
			totalSimiliarAllocs++;

			//Print total allocs for type and total size
			float reportedBytes = convertToReadableBytes(totalSimiliarSize);
			char size[4] = { 'B', NULL, NULL, NULL };

			if (g_allocated_byte_count > (2 * 1024) && g_allocated_byte_count < (2 * 1024 * 1024))
			{
				size[0] = 'K';
				size[1] = 'i';
				size[2] = 'B';
			}
			else if ((unsigned long)g_allocated_byte_count >(2UL * 1024UL * 1024UL) && (unsigned long)g_allocated_byte_count < (2UL * 1024UL * 1024UL * 1024UL))
			{
				//MB
				size[0] = 'M';
				size[1] = 'i';
				size[2] = 'B';
			}
			else if ((unsigned long)g_allocated_byte_count >(2UL * 1024UL * 1024UL * 1024UL))
			{
				//GB
				size[0] = 'G';
				size[1] = 'i';
				size[2] = 'B';
			}

			char collection_buffer[128];
			sprintf_s(collection_buffer, 128, "\nGroup contained %u allocation(s), Total: %0.3f %s\n", totalSimiliarAllocs, reportedBytes, size);
			//OutputDebugStringA(collection_buffer);
			printf(collection_buffer);
			//LogTaggedPrintf("allocs", collection_buffer);

			//Reset total allocs and size
			totalSimiliarAllocs = 0;
			totalSimiliarSize = 0;

			// Printing a call stack, happens when making report
			char line_buffer[512];
			callstack_line_t lines[128];
			uint line_count = CallstackGetLines(lines, 128, currentNode->current_stack);
			for (uint i = 0; i < line_count; ++i)
			{
				// this specific format will make it double click-able in an output window 
				// taking you to the offending line.

				if ((currentNode->current_stack->m_time >= startTime - 0.00001 && currentNode->current_stack->m_time <= endTime + 0.00001))
				{
					//Print Line For Call Stack
					sprintf_s(line_buffer, 512, "     %s(%u): %s\n", lines[i].file_name, lines[i].line, lines[i].function_name);
				}

				// print to output and console
				//OutputDebugStringA(line_buffer);
				printf(line_buffer);
				//LogTaggedPrintf("stack", line_buffer);
			}
		}

		currentNode = currentNode->next;
	}

	FreeCopyOfCallstackList(&sorted_list);
}

void ReportEntireVerboseCallStackList(bool print_long_report /*= false*/)
{
	ReportVerboseCallStacks("", "", print_long_report);
}

//////////////////////////////////////////////////////
//													//
//					Operators						//
//													//
//////////////////////////////////////////////////////
void* operator new(const size_t size)
{
	++g_alloc_count;
	++g_frame_allocs;
	g_allocated_byte_count += size;

	#if (g_allocated_byte_count > g_max_allocated_byte_count && DETECT_MEMORY_OVERRUN > g_allocated_byte_count)
		#define DETECT_MEMORY_OVERRUN g_allocated_byte_count - g_max_allocated_byte_count;
	#endif

	if (g_alloc_count > g_alloc_hw)
		g_alloc_hw = g_alloc_count;

	uint16_t alloc_size = (uint16_t)size + (uint16_t)sizeof(allocation_meta);
	allocation_meta *ptr = (allocation_meta*) ::calloc(1, (size_t)alloc_size);
	ptr->size = (uint16_t)size;

	if (alloc_size > g_byte_hw)
		g_byte_hw = alloc_size;

	// Verbose Tracking
	#if (TRACK_MEMORY == TRACK_MEMORY_VERBOSE)
		ptr->callstack_node.current_stack = CreateCallstack(0);
		ptr->callstack_node.total_allocation = (uint16_t)size;
		ptr->callstack_node.next = nullptr;
	
		if(g_last_node)
			g_last_node->next = &ptr->callstack_node;

		g_last_node = &ptr->callstack_node;
	
		if (g_callstack_root == nullptr)
		{
			g_callstack_root = &ptr->callstack_node;
		}
	#endif

	return ptr + 1;
}

void operator delete(void* ptr)
{
	if (nullptr == ptr)
		return;

	--g_alloc_count;
	++g_frame_frees;

	allocation_meta *data = (allocation_meta*)ptr;
	data--;

	g_allocated_byte_count -= (uint32_t)data->size;

	// Verbose Tracking
	#if (TRACK_MEMORY == TRACK_MEMORY_VERBOSE)
		if (data->callstack_node.current_stack != nullptr)
			DestroyCallstack(data->callstack_node.current_stack);
	
		//Set pointer to current stack null
		bool run = true;
		callstack_list* currentNode = nullptr;
		while (g_callstack_root != nullptr && run && &data->callstack_node != NULL)
		{
			if (currentNode == nullptr && g_callstack_root != &data->callstack_node)
			{
				currentNode = g_callstack_root;
			}
			else
			{
				g_callstack_root = nullptr;
				run = false;
				continue;
			}
	
			if (currentNode->next != nullptr && currentNode->next != &data->callstack_node)
			{
				currentNode = currentNode->next;
			}
			else
			{
				currentNode->next = nullptr;
				run = false;
			}
		}
	#endif

	::free(data);
}
