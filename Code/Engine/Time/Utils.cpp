#include "Engine/Time/Utils.hpp"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include <cstring>

//////////////////////////////////////////////////////
//													//
//					Definitions						//
//													//
//////////////////////////////////////////////////////
static InternalTimeSystem g_time;
const float MIN_FRAMES_PER_SECOND = 10.0f;
const float MAX_FRAMES_PER_SECOND = 60.0f;
const float MIN_SECONDS_PER_FRAME = (1.0f / MAX_FRAMES_PER_SECOND);
const float MAX_SECONDS_PER_FRAME = (1.0f / MIN_FRAMES_PER_SECOND);

//////////////////////////////////////////////////////
//													//
//					Functions						//
//													//
//////////////////////////////////////////////////////
double InitializeTime(LARGE_INTEGER& out_initial_time)
{
	LARGE_INTEGER counts_per_second;
	QueryPerformanceFrequency(&counts_per_second);
	QueryPerformanceCounter(&out_initial_time);
	return(1.0 / static_cast< double >(counts_per_second.QuadPart));
}

void SleepSeconds(float seconds_to_sleep)
{
	int ms_to_sleep = (int)(1000.f * seconds_to_sleep);
	Sleep(ms_to_sleep);
}

uint64_t TimeOpCountTo_us(uint64_t op_count)
{
	op_count *= (1000U * 1000U);
	const uint64_t micro = (uint64_t)(op_count * g_time.seconds_per_op);
	return micro;
}

double TimeOpCountTo_ms(uint64_t op_count)
{
	double seconds = op_count * g_time.seconds_per_op;
	return seconds * 1000.0;
}

uint64_t TimeOpCountFrom_ms(double micro_seconds)
{
	double seconds = micro_seconds / 1000.0;
	const uint64_t ops = (uint64_t)(seconds * g_time.ops_per_second);
	return ops;
}

void TimeOpCountToString(uint64_t op_count, char* out_time_len_128)
{
	char buffer[128];
	uint64_t micro = TimeOpCountTo_us(op_count);

	if (micro < 1500) 
	{
		sprintf_s(buffer, 128, "%llu us", micro);
	}
	else if (micro < 1500000) 
	{
		double milli = (double)micro / (double)1000.0;
		sprintf_s(buffer, 128, "%.4f ms", milli);
	}
	else 
	{
		double seconds = (double)micro / (double)(1000000.0);
		sprintf_s(buffer, 128, "%.4f s", seconds);
	}

	strcpy(out_time_len_128, buffer);
}

double TimeOpCountToSeconds(uint64_t op_count, char* out_units_len_4 /*= nullptr*/)
{
	uint64_t micro = TimeOpCountTo_us(op_count);

	if (micro < 1500) 
	{
		out_units_len_4[0] = 'u';
		out_units_len_4[1] = 's';
		out_units_len_4[2] = NULL;
		out_units_len_4[3] = NULL;
		return (double)micro;
	}
	else if (micro < 1500000) 
	{
		out_units_len_4[0] = 'm';
		out_units_len_4[1] = 's';
		out_units_len_4[2] = NULL;
		out_units_len_4[3] = NULL;
		return (double)micro / (double)1000.0;
	}
	else 
	{
		out_units_len_4[0] = 's';
		out_units_len_4[1] = 'e';
		out_units_len_4[2] = 'c';
		out_units_len_4[3] = NULL;
		return (double)micro / (double)(1000000.0);
	}
}

InternalTimeSystem::InternalTimeSystem()
{
	::QueryPerformanceFrequency((LARGE_INTEGER*)&ops_per_second);
	seconds_per_op = 1.0 / (double)ops_per_second;

	::QueryPerformanceCounter((LARGE_INTEGER*)&start_ops);
}

float CalculateDeltaSeconds()
{
	double time_now = GetCurrentTimeSeconds();
	static double last_frame_time = time_now;
	double delta_seconds = time_now - last_frame_time;

	// Wait until [nearly] the minimum frame time has elapsed (limit framerate to within the max)
	while (delta_seconds < MIN_SECONDS_PER_FRAME * .999f)
	{
		time_now = GetCurrentTimeSeconds();
		delta_seconds = time_now - last_frame_time;
	}
	last_frame_time = time_now;

	// Clamp deltaSeconds to be within the max time allowed (e.g. sitting at a debug break point)
	if (delta_seconds > MAX_SECONDS_PER_FRAME)
	{
		delta_seconds = MAX_SECONDS_PER_FRAME;
	}

	return (float)delta_seconds;
}

//////////////////////////////////////////////////////
//													//
//					Getters							//
//													//
//////////////////////////////////////////////////////
double GetCurrentTimeSeconds()
{
	static LARGE_INTEGER initial_time;
	static double seconds_per_count = InitializeTime(initial_time);
	LARGE_INTEGER current_count;
	QueryPerformanceCounter(&current_count);
	LONGLONG elapsed_counts_since_init_time = current_count.QuadPart - current_count.QuadPart;

	double current_seconds = static_cast< double >(elapsed_counts_since_init_time) * seconds_per_count;
	return current_seconds;
}

uint64_t __fastcall TimeGetOpCount()
{
	uint64_t count;
	QueryPerformanceCounter((LARGE_INTEGER*)&count);
	return count;
}

uint __fastcall TimeGet_ms()
{
	uint64_t count = TimeGetOpCount() - g_time.start_ops;
	count = (count * 1000U) / g_time.ops_per_second;
	return (uint)count;
}

uint __fastcall TimeGet_us()
{
	uint64_t count = TimeGetOpCount() - g_time.start_ops;;
	count = (count * 1000U * 1000U) / g_time.ops_per_second;
	return (uint)count;
}

double __fastcall TimeGetSeconds()
{
	uint64_t count = TimeGetOpCount() - g_time.start_ops;
	return (double)count * g_time.seconds_per_op;
}
