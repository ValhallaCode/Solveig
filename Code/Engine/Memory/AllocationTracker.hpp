#pragma once
#include <cstdint>

// Defines
#define DETECT_MEMORY_OVERRUN (0)
#define TRACK_MEMORY_BASIC    (0)
#define TRACK_MEMORY_VERBOSE  (1)

	// If not defined, we will not track memory at all
	// BASIC will track bytes used, and count
	// VERBOSE will track individual call stacks
#if defined(_DEBUG)
	#define TRACK_MEMORY           TRACK_MEMORY_VERBOSE
	#define PROFILED_BUILD
#elif defined(FINAL_BUILD)
	// undefined
#else 
	#define TRACK_MEMORY           TRACK_MEMORY_BASIC
#endif

// Datatypes


// Getters
uint64_t GetCurrentAllocationCount();
uint64_t GetCurrentFrameAllocationCount();
uint64_t GetCurrentFrameFreeCount();
uint64_t GetCurrentAllocationCountHighWater();
uint32_t GetCurrentAllocationSizeInBytes();
uint16_t GetCurrentAllocationSizeHighWaterInBytes();
uint32_t GetCurrentMaxAllocationSizeInBytes();
uint64_t GetCurrentAllocationOverflowInBytes();

//Setters
void SetMaxMemoryInBytes(uint32_t bytes);
void SetMaxMemoryInKiB(uint32_t kib);
void SetMaxMemoryInMiB(uint32_t mib);
void SetMaxMemoryInGiB(uint32_t gib);

// Functions
void ResetFrameMemTrack();
void ReportVerboseCallStacks(const char* start_time_str = "", const char* end_time_str = "", bool print_long_report = false);
void ReportEntireVerboseCallStackList(bool print_long_report = false);

// Operators
#ifndef TRACK_MEMORY
void* operator new(const size_t size);
void operator delete(void* ptr);
#endif 
