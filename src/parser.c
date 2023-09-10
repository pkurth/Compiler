#include "parser.h"

#include <assert.h>


static ExpressionHandle push_expression(Program* program, Expression expression)
{
	ExpressionHandle result = (i32)program->expressions.count;
	array_push(&program->expressions, expression);
	return result;
}

struct ParseContext
{
	TokenStream tokens;
	i64 current_token;
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

static void context_withdraw(ParseContext* context)
{
	--context->current_token;
}

static Token context_consume(ParseContext* context)
{
	return context->tokens.items[context->current_token++];
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
		fprintf(stderr, "LINE %d: Unexpected EOF.\n", context_peek(context).line);
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
			fprintf(stderr, "LINE %d: Expected identifier, got '%.*s'.\n", unexpected_token.line, (i32)unexpected_token.str.len, unexpected_token.str.str);
		}
		else if (expected == TokenType_NumericLiteral)
		{
			fprintf(stderr, "LINE %d: Expected literal, got '%.*s'.\n", unexpected_token.line, (i32)unexpected_token.str.len, unexpected_token.str.str);
		}
		else
		{
			const char* expected_str = token_type_to_string(expected);
			fprintf(stderr, "LINE %d: Expected '%s', got '%.*s'.\n", unexpected_token.line, expected_str, (i32)unexpected_token.str.len, unexpected_token.str.str);
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
	else if (token.type == TokenType_NumericLiteral)
	{
		Expression expression = { .type = ExpressionType_NumericLiteral, .literal = token.data, .result_data_type = token.data.type };
		return push_expression(program, expression);
	}
	else if (token.type == TokenType_Identifier)
	{
		Expression expression = { .type = ExpressionType_Identifier, .identifier = token.str, .result_data_type = PrimitiveDatatype_I32 }; // TODO: result_data_type.
		return push_expression(program, expression);
	}
	else if (token_is_unary_operator(token.type) && context_expect_not_eof(context))
	{
		ExpressionHandle rhs = parse_atom(context, program);

		Expression expression = { .type = unary_operator_infos[token.type], .unary = {.rhs = rhs } };
		return push_expression(program, expression);
	}
	else
	{
		fprintf(stderr, "LINE %d: Unexpected token '%.*s'.\n", token.line, (i32)token.str.len, token.str.str);
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

		ExpressionHandle rhs = parse_expression(context, program, next_min_precedence);

		if (token_is_binary_operator(next_token_type))
		{
			Expression operation = { .type = info.expression_type, .binary = {.lhs = lhs, .rhs = rhs } };
			lhs = push_expression(program, operation);
		}
		else if (token_is_assignment_operator(next_token_type))
		{
			if (next_token_type != TokenType_Equal)
			{
				Expression operation = { .type = info.expression_type, .binary = {.lhs = lhs, .rhs = rhs } };
				rhs = push_expression(program, operation);
			}

			Expression assignment = { .type = ExpressionType_Assignment, .assignment = {.lhs = lhs, .rhs = rhs } };
			lhs = push_expression(program, assignment);
		}
	}

	return lhs;
}

static ExpressionHandle parse_top_level_expression(ParseContext* context, Program* program)
{
	ExpressionHandle result = ExpressionType_Error;

	Token token = context_peek(context);
	if (token_is_datatype(token.type))
	{
		context_advance(context);

		if (context_expect(context, TokenType_Identifier))
		{
			Token identifier = context_consume(context);

			Expression lhs_expression = { .type = ExpressionType_Identifier, .identifier = {.name = identifier.str } };
			ExpressionHandle lhs = push_expression(program, lhs_expression);

			TokenType next = context_peek_type(context);
			if (next == TokenType_Semicolon)
			{
				context_advance(context);

				Expression expression =
				{
					.type = ExpressionType_Declaration,
					.declaration = {.data_type = token_to_datatype(token.type), .lhs = lhs }
				};
				result = push_expression(program, expression);
			}
			else if (next == TokenType_Equal)
			{
				context_advance(context);

				ExpressionHandle rhs = parse_expression(context, program, 0);
				if (rhs)
				{
					Expression expression =
					{
						.type = ExpressionType_DeclarationAssignment,
						.declaration_assignment = {.data_type = token_to_datatype(token.type), .lhs = lhs, .rhs = rhs }
					};
					result = push_expression(program, expression);
				}

				if (context_expect(context, TokenType_Semicolon))
				{
					context_advance(context);
				}
			}
			else
			{
				Token unexpected_token = context_peek(context);
				fprintf(stderr, "LINE %d: Unexpected token '%.*s'.\n", unexpected_token.line, (i32)unexpected_token.str.len, unexpected_token.str.str);
			}
		}
	}
	else if (token.type == TokenType_If)
	{
		context_advance(context);

		if (context_expect(context, TokenType_OpenParenthesis))
		{
			ExpressionHandle condition = parse_expression(context, program, 0);
			if (condition)
			{
				ExpressionHandle then_expression = parse_top_level_expression(context, program);
				if (then_expression)
				{
					ExpressionHandle else_expression = 0;

					TokenType next_token = context_peek_type(context);
					if (next_token == TokenType_Else)
					{
						context_advance(context);
						else_expression = parse_top_level_expression(context, program);
					}

					Expression branch_expression =
					{
						.type = ExpressionType_Branch,
						.branch = {.condition = condition, .then_expression = then_expression, .else_expression = else_expression }
					};
					result = push_expression(program, branch_expression);
				}
			}
		}
	}
	else if (token.type == TokenType_While)
	{
		context_advance(context);

		if (context_expect(context, TokenType_OpenParenthesis))
		{
			ExpressionHandle condition = parse_expression(context, program, 0);
			if (condition)
			{
				ExpressionHandle then_expression = parse_top_level_expression(context, program);
				if (then_expression)
				{
					Expression loop_expression =
					{
						.type = ExpressionType_Loop,
						.loop = {.condition = condition, .then_expression = then_expression }
					};
					result = push_expression(program, loop_expression);
				}
			}
		}
	}
	else if (token.type == TokenType_Return)
	{
		context_advance(context);

		if (context_expect_not_eof(context))
		{
			ExpressionHandle rhs = parse_expression(context, program, 0);
			if (rhs)
			{
				if (context_expect(context, TokenType_Semicolon))
				{
					context_advance(context);

					Expression return_expression =
					{
						.type = ExpressionType_Return,
						.ret = {.rhs = rhs }
					};
					result = push_expression(program, return_expression);
				}
			}
		}
	}
	else if (token.type == TokenType_OpenBrace)
	{
		context_advance(context);

		Expression block_expression =
		{
			.type = ExpressionType_Block
		};

		ExpressionHandle last = 0;
		while (context_expect_not_eof(context) && context_peek_type(context) != TokenType_CloseBrace)
		{
			ExpressionHandle expression = parse_top_level_expression(context, program);
			if (last)
			{
				program_get_expression(program, last)->next = expression;
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

		result = push_expression(program, block_expression);
	}
	else
	{
		result = parse_expression(context, program, 0);
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

static b32 parse_function_parameters(ParseContext* context, Function* function, Program* program)
{
	Token token = context_consume(context);

	if (token.type == TokenType_OpenParenthesis)
	{
		i64 first_parameter = program->function_parameters.count;
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

	if (context_expect(context, TokenType_OpenBrace))
	{
		// Don't advance.
	}

	function.first_expression = parse_top_level_expression(context, program);

	array_push(&program->functions, function);

	return true;
}

Program parse(TokenStream stream)
{
	Program program = { 0 };
	ParseContext context = { stream, 0 };

	push_expression(&program, (Expression) { .type = ExpressionType_Error }); // Dummy.

	while (context_peek_type(&context) != TokenType_EOF)
	{
		b32 success = parse_function(&context, &program);
		program.has_errors |= !success;
	}

	return program;
}

void free_program(Program* program)
{
	array_free(&program->functions);
	array_free(&program->function_parameters);
	array_free(&program->expressions);
}


static const char* operator_strings[ExpressionType_Count] =
{
	[ExpressionType_Error] = "",
	[ExpressionType_LogicalOr] = "||",
	[ExpressionType_LogicalAnd] = "&&",
	[ExpressionType_BitwiseOr] = "|",
	[ExpressionType_BitwiseXor] = "^",
	[ExpressionType_BitwiseAnd] = "&",
	[ExpressionType_Equal] = "==",
	[ExpressionType_NotEqual] = "!=",
	[ExpressionType_Less] = "<",
	[ExpressionType_Greater] = ">",
	[ExpressionType_LessEqual] = "<=",
	[ExpressionType_GreaterEqual] = ">=",
	[ExpressionType_LeftShift] = "<<",
	[ExpressionType_RightShift] = ">>",
	[ExpressionType_Addition] = "+",
	[ExpressionType_Subtraction] = "-",
	[ExpressionType_Multiplication] = "*",
	[ExpressionType_Division] = "/",
	[ExpressionType_Modulo] = "%",

	[ExpressionType_Negate] = "-",
	[ExpressionType_BitwiseNot] = "~",
	[ExpressionType_Not] = "!",
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

static void print_expression(Program* program, ExpressionHandle expression_handle, i32 indent, i32* active_mask)
{
	Expression* expression = program_get_expression(program, expression_handle);
	if (expression->type == ExpressionType_Error)
	{
		return;
	}

	for (i32 i = 0; i <= indent; ++i)
	{
		b32 last = (i == indent);
		b32 is_active = is_bit_set(*active_mask, i);
		printf((is_active || last) ? "|" : " ");
		printf(last ? "-> " : "   ");
	}

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
	else if (expression->type == ExpressionType_Declaration)
	{
		DeclarationExpression e = expression->declaration;

		Expression* lhs = program_get_expression(program, e.lhs);
		assert(lhs->type == ExpressionType_Identifier); // Temporary.

		String identifier = lhs->identifier.name;

		printf("Variable declaration %.*s\n", (i32)identifier.len, identifier.str);
	}
	else if (expression->type == ExpressionType_DeclarationAssignment)
	{
		DeclarationAssignmentExpression e = expression->declaration_assignment;

		Expression* lhs = program_get_expression(program, e.lhs);
		assert(lhs->type == ExpressionType_Identifier); // Temporary.

		String identifier = lhs->identifier.name;

		printf("Variable declaration & assignment %.*s\n", (i32)identifier.len, identifier.str);
		print_expression(program, e.rhs, indent + 1, active_mask);
	}
	else if (expression->type == ExpressionType_Return)
	{
		printf("Return\n");
		print_expression(program, expression->ret.rhs, indent + 1, active_mask);
	}
	else if (expression->type == ExpressionType_Block)
	{
		printf("Block\n");

		set_bit(active_mask, indent + 1);
		ExpressionHandle current = expression->block.first_expression;
		while (current)
		{
			ExpressionHandle next = program_get_expression(program, current)->next;
			if (!next) { clear_bit(active_mask, indent + 1); }

			print_expression(program, current, indent + 1, active_mask);
			current = next;
		}
	}
	else if (expression->type == ExpressionType_Branch)
	{
		BranchExpression e = expression->branch;

		printf("Branch\n");

		set_bit(active_mask, indent + 1);
		print_expression(program, e.condition, indent + 1, active_mask);

		if (!e.else_expression) { clear_bit(active_mask, indent + 1); }
		print_expression(program, e.then_expression, indent + 1, active_mask);

		if (e.else_expression)
		{
			clear_bit(active_mask, indent + 1);
			print_expression(program, e.else_expression, indent + 1, active_mask);
		}
	}
	else if (expression->type == ExpressionType_Loop)
	{
		LoopExpression e = expression->loop;

		printf("Loop\n");

		set_bit(active_mask, indent + 1);
		print_expression(program, e.condition, indent + 1, active_mask);

		clear_bit(active_mask, indent + 1);
		print_expression(program, e.then_expression, indent + 1, active_mask);
	}
	else
	{
		assert(!"Unknown expression type");
	}
}

static void print_function(Program* program, Function function)
{
	printf("FUNCTION %.*s\n", (i32)function.name.len, function.name.str);

	i32 active_mask = 0;
	print_expression(program, function.first_expression, 0, &active_mask);
	printf("\n");
}

void print_program(Program* program)
{
	for (i64 i = 0; i < program->functions.count; ++i)
	{
		print_function(program, program->functions.items[i]);
	}
}
