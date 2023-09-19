#pragma once

#include "common.h"
#include "token.h"




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

	// Others.
	ExpressionType_NumericLiteral,
	ExpressionType_StringLiteral,
	ExpressionType_Identifier,
	ExpressionType_Assignment,
	ExpressionType_FunctionCall,

	ExpressionType_Count,
};
typedef enum ExpressionType ExpressionType;

enum StatementType
{
	StatementType_Error,

	StatementType_Simple,
	StatementType_Declaration,
	StatementType_DeclarationAssignment,
	StatementType_Return,
	StatementType_Block,

	StatementType_Branch,
	StatementType_Loop,

	StatementType_Count,
};
typedef enum StatementType StatementType;

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
	ExpressionHandle next;

	union
	{
		NumericLiteral numeric_literal;
		String string_literal;
		IdentifierExpression identifier;
		BinaryExpression binary;
		UnaryExpression unary;
		AssignmentExpression assignment;
		FunctionCallExpression function_call;
	};
};
typedef struct Expression Expression;


struct SimpleStatement
{
	ExpressionHandle expression;
};
typedef struct SimpleStatement SimpleStatement;

struct DeclarationStatement
{
	ExpressionHandle lhs;
	NumericDatatype data_type; // TODO: Generalize.
};
typedef struct DeclarationStatement DeclarationStatement;

struct DeclarationAssignmentStatement
{
	ExpressionHandle lhs;
	NumericDatatype data_type; // TODO: Generalize.
	ExpressionHandle rhs;
};
typedef struct DeclarationAssignmentStatement DeclarationAssignmentStatement;

struct ReturnStatement
{
	ExpressionHandle rhs;
};
typedef struct ReturnStatement ReturnStatement;

struct BlockStatement
{
	i32 statement_count;
};
typedef struct BlockStatement BlockStatement;

struct BranchStatement
{
	ExpressionHandle condition;
	i32 then_statement_count;
	i32 else_statement_count;
};
typedef struct BranchStatement BranchStatement;

struct LoopStatement
{
	ExpressionHandle condition;
	i32 then_statement_count;
};
typedef struct LoopStatement LoopStatement;

struct Statement
{
	StatementType type;
	SourceLocation source_location;

	union
	{
		SimpleStatement simple;
		DeclarationStatement declaration;
		DeclarationAssignmentStatement declaration_assignment;
		ReturnStatement ret;
		BlockStatement block;
		BranchStatement branch;
		LoopStatement loop;
	};
};
typedef struct Statement Statement;









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
	NumericDatatype data_type; // TODO: Generalize.
	SourceLocation source_location;
};
typedef struct LocalVariable LocalVariable;

struct Function
{
	String name;
	SourceLocation source_location;

	CallingConvention calling_convention;

	i32 body_first_statement;
	i32 body_statement_count;



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
	DynamicArray(Statement) statements;
	DynamicArray(Expression) expressions;
};
typedef struct Program Program;


static Expression* program_get_expression(Program* program, ExpressionHandle expression_handle)
{
	return &program->expressions.items[expression_handle];
}

static Statement* program_get_statement(Program* program, i32 statement_index)
{
	return &program->statements.items[statement_index];
}

b32 parse(Program* program, TokenStream stream);
b32 analyze(Program* program);
String generate(Program program);

void program_print_ast(Program* program);

String program_get_line(Program* program, i32 character_index);
i32 program_get_column(Program* program, SourceLocation source_location, String line);

void program_print_line(Program* program, SourceLocation source_location, const FILE* stream);
void program_print_line_error(Program* program, SourceLocation source_location);

void free_program(Program* program);
