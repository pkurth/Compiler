#include "parser.h"
#include "error.h"

#include <assert.h>


static ExpressionHandle push_expression(Program* program, Expression expression)
{
	ExpressionHandle result = (i32)program->expressions.count;
	array_push(&program->expressions, expression);
	return result;
}

struct ParseContext
{
	Program* program;
	TokenStream tokens;
	i64 current_token;
};
typedef struct ParseContext ParseContext;

static Token context_peek(ParseContext* context)
{
	return context->tokens.tokens.items[context->current_token];
}

static TokenType context_peek_type(ParseContext* context)
{
	return context->tokens.tokens.items[context->current_token].type;
}

static void context_advance(ParseContext* context)
{
	++context->current_token;
}

static void context_withdraw(ParseContext* context)
{
	--context->current_token;
}

static Token context_consume(ParseContext* context)
{
	return context->tokens.tokens.items[context->current_token++];
}



enum Associativity
{
	Associativity_Right = 0,
	Associativity_Left	= 1,
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
	[TokenType_Equal]               = { ExpressionType_Assignment,      Associativity_Right,    0 },
    [TokenType_PipeEqual]           = { ExpressionType_BitwiseOr,       Associativity_Right,    0 },
    [TokenType_HatEqual]			= { ExpressionType_BitwiseXor,      Associativity_Right,    0 },
    [TokenType_AmpersandEqual]      = { ExpressionType_BitwiseAnd,      Associativity_Right,    0 },
    [TokenType_LessLessEqual]       = { ExpressionType_LeftShift,       Associativity_Right,    0 },
    [TokenType_GreaterGreaterEqual] = { ExpressionType_RightShift,      Associativity_Right,    0 },
    [TokenType_PlusEqual]           = { ExpressionType_Addition,        Associativity_Right,    0 },
    [TokenType_MinusEqual]          = { ExpressionType_Subtraction,     Associativity_Right,    0 },
    [TokenType_StarEqual]           = { ExpressionType_Multiplication,  Associativity_Right,    0 },
    [TokenType_ForwardSlashEqual]   = { ExpressionType_Division,        Associativity_Right,    0 },
    [TokenType_PercentEqual]        = { ExpressionType_Modulo,          Associativity_Right,    0 },
    [TokenType_PipePipe]            = { ExpressionType_LogicalOr,       Associativity_Left,		1 },
    [TokenType_AmpersandAmpersand]  = { ExpressionType_LogicalAnd,      Associativity_Left,		2 },
    [TokenType_Pipe]                = { ExpressionType_BitwiseOr,       Associativity_Left,		3 },
    [TokenType_Hat]                 = { ExpressionType_BitwiseXor,      Associativity_Left,		4 },
    [TokenType_Ampersand]           = { ExpressionType_BitwiseAnd,      Associativity_Left,		5 },
    [TokenType_EqualEqual]          = { ExpressionType_Equal,           Associativity_Left,		6 },
    [TokenType_ExclamationEqual]    = { ExpressionType_NotEqual,        Associativity_Left,		6 },
    [TokenType_Less]                = { ExpressionType_Less,            Associativity_Left,		7 },
    [TokenType_Greater]             = { ExpressionType_Greater,         Associativity_Left,		7 },
    [TokenType_LessEqual]           = { ExpressionType_LessEqual,       Associativity_Left,		7 },
    [TokenType_GreaterEqual]        = { ExpressionType_GreaterEqual,    Associativity_Left,		7 },
    [TokenType_LessLess]            = { ExpressionType_LeftShift,       Associativity_Left,		8 },
    [TokenType_GreaterGreater]      = { ExpressionType_RightShift,      Associativity_Left,		8 },
    [TokenType_Plus]                = { ExpressionType_Addition,        Associativity_Left,		9 },
    [TokenType_Minus]               = { ExpressionType_Subtraction,     Associativity_Left,		9 },
    [TokenType_Star]                = { ExpressionType_Multiplication,  Associativity_Left,		10 },
    [TokenType_ForwardSlash]        = { ExpressionType_Division,        Associativity_Left,		10 },
    [TokenType_Percent]             = { ExpressionType_Modulo,          Associativity_Left,		10 },
};

static ExpressionType unary_operator_infos[TokenType_Count] =
{
	[TokenType_Minus]				= ExpressionType_Negate,
	[TokenType_Tilde]				= ExpressionType_BitwiseNot,
	[TokenType_Exclamation]			= ExpressionType_Not,
};

static b32 context_expect_not_eof(ParseContext* context)
{
	b32 result = context_peek_type(context) != TokenType_EOF;
	if (!result)
	{
		fprintf(stderr, "LINE %d: Unexpected EOF.\n", context_peek(context).source_location.line);
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
			fprintf(stderr, "LINE %d: Expected identifier, got '%s'.\n", unexpected_token.source_location.line, token_type_to_string(unexpected_token.type));
		}
		else if (expected == TokenType_NumericLiteral)
		{
			fprintf(stderr, "LINE %d: Expected literal, got '%s'.\n", unexpected_token.source_location.line, token_type_to_string(unexpected_token.type));
		}
		else
		{
			fprintf(stderr, "LINE %d: Expected '%s', got '%s'.\n", unexpected_token.source_location.line, token_type_to_string(expected), token_type_to_string(unexpected_token.type));
		}

		print_line_error(context->program->source_code, unexpected_token.source_location);
	}
	return result;
}

static ExpressionHandle parse_expression(ParseContext* context, i32 min_precedence);

static ExpressionHandle parse_atom(ParseContext* context)
{
	Token token = context_consume(context);

	if (token.type == TokenType_OpenParenthesis)
	{
		ExpressionHandle result = parse_expression(context, 1);
		if (!context_expect(context, TokenType_CloseParenthesis))
		{
			return 0;
		}
		context_advance(context);
		return result;
	}
	else if (token.type == TokenType_NumericLiteral)
	{
		PrimitiveData literal = context->tokens.literals.items[token.data_index];
		Expression expression = 
		{ 
			.type = ExpressionType_NumericLiteral,
			.source_location = token.source_location,
			.literal = literal,
			.result_data_type = literal.type
		};
		return push_expression(context->program, expression);
	}
	else if (token.type == TokenType_Identifier)
	{
		String identifier = context->tokens.identifiers.items[token.data_index];
		Expression expression = 
		{ 
			.type = ExpressionType_Identifier,
			.source_location = token.source_location,
			.identifier = identifier, 
			.result_data_type = PrimitiveDatatype_I32 
		}; // TODO: result_data_type.
		return push_expression(context->program, expression);
	}
	else if (token_is_unary_operator(token.type) && context_expect_not_eof(context))
	{
		ExpressionHandle rhs = parse_atom(context);

		Expression expression = 
		{ 
			.type = unary_operator_infos[token.type],
			.source_location = token.source_location,
			.unary = {.rhs = rhs } 
		};
		return push_expression(context->program, expression);
	}
	else
	{
		fprintf(stderr, "LINE %d: Unexpected token '%s'.\n", token.source_location.line, token_type_to_string(token.type));

		print_line_error(context->program->source_code, token.source_location);
	}

	return 0;
}

static ExpressionHandle parse_expression(ParseContext* context, i32 min_precedence)
{
	// https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing

	ExpressionHandle lhs = parse_atom(context);

	for (;;)
	{
		Token next_token = context_peek(context);
		TokenType next_token_type = next_token.type;

		if (!token_is_binary_operator(next_token_type) && !token_is_assignment_operator(next_token_type))
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

		ExpressionHandle rhs = parse_expression(context, next_min_precedence);

		if (token_is_binary_operator(next_token_type))
		{
			Expression operation = 
			{ 
				.type = info.expression_type,
				.source_location = next_token.source_location,
				.binary = {.lhs = lhs, .rhs = rhs }
			};
			lhs = push_expression(context->program, operation);
		}
		else if (token_is_assignment_operator(next_token_type))
		{
			if (next_token_type != TokenType_Equal)
			{
				Expression operation = 
				{ 
					.type = info.expression_type,
					.source_location = next_token.source_location,
					.binary = {.lhs = lhs, .rhs = rhs } 
				};
				rhs = push_expression(context->program, operation);
			}

			Expression assignment = 
			{ 
				.type = ExpressionType_Assignment,
				.source_location = next_token.source_location,
				.assignment = {.lhs = lhs, .rhs = rhs } 
			};
			lhs = push_expression(context->program, assignment);
		}
	}

	return lhs;
}

static ExpressionHandle parse_top_level_expression(ParseContext* context)
{
	ExpressionHandle result = ExpressionType_Error;

	Token token = context_peek(context);
	if (token_is_datatype(token.type))
	{
		context_advance(context);

		if (context_expect(context, TokenType_Identifier))
		{
			Token identifier_token = context_consume(context);
			String identifier = context->tokens.identifiers.items[identifier_token.data_index];

			Expression lhs_expression = 
			{ 
				.type = ExpressionType_Identifier,
				.source_location = identifier_token.source_location,
				.identifier = { .name = identifier } 
			};
			ExpressionHandle lhs = push_expression(context->program, lhs_expression);

			TokenType next = context_peek_type(context);
			if (next == TokenType_Semicolon)
			{
				context_advance(context);

				Expression expression =
				{
					.type = ExpressionType_Declaration,
					.source_location = token.source_location,
					.declaration = {.data_type = token_to_datatype(token.type), .lhs = lhs }
				};
				result = push_expression(context->program, expression);
			}
			else if (next == TokenType_Equal)
			{
				context_advance(context);

				ExpressionHandle rhs = parse_expression(context, 0);
				if (rhs)
				{
					Expression expression =
					{
						.type = ExpressionType_DeclarationAssignment,
						.source_location = token.source_location,
						.declaration_assignment = { .data_type = token_to_datatype(token.type), .lhs = lhs, .rhs = rhs }
					};
					result = push_expression(context->program, expression);
				}

				if (context_expect(context, TokenType_Semicolon))
				{
					context_advance(context);
				}
			}
			else
			{
				Token unexpected_token = context_peek(context);
				print_line_error(context->program->source_code, unexpected_token.source_location);
			}
		}
	}
	else if (token.type == TokenType_If)
	{
		context_advance(context);

		if (context_expect(context, TokenType_OpenParenthesis))
		{
			ExpressionHandle condition = parse_expression(context, 0);
			if (condition)
			{
				ExpressionHandle then_expression = parse_top_level_expression(context);
				if (then_expression)
				{
					ExpressionHandle else_expression = 0;

					TokenType next_token = context_peek_type(context);
					if (next_token == TokenType_Else)
					{
						context_advance(context);
						else_expression = parse_top_level_expression(context);
					}

					Expression branch_expression =
					{
						.type = ExpressionType_Branch,
						.source_location = token.source_location,
						.branch = {.condition = condition, .then_expression = then_expression, .else_expression = else_expression }
					};
					result = push_expression(context->program, branch_expression);
				}
			}
		}
	}
	else if (token.type == TokenType_While)
	{
		context_advance(context);

		if (context_expect(context, TokenType_OpenParenthesis))
		{
			ExpressionHandle condition = parse_expression(context, 0);
			if (condition)
			{
				ExpressionHandle then_expression = parse_top_level_expression(context);
				if (then_expression)
				{
					Expression loop_expression =
					{
						.type = ExpressionType_Loop,
						.source_location = token.source_location,
						.loop = {.condition = condition, .then_expression = then_expression }
					};
					result = push_expression(context->program, loop_expression);
				}
			}
		}
	}
	else if (token.type == TokenType_Return)
	{
		context_advance(context);

		if (context_expect_not_eof(context))
		{
			ExpressionHandle rhs = parse_expression(context, 0);
			if (rhs)
			{
				if (context_expect(context, TokenType_Semicolon))
				{
					context_advance(context);

					Expression return_expression =
					{
						.type = ExpressionType_Return,
						.source_location = token.source_location,
						.ret = {.rhs = rhs }
					};
					result = push_expression(context->program, return_expression);
				}
			}
		}
	}
	else if (token.type == TokenType_OpenBrace)
	{
		context_advance(context);

		Expression block_expression =
		{
			.type = ExpressionType_Block,
			.source_location = token.source_location,
		};

		ExpressionHandle last = 0;
		while (context_expect_not_eof(context) && context_peek_type(context) != TokenType_CloseBrace)
		{
			ExpressionHandle expression = parse_top_level_expression(context);
			if (last)
			{
				program_get_expression(context->program, last)->next = expression;
			}
			else
			{
				block_expression.block.first_expression = expression;
			}
			last = expression;
		}

		if (context_expect(context, TokenType_CloseBrace))
		{
			context_advance(context);
		}

		result = push_expression(context->program, block_expression);
	}
	else
	{
		result = parse_expression(context, 0);
		if (context_expect(context, TokenType_Semicolon))
		{
			context_advance(context);
		}
	}

	if (result == ExpressionType_Error)
	{
		// Error -> flush to next semicolon.
		for (; context_peek_type(context) != TokenType_Semicolon && context_peek_type(context) != TokenType_EOF; context_advance(context))
		{
		}

		if (context_peek_type(context) != TokenType_EOF)
		{
			assert(context_peek_type(context) == TokenType_Semicolon);
			context_advance(context);
		}
	}

	return result;
}

static b32 parse_function_parameters(ParseContext* context, Function* function)
{
	Token token = context_consume(context);

	if (token.type == TokenType_OpenParenthesis)
	{
		i64 first_parameter = context->program->function_parameters.count;
		while (context_peek_type(context) != TokenType_CloseParenthesis && context_peek_type(context) != TokenType_EOF)
		{
			if (!context_expect(context, TokenType_I32))
			{
				return false;
			}
			context_advance(context);

			if (!context_expect(context, TokenType_Identifier))
			{
				return false;
			}
			Token identifier_token = context_consume(context);
			String identifier = context->tokens.identifiers.items[identifier_token.data_index];

			FunctionParameter parameter = { .name = identifier.str };
			array_push(&context->program->function_parameters, parameter);

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
		function->parameter_count = context->program->function_parameters.count - first_parameter;

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
			if (!context_expect(context, TokenType_I32))
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

static b32 parse_function(ParseContext* context)
{
	Function function = { 0 };

	if (!parse_function_parameters(context, &function))
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
		Token identifier_token = context_consume(context);
		String identifier = context->tokens.identifiers.items[identifier_token.data_index];
		function.name = identifier;
	}

	if (context_expect(context, TokenType_Equal))
	{
		context_advance(context);
	}

	if (context_expect(context, TokenType_OpenBrace))
	{
		// Don't advance.
	}

	function.first_expression = parse_top_level_expression(context);

	array_push(&context->program->functions, function);

	return true;
}

b32 parse(Program* program, TokenStream stream)
{
	ParseContext context = { program, stream, 0 };

	push_expression(program, (Expression) { .type = ExpressionType_Error }); // Dummy.

	b32 result = true;

	while (context_peek_type(&context) != TokenType_EOF)
	{
		b32 success = parse_function(&context);
		result &= success;
	}

	return result;
}
