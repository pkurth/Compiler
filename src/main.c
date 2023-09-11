#include "lexer.h"
#include "parser.h"
#include "analyzer.h"
#include "generator.h"
#include "platform.h"

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
	u64 file_size = ftell(f);
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

	fprintf(f, "%.*s", (i32)s.len, s.str);

	fclose(f);
}

static String path_get_parent(String path)
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

static String path_get_filename(String path)
{
	String parent = path_get_parent(path);
	if (!string_equal(parent, string_from_cstr(".")))
	{
		path.str += parent.len + 1;
		path.len -= parent.len + 1;
	}

	return path;
}

static String path_get_stem(String path)
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

i32 main(i32 argc, char** argv)
{
	if (argc != 3)
	{
		fprintf(stderr, "Invalid number of arguments.\nUsage: %s <file.o2> <out.obj>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	String obj_path = { .str = argv[2], strlen(argv[2]) };
	String obj_dir = path_get_parent(obj_path);
	String obj_stem = path_get_stem(obj_path);

	create_directory(obj_dir);

	char asm_path[128];
	snprintf(asm_path, sizeof(asm_path), "%.*s/%.*s.asm", (i32)obj_dir.len, obj_dir.str, (i32)obj_stem.len, obj_stem.str);


	String contents = read_file(argv[1]);

	if (contents.len > 0)
	{
		TokenStream tokens = tokenize(contents);
		print_tokens(&tokens);

		Program program = parse(tokens);
		if (!program.has_errors)
		{
			analyze(&program);
			if (!program.has_errors)
			{
				print_program(&program);

				String result = generate(program);
				write_file(asm_path, result);

				free_string(&result);


				char nasm_command[128] = "";

#if defined(_WIN32)
				snprintf(nasm_command, sizeof(nasm_command), ".\\nasm\\nasm.exe -f win64 -o %.*s %s", (i32)obj_path.len, obj_path.str, asm_path);
#elif defined(__linux__)

#endif

				system(nasm_command);
			}
		}

		free_program(&program);
		free_token_stream(&tokens);
	}

	free_string(&contents);

	exit(EXIT_SUCCESS);
}
