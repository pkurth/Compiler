#include "program.h"

#include <assert.h>
#include <ctype.h>


String program_get_line(Program* program, i32 character_index)
{
	i64 left = character_index;
	while (left > 0 && program->source_code.str[left - 1] != '\n')
	{
		--left;
	}
	while (isspace(program->source_code.str[left]))
	{
		++left;
	}

	i64 right = character_index;
	while (right < program->source_code.len && program->source_code.str[right + 1] != '\n')
	{
		++right;
	}

	String result = { .str = program->source_code.str + left, .len = right - left };
	return result;
}

i32 program_get_column(Program* program, SourceLocation source_location, String line)
{
	return (i32)((program->source_code.str + source_location.global_character_index) - line.str);
}

void program_print_line(Program* program, SourceLocation source_location, const FILE* stream)
{
	String line = program_get_line(program, source_location.global_character_index);
	fprintf(stderr, "%.*s\n", (i32)line.len, line.str);
}

void program_print_line_error(Program* program, SourceLocation source_location)
{
	String line = program_get_line(program, source_location.global_character_index);
	i32 column = program_get_column(program, source_location, line);

	fprintf(stderr, "%.*s\n", (i32)line.len, line.str);
	fprintf(stderr, "%*s^\n", column, "");
}


static const char* operator_strings[ExpressionType_Count] =
{
	[ExpressionType_Error]			= "",
	[ExpressionType_LogicalOr]		= "||",
	[ExpressionType_LogicalAnd]		= "&&",
	[ExpressionType_BitwiseOr]		= "|",
	[ExpressionType_BitwiseXor]		= "^",
	[ExpressionType_BitwiseAnd]		= "&",
	[ExpressionType_Equal]			= "==",
	[ExpressionType_NotEqual]		= "!=",
	[ExpressionType_Less]			= "<",
	[ExpressionType_Greater]		= ">",
	[ExpressionType_LessEqual]		= "<=",
	[ExpressionType_GreaterEqual]	= ">=",
	[ExpressionType_LeftShift]		= "<<",
	[ExpressionType_RightShift]		= ">>",
	[ExpressionType_Addition]		= "+",
	[ExpressionType_Subtraction]	= "-",
	[ExpressionType_Multiplication]	= "*",
	[ExpressionType_Division]		= "/",
	[ExpressionType_Modulo]			= "%",

	[ExpressionType_Negate]			= "-",
	[ExpressionType_BitwiseNot]		= "~",
	[ExpressionType_Not]			= "!",
};

static void set_bit(i32* mask, i32 bit_index)
{
	*mask |= (1 << bit_index);
}

static void clear_bit(i32* mask, i32 bit_index)
{
	*mask &= ~(1 << bit_index);
}

static b32 is_bit_set(i32 mask, i32 bit_index)
{
	return (mask & (1 << bit_index)) != 0;
}

static void print_indent(i32 indent, i32 active_mask)
{
	for (i32 i = 0; i <= indent; ++i)
	{
		b32 last = (i == indent);
		b32 is_active = is_bit_set(active_mask, i);
		printf((is_active || last) ? "|" : " ");
		printf(last ? "-> " : "   ");
	}
}

static void print_expression(Program* program, ExpressionHandle expression_handle, i32 indent, i32* active_mask)
{
	Expression* expression = program_get_expression(program, expression_handle);
	if (expression->type == ExpressionType_Error)
	{
		return;
	}

	print_indent(indent, *active_mask);

	if (expression->type == ExpressionType_NumericLiteral)
	{
		printf("%s\n", serialize_numeric_literal(expression->numeric_literal));
	}
	else if (expression->type == ExpressionType_StringLiteral)
	{
		printf("%.*s\n", (i32)expression->string_literal.len, expression->string_literal.str);
	}
	else if (expression->type == ExpressionType_Identifier)
	{
		IdentifierExpression e = expression->identifier;

		printf("%.*s\n", (i32)e.name.len, e.name.str);
	}
	else if (expression_is_binary_operation(expression->type))
	{
		BinaryExpression e = expression->binary;

		printf("%s\n", operator_strings[expression->type]);

		set_bit(active_mask, indent + 1);
		print_expression(program, e.lhs, indent + 1, active_mask);

		clear_bit(active_mask, indent + 1);
		print_expression(program, e.rhs, indent + 1, active_mask);
	}
	else if (expression_is_unary_operation(expression->type))
	{
		UnaryExpression e = expression->unary;

		printf("%s\n", operator_strings[expression->type]);
		print_expression(program, e.rhs, indent + 1, active_mask);
	}
	else if (expression->type == ExpressionType_Assignment)
	{
		AssignmentExpression e = expression->assignment;

		Expression* lhs = program_get_expression(program, e.lhs);
		assert(lhs->type == ExpressionType_Identifier); // Temporary.

		String identifier = lhs->identifier.name;

		printf("Variable assignment %.*s\n", (i32)identifier.len, identifier.str);
		print_expression(program, e.rhs, indent + 1, active_mask);
	}
	else if (expression->type == ExpressionType_FunctionCall)
	{
		FunctionCallExpression e = expression->function_call;

		printf("Function call %.*s\n", (i32)e.function_name.len, e.function_name.str);

		set_bit(active_mask, indent + 1);
		ExpressionHandle current = e.first_argument;
		while (current)
		{
			ExpressionHandle next = program_get_expression(program, current)->next;
			if (!next) { clear_bit(active_mask, indent + 1); }

			print_expression(program, current, indent + 1, active_mask);
			current = next;
		}
	}
	else
	{
		assert(!"Unknown expression type");
	}
}

static void print_statements(Program* program, i32 first_statement, i32 statement_count, i32 indent, i32* active_mask)
{
	for (i32 i = 0; i < statement_count; ++i)
	{
		i32 statement_index = first_statement + i;

		Statement* statement = program_get_statement(program, statement_index);
		if (statement->type == StatementType_Error)
		{
			continue;
		}

		if (statement->type == StatementType_Simple)
		{
			print_expression(program, statement->simple.expression, indent, active_mask);
			continue;
		}

		print_indent(indent, *active_mask);

		if (statement->type == StatementType_Declaration)
		{
			DeclarationStatement e = statement->declaration;

			Expression* lhs = program_get_expression(program, e.lhs);
			assert(lhs->type == ExpressionType_Identifier); // Temporary.

			String identifier = lhs->identifier.name;

			printf("Variable declaration %.*s\n", (i32)identifier.len, identifier.str);
		}
		else if (statement->type == StatementType_DeclarationAssignment)
		{
			DeclarationAssignmentStatement e = statement->declaration_assignment;

			Expression* lhs = program_get_expression(program, e.lhs);
			assert(lhs->type == ExpressionType_Identifier); // Temporary.

			String identifier = lhs->identifier.name;

			printf("Variable declaration & assignment %.*s\n", (i32)identifier.len, identifier.str);
			print_expression(program, e.rhs, indent + 1, active_mask);
		}
		else if (statement->type == StatementType_Return)
		{
			printf("Return\n");
			print_expression(program, statement->ret.rhs, indent + 1, active_mask);
		}
		else if (statement->type == StatementType_Block)
		{
			printf("Block\n");

			set_bit(active_mask, indent + 1);
			print_statements(program, statement_index + 1, statement->block.statement_count, indent + 1, active_mask);
			i += statement->block.statement_count;
		}
		else if (statement->type == StatementType_Branch)
		{
			BranchStatement e = statement->branch;

			printf("Branch\n");

			set_bit(active_mask, indent + 1);
			print_expression(program, e.condition, indent + 1, active_mask);

			if (!e.then_statement_count) { clear_bit(active_mask, indent + 1); }
			print_statements(program, statement_index + 1, e.then_statement_count, indent + 1, active_mask);
			clear_bit(active_mask, indent + 1);
			print_statements(program, statement_index + e.then_statement_count + 1, e.else_statement_count, indent + 1, active_mask);
			i += e.then_statement_count + e.else_statement_count;
		}
		else if (statement->type == StatementType_Loop)
		{
			LoopStatement e = statement->loop;

			printf("Loop\n");

			set_bit(active_mask, indent + 1);
			print_expression(program, e.condition, indent + 1, active_mask);

			clear_bit(active_mask, indent + 1);
			print_statements(program, statement_index + 1, e.then_statement_count, indent + 1, active_mask);
			i += e.then_statement_count;
		}
		else
		{
			assert(false);
		}
	}
}

static void print_function(Program* program, Function function)
{
	printf("FUNCTION %.*s\n", (i32)function.name.len, function.name.str);

	i32 active_mask = 0;
	print_statements(program, function.body_first_statement, function.body_statement_count, 0, &active_mask);
	printf("\n");
}

void program_print_ast(Program* program)
{
	for (i64 i = 0; i < program->functions.count; ++i)
	{
		print_function(program, program->functions.items[i]);
	}
}

void free_program(Program* program)
{
	array_free(&program->functions);
	array_free(&program->function_parameters);
	array_free(&program->statements);
	array_free(&program->expressions);

	string_free(&program->source_code);
}

