#include "lexer.h"

#include <ctype.h>


static void push_token(TokenStream* stream, Token token)
{
	if (stream->count == stream->capacity)
	{
		stream->capacity = max(stream->capacity * 2, 16);
		stream->tokens = realloc(stream->tokens, sizeof(Token) * stream->capacity);
	}
	stream->tokens[stream->count++] = token;
}

TokenStream tokenize(String contents)
{
	TokenType character_to_token_type[256] = { 0 };
	character_to_token_type['!'] = TokenType_ExclamationPoint;
	character_to_token_type['"'] = TokenType_DoubleQuotationMark;
	character_to_token_type['#'] = TokenType_Hashtag;
	character_to_token_type['$'] = TokenType_Dollar;
	character_to_token_type['%'] = TokenType_Percent;
	character_to_token_type['&'] = TokenType_Ampersand;
	character_to_token_type['\''] = TokenType_SingleQuotationMark;
	character_to_token_type['('] = TokenType_OpenParenthesis;
	character_to_token_type[')'] = TokenType_CloseParenthesis;
	character_to_token_type['*'] = TokenType_Asterisk;
	character_to_token_type['+'] = TokenType_Plus;
	character_to_token_type[','] = TokenType_Comma;
	character_to_token_type['-'] = TokenType_Minus;
	character_to_token_type['.'] = TokenType_Period;
	character_to_token_type['/'] = TokenType_ForwardSlash;
	character_to_token_type[':'] = TokenType_Colon;
	character_to_token_type[';'] = TokenType_Semicolon;
	character_to_token_type['<'] = TokenType_Less;
	character_to_token_type['='] = TokenType_Equals;
	character_to_token_type['>'] = TokenType_Greater;
	character_to_token_type['?'] = TokenType_QuestionMark;
	character_to_token_type['@'] = TokenType_At;
	character_to_token_type['['] = TokenType_OpenBracket;
	character_to_token_type['\\'] = TokenType_BackwardSlash;
	character_to_token_type[']'] = TokenType_CloseBracket;
	character_to_token_type['^'] = TokenType_Hat;
	character_to_token_type['_'] = TokenType_Underscore;
	character_to_token_type['{'] = TokenType_OpenBrace;
	character_to_token_type['|'] = TokenType_Pipe;
	character_to_token_type['}'] = TokenType_CloseBrace;
	character_to_token_type['~'] = TokenType_Tilde;



	TokenStream stream = { 0 };

	u64 line = 1;

	for (u64 i = 0; i < contents.len; ++i)
	{
		char c = contents.str[i];
		if (c == '\n')
		{
			++line;
		}
		if (isspace(c))
		{
			continue;
		}

		if (c == '/' && i < contents.len - 1)
		{
			char next_c = contents.str[i + 1];
			if (next_c == '/')
			{
				for (; i < contents.len && contents.str[i] != '\n'; ++i) {}
				--i;
				continue;
			}
		}

		Token token = { .type = character_to_token_type[c], .str = {.str = contents.str + i, .len = 1 }, .line = line };

		if (isalpha(c) || c == '_')
		{
			for (u64 j = i + 1; j < contents.len && (isalnum(contents.str[j]) || contents.str[j] == '_'); ++j)
			{
				++token.str.len;
			}

			if (string_equal(token.str, string_from_cstr("if")))
			{
				token.type = TokenType_If;
			}
			else if (string_equal(token.str, string_from_cstr("while")))
			{
				token.type = TokenType_While;
			}
			else if (string_equal(token.str, string_from_cstr("for")))
			{
				token.type = TokenType_For;
			}
			else if (string_equal(token.str, string_from_cstr("return")))
			{
				token.type = TokenType_Return;
			}
			else if (string_equal(token.str, string_from_cstr("int")))
			{
				token.type = TokenType_Int;
			}
			else
			{
				token.type = TokenType_Identifier;
			}
		}
		else if (isdigit(c))
		{
			for (u64 j = i + 1; j < contents.len && isdigit(contents.str[j]); ++j)
			{
				++token.str.len;
			}

			token.type = TokenType_IntLiteral;
		}

		push_token(&stream, token);

		i += token.str.len - 1;
	}

	Token eof_token = { .type = TokenType_EOF };
	push_token(&stream, eof_token);

	return stream;
}

void free_token_stream(TokenStream* tokens)
{
	free(tokens->tokens);
	tokens->tokens = 0;
	tokens->capacity = 0;
	tokens->count = 0;
}

void print_tokens(TokenStream* tokens)
{
	const char token_type_to_character[] =
	{
		'!', '"' ,'#', '$','%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\', ']', '^', '_', '{', '|', '}', '~',
	};

	const char* token_type_to_keyword[] =
	{
		"if", "while", "for", "return", "int",
	};

	for (u64 i = 0; i < tokens->count; ++i)
	{
		Token token = tokens->tokens[i];

		if (token.type >= TokenType_ExclamationPoint && token.type <= TokenType_Tilde)
		{
			printf("%c\n", token_type_to_character[token.type - TokenType_ExclamationPoint]);
		}
		else if (token.type >= TokenType_If && token.type <= TokenType_Int)
		{
			printf("%s\n", token_type_to_keyword[token.type - TokenType_If]);
		}
		else if (token.type == TokenType_Identifier)
		{
			printf("Identifier: %.*s\n", (i32)token.str.len, token.str.str);
		}
		else if (token.type == TokenType_IntLiteral)
		{
			printf("Integer literal: %.*s\n", (i32)token.str.len, token.str.str);
		}
		else
		{
			printf("UNKNOWN TOKEN TYPE\n");
		}
	}
}

