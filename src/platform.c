#include "platform.h"

#if defined(_WIN32)

// Some windows.h symbols collide with our own symbols, so we avoid including windows.h.
#define MAX_PATH          260
__declspec(dllimport) i32 __stdcall CreateDirectoryA(const char* lpPathName, struct _SECURITY_ATTRIBUTES* lpSecurityAttributes);

void create_directory(String path)
{
	char zero_terminated_path[MAX_PATH];
	snprintf(zero_terminated_path, sizeof(zero_terminated_path), "%.*s", (i32)path.len, path.str);

	CreateDirectoryA(zero_terminated_path, 0);
}

#elif defined(__linux__)

#include <sys/stat.h>
#include <sys/types.h>

void create_directory(String path)
{
	char zero_terminated_path[MAX_PATH];
	snprintf(zero_terminated_path, sizeof(zero_terminated_path), "%.*s", (i32)path.len, path.str);

	mkdir(zero_terminated_path, 0777);
}

#endif
