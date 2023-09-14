#pragma once

#include "types.h"

#include <ctype.h>

static String get_line(String program_string, i32 character_index)
{
	i64 left = character_index;
	while (left > 0 && program_string.str[left - 1] != '\n')
	{
		--left;
	}
	while (isspace(program_string.str[left]))
	{
		++left;
	}

	i64 right = character_index;
	while (right < program_string.len && program_string.str[right + 1] != '\n')
	{
		++right;
	}

	String result = { .str = program_string.str + left, .len = right - left };
	return result;
}

static void print_line_error(String program_string, SourceLocation source_location)
{
	String line = get_line(program_string, source_location.global_character_index);
	fprintf(stderr, "%.*s\n", (i32)line.len, line.str);

	i32 column = (i32)((program_string.str + source_location.global_character_index) - line.str);
	fprintf(stderr, "%*s^\n", column, "");
}
