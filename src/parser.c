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

static ExpressionHandle parse_expression(ParseContext* context, Program* program, int min_precedence);

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

static ExpressionHandle parse_expression(ParseContext* context, Program* program, int min_precedence)
{
	enum Associativity
	{
		Associativity_Left,
		Associativity_Right,
	};
	typedef enum Associativity Associativity;

	int operator_precedences[] =
	{
		1, // ExpressionType_Addition
		1, // ExpressionType_Subtraction
	};

	ExpressionHandle lhs = parse_atom(context, program);

	for (;;)
	{
		TokenType next_token_type = context_peek(context);

		int is_binary_operator = next_token_type == TokenType_Plus || next_token_type == TokenType_Minus;
		if (!is_binary_operator)
		{
			break;
		}

		ExpressionType type = (next_token_type == TokenType_Plus) ? ExpressionType_Addition : ExpressionType_Subtraction;
		int precedence = operator_precedences[type - ExpressionType_Addition];
		if (precedence < min_precedence)
		{
			break;
		}

		Associativity associativity = Associativity_Left;
		int next_min_precedence = (associativity == Associativity_Left) ? (precedence + 1) : precedence;
		
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

static void print_expression(Program* program, Expression expression, int indent)
{
	for (int i = 0; i < indent; ++i)
	{
		printf((i == indent - 1) ? "|-> " : "|   ");
	}

	if (expression.type == ExpressionType_IntLiteral)
	{
		printf("Integer literal: %.*s\n", (int)expression.int_literal.str.len, expression.int_literal.str.str);
	}
	else if (expression.type == ExpressionType_Identifier)
	{
		printf("Identifier: %.*s\n", (int)expression.identifier.str.len, expression.identifier.str.str);
	}
	else if (expression.type == ExpressionType_Addition)
	{
		printf("+\n");

		Expression lhs = program_get_expression(program, expression.binary.lhs);
		Expression rhs = program_get_expression(program, expression.binary.rhs);
		print_expression(program, lhs, indent + 1);
		print_expression(program, rhs, indent + 1);
	}
	else if (expression.type == ExpressionType_Subtraction)
	{
		printf("-\n");

		Expression lhs = program_get_expression(program, expression.binary.lhs);
		Expression rhs = program_get_expression(program, expression.binary.rhs);
		print_expression(program, lhs, indent + 1);
		print_expression(program, rhs, indent + 1);
	}
}

void print_program(Program program)
{
	for (size_t i = 0; i < program.statement_count; ++i)
	{
		Statement statement = program.statements[i];

		if (statement.type == StatementType_VariableAssignment)
		{
			Token identifier = statement.variable_assignment.identifier;
			Expression expression = program_get_expression(&program, statement.variable_assignment.expression);

			printf("* Variable assignment %.*s\n", (int)identifier.str.len, identifier.str.str);
			print_expression(&program, expression, 1);
		}
		else if (statement.type == StatementType_Return)
		{
			Expression expression = program_get_expression(&program, statement.ret.expression);

			printf("* Return\n");
			print_expression(&program, expression, 1);
		}
	}
}
