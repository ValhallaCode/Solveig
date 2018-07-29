#pragma once
#include <stdint.h>

// Datatypes
typedef unsigned int uint;

class InternalTimeSystem
{
public:
	InternalTimeSystem();

	uint64_t start_ops;
	uint64_t ops_per_second;

	double seconds_per_op;
};

// Getters
double GetCurrentTimeSeconds();
uint __fastcall TimeGet_us();
uint __fastcall TimeGet_ms();
uint64_t __fastcall TimeGetOpCount();
double __fastcall TimeGetSeconds();

// Functions
float CalculateDeltaSeconds();
void SleepSeconds(float seconds_to_sleep);
void TimeOpCountToString(uint64_t op_count, char* out_time_len_128);
uint64_t TimeOpCountFrom_ms(double micro_seconds);
double TimeOpCountTo_ms(uint64_t op_count);
uint64_t TimeOpCountTo_us(uint64_t op_count);
double TimeOpCountToSeconds(uint64_t op_count, char* out_units_len_4 = nullptr);