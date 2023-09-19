#include "token.h"

#include <assert.h>

static const char* token_strings[TokenType_Count] =
{
	[TokenType_Unknown]				= "UNKNOWN",
	[TokenType_EOF]					= "EOF",
	[TokenType_Function]			= "fn",
	[TokenType_If]					= "if",
	[TokenType_Else]				= "else",
	[TokenType_While]				= "while",
	[TokenType_For]					= "for",
	[TokenType_Return]				= "return",
	[TokenType_B32]					= "b32",
	[TokenType_I32]					= "i32",
	[TokenType_U32]					= "u32",
	[TokenType_F32]					= "f32",
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
	[TokenType_ColonColon]			= "::",
	[TokenType_ColonEqual]			= ":=",
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
	[TokenType_Arrow]				= "->",
};

const char* token_type_to_string(TokenType type)
{
	return token_strings[type];
}

NumericDatatype token_type_to_numeric(TokenType type)
{
	switch (type)
	{
		case TokenType_B32: return NumericDatatype_B32;
		case TokenType_U32: return NumericDatatype_U32;
		case TokenType_I32: return NumericDatatype_I32;
		case TokenType_F32: return NumericDatatype_F32;
		default:
			assert(false);
			return NumericDatatype_Unknown;
	}
}


