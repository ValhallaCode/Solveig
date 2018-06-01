#pragma once
#include <cstdint>

// Datatypes


// Getters
uint64_t GetCurrentAllocationCount();
uint64_t GetCurrentFrameAllocationCount();
uint64_t GetCurrentFrameFreeCount();
uint64_t GetCurrentAllocationCountHighWater();
size_t GetCurrentAllocationSizeInBytes();
size_t GetCurrentAllocationSizeHighWaterInBytes();

// Functions
void ResetFrameMemTrack();

// Operators
#ifndef _DEBUG
void* operator new(const size_t size);
void operator delete(void* ptr);
#endif 
