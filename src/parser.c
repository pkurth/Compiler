#include "parser.h"

#include <assert.h>


static ExpressionHandle push_expression(Program* stream, Expression expression)
{
	ExpressionHandle result = stream->expressions.count;
	array_push(&stream->expressions, expression);
	return result;
}

struct ParseContext
{
	TokenStream tokens;
	u64 current_token;
};
typedef struct ParseContext ParseContext;

static Token context_peek(ParseContext* context)
{
	return context->tokens.items[context->current_token];
}

static TokenType context_peek_type(ParseContext* context)
{
	return context->tokens.items[context->current_token].type;
}

static void context_advance(ParseContext* context)
{
	++context->current_token;
}

static Token context_consume(ParseContext* context)
{
	return context->tokens.items[context->current_token++];
}



enum Associativity
{
	Associativity_Right = 0,
	Associativity_Left  = 1,
};
typedef enum Associativity Associativity;

struct OperatorInfo
{
	ExpressionType expression_type;
	Associativity associativity;
	i32 precedence;
};
typedef struct OperatorInfo OperatorInfo;


static OperatorInfo binary_operator_infos[TokenType_Count] =
{
	[TokenType_PipePipe]			= { ExpressionType_LogicalOr,		Associativity_Left,		1 },
	[TokenType_AmpersandAmpersand]	= { ExpressionType_LogicalAnd,		Associativity_Left,		2 },
	[TokenType_Pipe]				= { ExpressionType_BitwiseOr,		Associativity_Left,		3 },
	[TokenType_Hat]					= { ExpressionType_BitwiseXor,		Associativity_Left,		4 },
	[TokenType_Ampersand]			= { ExpressionType_BitwiseAnd,		Associativity_Left,		5 },
	[TokenType_EqualEqual]			= { ExpressionType_Equal,			Associativity_Left,		6 },
	[TokenType_ExclamationEqual]	= { ExpressionType_NotEqual,		Associativity_Left,		6 },
	[TokenType_Less]				= { ExpressionType_Less,			Associativity_Left,		7 },
	[TokenType_Greater]				= { ExpressionType_Greater,			Associativity_Left,		7 },
	[TokenType_LessEqual]			= { ExpressionType_LessEqual,		Associativity_Left,		7 },
	[TokenType_GreaterEqual]		= { ExpressionType_GreaterEqual,	Associativity_Left,		7 },
	[TokenType_LessLess]			= { ExpressionType_LeftShift,		Associativity_Left,		8 },
	[TokenType_GreaterGreater]		= { ExpressionType_RightShift,		Associativity_Left,		8 },
	[TokenType_Plus]				= { ExpressionType_Addition,		Associativity_Left,		9 },
	[TokenType_Minus]				= { ExpressionType_Subtraction,		Associativity_Left,		9 },
	[TokenType_Star]				= { ExpressionType_Multiplication,	Associativity_Left,		10 },
	[TokenType_ForwardSlash]		= { ExpressionType_Division,		Associativity_Left,		10 },
	[TokenType_Percent]				= { ExpressionType_Modulo,			Associativity_Left,		10 },
};

static ExpressionType unary_operator_infos[TokenType_Count] =
{
	[TokenType_Minus]				= ExpressionType_Negate,
	[TokenType_Tilde]				= ExpressionType_BitwiseNot,
	[TokenType_Exclamation]			= ExpressionType_Not,
};

static ExpressionType assignment_operator_infos[TokenType_Count] =
{
	[TokenType_PipeEqual]			= ExpressionType_BitwiseOr,
	[TokenType_HatEqual]			= ExpressionType_BitwiseXor,
	[TokenType_AmpersandEqual]		= ExpressionType_BitwiseAnd,
	[TokenType_LessLessEqual]		= ExpressionType_LeftShift,
	[TokenType_GreaterGreaterEqual]	= ExpressionType_RightShift,
	[TokenType_PlusEqual]			= ExpressionType_Addition,
	[TokenType_MinusEqual]			= ExpressionType_Subtraction,
	[TokenType_StarEqual]			= ExpressionType_Multiplication,
	[TokenType_ForwardSlashEqual]	= ExpressionType_Division,
	[TokenType_PercentEqual]		= ExpressionType_Modulo,
};


static b32 context_expect_not_eof(ParseContext* context)
{
	b32 result = context_peek_type(context) != TokenType_EOF;
	if (!result)
	{
		fprintf(stderr, "LINE %d: Unexpected EOF.\n", (i32)context_peek(context).line);
	}
	return result;
}

static b32 context_expect(ParseContext* context, TokenType expected)
{
	b32 result = context_peek_type(context) == expected;
	if (!result)
	{
		Token unexpected_token = context_peek(context);
		if (expected == TokenType_Identifier)
		{
			fprintf(stderr, "LINE %d: Expected identifier, got '%.*s'.\n", (i32)unexpected_token.line, (i32)unexpected_token.str.len, unexpected_token.str.str);
		}
		else if (expected == TokenType_IntLiteral)
		{
			fprintf(stderr, "LINE %d: Expected literal, got '%.*s'.\n", (i32)unexpected_token.line, (i32)unexpected_token.str.len, unexpected_token.str.str);
		}
		else
		{
			const char* expected_str = token_type_to_string(expected);
			fprintf(stderr, "LINE %d: Expected '%s', got '%.*s'.\n", (i32)unexpected_token.line, expected_str, (i32)unexpected_token.str.len, unexpected_token.str.str);
		}
	}
	return result;
}

static ExpressionHandle parse_expression(ParseContext* context, Program* program, i32 min_precedence);

static ExpressionHandle parse_atom(ParseContext* context, Program* program)
{
	Token token = context_consume(context);
	
	if (token.type == TokenType_OpenParenthesis)
	{
		ExpressionHandle result = parse_expression(context, program, 1);
		if (!context_expect(context, TokenType_CloseParenthesis))
		{
			return 0;
		}
		context_advance(context);
		return result;
	}
	else if (token.type == TokenType_IntLiteral)
	{
		Expression expression = { .type = ExpressionType_IntLiteral, .int_literal = token };
		return push_expression(program, expression);
	}
	else if (token.type == TokenType_Identifier)
	{
		Expression expression = { .type = ExpressionType_Identifier, .identifier = token };
		return push_expression(program, expression);
	}
	else if (token_is_unary_operator(token.type) && context_expect_not_eof(context))
	{
		Expression expression = { .type = unary_operator_infos[token.type], {.unary = { .expression = parse_atom(context, program) } } };
		return push_expression(program, expression);
	}
	else
	{
		fprintf(stderr, "LINE %d: Unexpected token '%.*s'.\n", (i32)token.line, (i32)token.str.len, token.str.str);
	}

	return 0;
}

static ExpressionHandle parse_expression(ParseContext* context, Program* program, i32 min_precedence)
{
	// https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing

	ExpressionHandle lhs = parse_atom(context, program);

	for (;;)
	{
		TokenType next_token_type = context_peek_type(context);
		if (!token_is_binary_operator(next_token_type))
		{
			break;
		}

		OperatorInfo info = binary_operator_infos[next_token_type];
		if (info.precedence < min_precedence)
		{
			break;
		}

		i32 next_min_precedence = info.precedence + info.associativity;
		
		context_advance(context);

		ExpressionHandle rhs = parse_expression(context, program, next_min_precedence);

		Expression operation = { .type = info.expression_type, { .binary = { .lhs = lhs, .rhs = rhs } } };
		lhs = push_expression(program, operation);
	}

	return lhs;
}

static void parse_statement(ParseContext* context, Program* program)
{
	Statement statement = { .type = StatementType_Error };

	Token token = context_consume(context);
	if (token.type == TokenType_I64)
	{
		if (context_expect(context, TokenType_Identifier))
		{
			Token identifier = context_consume(context);

			if (context_expect(context, TokenType_Equal))
			{
				context_advance(context);

				if (context_expect_not_eof(context))
				{
					ExpressionHandle expression = parse_expression(context, program, 1);
					if (expression)
					{
						if (context_expect(context, TokenType_Semicolon))
						{
							context_advance(context);

							statement.type = StatementType_VariableAssignment;
							statement.variable_assignment.identifier = identifier;
							statement.variable_assignment.expression = expression;
						}
					}
				}
			}
		}
	}
	else if (token.type == TokenType_Identifier)
	{
		Token identifier = token;

		if (token_is_assignment_operator(context_peek_type(context)))
		{
			Token assignment = context_consume(context);

			if (context_expect_not_eof(context))
			{
				ExpressionHandle expression = parse_expression(context, program, 1);
				if (expression)
				{
					if (context_expect(context, TokenType_Semicolon))
					{
						context_advance(context);

						statement.type = StatementType_VariableReassignment;
						statement.variable_assignment.identifier = identifier;
						statement.variable_assignment.expression = expression;

						if (assignment.type != TokenType_Equal)
						{
							Expression lhs = { .type = ExpressionType_Identifier, .identifier = identifier };
							ExpressionHandle lhs_handle = push_expression(program, lhs);

							Expression operation = { .type = assignment_operator_infos[assignment.type], { .binary = { .lhs = lhs_handle, .rhs = expression } } };
							statement.variable_assignment.expression = push_expression(program, operation);
						}
					}
				}
			}
		}
		else
		{
			Token unexpected_token = context_peek(context);
			fprintf(stderr, "LINE %d: Expected assignment operator, got '%.*s'.\n", (i32)unexpected_token.line, (i32)unexpected_token.str.len, unexpected_token.str.str);
		}
	}
	else if (token.type == TokenType_Return)
	{
		if (context_expect_not_eof(context))
		{
			ExpressionHandle expression = parse_expression(context, program, 1);
			if (expression)
			{
				if (context_expect(context, TokenType_Semicolon))
				{
					context_advance(context);

					statement.type = StatementType_Return;
					statement.ret.expression = expression;
				}
			}
		}
	}
	else if (token.type == TokenType_OpenBrace)
	{
		u64 block_index = program->statements.count;
		statement.type = StatementType_Block;
		array_push(&program->statements, statement);

		u64 first_statement = program->statements.count;

		while (context_expect_not_eof(context) && context_peek_type(context) != TokenType_CloseBrace)
		{
			parse_statement(context, program);
		}

		program->statements.items[block_index].block.statement_count = program->statements.count - first_statement;

		if (context_expect(context, TokenType_CloseBrace))
		{
			context_advance(context);
		}

		return;
	}
	else
	{
		fprintf(stderr, "LINE %d: Unexpected token '%.*s'.\n", (i32)token.line, (i32)token.str.len, token.str.str);
	}

	if (statement.type == StatementType_Error)
	{
		for (; context_peek_type(context) != TokenType_Semicolon && context_peek_type(context) != TokenType_EOF; context_advance(context))
		{
		}

		if (context_peek_type(context) != TokenType_EOF)
		{
			assert(context_peek_type(context) == TokenType_Semicolon);
			context_advance(context);
		}
	}

	array_push(&program->statements, statement);
}

static b32 parse_function_parameters(ParseContext* context, Function* function, Program* program)
{
	Token token = context_consume(context);

	if (token.type == TokenType_OpenParenthesis)
	{
		u64 first_parameter = program->function_parameters.count;
		while (context_peek_type(context) != TokenType_CloseParenthesis && context_peek_type(context) != TokenType_EOF)
		{
			if (!context_expect(context, TokenType_I64))
			{
				return false;
			}
			context_advance(context);

			if (!context_expect(context, TokenType_Identifier))
			{
				return false;
			}
			Token identifier = context_consume(context);

			FunctionParameter parameter = { .name = identifier.str };
			array_push(&program->function_parameters, parameter);

			if (context_peek_type(context) == TokenType_Comma)
			{
				context_advance(context);
			}
		}

		if (!context_expect(context, TokenType_CloseParenthesis))
		{
			return false;
		}

		context_advance(context);
		function->first_parameter = first_parameter;
		function->parameter_count = program->function_parameters.count - first_parameter;

		return true;
	}

	return false;
}

static b32 parse_function_return_types(ParseContext* context, Function* function)
{
	Token token = context_consume(context);

	if (token.type == TokenType_OpenParenthesis)
	{
		while (context_peek_type(context) != TokenType_CloseParenthesis && context_peek_type(context) != TokenType_EOF)
		{
			if (!context_expect(context, TokenType_I64))
			{
				return false;
			}
			context_advance(context);

			if (context_peek_type(context) == TokenType_Comma)
			{
				context_advance(context);
			}
		}

		if (context_expect(context, TokenType_CloseParenthesis))
		{
			context_advance(context);
		}

		return true;
	}

	return false;
}


struct StackInfo
{
	i32 stack_size;
	i32 current_offset_from_frame_pointer;
};
typedef struct StackInfo StackInfo;

static void collect_local_variables(Program* program, u64 first_statement, u64 statement_count, StackInfo* info)
{
	for (u64 i = 0; i < statement_count; ++i)
	{
		u64 j = i + first_statement;

		Statement statement = program->statements.items[j];

		if (statement.type == StatementType_VariableAssignment)
		{
			Token identifier = statement.variable_assignment.identifier;

			info->current_offset_from_frame_pointer += 8; // Increment first!
			info->stack_size = max(info->stack_size, info->current_offset_from_frame_pointer);

			LocalVariable variable = { .name = identifier.str, .offset_from_frame_pointer = info->current_offset_from_frame_pointer };
			array_push(&program->local_variables, variable);
		}
		else if (statement.type == StatementType_Block)
		{
			i32 offset = info->current_offset_from_frame_pointer;

			assert(i + statement.block.statement_count <= statement_count);

			collect_local_variables(program, j + 1, statement.block.statement_count, info);
			i += statement.block.statement_count;

			info->current_offset_from_frame_pointer = offset;
		}
	}
}

static b32 parse_function(ParseContext* context, Program* program)
{
	Function function = { 0 };

	if (!parse_function_parameters(context, &function, program))
	{
		return false;
	}
	
	if (context_expect(context, TokenType_Arrow))
	{
		context_advance(context);
	}

	if (!parse_function_return_types(context, &function))
	{
		return false;
	}

	if (context_expect(context, TokenType_Identifier))
	{
		function.name = context_consume(context).str;
	}

	if (context_expect(context, TokenType_Equal))
	{
		context_advance(context);
	}

	function.first_statement = program->statements.count;

	parse_statement(context, program);

	function.statement_count = program->statements.count - function.first_statement;

	StackInfo info = { 0 };
	function.first_local_variable = program->local_variables.count;
	collect_local_variables(program, function.first_statement, function.statement_count, &info);
	function.local_variable_count = program->local_variables.count - function.first_local_variable;
	function.stack_size = info.stack_size;

	array_push(&program->functions, function);

	return true;
}

Program parse(TokenStream stream)
{
	Program program = { 0 };
	ParseContext context = { stream, 0 };

	push_expression(&program, (Expression) { .type = ExpressionType_Error }); // Dummy.

	b32 has_errors = 0;

	while (context_peek_type(&context) != TokenType_EOF)
	{
		b32 success = parse_function(&context, &program);
		has_errors |= !success;
	}

	program.has_errors = has_errors;

	return program;
}

void free_program(Program* program)
{
	array_free(&program->functions);
	array_free(&program->function_parameters);
	array_free(&program->local_variables);
	array_free(&program->statements);
	array_free(&program->expressions);
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
	[ExpressionType_Multiplication] = "*",
	[ExpressionType_Division]		= "/",
	[ExpressionType_Modulo]			= "%",

	[ExpressionType_Negate]			= "-",
	[ExpressionType_BitwiseNot]		= "~",
	[ExpressionType_Not]			= "!",
};

static void print_expression(Program* program, ExpressionHandle expression_handle, i32 indent, i32* active_mask)
{
	for (i32 i = 0; i < indent; ++i)
	{
		i32 last = (i == indent - 1);
		i32 is_active = ((1 << i) & *active_mask);
		printf((is_active || last) ? "|" : " ");
		printf(last ? "-> " : "   ");
	}

	Expression expression = program_get_expression(program, expression_handle);
	if (expression.type == ExpressionType_Error)
	{
		return;
	}

	*active_mask |= (1 << indent);

	if (expression.type == ExpressionType_IntLiteral)
	{
		printf("%.*s (%d)\n", (i32)expression.int_literal.str.len, expression.int_literal.str.str, (i32)expression_handle);
	}
	else if (expression.type == ExpressionType_Identifier)
	{
		printf("%.*s (%d)\n", (i32)expression.identifier.str.len, expression.identifier.str.str, (i32)expression_handle);
	}
	else if (expression_is_binary_operation(expression.type))
	{
		printf("%s (%d)\n", operator_strings[expression.type], (i32)expression_handle);

		print_expression(program, expression.binary.lhs, indent + 1, active_mask);
		*active_mask &= ~(1 << indent);
		print_expression(program, expression.binary.rhs, indent + 1, active_mask);
	}
	else if (expression_is_unary_operation(expression.type))
	{
		printf("%s (%d)\n", operator_strings[expression.type], (i32)expression_handle);
		print_expression(program, expression.unary.expression, indent + 1, active_mask);
	}
	else
	{
		assert(!"Unknown expression type");
	}

	*active_mask &= ~(1 << indent);
}

static void print_statements(Program* program, u64 first_statement, u64 statement_count, i32 indent)
{
	for (u64 i = 0; i < statement_count; ++i)
	{
		u64 j = i + first_statement;

		Statement statement = program->statements.items[j];

		for (i32 i = 0; i < indent; ++i)
		{
			printf("   ");
		}

		i32 active_mask = 0;

		if (statement.type == ExpressionType_Error)
		{
			continue;
		}

		if (statement.type == StatementType_VariableAssignment || statement.type == StatementType_VariableReassignment)
		{
			Token identifier = statement.variable_assignment.identifier;

			printf("* Variable assignment %.*s\n", (i32)identifier.str.len, identifier.str.str);
			print_expression(program, statement.variable_assignment.expression, indent + 1, &active_mask);
		}
		else if (statement.type == StatementType_Return)
		{
			printf("* Return\n");
			print_expression(program, statement.ret.expression, indent + 1, &active_mask);
		}
		else if (statement.type == StatementType_Block)
		{
			printf("* BLOCK\n");
			print_statements(program, j + 1, statement.block.statement_count, indent + 1);
			i += statement.block.statement_count;
		}
		else
		{
			assert(!"Unknown statement type");
		}
	}
}

static void print_function(Program* program, Function function)
{
	printf("FUNCTION %.*s\n", (i32)function.name.len, function.name.str);

	print_statements(program, function.first_statement, function.statement_count, 1);
	printf("\n");
}

void print_program(Program* program)
{
	for (u64 i = 0; i < program->functions.count; ++i)
	{
		print_function(program, program->functions.items[i]);
	}
}
