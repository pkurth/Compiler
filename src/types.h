#pragma once

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
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

typedef u32 b32;

#define arraysize(arr) (i64)(sizeof(arr) / sizeof((arr)[0]))


struct String
{
	char* str;
	i64 len;
};
typedef struct String String;

#define string_from_cstr(cstr) (String){ .str = cstr, .len = sizeof(cstr) - 1 }

static void string_free(String* s)
{
	free(s->str);
	s->str = 0;
	s->len = 0;
}

static b32 string_equal(String s1, String s2)
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


#define DynamicArray(ItemType)														\
	struct																			\
	{																				\
		ItemType* items;															\
		i64 count;																	\
		i64 capacity;																\
	}

#define array_push(a, item)															\
	do																				\
	{																				\
		if ((a)->count == (a)->capacity)											\
		{																			\
			(a)->capacity = max((a)->capacity * 2, 16);								\
			(a)->items = realloc((a)->items, sizeof(*(a)->items) * (a)->capacity);	\
		}																			\
		(a)->items[(a)->count++] = item;											\
	} while (0)

#define array_free(a)																\
	do																				\
	{																				\
		free((a)->items);															\
		(a)->items = 0;																\
		(a)->capacity = 0;															\
		(a)->count = 0;																\
	} while (0)


#if 0
#define HashMap(KeyType, ValueType, capacity)										\
	struct																			\
	{																				\
		i64 (*hash_function)(KeyType);												\
		struct KeyIndexPair															\
		{																			\
			KeyType key;															\
			i64 index;																\
		} keys[capacity];															\
		ValueType values[capacity];													\
		i64 count;																	\
	}

#define hashmap_insert(map, key, value)												\
	do																				\
	{																				\
		i64 hash = (map)->hash_function(key);										\
		i64 capacity = arraysize((map)->keys);										\
		i64 key_index = hash % capacity;											\
		i64 value_index = (map)->count++;											\
		(map)->values[value_index] = value;											\
		(map)->keys[key_index].key = key;											\
		(map)->keys[key_index].index = value_index;									\
	} while (0)

//#define hashmap_lookup(map, key) \
//	(map)->values[(map)->keys[(map)->hash_function(key) % arraysize((map)->keys)].index]

static i64 hash_string(String str)
{
	return str.len * str.str[0];
}

static void test()
{
	typedef HashMap(String, int, 512) StringToIntMap;

	StringToIntMap map = { &hash_string };

	String key = string_from_cstr("Hallo Welt");
	hashmap_insert(&map, key, 12);

	int key = hashmap_lookup(&map, key);

	int a = 0;

	int index = (int i = 3, i);
}
#endif

enum PrimitiveDatatype
{
	PrimitiveDatatype_Unknown,

	PrimitiveDatatype_B32,
	PrimitiveDatatype_U32,
	PrimitiveDatatype_I32,
	PrimitiveDatatype_F32,

	PrimitiveDatatype_Count,
};
typedef enum PrimitiveDatatype PrimitiveDatatype;

static b32 data_type_is_integral(PrimitiveDatatype type)
{
	return type == PrimitiveDatatype_I32 || type == PrimitiveDatatype_U32;
}

static b32 data_type_converts_to_b32(PrimitiveDatatype type)
{
	return type == PrimitiveDatatype_B32 || data_type_is_integral(type);
}

const char* data_type_to_string(PrimitiveDatatype type);

struct PrimitiveData
{
	PrimitiveDatatype type;

	union
	{
		b32 data_b32;
		u32 data_u32;
		i32 data_i32;
		f32 data_f32;
	};
};
typedef struct PrimitiveData PrimitiveData;

const char* serialize_primitive_data(PrimitiveData data);



// https://github.com/stuartcarnie/clang/blob/master/include/clang/Basic/TokenKinds.def

enum TokenType
{
	TokenType_Unknown,
	TokenType_EOF,


	// Keywords.
	TokenType_If,
	TokenType_Else,
	TokenType_While,
	TokenType_For,
	TokenType_Return,
	TokenType_B32,
	TokenType_U32,
	TokenType_I32,
	TokenType_F32,
	TokenType_String,


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


	TokenType_FirstKeyword = TokenType_If,
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
PrimitiveDatatype token_to_datatype(TokenType type);

struct SourceLocation
{
	i32 line;
	i32 global_character_index;
};
typedef struct SourceLocation SourceLocation;

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
	DynamicArray(PrimitiveData) numeric_literals;
};
typedef struct TokenStream TokenStream;










typedef i32 ExpressionHandle;

enum ExpressionType
{
	ExpressionType_Error,

	// Binary expressions.
	ExpressionType_LogicalOr,
	ExpressionType_LogicalAnd,
	ExpressionType_BitwiseOr,
	ExpressionType_BitwiseXor,
	ExpressionType_BitwiseAnd,
	ExpressionType_Equal,
	ExpressionType_NotEqual,
	ExpressionType_Less,
	ExpressionType_Greater,
	ExpressionType_LessEqual,
	ExpressionType_GreaterEqual,
	ExpressionType_LeftShift,
	ExpressionType_RightShift,
	ExpressionType_Addition,
	ExpressionType_Subtraction,
	ExpressionType_Multiplication,
	ExpressionType_Division,
	ExpressionType_Modulo,

	// Unary expressions.
	ExpressionType_Negate,
	ExpressionType_BitwiseNot,
	ExpressionType_Not,

	ExpressionType_NumericLiteral,
	ExpressionType_Identifier,

	ExpressionType_Assignment,
	ExpressionType_Declaration,
	ExpressionType_DeclarationAssignment,
	ExpressionType_Return,
	ExpressionType_Block,

	ExpressionType_Branch,
	ExpressionType_Loop,

	ExpressionType_FunctionCall,

	ExpressionType_Count,
};
typedef enum ExpressionType ExpressionType;

static b32 expression_is_binary_operation(ExpressionType type)
{
	return (type >= ExpressionType_LogicalOr) && (type <= ExpressionType_Modulo);
}

static b32 expression_is_unary_operation(ExpressionType type)
{
	return (type == ExpressionType_Negate) || (type == ExpressionType_BitwiseNot) || (type == ExpressionType_Not);
}

static b32 expression_is_comparison_operation(ExpressionType type)
{
	return (type >= ExpressionType_Equal) && (type <= ExpressionType_GreaterEqual);
}

struct IdentifierExpression
{
	String name;
	i32 offset_from_frame_pointer; // Temporary: This will eventually move into the intermediate representation.
};
typedef struct IdentifierExpression IdentifierExpression;

struct BinaryExpression
{
	ExpressionHandle lhs;
	ExpressionHandle rhs;
};
typedef struct BinaryExpression BinaryExpression;

struct UnaryExpression
{
	ExpressionHandle rhs;
};
typedef struct UnaryExpression UnaryExpression;

struct AssignmentExpression
{
	ExpressionHandle lhs;
	ExpressionHandle rhs;
};
typedef struct AssignmentExpression AssignmentExpression;

struct DeclarationExpression
{
	ExpressionHandle lhs;
	PrimitiveDatatype data_type;
};
typedef struct DeclarationExpression DeclarationExpression;

struct DeclarationAssignmentExpression
{
	ExpressionHandle lhs;
	PrimitiveDatatype data_type;
	ExpressionHandle rhs;
};
typedef struct DeclarationAssignmentExpression DeclarationAssignmentExpression;

struct ReturnExpression
{
	ExpressionHandle rhs;
};
typedef struct ReturnExpression ReturnExpression;

struct BlockExpression
{
	ExpressionHandle first_expression;
};
typedef struct BlockExpression BlockExpression;

struct BranchExpression
{
	ExpressionHandle condition;
	ExpressionHandle then_expression;
	ExpressionHandle else_expression;
};
typedef struct BranchExpression BranchExpression;

struct LoopExpression
{
	ExpressionHandle condition;
	ExpressionHandle then_expression;
};
typedef struct LoopExpression LoopExpression;

struct FunctionCallExpression
{
	String function_name;
	ExpressionHandle first_argument;
	i32 function_index;
};
typedef struct FunctionCallExpression FunctionCallExpression;

struct Expression
{
	ExpressionType type;
	SourceLocation source_location;
	PrimitiveDatatype result_data_type;

	ExpressionHandle next;

	union
	{
		PrimitiveData literal;
		IdentifierExpression identifier;
		BinaryExpression binary;
		UnaryExpression unary;
		AssignmentExpression assignment;
		DeclarationExpression declaration;
		DeclarationAssignmentExpression declaration_assignment;
		ReturnExpression ret;
		BlockExpression block;
		BranchExpression branch;
		LoopExpression loop;
		FunctionCallExpression function_call;
	};
};
typedef struct Expression Expression;


enum CallingConvention
{
	CallingConvention_Windows_x64,
	CallingConvention_stdcall,
	CallingConvention_cdecl,
};
typedef enum CallingConvention CallingConvention;

struct FunctionParameter
{
	String name;
};
typedef struct FunctionParameter FunctionParameter;

struct LocalVariable
{
	String name;
	i32 offset_from_frame_pointer;
	PrimitiveDatatype data_type;
	SourceLocation source_location;
};
typedef struct LocalVariable LocalVariable;

struct Function
{
	String name;
	SourceLocation source_location;

	CallingConvention calling_convention;
	ExpressionHandle first_expression;

	i64 first_parameter;
	i64 parameter_count;

	i64 stack_size;
};
typedef struct Function Function;


struct Program
{
	String source_code;

	DynamicArray(Function) functions;
	DynamicArray(FunctionParameter) function_parameters;
	DynamicArray(Expression) expressions;
};
typedef struct Program Program;

void free_program(Program* program);
void print_ast(Program* program);

static Expression* program_get_expression(Program* program, ExpressionHandle expression_handle)
{
	return &program->expressions.items[expression_handle];
}

