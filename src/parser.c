#include "parser.h"
#include "error.h"

#include <assert.h>


static ExpressionHandle push_expression(Program* program, Expression expression)
{
	ExpressionHandle result = (i32)program->expressions.count;
	array_push(&program->expressions, expression);
	return result;
}

static i32 push_statement(Program* program, Statement statement)
{
	i32 result = (i32)program->statements.count;
	array_push(&program->statements, statement);
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

static String get_token_string(ParseContext* context, Token token)
{
	return context->tokens.strings.items[token.data_index];
}

static ExpressionHandle parse_expression(ParseContext* context, i32 min_precedence);

static ExpressionHandle parse_atom(ParseContext* context)
{
	Token token = context_consume(context);

	if (token.type == TokenType_OpenParenthesis)
	{
		ExpressionHandle result = parse_expression(context, 0);
		if (!context_expect(context, TokenType_CloseParenthesis))
		{
			return 0;
		}
		context_advance(context);
		return result;
	}
	else if (token.type == TokenType_NumericLiteral)
	{
		PrimitiveData literal = context->tokens.numeric_literals.items[token.data_index];
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
		String identifier = get_token_string(context, token);

		if (context_peek_type(context) == TokenType_OpenParenthesis)
		{
			context_advance(context);

			Expression function_call_expression =
			{
				.type = ExpressionType_FunctionCall,
				.source_location = token.source_location,
				.function_call = {.function_name = identifier, .first_argument = 0 },
				.result_data_type = PrimitiveDatatype_I32
			}; // TODO: result_data_type.


			ExpressionHandle last = 0;
			while (context_expect_not_eof(context) && context_peek_type(context) != TokenType_CloseParenthesis)
			{
				if (function_call_expression.function_call.first_argument)
				{
					if (!context_expect(context, TokenType_Comma))
					{
						return 0;
					}
					context_advance(context);
				}

				ExpressionHandle expression = parse_expression(context, 0);
				if (last)
				{
					program_get_expression(context->program, last)->next = expression;
				}
				else
				{
					function_call_expression.function_call.first_argument = expression;
				}
				last = expression;
			}

			if (!context_expect(context, TokenType_CloseParenthesis))
			{
				return 0;
			}
			context_advance(context);
			
			return push_expression(context->program, function_call_expression);
		}
		else
		{
			Expression expression =
			{
				.type = ExpressionType_Identifier,
				.source_location = token.source_location,
				.identifier = identifier,
				.result_data_type = PrimitiveDatatype_I32
			}; // TODO: result_data_type.
			return push_expression(context->program, expression);
		}
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

static i32 parse_statement(ParseContext* context)
{
	Token token = context_peek(context);

	if (token.type == TokenType_Identifier)
	{
		context_advance(context);

		String identifier = get_token_string(context, token);

		Expression lhs_expression =
		{
			.type = ExpressionType_Identifier,
			.source_location = token.source_location,
			.identifier = {.name = identifier }
		};
		ExpressionHandle lhs = push_expression(context->program, lhs_expression);

		TokenType declaration_assignment_token = context_peek_type(context);

		Statement statement =
		{
			.type = StatementType_Error,
			.source_location = token.source_location,
		};

		if (declaration_assignment_token == TokenType_Colon)
		{
			context_advance(context);

			Token data_type_token = context_consume(context);
			PrimitiveDatatype data_type = token_to_datatype(data_type_token.type);

			statement.type = StatementType_Declaration;
			statement.declaration = (DeclarationStatement) { .data_type = data_type, .lhs = lhs };

			TokenType next = context_peek_type(context);
			if (next == TokenType_Semicolon)
			{
				statement.type = StatementType_Declaration;
				statement.declaration = (DeclarationStatement) { .data_type = data_type, .lhs = lhs };
			}
			else if (next == TokenType_Equal)
			{
				context_advance(context);

				ExpressionHandle rhs = parse_expression(context, 0);

				statement.type = StatementType_DeclarationAssignment;
				statement.declaration_assignment = (DeclarationAssignmentStatement) { .data_type = data_type, .lhs = lhs, .rhs = rhs };
			}
			else
			{
				Token unexpected_token = context_peek(context);
				print_line_error(context->program->source_code, unexpected_token.source_location);
			}
		}
		else if (declaration_assignment_token == TokenType_ColonEqual)
		{
			context_advance(context);

			ExpressionHandle rhs = parse_expression(context, 0);

			statement.type = StatementType_DeclarationAssignment;
			statement.declaration_assignment = (DeclarationAssignmentStatement){ .data_type = PrimitiveDatatype_Unknown, .lhs = lhs, .rhs = rhs };
		}
		else if (token_is_assignment_operator(declaration_assignment_token))
		{
			context_withdraw(context);
			ExpressionHandle expression = parse_expression(context, 0);

			statement.type = StatementType_Simple;
			statement.simple = (SimpleStatement) { .expression = expression };
		}

		if (statement.type != StatementType_Error)
		{
			if (context_expect(context, TokenType_Semicolon))
			{
				context_advance(context);
				push_statement(context->program, statement);
				return 1;
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
				Statement statement =
				{
					.type = StatementType_Branch,
					.source_location = token.source_location,
					.branch = {.condition = condition }
				};
				i32 statement_index = push_statement(context->program, statement);

				i32 then_statement_count = parse_statement(context);
				i32 else_statement_count = 0;

				TokenType next_token = context_peek_type(context);
				if (next_token == TokenType_Else)
				{
					context_advance(context);
					else_statement_count = parse_statement(context);
				}

				context->program->statements.items[statement_index].branch.then_statement_count = then_statement_count;
				context->program->statements.items[statement_index].branch.else_statement_count = else_statement_count;

				return then_statement_count + else_statement_count + 1;
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
				Statement statement =
				{
					.type = StatementType_Loop,
					.source_location = token.source_location,
					.loop = { .condition = condition }
				};
				i32 statement_index = push_statement(context->program, statement);

				i32 then_statement_count = parse_statement(context);
				context->program->statements.items[statement_index].loop.then_statement_count = then_statement_count;

				return then_statement_count + 1;
			}
		}
	}
	else if (token.type == TokenType_Return)
	{
		context_advance(context);

		ExpressionHandle rhs = parse_expression(context, 0);
		if (rhs)
		{
			if (context_expect(context, TokenType_Semicolon))
			{
				context_advance(context);

				Statement statement =
				{
					.type = StatementType_Return,
					.source_location = token.source_location,
					.ret = {.rhs = rhs }
				};
				push_statement(context->program, statement);
				return true;
			}
		}
	}
	else if (token.type == TokenType_OpenBrace)
	{
		context_advance(context);

		Statement statement =
		{
			.type = StatementType_Block,
			.source_location = token.source_location,
		};

		i32 statement_index = push_statement(context->program, statement);

		i32 statement_count = 0;
		while (context_expect_not_eof(context) && context_peek_type(context) != TokenType_CloseBrace)
		{
			statement_count += parse_statement(context);
		}
		context->program->statements.items[statement_index].block.statement_count = statement_count;

		if (context_expect(context, TokenType_CloseBrace))
		{
			context_advance(context);
			return statement_count + 1;
		}
	}
	else
	{
		ExpressionHandle expression = parse_expression(context, 0);
		if (expression)
		{
			if (context_expect(context, TokenType_Semicolon))
			{
				context_advance(context);
				Statement statement =
				{
					.type = StatementType_Simple,
					.source_location = token.source_location,
					.simple = {.expression = expression },
				};
				push_statement(context->program, statement);
				return 1;
			}
		}
	}

	// Error -> flush to next semicolon.
	for (; context_peek_type(context) != TokenType_Semicolon && context_peek_type(context) != TokenType_EOF; context_advance(context))
	{
	}

	if (context_peek_type(context) != TokenType_EOF)
	{
		assert(context_peek_type(context) == TokenType_Semicolon);
		context_advance(context);
	}

	return 0;
}

static b32 parse_function(ParseContext* context)
{
	SourceLocation source_location = context_peek(context).source_location;

	if (!context_expect(context, TokenType_Function))
	{
		return false;
	}
	context_advance(context);

	if (!context_expect(context, TokenType_Identifier))
	{
		return false;
	}
	Token name_token = context_consume(context);

	if (!context_expect(context, TokenType_ColonColon))
	{
		return false;
	}
	context_advance(context);

	if (!context_expect(context, TokenType_OpenParenthesis))
	{
		return false;
	}
	context_advance(context);


	i64 first_parameter = context->program->function_parameters.count;
	while (context_peek_type(context) != TokenType_CloseParenthesis && context_peek_type(context) != TokenType_EOF)
	{
		if (!context_expect(context, TokenType_Identifier))
		{
			return false;
		}
		Token parameter_name_token = context_consume(context);

		if (!context_expect(context, TokenType_Colon))
		{
			return false;
		}
		context_advance(context);

		if (!context_expect(context, TokenType_I32))
		{
			return false;
		}
		context_advance(context);



		String parameter_name = get_token_string(context, parameter_name_token);

		FunctionParameter parameter = { .name = parameter_name };
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

	i64 parameter_count = context->program->function_parameters.count - first_parameter;


	if (!context_expect(context, TokenType_Arrow))
	{
		return false;
	}
	context_advance(context);

	if (!context_expect(context, TokenType_OpenParenthesis))
	{
		return false;
	}
	context_advance(context);

	if (!context_expect(context, TokenType_I32))
	{
		return false;
	}
	context_advance(context);

	if (!context_expect(context, TokenType_CloseParenthesis))
	{
		return false;
	}
	context_advance(context);

	if (!context_expect(context, TokenType_OpenBrace))
	{
		return false;
	}
	// Don't advance.

	i32 body_statement_index = (i32)context->program->statements.count;
	i32 body_statement_count = parse_statement(context);
	if (!body_statement_count)
	{
		return false;
	}

	Function function = { 0 };
	function.name = get_token_string(context, name_token);
	function.source_location = source_location;
	function.calling_convention = CallingConvention_Windows_x64;
	function.body_first_statement = body_statement_index;
	function.body_statement_count = body_statement_count;
	function.first_parameter = first_parameter;
	function.parameter_count = parameter_count;

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

		if (!success)
		{
			// Temporary.
			return false;
		}
	}

	return result;
}
