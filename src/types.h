#pragma once

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>


struct String
{
	char* str;
	size_t len;
};
typedef struct String String;

#define string_from_cstr(cstr) (String){ .str = cstr, .len = sizeof(cstr) - 1 }

static void free_string(String* s)
{
	free(s->str);
	s->str = 0;
	s->len = 0;
}

//static int string_cstr_equal(String s, const char* v)
//{
//	return strncmp(s.str, v, s.len) == 0;
//}

static int string_equal(String s1, String s2)
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
	size_t line;
};
typedef struct Token Token;


struct TokenStream
{
	Token* tokens;
	size_t count;
	size_t capacity;
};
typedef struct TokenStream TokenStream;














enum ExpressionType
{
	ExpressionType_Error,
	ExpressionType_IntLiteral,
	ExpressionType_Identifier,
};
typedef enum ExpressionType ExpressionType;

struct Expression
{
	ExpressionType type;

	union
	{
		Token int_literal;
		Token identifier;
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
	Expression expression;
};
typedef struct VariableAssignmentStatement VariableAssignmentStatement;

struct ReturnStatement
{
	Expression expression;
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
	size_t count;
	size_t capacity;
};
typedef struct Program Program;

