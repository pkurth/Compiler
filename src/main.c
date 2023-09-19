#include "program.h"
#include "platform.h"

#include <time.h>


#define timer_start(name) clock_t name##_start = clock();
#define timer_end(name) name = (float)(clock() - name##_start) / CLOCKS_PER_SEC;


static void assemble(String assembly, const char* obj)
{
	String obj_path = { .str = (char*)obj, strlen(obj) };

	String obj_dir = path_get_parent(obj_path);
	String obj_stem = path_get_stem(obj_path);

	create_directory(obj_dir);


	char asm_path[128];
	snprintf(asm_path, sizeof(asm_path), "%.*s/%.*s.asm", (i32)obj_dir.len, obj_dir.str, (i32)obj_stem.len, obj_stem.str);

	write_file(asm_path, assembly);



	char nasm_command[128] = "";

#if defined(_WIN32)
	snprintf(nasm_command, sizeof(nasm_command), ".\\nasm\\nasm.exe -f win64 -o %.*s %s", (i32)obj_path.len, obj_path.str, asm_path);
#elif defined(__linux__)

#endif

	system(nasm_command);
}

i32 main(i32 argc, char** argv)
{
	if (argc != 3)
	{
		fprintf(stderr, "Invalid number of arguments.\nUsage: %s <file.o2> <out.obj>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	

	float lexer_time = 0.f;
	float parser_time = 0.f;
	float analyzer_time = 0.f;
	float generator_time = 0.f;
	float total_time = 0.f;

	timer_start(total_time);


	Program program = { 0 };
	program.source_code = read_file(argv[1]);

	if (program.source_code.len > 0)
	{
		timer_start(lexer_time);
		TokenStream tokens = tokenize(program.source_code);
		timer_end(lexer_time);

		//print_tokens(&tokens);


		timer_start(parser_time);
		b32 parse_result = parse(&program, tokens);
		timer_end(parser_time);

		if (parse_result)
		{
			timer_start(analyzer_time);
			b32 analysis_result = analyze(&program);
			timer_end(analyzer_time);

			if (analysis_result)
			{
				program_print_ast(&program);

				timer_start(generator_time);
				String assembly = generate(program);
				timer_end(generator_time);

				assemble(assembly, argv[2]);

				string_free(&assembly);
			}
		}

		free_program(&program);
		free_token_stream(&tokens);
	}

	timer_end(total_time);

	printf("Lexer: %.3fs.\n", lexer_time);
	printf("Parser: %.3fs.\n", parser_time);
	printf("Analyzer: %.3fs.\n", analyzer_time);
	printf("Generator: %.3fs.\n", generator_time);
	printf("Finished after %.3f seconds.\n", total_time);

	exit(EXIT_SUCCESS);
}
