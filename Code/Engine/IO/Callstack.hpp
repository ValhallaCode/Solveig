#pragma once
#include <stdint.h>


// Datatypes
const uint8_t MAX_FRAMES_PER_CALLSTACK = 128;

struct callstack_line_t
{
	char file_name[128];
	char function_name[256];
	uint32_t line;
	uint32_t offset;
};

class CallStack
{
public:
	CallStack();
public:
	uint32_t m_hash;
	uint8_t m_frame_count;
	double m_time;
	void* m_frames[MAX_FRAMES_PER_CALLSTACK];
};


// Functions
bool CallstackSystemInit();
void CallstackSystemDeinit();
CallStack* CreateCallstack(uint8_t skip_frames);
void DestroyCallstack(CallStack *c);
uint16_t CallstackGetLines(callstack_line_t *line_buffer, const uint16_t max_lines, CallStack *cs);