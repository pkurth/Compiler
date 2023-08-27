#include "generator.h"

#include <assert.h>


struct StackVariable
{
	String name;
	u64 stack_location;
};
typedef struct StackVariable StackVariable;

struct Stack
{
	u64 stack_ptr;

	StackVariable variables[128];
	u64 variable_count;
};
typedef struct Stack Stack;

static void stack_push(Stack* stack, const char* from, String* assembly)
{
	string_push(assembly, "push %s\n", from);
	stack->stack_ptr += 8;
}

static void stack_pop(Stack* stack, const char* reg, String* assembly)
{
	string_push(assembly, "pop %s\n", reg);
	stack->stack_ptr -= 8;
}

static StackVariable* stack_find_variable(Stack* stack, String variable)
{
	StackVariable* result = 0;
	for (u64 i = 0; i < stack->variable_count; ++i)
	{
		if (string_equal(stack->variables[i].name, variable))
		{
			result = &stack->variables[i];
		}
	}
	return result;
}

static void register_load_literal(String literal, const char* reg, String* assembly)
{
	string_push(assembly,
		"mov %s, %.*s\n",
		reg,
		(i32)literal.len, literal.str
	);
}

static void generate_exit(const char* result, String* assembly)
{
	string_push(assembly,
		"mov ecx, %s\n"
		"call exit\n",
		result
	);
}

static void generate_expression(Program* program, Expression expression, Stack* stack, String* assembly)
{
	if (expression.type == ExpressionType_Identifier)
	{
		StackVariable* variable = stack_find_variable(stack, expression.identifier.str);
		if (!variable)
		{
			fprintf(stderr, "Undeclared identifier '%.*s' (line %d)\n", (i32)expression.identifier.str.len, expression.identifier.str.str, (i32)expression.identifier.line);
		}
		assert(variable);

		i32 offset = (i32)(stack->stack_ptr - variable->stack_location);

		char from[32];
		snprintf(from, sizeof(from), "[rsp+%d]", offset);

		stack_push(stack, from, assembly);
	}
	else if (expression.type == ExpressionType_IntLiteral)
	{
		register_load_literal(expression.int_literal.str, "rax", assembly);
		stack_push(stack, "rax", assembly);
	}
	else if (expression_is_binary_operation(expression.type))
	{
		Expression a = program_get_expression(program, expression.binary.lhs);
		Expression b = program_get_expression(program, expression.binary.rhs);
		generate_expression(program, a, stack, assembly);
		generate_expression(program, b, stack, assembly);

		stack_pop(stack, "rbx", assembly);
		stack_pop(stack, "rax", assembly);

		switch (expression.type)
		{
			case ExpressionType_LogicalOr:		break;
			case ExpressionType_LogicalAnd:		break;
			case ExpressionType_BitwiseOr:		string_push(assembly, "or rax, rbx\n"); break; // https://www.felixcloutier.com/x86/or
			case ExpressionType_BitwiseAnd:		string_push(assembly, "and rax, rbx\n"); break; // https://www.felixcloutier.com/x86/and
			case ExpressionType_BitwiseXor:		string_push(assembly, "xor rax, rbx\n"); break; // https://www.felixcloutier.com/x86/xor
			case ExpressionType_Equal:			string_push(assembly, "cmp rax, rbx\nsete al\nmovzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			case ExpressionType_NotEqual:		string_push(assembly, "cmp rax, rbx\nsetne al\nmovzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			case ExpressionType_Less:			string_push(assembly, "cmp rax, rbx\nsetl al\nmovzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			case ExpressionType_Greater:		string_push(assembly, "cmp rax, rbx\nsetg al\nmovzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			case ExpressionType_LessEqual:		string_push(assembly, "cmp rax, rbx\nsetle al\nmovzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			case ExpressionType_GreaterEqual:	string_push(assembly, "cmp rax, rbx\nsetge al\nmovzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			case ExpressionType_LeftShift:		string_push(assembly, "shlx rax, rax, rbx\n"); break; // https://www.felixcloutier.com/x86/sarx:shlx:shrx
			case ExpressionType_RightShift:		string_push(assembly, "shrx rax, rax, rbx\n"); break; // https://www.felixcloutier.com/x86/sarx:shlx:shrx
			case ExpressionType_Addition:		string_push(assembly, "add rax, rbx\n"); break; // https://www.felixcloutier.com/x86/add
			case ExpressionType_Subtraction:	string_push(assembly, "sub rax, rbx\n"); break; // https://www.felixcloutier.com/x86/sub
			case ExpressionType_Multiplication: string_push(assembly, "imul rax, rbx\n"); break; // https://www.felixcloutier.com/x86/imul
			case ExpressionType_Division:		string_push(assembly, "cqo\nidiv rbx\n"); break; // https://www.felixcloutier.com/x86/idiv
			case ExpressionType_Modulo:			string_push(assembly, "cqo\nidiv rbx\nmov rax, rdx\n"); break; // https://www.felixcloutier.com/x86/idiv
			default:							assert(0);
		}

		stack_push(stack, "rax", assembly);
	}
	else if (expression_is_unary_operation(expression.type))
	{
		Expression a = program_get_expression(program, expression.unary.expression);
		generate_expression(program, a, stack, assembly);

		stack_pop(stack, "rax", assembly);

		switch (expression.type)
		{
			case ExpressionType_Negate:			string_push(assembly, "neg rax\n"); break; // https://www.felixcloutier.com/x86/neg
			case ExpressionType_BitwiseNot:		string_push(assembly, "not rax\n"); break; // https://www.felixcloutier.com/x86/not
			case ExpressionType_Not:			string_push(assembly, "cmp rax, 0\nsete al\nmovzx eax, al\n"); break; // https://www.felixcloutier.com/x86/cmp
			default:							assert(0);
		}

		stack_push(stack, "rax", assembly);
	}
}

static void generate_statement(Program* program, Statement statement, Stack* stack, String* assembly)
{
	if (statement.type == StatementType_VariableAssignment)
	{
		Token identifier = statement.variable_assignment.identifier;
		assert(!stack_find_variable(stack, identifier.str));

		Expression expression = program_get_expression(program, statement.variable_assignment.expression);
		generate_expression(program, expression, stack, assembly);

		StackVariable variable = { .name = identifier.str, .stack_location = stack->stack_ptr };
		stack->variables[stack->variable_count++] = variable;
	}
	else if (statement.type == StatementType_Return)
	{
		Expression expression = program_get_expression(program, statement.ret.expression);
		generate_expression(program, expression, stack, assembly);

		stack_pop(stack, "rax", assembly);
		generate_exit("eax", assembly);
	}
}

String generate(Program program)
{
	u64 max_len = 1024 * 10;
	String assembly = { malloc(max_len), 0 };
	Stack stack = { 0 };

	string_push(&assembly,
		"includelib ucrt.lib\n"
		"\n"
		".code\n"
		"externdef exit : proc\n"
		"\n"
		"main proc\n"
	);


	for (u64 i = 0; i < program.statements.count; ++i)
	{
		Statement statement = program.statements.items[i];
		generate_statement(&program, statement, &stack, &assembly);
	}

	generate_exit("0", &assembly); // In case program didn't have return statement.

	string_push(&assembly,
		"main endp\n"
		"\n"
		"end\n"
	);

	return assembly;
}
