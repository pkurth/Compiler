#pragma once

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>


typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;


struct String
{
	char* str;
	u64 len;
};
typedef struct String String;

#define string_from_cstr(cstr) (String){ .str = cstr, .len = sizeof(cstr) - 1 }

static void free_string(String* s)
{
	free(s->str);
	s->str = 0;
	s->len = 0;
}

static u32 string_equal(String s1, String s2)
{
	return s1.len == s2.len && strncmp(s1.str, s2.str, s1.len) == 0;
}

static void string_push(String* string, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	string->len += vsprintf(string->str + string->len, format, args);
	va_end(args);
}







enum TokenType
{
	TokenType_Unknown,

	// Keywords.
	TokenType_If,
	TokenType_While,
	TokenType_For,
	TokenType_Return,
	TokenType_Int,

	// Single characters.
	TokenType_ExclamationPoint,
	TokenType_DoubleQuotationMark,
	TokenType_Hashtag,
	TokenType_Dollar,
	TokenType_Percent,
	TokenType_Ampersand,
	TokenType_SingleQuotationMark,
	TokenType_OpenParenthesis,
	TokenType_CloseParenthesis,
	TokenType_Asterisk,
	TokenType_Plus,
	TokenType_Comma,
	TokenType_Minus,
	TokenType_Period,
	TokenType_ForwardSlash,
	TokenType_Colon,
	TokenType_Semicolon,
	TokenType_Less,
	TokenType_Equals,
	TokenType_Greater,
	TokenType_QuestionMark,
	TokenType_At,
	TokenType_OpenBracket,
	TokenType_BackwardSlash,
	TokenType_CloseBracket,
	TokenType_Hat,
	TokenType_Underscore,
	TokenType_OpenBrace,
	TokenType_Pipe,
	TokenType_CloseBrace,
	TokenType_Tilde,


	TokenType_Identifier,
	TokenType_IntLiteral,

	TokenType_EOF,

	TokenType_Count,
};
typedef enum TokenType TokenType;


struct Token
{
	TokenType type;
	String str;
	u64 line;
};
typedef struct Token Token;


struct TokenStream
{
	Token* tokens;
	u64 count;
	u64 capacity;
};
typedef struct TokenStream TokenStream;











typedef u64 ExpressionHandle;

enum ExpressionType
{
	ExpressionType_Error,

	ExpressionType_IntLiteral,
	ExpressionType_Identifier,

	ExpressionType_Addition,
	ExpressionType_Subtraction,
};
typedef enum ExpressionType ExpressionType;

struct BinaryExpression
{
	ExpressionHandle lhs;
	ExpressionHandle rhs;
};
typedef struct BinaryExpression BinaryExpression;

struct Expression
{
	ExpressionType type;

	union
	{
		Token int_literal;
		Token identifier;
		BinaryExpression binary;
	};
};
typedef struct Expression Expression;

enum StatementType
{
	StatementType_Error,
	StatementType_VariableAssignment,
	StatementType_Return,
};
typedef enum StatementType StatementType;

struct VariableAssignmentStatement
{
	Token identifier;
	ExpressionHandle expression;
};
typedef struct VariableAssignmentStatement VariableAssignmentStatement;

struct ReturnStatement
{
	ExpressionHandle expression;
};
typedef struct ReturnStatement ReturnStatement;

struct Statement
{
	StatementType type;

	union
	{
		VariableAssignmentStatement variable_assignment;
		ReturnStatement ret;
	};
};
typedef struct Statement Statement;

struct Program
{
	Statement* statements;
	u64 statement_count;
	u64 statement_capacity;

	Expression* expressions;
	u64 expression_count;
	u64 expression_capacity;
};
typedef struct Program Program;

static Expression program_get_expression(Program* program, ExpressionHandle expression_handle)
{
	return program->expressions[expression_handle];
}

