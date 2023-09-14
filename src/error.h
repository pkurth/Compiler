#pragma once

#include "types.h"

#include <ctype.h>

static String get_line(String source_code, i32 character_index)
{
	i64 left = character_index;
	while (left > 0 && source_code.str[left - 1] != '\n')
	{
		--left;
	}
	while (isspace(source_code.str[left]))
	{
		++left;
	}

	i64 right = character_index;
	while (right < source_code.len && source_code.str[right + 1] != '\n')
	{
		++right;
	}

	String result = { .str = source_code.str + left, .len = right - left };
	return result;
}

static void print_line_error(String source_code, SourceLocation source_location)
{
	String line = get_line(source_code, source_location.global_character_index);
	fprintf(stderr, "%.*s\n", (i32)line.len, line.str);

	i32 column = (i32)((source_code.str + source_location.global_character_index) - line.str);
	fprintf(stderr, "%*s^\n", column, "");
}
