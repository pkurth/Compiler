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

		if (c == '\n')
		{
			++line;
		}
		if (isspace(c))
		{
			continue;
		}

		if (c == '/')
		{
			if (next_c == '/')
			{
				for (; i < contents.len && contents.str[i] != '\n'; ++i) {}
				--i;
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

void print_tokens(TokenStream* tokens)
{
	const char* token_strings[TokenType_Count] = { 0 };
	token_strings[TokenType_Unknown] = "UNKNOWN";
	token_strings[TokenType_EOF] = "";
	token_strings[TokenType_If] = "if";
	token_strings[TokenType_While] = "while";
	token_strings[TokenType_For] = "for";
	token_strings[TokenType_Return] = "return";
	token_strings[TokenType_Int] = "int";
	token_strings[TokenType_OpenParenthesis] = "(";
	token_strings[TokenType_CloseParenthesis] = ")";
	token_strings[TokenType_OpenBracket] = "[";
	token_strings[TokenType_CloseBracket] = "]";
	token_strings[TokenType_OpenBrace] = "{";
	token_strings[TokenType_CloseBrace] = "}";
	token_strings[TokenType_Semicolon] = ";";
	token_strings[TokenType_Period] = ".";
	token_strings[TokenType_Comma] = ",";
	token_strings[TokenType_Colon] = ":";
	token_strings[TokenType_Hashtag] = "#";
	token_strings[TokenType_Dollar] = "$";
	token_strings[TokenType_At] = "@";
	token_strings[TokenType_QuestionMark] = "?";
	token_strings[TokenType_Exclamation] = "!";
	token_strings[TokenType_ExclamationEqual] = "!=";
	token_strings[TokenType_Plus] = "+";
	token_strings[TokenType_PlusEqual] = "+=";
	token_strings[TokenType_Minus] = "-";
	token_strings[TokenType_MinusEqual] = "-=";
	token_strings[TokenType_Star] = "*";
	token_strings[TokenType_StarEqual] = "*=";
	token_strings[TokenType_ForwardSlash] = "/";
	token_strings[TokenType_ForwardSlashEqual] = "/=";
	token_strings[TokenType_Ampersand] = "&";
	token_strings[TokenType_AmpersandEqual] = "&=";
	token_strings[TokenType_Pipe] = "|";
	token_strings[TokenType_PipeEqual] = "|=";
	token_strings[TokenType_Hat] = "^";
	token_strings[TokenType_HatEqual] = "^=";
	token_strings[TokenType_Tilde] = "~";
	token_strings[TokenType_Percent] = "%";
	token_strings[TokenType_PercentEqual] = "%=";
	token_strings[TokenType_Equal] = "=";
	token_strings[TokenType_EqualEqual] = "==";
	token_strings[TokenType_Less] = "<";
	token_strings[TokenType_LessEqual] = "<=";
	token_strings[TokenType_LessLess] = "<<";
	token_strings[TokenType_LessLessEqual] = "<<=";
	token_strings[TokenType_Greater] = ">";
	token_strings[TokenType_GreaterEqual] = ">=";
	token_strings[TokenType_GreaterGreater] = ">>";
	token_strings[TokenType_GreaterGreaterEqual] = ">>=";

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
			printf("%s ", token_strings[token.type]);
		}
		
		if (token.type == TokenType_Semicolon || token.type == TokenType_EOF)
		{
			printf("\n");
		}
	}
}

