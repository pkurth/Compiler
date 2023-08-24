#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "generator.h"

static String read_file(const char* filename)
{
	String contents = { 0 };

	FILE* f = fopen(filename, "rb");
	if (!f)
	{
		fprintf(stderr, "Could not open file '%s'.\n", filename);
		return contents;
	}
	fseek(f, 0, SEEK_END);
	size_t file_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	contents.str = malloc(file_size + 1);
	if (!contents.str)
	{
		fprintf(stderr, "Could not allocate space for file '%s'.\n", filename);
		return contents;
	}

	contents.len = file_size;

	fread(contents.str, file_size, 1, f);
	contents.str[file_size] = 0;
	fclose(f);

	return contents;
}

static void write_file(const char* filename, String s)
{
	FILE* f = fopen(filename, "w");
	if (!f)
	{
		fprintf(stderr, "Could not open file '%s'.\n", filename);
		return;
	}

	fprintf(f, "%.*s", (int)s.len, s.str);

	fclose(f);
}

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		fprintf(stderr, "Invalid number of arguments.\nUsage: oxygen <file.o2> <out.asm>\n");
		exit(EXIT_FAILURE);
	}

	String contents = read_file(argv[1]);

	if (contents.len > 0)
	{
		TokenStream tokens = tokenize(contents);
		//print_tokens(tokens);

		Program program = parse(tokens);
		print_program(program);

		String result = generate(program);

		write_file(argv[2], result);

		free_string(&result);
		free_program(&program);
		free_token_stream(&tokens);
	}

	free_string(&contents);

	exit(EXIT_SUCCESS);
}
