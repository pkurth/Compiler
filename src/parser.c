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

static TokenType context_peek(ParseContext* context)
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

static ExpressionHandle parse_expression(ParseContext* context, Program* program, i32 min_precedence);

static ExpressionHandle parse_atom(ParseContext* context, Program* program)
{
	Token token = context_consume(context);
	
	if (token.type == TokenType_OpenParenthesis)
	{
		ExpressionHandle result = parse_expression(context, program, 1);
		TokenType next_token_type = context_peek(context);
		if (next_token_type != TokenType_CloseParenthesis)
		{
			fprintf(stderr, "Expected ')'\n");
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

	return 0;
}

enum Associativity
{
	Associativity_Right,
	Associativity_Left,
};
typedef enum Associativity Associativity;

struct OperatorInfo
{
	ExpressionType expression_type;
	Associativity associativity;
	i32 precedence;
};
typedef struct OperatorInfo OperatorInfo;


static OperatorInfo operator_infos[] =
{
	{ ExpressionType_Less, Associativity_Left, 7 },
	{ ExpressionType_Greater, Associativity_Left, 7 },
	{ ExpressionType_LeftShift, Associativity_Left, 8 },
	{ ExpressionType_RightShift, Associativity_Left, 8 },
	{ ExpressionType_Addition, Associativity_Left, 9 },
	{ ExpressionType_Subtraction, Associativity_Left, 9 },
	{ ExpressionType_Multiplication, Associativity_Left, 10 },
	{ ExpressionType_Division, Associativity_Left, 10 },
	{ ExpressionType_BitwiseAnd, Associativity_Left, 5 },
	{ ExpressionType_BitwiseOr, Associativity_Left, 3 },
	{ ExpressionType_BitwiseXor, Associativity_Left, 4 },
	{ ExpressionType_Modulo, Associativity_Left, 10 },
	{ ExpressionType_LogicalAnd, Associativity_Left, 2 },
	{ ExpressionType_LogicalOr, Associativity_Left, 1 },
	{ ExpressionType_Equal, Associativity_Left, 6 },
	{ ExpressionType_NotEqual, Associativity_Left, 6 },
	{ ExpressionType_LessEqual, Associativity_Left, 7 },
	{ ExpressionType_GreaterEqual, Associativity_Left, 7 },
};

static ExpressionHandle parse_expression(ParseContext* context, Program* program, i32 min_precedence)
{
	// https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing

	ExpressionHandle lhs = parse_atom(context, program);

	for (;;)
	{
		TokenType next_token_type = context_peek(context);
		if (!token_is_binary_operator(next_token_type))
		{
			break;
		}

		OperatorInfo info = operator_infos[next_token_type - TokenType_FirstBinaryOperator];
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

static Statement parse_statement(ParseContext* context, Program* program)
{
	Statement statement = { .type = StatementType_Error };

	Token token = context_consume(context);
	if (token.type == TokenType_Int)
	{
		if (context_peek(context) == TokenType_Identifier)
		{
			Token identifier = context_consume(context);

			if (context_peek(context) == TokenType_Equal)
			{
				context_advance(context);

				if (context_peek(context) != TokenType_EOF)
				{
					ExpressionHandle expression = parse_expression(context, program, 1);
					if (expression)
					{
						if (context_peek(context) == TokenType_Semicolon)
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
	else if (token.type == TokenType_Return)
	{
		if (context_peek(context) != TokenType_EOF)
		{
			ExpressionHandle expression = parse_expression(context, program, 1);
			if (expression)
			{
				if (context_peek(context) == TokenType_Semicolon)
				{
					context_advance(context);

					statement.type = StatementType_Return;
					statement.ret.expression = expression;
				}
			}
		}
	}

	if (statement.type == StatementType_Error)
	{
		if (context_peek(context) != TokenType_EOF)
		{
			fprintf(stderr, "Error in line %d\n", (i32)context->tokens.items[context->current_token].line);
		}
		else
		{
			fprintf(stderr, "Unexpected end of file\n");
		}

		for (; context_peek(context) != TokenType_Semicolon && context_peek(context) != TokenType_EOF; context_advance(context)) 
		{
		}

		if (context_peek(context) != TokenType_EOF)
		{
			assert(context_peek(context) == TokenType_Semicolon);
			context_advance(context);
		}
	}

	return statement;
}

Program parse(TokenStream stream)
{
	Program program = { 0 };
	ParseContext context = { stream, 0 };

	push_expression(&program, (Expression) { .type = ExpressionType_Error }); // Dummy.

	while (context_peek(&context) != TokenType_EOF)
	{
		Statement statement = parse_statement(&context, &program);
		if (statement.type != StatementType_Error)
		{
			array_push(&program.statements, statement);
		}
	}

	return program;
}

void free_program(Program* program)
{
	array_free(&program->statements);
	array_free(&program->expressions);
}


static const char* operator_strings[] =
{
	"",
	"<",
	">",
	"<<",
	">>",
	"+",
	"-",
	"*",
	"/",
	"&",
	"|",
	"^",
	"%",
	"&&",
	"||",
	"==",
	"!=",
	"<=",
	">=",
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
	else
	{
		printf("%s (%d)\n", operator_strings[expression.type], (i32)expression_handle);

		print_expression(program, expression.binary.lhs, indent + 1, active_mask);
		*active_mask &= ~(1 << indent);
		print_expression(program, expression.binary.rhs, indent + 1, active_mask);
	}

	*active_mask &= ~(1 << indent);
}

void print_program(Program* program)
{
	for (u64 i = 0; i < program->statements.count; ++i)
	{
		Statement statement = program->statements.items[i];

		i32 active_mask = 0;

		if (statement.type == StatementType_VariableAssignment)
		{
			Token identifier = statement.variable_assignment.identifier;

			printf("* Variable assignment %.*s\n", (i32)identifier.str.len, identifier.str.str);
			print_expression(program, statement.variable_assignment.expression, 1, &active_mask);
		}
		else if (statement.type == StatementType_Return)
		{
			printf("* Return\n");
			print_expression(program, statement.ret.expression, 1, &active_mask);
		}
	}
}
