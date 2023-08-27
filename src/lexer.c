#include "lexer.h"

#include <ctype.h>


struct TokenContinuation
{
	i32 num_continuations;
	struct
	{
		char c;
		TokenType type;
	} continuations[2];
};
typedef struct TokenContinuation TokenContinuation;

static TokenType character_to_token_type[256] =
{
	['!'] = TokenType_Exclamation,
	['#'] = TokenType_Hashtag,
	['$'] = TokenType_Dollar,
	['%'] = TokenType_Percent,
	['&'] = TokenType_Ampersand,
	['('] = TokenType_OpenParenthesis,
	[')'] = TokenType_CloseParenthesis,
	['*'] = TokenType_Star,
	['+'] = TokenType_Plus,
	[','] = TokenType_Comma,
	['-'] = TokenType_Minus,
	['.'] = TokenType_Period,
	['/'] = TokenType_ForwardSlash,
	[':'] = TokenType_Colon,
	[';'] = TokenType_Semicolon,
	['<'] = TokenType_Less,
	['='] = TokenType_Equal,
	['>'] = TokenType_Greater,
	['?'] = TokenType_QuestionMark,
	['@'] = TokenType_At,
	['['] = TokenType_OpenBracket,
	[']'] = TokenType_CloseBracket,
	['^'] = TokenType_Hat,
	['{'] = TokenType_OpenBrace,
	['|'] = TokenType_Pipe,
	['}'] = TokenType_CloseBrace,
	['~'] = TokenType_Tilde,
};

static TokenContinuation token_continuations[TokenType_Count] =
{
	[TokenType_Exclamation] = { 1, { {.c = '=', .type = TokenType_ExclamationEqual } } },
	[TokenType_Ampersand] = { 2, { {.c = '=', .type = TokenType_AmpersandEqual }, {.c = '&', .type = TokenType_AmpersandAmpersand } } },
	[TokenType_Pipe] = { 2, { {.c = '=', .type = TokenType_PipeEqual }, {.c = '|', .type = TokenType_PipePipe } } },
	[TokenType_Hat] = { 1, { {.c = '=', .type = TokenType_HatEqual } } },

	[TokenType_Plus] = { 1, { {.c = '=', .type = TokenType_PlusEqual } } },
	[TokenType_Minus] = { 2, { {.c = '=', .type = TokenType_MinusEqual }, {.c = '>', .type = TokenType_Arrow } } },
	[TokenType_Star] = { 1, { {.c = '=', .type = TokenType_StarEqual } } },
	[TokenType_ForwardSlash] = { 1, { {.c = '=', .type = TokenType_ForwardSlashEqual } } },
	[TokenType_Percent] = { 1, { {.c = '=', .type = TokenType_PercentEqual } } },

	[TokenType_Less] = { 2, { {.c = '=', .type = TokenType_LessEqual }, {.c = '<', .type = TokenType_LessLess } } },
	[TokenType_Greater] = { 2, { {.c = '=', .type = TokenType_GreaterEqual }, {.c = '>', .type = TokenType_GreaterGreater } } },
	[TokenType_Equal] = { 1, { {.c = '=', .type = TokenType_EqualEqual } } },

	[TokenType_LessLess] = { 1, { {.c = '=', .type = TokenType_LessLessEqual } } },
	[TokenType_GreaterGreater] = { 1, { {.c = '=', .type = TokenType_GreaterGreaterEqual } } },
};

TokenStream tokenize(String contents)
{
	TokenStream stream = { 0 };

	u64 line = 1;

	for (u64 c_index = 0; c_index < contents.len; ++c_index)
	{
		char c = contents.str[c_index];
		char next_c = (c_index + 1 < contents.len) ? contents.str[c_index + 1] : 0;

		if (isspace(c))
		{
			if (c == '\n')
			{
				++line;
			}
			continue;
		}

		if (c == '/')
		{
			if (next_c == '/')
			{
				for (; c_index < contents.len && contents.str[c_index] != '\n'; ++c_index) {}
				++line;
				continue;
			}
		}

		Token token = { .type = character_to_token_type[c], .str = { .str = contents.str + c_index, .len = 1 }, .line = line };

		TokenContinuation continuation;

Continuations:
		continuation = token_continuations[token.type];
		for (i32 continuation_index = 0; continuation_index < continuation.num_continuations; ++continuation_index)
		{
			if (next_c == continuation.continuations[continuation_index].c)
			{
				token.type = continuation.continuations[continuation_index].type;
				++token.str.len;

				next_c = (c_index + token.str.len < contents.len) ? contents.str[c_index + token.str.len] : 0;
				goto Continuations;
			}
		}

		

		if (isalpha(c) || c == '_')
		{
			for (u64 j = c_index + 1; j < contents.len && (isalnum(contents.str[j]) || contents.str[j] == '_'); ++j)
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
			else if (string_equal(token.str, string_from_cstr("i64")))
			{
				token.type = TokenType_I64;
			}
			else
			{
				token.type = TokenType_Identifier;
			}
		}
		else if (isdigit(c))
		{
			for (u64 j = c_index + 1; j < contents.len && isdigit(contents.str[j]); ++j)
			{
				++token.str.len;
			}

			token.type = TokenType_IntLiteral;
		}

		array_push(&stream, token);

		c_index += token.str.len - 1;
	}

	Token eof_token = { .type = TokenType_EOF };
	array_push(&stream, eof_token);

	return stream;
}

void free_token_stream(TokenStream* tokens)
{
	array_free(tokens);
}



static const char* token_strings[TokenType_Count] =
{
	[TokenType_Unknown]				= "UNKNOWN",
	[TokenType_EOF]					= "",
	[TokenType_If]					= "if",
	[TokenType_While]				= "while",
	[TokenType_For]					= "for",
	[TokenType_Return]				= "return",
	[TokenType_I64]					= "i64",
	[TokenType_OpenParenthesis]		= "(",
	[TokenType_CloseParenthesis]	= ")",
	[TokenType_OpenBracket]			= "[",
	[TokenType_CloseBracket]		= "]",
	[TokenType_OpenBrace]			= "{",
	[TokenType_CloseBrace]			= "}",
	[TokenType_Semicolon]			= ";",
	[TokenType_Period]				= ".",
	[TokenType_Comma]				= ",",
	[TokenType_Colon]				= ":",
	[TokenType_Hashtag]				= "#",
	[TokenType_Dollar]				= "$",
	[TokenType_At]					= "@",
	[TokenType_QuestionMark]		= "?",
	[TokenType_Exclamation]			= "!",
	[TokenType_ExclamationEqual]	= "!=",
	[TokenType_Plus]				= "+",
	[TokenType_PlusEqual]			= "+=",
	[TokenType_Minus]				= "-",
	[TokenType_MinusEqual]			= "-=",
	[TokenType_Star]				= "*",
	[TokenType_StarEqual]			= "*=",
	[TokenType_ForwardSlash]		= "/",
	[TokenType_ForwardSlashEqual]	= "/=",
	[TokenType_Ampersand]			= "&",
	[TokenType_AmpersandEqual]		= "&=",
	[TokenType_Pipe]				= "|",
	[TokenType_PipeEqual]			= "|=",
	[TokenType_Hat]					= "^",
	[TokenType_HatEqual]			= "^=",
	[TokenType_Tilde]				= "~",
	[TokenType_Percent]				= "%",
	[TokenType_PercentEqual]		= "%=",
	[TokenType_Equal]				= "=",
	[TokenType_EqualEqual]			= "==",
	[TokenType_Less]				= "<",
	[TokenType_LessEqual]			= "<=",
	[TokenType_LessLess]			= "<<",
	[TokenType_LessLessEqual]		= "<<=",
	[TokenType_Greater]				= ">",
	[TokenType_GreaterEqual]		= ">=",
	[TokenType_GreaterGreater]		= ">>",
	[TokenType_GreaterGreaterEqual] = ">>=",
};

const char* token_type_to_string(TokenType type)
{
	return token_strings[type];
}

void print_tokens(TokenStream* tokens)
{
	for (u64 i = 0; i < tokens->count; ++i)
	{
		Token token = tokens->items[i];

		if (token.type == TokenType_Identifier)
		{
			printf("%.*s ", (i32)token.str.len, token.str.str);
		}
		else if (token.type == TokenType_IntLiteral)
		{
			printf("%.*s ", (i32)token.str.len, token.str.str);
		}
		else
		{
			printf("%s ", token_type_to_string(token.type));
		}
		
		if (token.type == TokenType_Semicolon || token.type == TokenType_EOF)
		{
			printf("\n");
		}
	}
}

