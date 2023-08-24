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
	else if (expression.type >= ExpressionType_Addition && expression.type <= ExpressionType_Subtraction)
	{
		Expression a = program_get_expression(program, expression.binary.lhs);
		Expression b = program_get_expression(program, expression.binary.rhs);
		generate_expression(program, a, stack, assembly);
		generate_expression(program, b, stack, assembly);

		stack_pop(stack, "rbx", assembly);
		stack_pop(stack, "rax", assembly);

		const char* ops[] = { "add", "sub" };

		string_push(assembly, "%s rax, rbx\n", ops[expression.type - ExpressionType_Addition]);

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


	for (u64 i = 0; i < program.statement_count; ++i)
	{
		Statement statement = program.statements[i];
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
