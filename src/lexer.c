#include "lexer.h"

#include <ctype.h>
#include <assert.h>


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


struct TokenKeywordMapping
{
	String str;
	TokenType type;
};
typedef struct TokenKeywordMapping TokenKeywordMapping;


#define string(cstr) { .str = cstr, .len = sizeof(cstr) - 1 }
static const TokenKeywordMapping token_keyword_map[] =
{
	{ .str = string("if"),		.type = TokenType_If },
	{ .str = string("else"),	.type = TokenType_Else },
	{ .str = string("while"),	.type = TokenType_While },
	{ .str = string("for"),		.type = TokenType_For },
	{ .str = string("return"),	.type = TokenType_Return },
	{.str = string("b32"),		.type = TokenType_B32 },
	{ .str = string("i32"),		.type = TokenType_I32 },
	{ .str = string("u32"),		.type = TokenType_U32 },
	{.str = string("f32"),		.type = TokenType_F32 },
	{ .str = string("String"),	.type = TokenType_String },
};
#undef string

TokenStream tokenize(String source_code)
{
	TokenStream stream = { 0 };

	i32 line = 1;

	for (i64 c_index = 0; c_index < source_code.len; ++c_index)
	{
		char c = source_code.str[c_index];
		char next_c = (c_index + 1 < source_code.len) ? source_code.str[c_index + 1] : 0;

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
				for (; c_index < source_code.len && source_code.str[c_index] != '\n'; ++c_index) {}
				++line;
				continue;
			}
		}

		String token_string = { .str = source_code.str + c_index, .len = 1 };
		Token token = 
		{ 
			.type = character_to_token_type[c], 
			.source_location = {.line = line, .global_character_index = (i32)c_index }
		};

		TokenContinuation continuation;

Continuations:
		continuation = token_continuations[token.type];
		for (i32 continuation_index = 0; continuation_index < continuation.num_continuations; ++continuation_index)
		{
			if (next_c == continuation.continuations[continuation_index].c)
			{
				token.type = continuation.continuations[continuation_index].type;
				++token_string.len;

				next_c = (c_index + token_string.len < source_code.len) ? source_code.str[c_index + token_string.len] : 0;
				goto Continuations;
			}
		}

		

		if (isalpha(c) || c == '_')
		{
			for (i64 i = c_index + 1; i < source_code.len && (isalnum(source_code.str[i]) || source_code.str[i] == '_'); ++i)
			{
				++token_string.len;
			}

			token.type = TokenType_Unknown;

			for (i64 keyword_index = 0; keyword_index < arraysize(token_keyword_map); ++keyword_index)
			{
				if (string_equal(token_string, token_keyword_map[keyword_index].str))
				{
					token.type = token_keyword_map[keyword_index].type;
					break;
				}
			}

			if (string_equal(token_string, string_from_cstr("true")))
			{
				token.type = TokenType_NumericLiteral;
				token.data_index = (i32)stream.numeric_literals.count;
				PrimitiveData literal = { .type = PrimitiveDatatype_B32, .data_b32 = true };
				array_push(&stream.numeric_literals, literal);
			}
			else if (string_equal(token_string, string_from_cstr("false")))
			{
				token.type = TokenType_NumericLiteral;
				token.data_index = (i32)stream.numeric_literals.count;
				PrimitiveData literal = { .type = PrimitiveDatatype_B32, .data_b32 = false };
				array_push(&stream.numeric_literals, literal);
			}
			
			if (token.type == TokenType_Unknown)
			{
				token.type = TokenType_Identifier;
				token.data_index = (i32)stream.strings.count;
				array_push(&stream.strings, token_string);
			}
		}
		else if (isdigit(c))
		{
			token.type = TokenType_NumericLiteral;

			PrimitiveData literal = { .type = PrimitiveDatatype_I32 };
			b32 e_found = false;

			for (i64 i = c_index + 1; i < source_code.len; ++i)
			{
				char c = source_code.str[i];
				if (!isdigit(c))
				{
					if (literal.type == PrimitiveDatatype_I32)
					{
						if (c == '.') { literal.type = PrimitiveDatatype_F32; }
						if (c == 'e') { literal.type = PrimitiveDatatype_F32; e_found = true; }
						else { break; }
					}
					else if (literal.type == PrimitiveDatatype_F32)
					{
						if (!e_found && c == 'e') { e_found = true; }
						else { break; }
					}
					else
					{
						break;
					}
				}

				++token_string.len;
			}

			char buf[32];
			memcpy(buf, token_string.str, token_string.len);
			buf[token_string.len] = 0;

			if (literal.type == PrimitiveDatatype_I32)
			{
				literal.data_i32 = atoi(buf);
			}
			else if (literal.type == PrimitiveDatatype_F32)
			{
				literal.data_f32 = (f32)atof(buf);
			}
			else
			{
				assert(false);
			}

			token.data_index = (i32)stream.numeric_literals.count;
			array_push(&stream.numeric_literals, literal);
		}
		else if (c == '"' && next_c)
		{
			assert(token_string.len == 1);

			for (i64 i = c_index + 1; i < source_code.len && source_code.str[i] != '"'; ++i)
			{
				++token_string.len;
			}

			token.data_index = (i32)stream.strings.count;
			String literal = { .str = token_string.str + 1, .len = token_string.len - 1 };
			array_push(&stream.strings, literal);
		}

		array_push(&stream.tokens, token);

		c_index += token_string.len - 1;
	}

	Token eof_token = { .type = TokenType_EOF, .source_location = { .global_character_index = source_code.len, .line = line } };
	array_push(&stream.tokens, eof_token);

	return stream;
}

void free_token_stream(TokenStream* tokens)
{
	array_free(&tokens->tokens);
	array_free(&tokens->strings);
	array_free(&tokens->numeric_literals);
}

void print_tokens(TokenStream* tokens)
{
	for (i64 i = 0; i < tokens->tokens.count; ++i)
	{
		Token token = tokens->tokens.items[i];

		if (token.type == TokenType_Identifier)
		{
			String identifer = tokens->strings.items[token.data_index];
			printf("%.*s ", (i32)identifer.len, identifer.str);
		}
		else if (token.type == TokenType_NumericLiteral)
		{
			PrimitiveData literal = tokens->numeric_literals.items[token.data_index];
			printf("%s ", serialize_primitive_data(literal));
		}
		else
		{
			printf("%s ", token_type_to_string(token.type));
		}
		
		if (token.type == TokenType_EOF || token.type == TokenType_Semicolon || token.type == TokenType_OpenBrace || token.type == TokenType_CloseBrace)
		{
			printf("\n");
		}
	}
}

