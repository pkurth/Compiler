#pragma once

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
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

#define true 1
#define false 0

#define arraysize(arr) (i64)(sizeof(arr) / sizeof((arr)[0]))


struct String
{
	char* str;
	i64 len;
};
typedef struct String String;

#define string_from_cstr(cstr) (String){ .str = cstr, .len = sizeof(cstr) - 1 }

static void free_string(String* s)
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
// Dummy hash map.
#define HashMap(KeyType, ValueType)													\
	struct																			\
	{																				\
		struct KeyValue																\
		{																			\
			KeyType key;															\
			ValueType value;														\
		};																			\
		struct KeyValue dummy;														\
		DynamicArray(struct KeyValue) items;										\
	}

#define hashmap_insert(map, insert_key, insert_value)								\
	do																				\
	{																				\
		(map)->dummy.key = insert_key;												\
		(map)->dummy.value = insert_value;											\
		array_push(&((map)->items), (map)->dummy);									\
	} while (0)
#endif


enum PrimitiveDatatype
{
	PrimitiveDatatype_I8,
	PrimitiveDatatype_I16,
	PrimitiveDatatype_I32,
	PrimitiveDatatype_I64,

	PrimitiveDatatype_U8,
	PrimitiveDatatype_U16,
	PrimitiveDatatype_U32,
	PrimitiveDatatype_U64,

	PrimitiveDatatype_B32,

	PrimitiveDatatype_F32,
	PrimitiveDatatype_F64,


	PrimitiveDatatype_Count,
};
typedef enum PrimitiveDatatype PrimitiveDatatype;

struct PrimitiveData
{
	PrimitiveDatatype type;

	union
	{
		i8 data_i8;
		i16 data_i16;
		i32 data_i32;
		i64 data_i64;

		u8 data_u8;
		u16 data_u16;
		u32 data_u32;
		u64 data_u64;

		b32 data_b32;

		f32 data_f32;
		f64 data_f64;
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
	TokenType_While,
	TokenType_For,
	TokenType_Return,
	TokenType_I8,
	TokenType_I16,
	TokenType_I32,
	TokenType_I64,
	TokenType_U8,
	TokenType_U16,
	TokenType_U32,
	TokenType_U64,
	TokenType_B32,
	TokenType_F32,
	TokenType_F64,


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


	TokenType_Count,


	TokenType_FirstKeyword = TokenType_If,
	TokenType_LastKeyword = TokenType_F64,

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


struct Token
{
	TokenType type;
	u32 line;

	String str;
	PrimitiveData data;
};
typedef struct Token Token;

typedef DynamicArray(Token) TokenStream;










typedef i64 ExpressionHandle;

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

struct BinaryExpression
{
	ExpressionHandle lhs;
	ExpressionHandle rhs;
};
typedef struct BinaryExpression BinaryExpression;

struct UnaryExpression
{
	ExpressionHandle expression;
};
typedef struct UnaryExpression UnaryExpression;

struct Expression
{
	ExpressionType type;

	union
	{
		PrimitiveData literal;
		String identifier;
		BinaryExpression binary;
		UnaryExpression unary;
	};
};
typedef struct Expression Expression;


enum StatementType
{
	StatementType_Error,
	StatementType_VariableAssignment,
	StatementType_VariableReassignment,
	StatementType_Return,
	StatementType_Block,
};
typedef enum StatementType StatementType;

struct VariableAssignmentStatement
{
	Token identifier;
	ExpressionHandle expression;
};
typedef struct VariableAssignmentStatement VariableAssignmentStatement;

struct ReturnStatement
{
	ExpressionHandle expression;
};
typedef struct ReturnStatement ReturnStatement;

struct BlockStatement
{
	i64 statement_count;
};
typedef struct BlockStatement BlockStatement;

struct Statement
{
	StatementType type;

	union
	{
		VariableAssignmentStatement variable_assignment;
		ReturnStatement ret;
		BlockStatement block;
	};
};
typedef struct Statement Statement;


struct FunctionParameter
{
	String name;
};
typedef struct FunctionParameter FunctionParameter;

struct LocalVariable
{
	String name;
	i32 offset_from_frame_pointer;
};
typedef struct LocalVariable LocalVariable;

struct Function
{
	String name;

	i64 first_statement;
	i64 statement_count;

	i64 first_parameter;
	i64 parameter_count;

	i64 first_local_variable;
	i64 local_variable_count;
	i64 stack_size;
};
typedef struct Function Function;


struct Program
{
	DynamicArray(Function) functions;
	DynamicArray(FunctionParameter) function_parameters;
	DynamicArray(LocalVariable) local_variables;
	DynamicArray(Statement) statements;
	DynamicArray(Expression) expressions;

	b32 has_errors;
};
typedef struct Program Program;

static Expression program_get_expression(Program* program, ExpressionHandle expression_handle)
{
	return program->expressions.items[expression_handle];
}

