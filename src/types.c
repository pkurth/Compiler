#include "types.h"

#include <assert.h>


static const char* data_type_strings[] =
{
	[PrimitiveDatatype_B32] = "b32",
	[PrimitiveDatatype_I32] = "i32",
	[PrimitiveDatatype_U32] = "u32",
	[PrimitiveDatatype_F32] = "f32",
};

const char* serialize_primitive_data(PrimitiveData data)
{
	static char result[128];

	switch (data.type)
	{
		case PrimitiveDatatype_B32:
			snprintf(result, sizeof(result), data.data_b32 ? "1" : "0");
			break;
		case PrimitiveDatatype_I32:
			snprintf(result, sizeof(result), "%" PRIi32, data.data_i32);
			break;
		case PrimitiveDatatype_U32:
			snprintf(result, sizeof(result), "%" PRIu32, data.data_u32);
			break;
		case PrimitiveDatatype_F32:
			snprintf(result, sizeof(result), "%f", data.data_f32);
			break;
	}

	return result;
}

const char* data_type_to_string(PrimitiveDatatype type)
{
	return data_type_strings[type];
}


static const char* token_strings[TokenType_Count] =
{
	[TokenType_Unknown]				= "UNKNOWN",
	[TokenType_EOF]					= "EOF",
	[TokenType_Function]			= "fn",
	[TokenType_If]					= "if",
	[TokenType_Else]				= "else",
	[TokenType_While]				= "while",
	[TokenType_For]					= "for",
	[TokenType_Return]				= "return",
	[TokenType_B32]					= "b32",
	[TokenType_I32]					= "i32",
	[TokenType_U32]					= "u32",
	[TokenType_F32]					= "f32",
	[TokenType_OpenParenthesis]		= "(",
	[TokenType_CloseParenthesis]	= ")",
	[TokenType_OpenBracket]			= "[",
	[TokenType_CloseBracket]		= "]",
	[TokenType_OpenBrace]			= "{",
	[TokenType_CloseBrace]			= "}",
	[TokenType_Semicolon]			= ";",
	[TokenType_Period]				= ".",
	[TokenType_Comma]				= ",",
	[TokenType_Colon]				= ":",
	[TokenType_ColonColon]			= "::",
	[TokenType_ColonEqual]			= ":=",
	[TokenType_Hashtag]				= "#",
	[TokenType_Dollar]				= "$",
	[TokenType_At]					= "@",
	[TokenType_QuestionMark]		= "?",
	[TokenType_Exclamation]			= "!",
	[TokenType_ExclamationEqual]	= "!=",
	[TokenType_Plus]				= "+",
	[TokenType_PlusEqual]			= "+=",
	[TokenType_Minus]				= "-",
	[TokenType_MinusEqual]			= "-=",
	[TokenType_Star]				= "*",
	[TokenType_StarEqual]			= "*=",
	[TokenType_ForwardSlash]		= "/",
	[TokenType_ForwardSlashEqual]	= "/=",
	[TokenType_Ampersand]			= "&",
	[TokenType_AmpersandEqual]		= "&=",
	[TokenType_Pipe]				= "|",
	[TokenType_PipeEqual]			= "|=",
	[TokenType_Hat]					= "^",
	[TokenType_HatEqual]			= "^=",
	[TokenType_Tilde]				= "~",
	[TokenType_Percent]				= "%",
	[TokenType_PercentEqual]		= "%=",
	[TokenType_Equal]				= "=",
	[TokenType_EqualEqual]			= "==",
	[TokenType_Less]				= "<",
	[TokenType_LessEqual]			= "<=",
	[TokenType_LessLess]			= "<<",
	[TokenType_LessLessEqual]		= "<<=",
	[TokenType_Greater]				= ">",
	[TokenType_GreaterEqual]		= ">=",
	[TokenType_GreaterGreater]		= ">>",
	[TokenType_GreaterGreaterEqual] = ">>=",
	[TokenType_Arrow]				= "->",
};

const char* token_type_to_string(TokenType type)
{
	return token_strings[type];
}

PrimitiveDatatype token_to_datatype(TokenType type)
{
	switch (type)
	{
		case TokenType_B32: return PrimitiveDatatype_B32;
		case TokenType_U32: return PrimitiveDatatype_U32;
		case TokenType_I32: return PrimitiveDatatype_I32;
		case TokenType_F32: return PrimitiveDatatype_F32;
		default:
			assert(false);
			return PrimitiveDatatype_Unknown;
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
		printf("%s (%s, %d)\n", serialize_primitive_data(expression->literal), data_type_to_string(expression->result_data_type), (i32)expression_handle);
	}
	else if (expression->type == ExpressionType_Identifier)
	{
		IdentifierExpression e = expression->identifier;

		printf("%.*s (%s, %d)\n", (i32)e.name.len, e.name.str, data_type_to_string(expression->result_data_type), (i32)expression_handle);
	}
	else if (expression_is_binary_operation(expression->type))
	{
		BinaryExpression e = expression->binary;

		printf("%s (%s, %d)\n", operator_strings[expression->type], data_type_to_string(expression->result_data_type), (i32)expression_handle);

		set_bit(active_mask, indent + 1);
		print_expression(program, e.lhs, indent + 1, active_mask);

		clear_bit(active_mask, indent + 1);
		print_expression(program, e.rhs, indent + 1, active_mask);
	}
	else if (expression_is_unary_operation(expression->type))
	{
		UnaryExpression e = expression->unary;

		printf("%s (%s, %d)\n", operator_strings[expression->type], data_type_to_string(expression->result_data_type), (i32)expression_handle);
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
	set_bit(active_mask, indent);

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

			print_statements(program, statement_index + 1, statement->block.statement_count, indent + 1, active_mask);
			i += statement->block.statement_count;
		}
		else if (statement->type == StatementType_Branch)
		{
			BranchStatement e = statement->branch;

			printf("Branch\n");

			print_expression(program, e.condition, indent + 1, active_mask);

			print_statements(program, statement_index + 1, e.then_statement_count, indent + 1, active_mask);
			print_statements(program, statement_index + e.then_statement_count + 1, e.else_statement_count, indent + 1, active_mask);
			i += e.then_statement_count + e.else_statement_count;
		}
		else if (statement->type == StatementType_Loop)
		{
			LoopStatement e = statement->loop;

			printf("Loop\n");

			print_expression(program, e.condition, indent + 1, active_mask);

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

void print_ast(Program* program)
{
	for (i64 i = 0; i < program->functions.count; ++i)
	{
		print_function(program, program->functions.items[i]);
	}
}


