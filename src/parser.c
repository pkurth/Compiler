#include "parser.h"

#include <assert.h>


static void push_statement(Program* stream, Statement statement)
{
	if (stream->statement_count == stream->statement_capacity)
	{
		stream->statement_capacity = max(stream->statement_capacity * 2, 16);
		stream->statements = realloc(stream->statements, sizeof(Statement) * stream->statement_capacity);
	}
	stream->statements[stream->statement_count++] = statement;
}

static ExpressionHandle push_expression(Program* stream, Expression expression)
{
	if (stream->expression_count == stream->expression_capacity)
	{
		stream->expression_capacity = max(stream->expression_capacity * 2, 16);
		stream->expressions = realloc(stream->expressions, sizeof(expression) * stream->expression_capacity);
	}
	ExpressionHandle result = stream->expression_count++;
	stream->expressions[result] = expression;
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
	return context->tokens.tokens[context->current_token].type;
}

static void context_advance(ParseContext* context)
{
	++context->current_token;
}

static Token context_consume(ParseContext* context)
{
	return context->tokens.tokens[context->current_token++];
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

static ExpressionHandle parse_expression(ParseContext* context, Program* program, i32 min_precedence)
{
	enum Associativity
	{
		Associativity_Left,
		Associativity_Right,
	};
	typedef enum Associativity Associativity;

	i32 operator_precedences[] =
	{
		1, // ExpressionType_Addition
		1, // ExpressionType_Subtraction
	};


	// https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing

	ExpressionHandle lhs = parse_atom(context, program);

	for (;;)
	{
		TokenType next_token_type = context_peek(context);

		i32 is_binary_operator = next_token_type == TokenType_Plus || next_token_type == TokenType_Minus;
		if (!is_binary_operator)
		{
			break;
		}

		ExpressionType type = (next_token_type == TokenType_Plus) ? ExpressionType_Addition : ExpressionType_Subtraction;
		i32 precedence = operator_precedences[type - ExpressionType_Addition];
		if (precedence < min_precedence)
		{
			break;
		}

		Associativity associativity = Associativity_Left;
		i32 next_min_precedence = (associativity == Associativity_Left) ? (precedence + 1) : precedence;
		
		context_advance(context);

		ExpressionHandle rhs = parse_expression(context, program, next_min_precedence);

		Expression operation = { .type = type, { .binary = { .lhs = lhs, .rhs = rhs } } };
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

			if (context_peek(context) == TokenType_Equals)
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
			fprintf(stderr, "Error in line %d\n", (i32)context->tokens.tokens[context->current_token].line);
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
			push_statement(&program, statement);
		}
	}

	return program;
}

void free_program(Program* program)
{
	free(program->statements);
	program->statements = 0;
	program->statement_capacity = 0;
	program->statement_count = 0;

	free(program->expressions);
	program->expressions = 0;
	program->expression_capacity = 0;
	program->expression_count = 0;
}

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

	*active_mask |= (1 << indent);

	if (expression.type == ExpressionType_IntLiteral)
	{
		printf("%.*s (%d)\n", (i32)expression.int_literal.str.len, expression.int_literal.str.str, (i32)expression_handle);
	}
	else if (expression.type == ExpressionType_Identifier)
	{
		printf("%.*s (%d)\n", (i32)expression.identifier.str.len, expression.identifier.str.str, (i32)expression_handle);
	}
	else if (expression.type == ExpressionType_Addition || expression.type == ExpressionType_Subtraction)
	{
		printf("%s (%d)\n", expression.type == ExpressionType_Addition ? "+" : "-", (i32)expression_handle);

		print_expression(program, expression.binary.lhs, indent + 1, active_mask);
		*active_mask &= ~(1 << indent);
		print_expression(program, expression.binary.rhs, indent + 1, active_mask);
	}

	*active_mask &= ~(1 << indent);
}

void print_program(Program* program)
{
	for (u64 i = 0; i < program->statement_count; ++i)
	{
		Statement statement = program->statements[i];

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
