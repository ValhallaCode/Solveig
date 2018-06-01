#include "Memory\AllocationTracker.hpp"
#include <stdlib.h>

//////////////////////////////////////////////////////
//													//
//					Definitions						//
//													//
//////////////////////////////////////////////////////
struct allocation_meta
{
	size_t size;
};

static uint64_t g_alloc_count = 0;
static uint64_t g_frame_allocs = 0;
static uint64_t g_frame_frees = 0;
static size_t g_allocated_byte_count = 0;
static uint64_t g_alloc_hw = 0;
static size_t g_byte_hw = 0;
static bool g_was_report_run = false;

//////////////////////////////////////////////////////
//													//
//					 Getters						//
//													//
//////////////////////////////////////////////////////
uint64_t GetCurrentAllocationCount() { return g_alloc_count; }
uint64_t GetCurrentFrameAllocationCount() { return g_frame_allocs; }
uint64_t GetCurrentFrameFreeCount() { return g_frame_frees; }
uint64_t GetCurrentAllocationCountHighWater() { return g_alloc_hw; }
size_t GetCurrentAllocationSizeInBytes() { return g_allocated_byte_count; }
size_t GetCurrentAllocationSizeHighWaterInBytes() { return g_byte_hw; }

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

	if (g_alloc_count > g_alloc_hw)
		g_alloc_hw = g_alloc_count;

	size_t alloc_size = size + sizeof(allocation_meta);
	allocation_meta *ptr = (allocation_meta*)::calloc(1, alloc_size);
	ptr->size = size;

	if (alloc_size > g_byte_hw)
		g_byte_hw = alloc_size;

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

	g_allocated_byte_count -= data->size;

	::free(data);
}
