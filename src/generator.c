#include "generator.h"

#include <assert.h>

struct StackVariable
{
	String name;
	size_t stack_location;
};
typedef struct StackVariable StackVariable;

struct Stack
{
	size_t stack_ptr;

	StackVariable variables[128];
	size_t variable_count;
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
	for (size_t i = 0; i < stack->variable_count; ++i)
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
		(int)literal.len, literal.str
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

static void generate_expression(Expression expression, Stack* stack, String* assembly)
{
	if (expression.type == ExpressionType_Identifier)
	{
		StackVariable* variable = stack_find_variable(stack, expression.identifier.str);
		if (!variable)
		{
			fprintf(stderr, "Undeclared identifier '%.*s' (line %d)\n", (int)expression.identifier.str.len, expression.identifier.str.str, (int)expression.identifier.line);
		}
		assert(variable);

		int offset = (int)(stack->stack_ptr - variable->stack_location);

		char from[32];
		snprintf(from, sizeof(from), "[rsp+%d]", offset);

		stack_push(stack, from, assembly);
	}
	else if (expression.type == ExpressionType_IntLiteral)
	{
		register_load_literal(expression.int_literal.str, "rax", assembly);
		stack_push(stack, "rax", assembly);
	}
}

static void generate_statement(Statement statement, Stack* stack, String* assembly)
{
	if (statement.type == StatementType_VariableAssignment)
	{
		Token identifier = statement.variable_assignment.identifier;
		assert(!stack_find_variable(stack, identifier.str));

		Expression expression = statement.variable_assignment.expression;
		generate_expression(expression, stack, assembly);

		StackVariable variable = { .name = identifier.str, .stack_location = stack->stack_ptr };
		stack->variables[stack->variable_count++] = variable;
	}
	else if (statement.type == StatementType_Return)
	{
		Expression expression = statement.ret.expression;
		generate_expression(expression, stack, assembly);

		stack_pop(stack, "rax", assembly);
		generate_exit("eax", assembly);
	}
}

String generate(Program program)
{
	size_t max_len = 1024 * 10;
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


	for (size_t i = 0; i < program.count; ++i)
	{
		Statement statement = program.statements[i];
		generate_statement(statement, &stack, &assembly);
	}

	generate_exit("0", &assembly); // In case program didn't have return statement.

	string_push(&assembly,
		"main endp\n"
		"\n"
		"end\n"
	);

	return assembly;
}
