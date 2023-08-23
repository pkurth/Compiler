#include "parser.h"

#include <assert.h>

static void push_statement(Program* stream, Statement statement)
{
	if (stream->count == stream->capacity)
	{
		stream->capacity = max(stream->capacity * 2, 16);
		stream->statements = realloc(stream->statements, sizeof(Statement) * stream->capacity);
	}
	stream->statements[stream->count++] = statement;
}

struct ParseContext
{
	TokenStream tokens;
	size_t current_token;
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

static Expression parse_expression(ParseContext* context)
{
	Expression expression = { .type = ExpressionType_Error };

	Token token = context_consume(context);
	if (token.type == TokenType_IntLiteral)
	{
		expression.type = ExpressionType_IntLiteral;
		expression.int_literal = token;
	}
	else if (token.type == TokenType_Identifier)
	{
		expression.type = ExpressionType_Identifier;
		expression.identifier = token;
	}

	return expression;
}

static Statement parse_statement(ParseContext* context)
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
					Expression expression = parse_expression(context);
					if (expression.type != ExpressionType_Error)
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
			Expression expression = parse_expression(context);
			if (expression.type != ExpressionType_Error)
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
			fprintf(stderr, "Error in line %d\n", (int)context->tokens.tokens[context->current_token].line);
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

	while (context_peek(&context) != TokenType_EOF)
	{
		Statement statement = parse_statement(&context);
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
	program->capacity = 0;
	program->count = 0;
}

static void print_expression(Expression expression)
{
	if (expression.type == ExpressionType_Identifier)
	{
		printf("%.*s\n", (int)expression.identifier.str.len, expression.identifier.str.str);
	}
	else if (expression.type == ExpressionType_IntLiteral)
	{
		printf("%.*s\n", (int)expression.int_literal.str.len, expression.int_literal.str.str);
	}
}

void print_program(Program program)
{
	for (size_t i = 0; i < program.count; ++i)
	{
		Statement statement = program.statements[i];

		if (statement.type == StatementType_VariableAssignment)
		{
			Token identifier = statement.variable_assignment.identifier;
			Expression expression = statement.variable_assignment.expression;

			printf("- Variable assignment: %.*s = ", (int)identifier.str.len, identifier.str.str);
			print_expression(expression);
		}
		else if (statement.type == StatementType_Return)
		{
			Expression expression = statement.ret.expression;

			printf("- Return ");
			print_expression(expression);
		}
	}
}
