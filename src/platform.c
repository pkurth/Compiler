#include "platform.h"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

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




String read_file(const char* filename)
{
	String source_code = { 0 };

	FILE* f = fopen(filename, "rb");
	if (!f)
	{
		fprintf(stderr, "Could not open file '%s'.\n", filename);
		return source_code;
	}
	fseek(f, 0, SEEK_END);
	u64 file_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	source_code.str = malloc(file_size + 1);
	if (!source_code.str)
	{
		fprintf(stderr, "Could not allocate space for file '%s'.\n", filename);
		return source_code;
	}

	source_code.len = file_size;

	fread(source_code.str, file_size, 1, f);
	source_code.str[file_size] = 0;
	fclose(f);

	return source_code;
}

void write_file(const char* filename, String s)
{
	FILE* f = fopen(filename, "w");
	if (!f)
	{
		fprintf(stderr, "Could not open file '%s'.\n", filename);
		return;
	}

	fprintf(f, "%.*s", (i32)s.len, s.str);

	fclose(f);
}

String path_get_parent(String path)
{
	while (--path.len)
	{
		if (path.str[path.len] == '/' || path.str[path.len] == '\\')
		{
			break;
		}
	}
	if (path.len == 0)
	{
		path = (String){ .str = ".", .len = 1 };
	}
	return path;
}

String path_get_filename(String path)
{
	String parent = path_get_parent(path);
	if (!string_equal(parent, string_from_cstr(".")))
	{
		path.str += parent.len + 1;
		path.len -= parent.len + 1;
	}

	return path;
}

String path_get_stem(String path)
{
	String filename = path_get_filename(path);

	String copy = filename;
	while (--copy.len)
	{
		if (copy.str[copy.len] == '.')
		{
			return copy;
		}
	}
	return filename;
}


