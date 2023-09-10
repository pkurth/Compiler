#include "types.h"

#include <assert.h>


static const char* data_type_strings[] =
{
	[PrimitiveDatatype_B32] = "b32",
	[PrimitiveDatatype_I32] = "i32",
	[PrimitiveDatatype_U32] = "u32",
	[PrimitiveDatatype_F32] = "f32",
};

const char* serialize_primitive_data(PrimitiveData data)
{
	static char result[128];

	switch (data.type)
	{
		case PrimitiveDatatype_B32:
			snprintf(result, sizeof(result), data.data_b32 ? "1" : "0");
			break;
		case PrimitiveDatatype_I32:
			snprintf(result, sizeof(result), "%" PRIi32, data.data_i32);
			break;
		case PrimitiveDatatype_U32:
			snprintf(result, sizeof(result), "%" PRIu32, data.data_u32);
			break;
		case PrimitiveDatatype_F32:
			snprintf(result, sizeof(result), "%f", data.data_f32);
			break;
	}

	return result;
}

const char* data_type_to_string(PrimitiveDatatype type)
{
	return data_type_strings[type];
}


static const char* token_strings[TokenType_Count] =
{
	[TokenType_Unknown]				= "UNKNOWN",
	[TokenType_EOF]					= "",
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

PrimitiveDatatype token_to_datatype(TokenType type)
{
	switch (type)
	{
		case TokenType_B32: return PrimitiveDatatype_B32;
		case TokenType_U32: return PrimitiveDatatype_U32;
		case TokenType_I32: return PrimitiveDatatype_I32;
		case TokenType_F32: return PrimitiveDatatype_F32;
		default:
			assert(false);
			return PrimitiveDatatype_Unknown;
	}
}


