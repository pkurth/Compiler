#include "analyzer.h"

#include <assert.h>

typedef DynamicArray(LocalVariable) LocalVariableContext;

struct StackInfo
{
	LocalVariableContext* current_local_variables;

	i32 stack_size;
	i32 current_offset_from_frame_pointer;
};
typedef struct StackInfo StackInfo;


typedef i64 LogicalRegister;


static PrimitiveDatatype unary_operation_result_datatype(PrimitiveDatatype rhs, ExpressionType expression_type)
{
	if (rhs == PrimitiveDatatype_Unknown)
	{
		return PrimitiveDatatype_Unknown;
	}

	PrimitiveDatatype result;

	switch (expression_type)
	{
		case ExpressionType_Negate: 
			result = min(rhs, PrimitiveDatatype_I32);
			break;

		case ExpressionType_BitwiseNot:
			result = data_type_is_integral(rhs) ? rhs : PrimitiveDatatype_Unknown;
			break;

		case ExpressionType_Not: 
			result = data_type_converts_to_b32(rhs) ? PrimitiveDatatype_B32 : PrimitiveDatatype_Unknown;
			break;
		
		default:
			result = PrimitiveDatatype_Unknown;
			break;
	}

	return result;
}

static PrimitiveDatatype binary_operation_result_datatype(PrimitiveDatatype lhs, PrimitiveDatatype rhs, ExpressionType expression_type)
{
	if (lhs == PrimitiveDatatype_Unknown || rhs == PrimitiveDatatype_Unknown)
	{
		return PrimitiveDatatype_Unknown;
	}

	PrimitiveDatatype result;

	switch (expression_type)
	{
		case ExpressionType_LogicalOr:
		case ExpressionType_LogicalAnd:
			result = (data_type_converts_to_b32(lhs) && data_type_converts_to_b32(rhs)) ? PrimitiveDatatype_B32 : PrimitiveDatatype_Unknown;
			break;

		case ExpressionType_BitwiseOr:
		case ExpressionType_BitwiseXor:
		case ExpressionType_BitwiseAnd:
			result = (data_type_converts_to_b32(lhs) && data_type_converts_to_b32(rhs)) ? max(lhs, rhs) : PrimitiveDatatype_Unknown;
			break;

		case ExpressionType_Equal:
		case ExpressionType_NotEqual:
		case ExpressionType_Less:
		case ExpressionType_Greater:
		case ExpressionType_LessEqual:
		case ExpressionType_GreaterEqual:
			result = PrimitiveDatatype_B32;
			break;

		case ExpressionType_LeftShift:
		case ExpressionType_RightShift:
			result = (data_type_is_integral(lhs) && data_type_is_integral(rhs)) ? max(lhs, rhs) : PrimitiveDatatype_Unknown;
			break;

		case ExpressionType_Addition:
		case ExpressionType_Subtraction:
		case ExpressionType_Multiplication:
		case ExpressionType_Division:
			result = max(lhs, rhs);
			break;

		case ExpressionType_Modulo:
			result = (data_type_is_integral(lhs) && data_type_is_integral(rhs)) ? max(lhs, rhs) : PrimitiveDatatype_Unknown;
			break;

		default:
			result = PrimitiveDatatype_Unknown;
			break;
	}

	return result;
}

static LocalVariable* find_local_variable(LocalVariableContext* local_variables, String name, i64 block_start)
{
	for (i64 i = local_variables->count - 1; i >= block_start; --i)
	{
		if (string_equal(local_variables->items[i].name, name))
		{
			return &local_variables->items[i];
		}
	}
	return 0;
}

static b32 add_local_variable(String identifier, PrimitiveDatatype data_type, StackInfo* stack_info, i64 first_local_variable_in_current_block)
{
	LocalVariable* var = find_local_variable(stack_info->current_local_variables, identifier, first_local_variable_in_current_block);

	if (var)
	{
		fprintf(stderr, "Identifier '%.*s' is already declared.\n", (i32)identifier.len, identifier.str);
		return false;
	}

	// Add to local variables.
	stack_info->current_offset_from_frame_pointer += 8; // Increment first!
	stack_info->stack_size = max(stack_info->stack_size, stack_info->current_offset_from_frame_pointer);

	LocalVariable variable =
	{
		.name = identifier,
		.offset_from_frame_pointer = stack_info->current_offset_from_frame_pointer,
		.data_type = data_type,
	};

	array_push(stack_info->current_local_variables, variable);

	return true;
}

static b32 analyze_expression(Program* program, ExpressionHandle expression_handle, StackInfo* stack_info,
	i64 first_local_variable_in_current_block)
{
	Expression* expression = program_get_expression(program, expression_handle);

	if (expression->type == ExpressionType_Assignment)
	{
		AssignmentExpression e = expression->assignment;

		if (!analyze_expression(program, e.rhs, stack_info, first_local_variable_in_current_block))
		{
			return false;
		}

		Expression* lhs_expression = program_get_expression(program, e.lhs);
		assert(lhs_expression->type == ExpressionType_Identifier); // Temporary.

		String identifier = lhs_expression->identifier.name;
		LocalVariable* var = find_local_variable(stack_info->current_local_variables, identifier, first_local_variable_in_current_block);

		if (!var)
		{
			fprintf(stderr, "Undeclared identifier '%.*s'.\n", (i32)identifier.len, identifier.str);
			return false;
		}

		if (!analyze_expression(program, e.lhs, stack_info, first_local_variable_in_current_block))
		{
			return false;
		}

		expression->result_data_type = var->data_type;
	}
	else if (expression_is_binary_operation(expression->type))
	{
		BinaryExpression e = expression->binary;

		if (!analyze_expression(program, e.lhs, stack_info, first_local_variable_in_current_block))
		{
			return false;
		}
		if (!analyze_expression(program, e.rhs, stack_info, first_local_variable_in_current_block))
		{
			return false;
		}

		Expression* lhs = program_get_expression(program, e.lhs);
		Expression* rhs = program_get_expression(program, e.rhs);

		expression->result_data_type = binary_operation_result_datatype(lhs->result_data_type, rhs->result_data_type, expression->type);
	}
	else if (expression_is_unary_operation(expression->type))
	{
		UnaryExpression e = expression->unary;

		if (!analyze_expression(program, e.rhs, stack_info, first_local_variable_in_current_block))
		{
			return false;
		}

		Expression* rhs = program_get_expression(program, e.rhs);

		expression->result_data_type = unary_operation_result_datatype(rhs->result_data_type, expression->type);
	}
	else if (expression->type == ExpressionType_NumericLiteral)
	{
		expression->result_data_type = expression->literal.type;
	}
	else if (expression->type == ExpressionType_Identifier)
	{
		IdentifierExpression* e = &expression->identifier;

		String name = e->name;
		LocalVariable* var = find_local_variable(stack_info->current_local_variables, name, 0);
		if (!var)
		{
			fprintf(stderr, "Undeclared identifier '%.*s'.\n", (i32)name.len, name.str);
			return false;
		}

		expression->result_data_type = var->data_type;
		e->offset_from_frame_pointer = var->offset_from_frame_pointer;
	}

	return true;
}

static b32 analyze_top_level_expression(Program* program, ExpressionHandle expression_handle, StackInfo* stack_info,
	i64 first_local_variable_in_current_block)
{
	while (expression_handle)
	{
		Expression* expression = program_get_expression(program, expression_handle);

		if (expression->type == ExpressionType_Declaration)
		{
			DeclarationExpression e = expression->declaration;

			Expression* lhs = program_get_expression(program, e.lhs);
			assert(lhs->type == ExpressionType_Identifier); // Temporary.

			String identifier = lhs->identifier.name;
			if (!add_local_variable(identifier, e.data_type, stack_info, first_local_variable_in_current_block))
			{
				return false;
			}

			if (!analyze_expression(program, e.lhs, stack_info, first_local_variable_in_current_block))
			{
				return false;
			}
		}
		else if (expression->type == ExpressionType_DeclarationAssignment)
		{
			DeclarationAssignmentExpression e = expression->declaration_assignment;

			if (!analyze_expression(program, e.rhs, stack_info, first_local_variable_in_current_block))
			{
				return false;
			}

			Expression* lhs = program_get_expression(program, e.lhs);
			assert(lhs->type == ExpressionType_Identifier); // Temporary.

			String identifier = lhs->identifier.name;
			if (!add_local_variable(identifier, e.data_type, stack_info, first_local_variable_in_current_block))
			{
				return false;
			}

			if (!analyze_expression(program, e.lhs, stack_info, first_local_variable_in_current_block))
			{
				return false;
			}
		}
		else if (expression->type == ExpressionType_Return)
		{
			if (!analyze_expression(program, expression->ret.rhs, stack_info, first_local_variable_in_current_block))
			{
				return false;
			}
		}
		else if (expression->type == ExpressionType_Block)
		{
			i32 current_offset = stack_info->current_offset_from_frame_pointer;
			i64 current_local_variable_count = stack_info->current_local_variables->count;

			if (!analyze_top_level_expression(program, expression->block.first_expression, stack_info, current_local_variable_count))
			{
				return false;
			}

			stack_info->current_local_variables->count = current_local_variable_count;
			stack_info->current_offset_from_frame_pointer = current_offset;
		}
		else
		{
			if (!analyze_expression(program, expression_handle, stack_info, first_local_variable_in_current_block))
			{
				return false;
			}
		}

		expression_handle = expression->next;
	}

	return true;
}

static b32 analyze_function(Program* program, Function* function, LocalVariableContext* local_variable_context)
{
	StackInfo stack_info = { .current_local_variables = local_variable_context };

	if (analyze_top_level_expression(program, function->first_expression, &stack_info, stack_info.current_local_variables->count))
	{
		function->stack_size = stack_info.stack_size;
		return true;
	}
	return false;
}

void analyze(Program* program)
{
	LocalVariableContext local_variable_context = { 0 };

	for (i64 i = 0; i < program->functions.count; ++i)
	{
		local_variable_context.count = 0;

		b32 success = analyze_function(program, &program->functions.items[i], &local_variable_context);
		program->has_errors |= !success;
	}

	array_free(&local_variable_context);
}
