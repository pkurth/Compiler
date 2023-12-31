#include "program.h"

#include <assert.h>


// https://sonictk.github.io/asm_tutorial/#hello,worldrevisted/callingfunctionsinassembly

static void stack_push(const char* from, String* assembly)
{
	string_push(assembly, "    push %s\n", from);
}

static void stack_pop(const char* reg, String* assembly)
{
	string_push(assembly, "    pop %s\n", reg);
}

static void generate_exit(String* assembly)
{
	stack_pop("rcx", assembly);
	string_push(assembly, "    call ExitProcess\n");
}

static void generate_function_header(String name, i64 stack_size, String* assembly)
{
	string_push(assembly,
		"_%.*s:\n"
		"    push rbp\n"
		"    mov rbp, rsp\n"
		"    sub rsp, %d\n",
		(i32)name.len, name.str, (i32)stack_size);
}

static void generate_return(String* assembly)
{
	string_push(assembly, 
		"    leave\n"
		"    ret\n"
	);
}

static void generate_expression(Program* program, ExpressionHandle expression_handle, String* assembly)
{
	Expression* expression = program_get_expression(program, expression_handle);

	if (expression->type == ExpressionType_Identifier)
	{
		char from[32];
		snprintf(from, sizeof(from), "QWORD [rbp%+d]", expression->identifier.offset_from_frame_pointer);

		stack_push(from, assembly);
	}
	else if (expression->type == ExpressionType_NumericLiteral)
	{
		string_push(assembly, "    mov rax, %s\n", serialize_numeric_literal(expression->numeric_literal));
		stack_push("rax", assembly);
	}
	else if (expression_is_binary_operation(expression->type))
	{
		BinaryExpression e = expression->binary;

		generate_expression(program, e.lhs, assembly);
		generate_expression(program, e.rhs, assembly);

		stack_pop("rbx", assembly);
		stack_pop("rax", assembly);

		switch (expression->type)
		{
			case ExpressionType_LogicalOr:		break;
			case ExpressionType_LogicalAnd:		break;
			case ExpressionType_BitwiseOr:		string_push(assembly, "    or rax, rbx\n"); break; // https://www.felixcloutier.com/x86/or
			case ExpressionType_BitwiseXor:		string_push(assembly, "    xor rax, rbx\n"); break; // https://www.felixcloutier.com/x86/xor
			case ExpressionType_BitwiseAnd:		string_push(assembly, "    and rax, rbx\n"); break; // https://www.felixcloutier.com/x86/and
			case ExpressionType_Equal:			string_push(assembly, "    cmp rax, rbx\n    sete al\n    movzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			case ExpressionType_NotEqual:		string_push(assembly, "    cmp rax, rbx\n    setne al\n    movzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			case ExpressionType_Less:			string_push(assembly, "    cmp rax, rbx\n    setl al\n    movzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			case ExpressionType_Greater:		string_push(assembly, "    cmp rax, rbx\n    setg al\n    movzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			case ExpressionType_LessEqual:		string_push(assembly, "    cmp rax, rbx\n    setle al\n    movzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			case ExpressionType_GreaterEqual:	string_push(assembly, "    cmp rax, rbx\n    setge al\n    movzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			case ExpressionType_LeftShift:		string_push(assembly, "    shlx rax, rax, rbx\n"); break; // https://www.felixcloutier.com/x86/sarx:shlx:shrx
			case ExpressionType_RightShift:		string_push(assembly, "    shrx rax, rax, rbx\n"); break; // https://www.felixcloutier.com/x86/sarx:shlx:shrx
			case ExpressionType_Addition:		string_push(assembly, "    add rax, rbx\n"); break; // https://www.felixcloutier.com/x86/add
			case ExpressionType_Subtraction:	string_push(assembly, "    sub rax, rbx\n"); break; // https://www.felixcloutier.com/x86/sub
			case ExpressionType_Multiplication: string_push(assembly, "    imul rax, rbx\n"); break; // https://www.felixcloutier.com/x86/imul
			case ExpressionType_Division:		string_push(assembly, "    cqo\n    idiv rbx\n"); break; // https://www.felixcloutier.com/x86/idiv
			case ExpressionType_Modulo:			string_push(assembly, "    cqo\n    idiv rbx\n    mov rax, rdx\n"); break; // https://www.felixcloutier.com/x86/idiv
			default:							assert(false);
		}

		stack_push("rax", assembly);
	}
	else if (expression_is_unary_operation(expression->type))
	{
		UnaryExpression e = expression->unary;

		generate_expression(program, e.rhs, assembly);

		stack_pop("rax", assembly);

		switch (expression->type)
		{
			case ExpressionType_Negate:			string_push(assembly, "    neg rax\n"); break; // https://www.felixcloutier.com/x86/neg
			case ExpressionType_BitwiseNot:		string_push(assembly, "    not rax\n"); break; // https://www.felixcloutier.com/x86/not
			case ExpressionType_Not:			string_push(assembly, "    cmp rax, 0\nsete al\nmovzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			default:							assert(false);
		}

		stack_push("rax", assembly);
	}
	else if (expression->type == ExpressionType_Assignment)
	{
		AssignmentExpression e = expression->assignment;

		Expression* lhs = program_get_expression(program, e.lhs);
		assert(lhs->type == ExpressionType_Identifier); // Temporary.

		generate_expression(program, e.rhs, assembly);

		stack_pop("rax", assembly);
		string_push(assembly, "    mov [rbp%+d], rax\n", lhs->identifier.offset_from_frame_pointer);
		stack_push("rax", assembly);
	}
	else if (expression->type == ExpressionType_FunctionCall)
	{
		FunctionCallExpression e = expression->function_call;
		Function* function = &program->functions.items[e.function_index];
		assert(function->calling_convention == CallingConvention_Windows_x64);

		const char* argument_registers[] = { "rcx", "rdx", "r8", "r9" };

		// Arguments: rcx, rdx, r8, r9
		// Stack	: [ Shadow space ] arg4 arg5 ...

		i32 parameter_count = (i32)function->parameter_count;

		ExpressionHandle argument = e.first_argument;
		i32 argument_index = 0;
		while (argument)
		{
			generate_expression(program, argument, assembly);

			if (argument_index < arraysize(argument_registers))
			{
				stack_pop(argument_registers[argument_index], assembly);
			}
			else
			{
				// Remaining arguments are pushed to the stack in reverse order.
				stack_pop("rax", assembly);
				i32 offset = (parameter_count - 1 - argument_index) * 8 + 8;
				string_push(assembly, "    mov [rsp-%d], rax\n", offset);
			}

			++argument_index;
			argument = program_get_expression(program, argument)->next;
		}

		i32 parameter_stack_size = max(32, parameter_count * 8);

		string_push(assembly, "    sub rsp, %d\n", parameter_stack_size);
		string_push(assembly, "    call _%.*s\n", (i32)e.function_name.len, e.function_name.str);
		string_push(assembly, "    add rsp, %d\n", parameter_stack_size);

		stack_push("rax", assembly);
	}
	else
	{
		assert(false);
	}
}

static i32 generate_label()
{
	static i32 label = 1;
	return label++;
}

static void generate_statements(Program* program, i32 first_statement, i32 statement_count, String* assembly)
{
	for (i32 i = 0; i < statement_count; ++i)
	{
		i32 statement_index = first_statement + i;

		Statement* statement = program_get_statement(program, statement_index);

		if (statement->type == StatementType_Simple)
		{
			generate_expression(program, statement->simple.expression, assembly);
		}
		else if (statement->type == StatementType_Declaration)
		{

		}
		else if (statement->type == StatementType_DeclarationAssignment)
		{
			DeclarationAssignmentStatement e = statement->declaration_assignment;

			Expression* lhs = program_get_expression(program, e.lhs);
			assert(lhs->type == ExpressionType_Identifier); // Temporary.

			generate_expression(program, e.rhs, assembly);

			stack_pop("rax", assembly);
			string_push(assembly, "    mov [rbp%+d], rax\n", lhs->identifier.offset_from_frame_pointer);
		}
		else if (statement->type == StatementType_Return)
		{
			generate_expression(program, statement->ret.rhs, assembly);

			stack_pop("rax", assembly);
			generate_return(assembly);
		}
		else if (statement->type == StatementType_Block)
		{
			generate_statements(program, statement_index + 1, statement->block.statement_count, assembly);
			i += statement->block.statement_count;
		}
		else if (statement->type == StatementType_Branch)
		{
			BranchStatement e = statement->branch;

			i32 else_label = generate_label();
			i32 end_label = generate_label();

			generate_expression(program, e.condition, assembly);
			stack_pop("rax", assembly);
			string_push(assembly, "    cmp rax, 0\n    je .L%d\n", else_label);

			generate_statements(program, statement_index + 1, e.then_statement_count, assembly);
			if (e.else_statement_count)
			{
				string_push(assembly, "    jmp .L%d\n", end_label);
			}

			string_push(assembly, "    .L%d:\n", else_label);

			if (e.else_statement_count)
			{
				generate_statements(program, statement_index + e.then_statement_count + 1, e.else_statement_count, assembly);
				string_push(assembly, "    .L%d:\n", end_label);
			}

			i += e.then_statement_count + e.else_statement_count;
		}
		else if (statement->type == StatementType_Loop)
		{
			LoopStatement e = statement->loop;

			i32 start_label = generate_label();
			i32 condition_label = generate_label();

			string_push(assembly, "    jmp .L%d\n", condition_label);
			string_push(assembly, "    .L%d:\n", start_label);
			generate_statements(program, statement_index + 1, e.then_statement_count, assembly);

			string_push(assembly, "    .L%d:\n", condition_label);
			generate_expression(program, e.condition, assembly);
			stack_pop("rax", assembly);
			string_push(assembly, "    cmp rax, 0\n    jne .L%d\n", start_label);
		}
		else
		{
			assert(false);
		}
	}
}

static void generate_function(Program* program, Function function, String* assembly)
{
	generate_function_header(function.name, function.stack_size, assembly);

	assert(function.calling_convention == CallingConvention_Windows_x64);
	const char* argument_registers[] = { "rcx", "rdx", "r8", "r9" };
	for (i64 i = 0; i < min(function.parameter_count, 4); ++i)
	{
		string_push(assembly, "    mov QWORD[rbp%+d], %s\n", 16 + i * 8, argument_registers[i]);
	}

	generate_statements(program, function.body_first_statement, function.body_statement_count, assembly);
	string_push(assembly, "\n");
}

static void generate_start_function(String* assembly)
{
	generate_function_header(string_from_cstr("_main"), 0, assembly);
	string_push(assembly, "    call _main\n");
	stack_push("rax", assembly);
	generate_exit(assembly);
}

String generate(Program program)
{
	u64 max_len = 1024 * 10;
	String assembly = { malloc(max_len), 0 };

	string_push(&assembly,
		"bits 64\n"
		"default rel\n"
		"\n"
		"global __main\n"
		"extern ExitProcess\n"
		"\n"
		"segment .text\n"
		"\n"
	);

	for (i64 i = 0; i < program.functions.count; ++i)
	{
		Function function = program.functions.items[i];
		generate_function(&program, function, &assembly);
	}

	generate_start_function(&assembly);

	return assembly;
}

