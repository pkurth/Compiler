#pragma once

#include "common.h"
#include "datatype.h"


enum TokenType
{
	TokenType_Unknown,
	TokenType_EOF,


	// Keywords.
	TokenType_Function,
	TokenType_If,
	TokenType_Else,
	TokenType_While,
	TokenType_For,
	TokenType_Return,
	TokenType_B32,
	TokenType_U32,
	TokenType_I32,
	TokenType_F32,


	// Parentheses, brackets, braces.
	TokenType_OpenParenthesis,
	TokenType_CloseParenthesis,
	TokenType_OpenBracket,
	TokenType_CloseBracket,
	TokenType_OpenBrace,
	TokenType_CloseBrace,


	// Random symbols.
	TokenType_Semicolon,
	TokenType_Period,
	TokenType_Comma,
	TokenType_Colon,
	TokenType_ColonColon,
	TokenType_ColonEqual,
	TokenType_Hashtag,
	TokenType_Dollar,
	TokenType_At,
	TokenType_QuestionMark,
	TokenType_Tilde,
	TokenType_Arrow,
	TokenType_Exclamation,


	// Assignment operators.
	TokenType_Equal,
	TokenType_LessLessEqual,
	TokenType_GreaterGreaterEqual,
	TokenType_PlusEqual,
	TokenType_MinusEqual,
	TokenType_StarEqual,
	TokenType_ForwardSlashEqual,
	TokenType_PercentEqual,
	TokenType_AmpersandEqual,
	TokenType_PipeEqual,
	TokenType_HatEqual,


	// Binary operators.
	TokenType_Less,
	TokenType_Greater,
	TokenType_LessLess,
	TokenType_GreaterGreater,
	TokenType_LessEqual,
	TokenType_GreaterEqual,
	TokenType_Plus,
	TokenType_Minus,
	TokenType_Star,
	TokenType_ForwardSlash,
	TokenType_Ampersand,
	TokenType_Pipe,
	TokenType_Hat,
	TokenType_Percent,
	TokenType_AmpersandAmpersand,
	TokenType_PipePipe,
	TokenType_EqualEqual,
	TokenType_ExclamationEqual,


	// Other.
	TokenType_Identifier,
	TokenType_NumericLiteral,
	TokenType_StringLiteral,


	TokenType_Count,


	TokenType_FirstKeyword = TokenType_Function,
	TokenType_LastKeyword = TokenType_F32,

	TokenType_FirstDatatype = TokenType_B32,
	TokenType_LastDatatype = TokenType_F32,

	TokenType_FirstAssignmentOperator = TokenType_Equal,
	TokenType_LastAssignmentOperator = TokenType_HatEqual,

	TokenType_FirstBinaryOperator = TokenType_Less,
	TokenType_LastBinaryOperator = TokenType_ExclamationEqual,
};
typedef enum TokenType TokenType;

static b32 token_is_keyword(TokenType type)
{
	return (type >= TokenType_FirstKeyword) && (type <= TokenType_LastKeyword);
}

static b32 token_is_datatype(TokenType type)
{
	return (type >= TokenType_FirstDatatype) && (type <= TokenType_LastDatatype);
}

static b32 token_is_assignment_operator(TokenType type)
{
	return (type >= TokenType_FirstAssignmentOperator) && (type <= TokenType_LastAssignmentOperator);
}

static b32 token_is_binary_operator(TokenType type)
{
	return (type >= TokenType_FirstBinaryOperator) && (type <= TokenType_LastBinaryOperator);
}

static b32 token_is_unary_operator(TokenType type)
{
	return (type == TokenType_Minus) || (type == TokenType_Tilde) || (type == TokenType_Exclamation);
}

const char* token_type_to_string(TokenType type);
NumericDatatype token_type_to_numeric(TokenType type);

struct Token
{
	TokenType type;
	SourceLocation source_location;
	i32 data_index;
};
typedef struct Token Token;

struct TokenStream
{
	DynamicArray(Token) tokens;
	DynamicArray(String) strings;
	DynamicArray(NumericLiteral) numeric_literals;
};
typedef struct TokenStream TokenStream;

TokenStream tokenize(String source_code);
void free_token_stream(TokenStream* tokens);
void print_tokens(TokenStream* tokens);


