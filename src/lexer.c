#include "lexer.h"

#include <ctype.h>


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

TokenStream tokenize(String contents)
{
	TokenStream stream = { 0 };

	u64 line = 1;

	for (u64 i = 0; i < contents.len; ++i)
	{
		char c = contents.str[i];
		char next_c = (i + 1 < contents.len) ? contents.str[i + 1] : 0;

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
				for (; i < contents.len && contents.str[i] != '\n'; ++i) {}
				++line;
				continue;
			}
		}

		Token token = { .type = character_to_token_type[c], .str = { .str = contents.str + i, .len = 1 }, .line = line };

		b32 is_angle = (c == '<') || (c == '>');
		b32 is_and_or_or = (c == '&') || (c == '|');
		b32 can_repeat = is_angle || is_and_or_or;
		b32 repeats = can_repeat && (c == next_c);
		token.type += repeats * (is_angle ? (TokenType_LessLess - TokenType_Less) : (TokenType_AmpersandAmpersand - TokenType_Ampersand));
		token.str.len += repeats;

		b32 can_have_equal_followup = (token.type >= TokenType_Equal && token.type <= TokenType_Percent);
		char followup = (i + 1 + repeats < contents.len) ? contents.str[i + 1 + repeats] : 0;
		u32 equal_followup_offset = (TokenType_EqualEqual - TokenType_Equal);
		
		b32 is_operator_equal = can_have_equal_followup && (followup == '=');
		token.type += is_operator_equal * equal_followup_offset;
		token.str.len += is_operator_equal;

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

		array_push(&stream, token);

		i += token.str.len - 1;
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
	[TokenType_Int]					= "int",
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

