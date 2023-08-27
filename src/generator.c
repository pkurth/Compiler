#include "generator.h"

#include <assert.h>


// https://sonictk.github.io/asm_tutorial/#hello,worldrevisted/callingfunctionsinassembly

struct LocalVariableSpan
{
	LocalVariable* variables;
	u64 variable_count;
};
typedef struct LocalVariableSpan LocalVariableSpan;

static LocalVariable* find_local_variable(LocalVariableSpan local_variables, String name)
{
	LocalVariable* result = 0;
	for (u64 i = 0; i < local_variables.variable_count; ++i)
	{
		if (string_equal(name, local_variables.variables[i].name))
		{
			result = &local_variables.variables[i];
			break;
		}
	}

	if (!result)
	{
		fprintf(stderr, "Undeclared identifier '%.*s'.\n", (i32)name.len, name.str); // Temporary. Should be before generator.
	}

	return result;
}

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

static void generate_function_header(String name, u64 stack_size, String* assembly)
{
	string_push(assembly,
		"%.*s:\n"
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

static void generate_expression(Program* program, Expression expression, LocalVariableSpan local_variables, String* assembly)
{
	if (expression.type == ExpressionType_Identifier)
	{
		LocalVariable* variable = find_local_variable(local_variables, expression.identifier.str);
		assert(variable);

		char from[32];
		snprintf(from, sizeof(from), "QWORD [rbp-%d]", variable->offset_from_rbp);

		stack_push(from, assembly);
	}
	else if (expression.type == ExpressionType_IntLiteral)
	{
		string_push(assembly, "    mov rax, %.*s\n", (i32)expression.int_literal.str.len, expression.int_literal.str.str);
		stack_push("rax", assembly);
	}
	else if (expression_is_binary_operation(expression.type))
	{
		Expression a = program_get_expression(program, expression.binary.lhs);
		Expression b = program_get_expression(program, expression.binary.rhs);
		generate_expression(program, a, local_variables, assembly);
		generate_expression(program, b, local_variables, assembly);

		stack_pop("rbx", assembly);
		stack_pop("rax", assembly);

		switch (expression.type)
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
			default:							assert(0);
		}

		stack_push("rax", assembly);
	}
	else if (expression_is_unary_operation(expression.type))
	{
		Expression a = program_get_expression(program, expression.unary.expression);
		generate_expression(program, a, local_variables, assembly);

		stack_pop("rax", assembly);

		switch (expression.type)
		{
			case ExpressionType_Negate:			string_push(assembly, "    neg rax\n"); break; // https://www.felixcloutier.com/x86/neg
			case ExpressionType_BitwiseNot:		string_push(assembly, "    not rax\n"); break; // https://www.felixcloutier.com/x86/not
			case ExpressionType_Not:			string_push(assembly, "    cmp rax, 0\nsete al\nmovzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			default:							assert(0);
		}

		stack_push("rax", assembly);
	}
}

static void generate_statement(Program* program, Statement statement, LocalVariableSpan local_variables, String* assembly)
{
	if (statement.type == StatementType_VariableAssignment || statement.type == StatementType_VariableReassignment)
	{
		Token identifier = statement.variable_assignment.identifier;

		Expression expression = program_get_expression(program, statement.variable_assignment.expression);
		generate_expression(program, expression, local_variables, assembly);

		LocalVariable* variable = find_local_variable(local_variables, identifier.str);
		assert(variable);

		stack_pop("rax", assembly);
		string_push(assembly, "    mov [rbp-%d], rax\n", variable->offset_from_rbp);
	}
	else if (statement.type == StatementType_Return)
	{
		Expression expression = program_get_expression(program, statement.ret.expression);
		generate_expression(program, expression, local_variables, assembly);

		stack_pop("rax", assembly);
		generate_return(assembly);
	}
	else if (statement.type == StatementType_Block)
	{
		for (u64 i = 0; i < statement.block.statement_count; ++i)
		{
			generate_statement(program, program->statements.items[i + statement.block.first_statement], local_variables, assembly);
		}
	}
}

static void generate_function(Program* program, Function function, String* assembly)
{
	generate_function_header(function.name, function.stack_size, assembly);

	LocalVariableSpan local_variables = { .variables = program->local_variables.items + function.first_local_variable, .variable_count = function.local_variable_count };

	Statement statement = { .type = StatementType_Block, .block = function.block };
	generate_statement(program, statement, local_variables, assembly);

	generate_return(assembly);

	string_push(assembly, "\n");
}

static void generate_start_function(String* assembly)
{
	generate_function_header(string_from_cstr("__main"), 0, assembly);
	string_push(assembly, "    call main\n");
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
		"segment .text\n"
		"global __main\n"
		"extern ExitProcess\n"
		"\n"
	);

	for (u64 i = 0; i < program.functions.count; ++i)
	{
		Function function = program.functions.items[i];
		generate_function(&program, function, &assembly);
	}

	generate_start_function(&assembly);

	return assembly;
}

