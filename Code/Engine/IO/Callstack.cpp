#include "IO/CallStack.hpp"
#include "Time/Utils.hpp"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdlib.h>
#include <DbgHelp.h>


//////////////////////////////////////////////////////
//													//
//					Typedefs						//
//													//
//////////////////////////////////////////////////////
typedef BOOL(__stdcall *sym_initialize_t)(IN HANDLE hProcess, IN PSTR UserSearchPath, IN BOOL fInvadeProcess);
typedef BOOL(__stdcall *sym_cleanup_t)(IN HANDLE hProcess);
typedef BOOL(__stdcall *sym_from_addr_t)(IN HANDLE hProcess, IN DWORD64 Address, OUT PDWORD64 Displacement, OUT PSYMBOL_INFO Symbol);
typedef BOOL(__stdcall *sym_get_line_t)(IN HANDLE hProcess, IN DWORD64 dwAddr, OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_LINE64 Symbol);

//////////////////////////////////////////////////////
//													//
//					Definitions						//
//													//
//////////////////////////////////////////////////////
const uint16_t MAX_SYMBOL_NAME_LENGTH = 128;
const uint32_t MAX_FILENAME_LENGTH = 1024;
const uint16_t MAX_DEPTH = 128;
static HMODULE g_debug_help;
static HANDLE g_process;
static SYMBOL_INFO* g_symbol;
static sym_initialize_t g_sym_initialize;
static sym_cleanup_t g_sym_cleanup;
static sym_from_addr_t g_sym_from_addr;
static sym_get_line_t g_sym_get_line_from_addr_64;
static int g_callstack_count = 0;

//////////////////////////////////////////////////////
//													//
//					Functions						//
//													//
//////////////////////////////////////////////////////
CallStack::CallStack()
	: m_hash(0)
	, m_frame_count(0) {}

bool CallstackSystemInit()
{
	// Load the dll, similar to OpenGL function fecthing.
	// This is where these functions will come from.
	g_debug_help = LoadLibraryA("dbghelp.dll");
	if (nullptr == g_debug_help) 
	{
		return false;
	}

	// Get pointers to the functions we want from the loded library.
	g_sym_initialize = (sym_initialize_t)GetProcAddress(g_debug_help, "SymInitialize");
	g_sym_cleanup = (sym_cleanup_t)GetProcAddress(g_debug_help, "SymCleanup");
	g_sym_from_addr = (sym_from_addr_t)GetProcAddress(g_debug_help, "SymFromAddr");
	g_sym_get_line_from_addr_64 = (sym_get_line_t)GetProcAddress(g_debug_help, "SymGetLineFromAddr64");

	// Initialize the system using the current process [see MSDN for details]
	g_process = ::GetCurrentProcess();
	g_sym_initialize(g_process, NULL, TRUE);

	// Preallocate some memory for loading symbol information. 
	g_symbol = (SYMBOL_INFO *) ::calloc(1, sizeof(SYMBOL_INFO) + (MAX_FILENAME_LENGTH * sizeof(char)));
	g_symbol->MaxNameLen = MAX_FILENAME_LENGTH;
	g_symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	return true;
}

void CallstackSystemDeinit()
{
	// cleanup after ourselves
	::free(g_symbol);
	g_symbol = nullptr;

	g_sym_cleanup(g_process);

	FreeLibrary(g_debug_help);
	g_debug_help = NULL;
}

// Can not be static - called when
// the callstack is freed.
void DestroyCallstack(CallStack *ptr)
{
	::free(ptr);
}

CallStack* CreateCallstack(uint8_t skip_frames)
{
	// Capture the callstack frames - uses a windows call
	void *stack[MAX_DEPTH];
	DWORD hash;

	// skip_frames:  number of frames to skip [starting at the top - so don't return the frames for "CreateCallstack" (+1), plus "skip_frame_" layers.
	// max_frames to return
	// memory to put this information into.
	// out pointer to back trace hash.
	uint8_t frames = CaptureStackBackTrace(1 + skip_frames, MAX_DEPTH, stack, &hash);

	// create the callstack using an untracked allocation
	CallStack *cs = (CallStack*) ::calloc(1, sizeof(CallStack));

	// force call the constructor (new in-place)
	//cs = new (cs) CallStack(); 
	cs->m_hash = 0;
	cs->m_frame_count = 0;
	

	// copy the frames to our callstack object
	uint8_t frame_count = min(MAX_FRAMES_PER_CALLSTACK, frames);
	cs->m_frame_count = frame_count;
	::memcpy(cs->m_frames, stack, sizeof(void*) * frame_count);

	cs->m_time = GetCurrentTimeSeconds();
	cs->m_hash = hash;

	return cs;
}

// Fills lines with human readable data for the given callstack
// Fills from top to bottom (top being most recently called, with each next one being the calling function of the previous)
//
// Additional features you can add;
// [ ] If a file exists in yoru src directory, clip the filename
// [ ] Be able to specify a list of function names which will cause this trace to stop.
uint16_t CallstackGetLines(callstack_line_t *line_buffer, const uint16_t max_lines, CallStack *cs)
{
	IMAGEHLP_LINE64 line_info;
	DWORD line_offset = 0; // Displacement from the beginning of the line 
	line_info.SizeOfStruct = sizeof(IMAGEHLP_LINE64);


	uint count = min(max_lines, cs->m_frame_count);
	uint16_t idx = 0;

	for (uint i = 0; i < count; ++i) 
	{
		callstack_line_t *line = &(line_buffer[idx]);
		DWORD64 ptr = (DWORD64)(cs->m_frames[i]);

		if (false == g_sym_from_addr(g_process, ptr, 0, g_symbol)) 
		{
			continue;
		}

		strcpy_s(line->function_name, 256, g_symbol->Name);

		BOOL bRet = g_sym_get_line_from_addr_64(
			GetCurrentProcess(), // Process handle of the current process 
			ptr,				 // Address 
			&line_offset,		 // Displacement will be stored here by the function 
			&line_info);         // File name / line information will be stored here 

		if (bRet) 
		{
			line->line = line_info.LineNumber;
			strcpy_s(line->file_name, 128, line_info.FileName);
			line->offset = line_offset;
		}
		else {
			// no information
			line->line = 0;
			line->offset = 0;
			strcpy_s(line->file_name, 128, "N/A");
		}

		++idx;
	}

	return idx;
}