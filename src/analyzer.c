#include "analyzer.h"
#include "error.h"

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

static b32 assert_no_variable_name_collision(Program* program, String identifier, SourceLocation source_location,
	StackInfo* stack_info, i64 first_local_variable_in_current_block)
{
	LocalVariable* var = find_local_variable(stack_info->current_local_variables, identifier, first_local_variable_in_current_block);

	if (var)
	{
		fprintf(stderr, "LINE %d: Identifier '%.*s' is already declared in line %d:\n", source_location.line, (i32)identifier.len, identifier.str, var->source_location.line);
		print_line_error(program->source_code, var->source_location);
		return false;
	}

	return true;
}

static b32 add_local_variable(Program* program, String identifier, PrimitiveDatatype data_type, SourceLocation source_location,
	StackInfo* stack_info, i64 first_local_variable_in_current_block)
{
	if (!assert_no_variable_name_collision(program, identifier, source_location, stack_info, first_local_variable_in_current_block))
	{
		return false;
	}

	stack_info->current_offset_from_frame_pointer += 8; // Increment first!
	stack_info->stack_size = max(stack_info->stack_size, stack_info->current_offset_from_frame_pointer);

	LocalVariable variable =
	{
		.name = identifier,
		.offset_from_frame_pointer = -stack_info->current_offset_from_frame_pointer,
		.data_type = data_type,
		.source_location = source_location,
	};

	array_push(stack_info->current_local_variables, variable);

	return true;
}

static b32 add_parameter_variable(Program* program, String identifier, PrimitiveDatatype data_type, SourceLocation source_location,
	i32 parameter_index, StackInfo* stack_info, i64 first_local_variable_in_current_block)
{
	if (!assert_no_variable_name_collision(program, identifier, source_location, stack_info, first_local_variable_in_current_block))
	{
		return false;
	}

	LocalVariable variable =
	{
		.name = identifier,
		.offset_from_frame_pointer = parameter_index * 8 + 16, // Skip over return address and pushed rbp.
		.data_type = data_type,
		.source_location = source_location,
	};

	array_push(stack_info->current_local_variables, variable);

	return true;
}

static b32 analyze_expression(Program* program, ExpressionHandle expression_handle, StackInfo* stack_info)
{
	Expression* expression = program_get_expression(program, expression_handle);

	if (expression->type == ExpressionType_Assignment)
	{
		AssignmentExpression e = expression->assignment;

		if (!analyze_expression(program, e.rhs, stack_info))
		{
			return false;
		}

		Expression* lhs_expression = program_get_expression(program, e.lhs);
		assert(lhs_expression->type == ExpressionType_Identifier); // Temporary.

		String identifier = lhs_expression->identifier.name;
		LocalVariable* var = find_local_variable(stack_info->current_local_variables, identifier, 0);

		if (!var)
		{
			fprintf(stderr, "LINE %d: Undeclared identifier '%.*s'.\n", lhs_expression->source_location.line, (i32)identifier.len, identifier.str);
			print_line_error(program->source_code, lhs_expression->source_location);
			return false;
		}

		if (!analyze_expression(program, e.lhs, stack_info))
		{
			return false;
		}

		expression->result_data_type = var->data_type;
	}
	else if (expression_is_binary_operation(expression->type))
	{
		BinaryExpression e = expression->binary;

		if (!analyze_expression(program, e.lhs, stack_info))
		{
			return false;
		}
		if (!analyze_expression(program, e.rhs, stack_info))
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

		if (!analyze_expression(program, e.rhs, stack_info))
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
			fprintf(stderr, "LINE %d: Undeclared identifier '%.*s'.\n", expression->source_location.line, (i32)name.len, name.str);
			print_line_error(program->source_code, expression->source_location);
			return false;
		}

		expression->result_data_type = var->data_type;
		e->offset_from_frame_pointer = var->offset_from_frame_pointer;
	}
	else if (expression->type == ExpressionType_FunctionCall)
	{
		FunctionCallExpression* e = &expression->function_call;

		i32 argument_count = 0;

		ExpressionHandle argument = e->first_argument;
		while (argument)
		{
			if (!analyze_expression(program, argument, stack_info))
			{
				return false;
			}
			++argument_count;
			argument = program_get_expression(program, argument)->next;
		}

		Function* called_function = 0;
		b32 error_printed = false;
		for (i64 function_index = 0; function_index < program->functions.count; ++function_index)
		{
			Function* function = &program->functions.items[function_index];
			if (string_equal(function->name, e->function_name) && function->parameter_count == argument_count)
			{
				if (called_function)
				{
					if (!error_printed)
					{
						fprintf(stderr, "LINE %d: More than one function matches call:\n", expression->source_location.line);
						print_line_error(program->source_code, expression->source_location);
						fprintf(stderr, "Could be either:\n");
						print_line_error(program->source_code, called_function->source_location);
						error_printed = true;
					}
					print_line_error(program->source_code, function->source_location);
				}
				called_function = function;
			}
		}
		if (!called_function)
		{
			fprintf(stderr, "LINE %d: No matching function found for call:\n", expression->source_location.line);
			print_line_error(program->source_code, expression->source_location);
			return false;
		}
		if (error_printed)
		{
			return false;
		}
		e->function_index = (i32)(called_function - program->functions.items);
	}
	else
	{
		assert(false);
	}

	return true;
}

static b32 analyze_top_level_expression(Program* program, ExpressionHandle expression_handle, StackInfo* stack_info)
{
	i64 first_local_variable_in_current_block = stack_info->current_local_variables->count;

	i32 current_offset_from_frame_pointer = stack_info->current_offset_from_frame_pointer;
	i64 current_local_variable_count = stack_info->current_local_variables->count;

	while (expression_handle)
	{
		Expression* expression = program_get_expression(program, expression_handle);

		if (expression->type == ExpressionType_Declaration)
		{
			DeclarationExpression e = expression->declaration;

			Expression* lhs = program_get_expression(program, e.lhs);
			assert(lhs->type == ExpressionType_Identifier); // Temporary.

			String identifier = lhs->identifier.name;

			if (!add_local_variable(program, identifier, e.data_type, expression->source_location, stack_info, first_local_variable_in_current_block)) { break; }
			if (!analyze_expression(program, e.lhs, stack_info)) { break; }
		}
		else if (expression->type == ExpressionType_DeclarationAssignment)
		{
			DeclarationAssignmentExpression e = expression->declaration_assignment;

			if (!analyze_expression(program, e.rhs, stack_info)) { break; }

			Expression* lhs = program_get_expression(program, e.lhs);
			assert(lhs->type == ExpressionType_Identifier); // Temporary.

			String identifier = lhs->identifier.name;
			if (!add_local_variable(program, identifier, e.data_type, expression->source_location, stack_info, first_local_variable_in_current_block)) { break; }
			if (!analyze_expression(program, e.lhs, stack_info)) { break; }
		}
		else if (expression->type == ExpressionType_Return)
		{
			if (!analyze_expression(program, expression->ret.rhs, stack_info)) { break; }
		}
		else if (expression->type == ExpressionType_Block)
		{
			if (!analyze_top_level_expression(program, expression->block.first_expression, stack_info)) { break; }
		}
		else if (expression->type == ExpressionType_Branch)
		{
			BranchExpression e = expression->branch;

			if (!analyze_expression(program, e.condition, stack_info)) { break; }
			if (e.then_expression)
			{
				if (!analyze_top_level_expression(program, e.then_expression, stack_info)) { break; }
			}
			if (e.else_expression)
			{
				if (!analyze_top_level_expression(program, e.else_expression, stack_info)) { break; }
			}
		}
		else if (expression->type == ExpressionType_Loop)
		{
			LoopExpression e = expression->loop;

			if (!analyze_expression(program, e.condition, stack_info)) { break; }
			if (e.then_expression)
			{
				if (!analyze_top_level_expression(program, e.then_expression, stack_info)) { break; }
			}
		}
		else
		{
			if (!analyze_expression(program, expression_handle, stack_info)) { break; }
		}

		expression_handle = expression->next;
	}

	stack_info->current_local_variables->count = current_local_variable_count;
	stack_info->current_offset_from_frame_pointer = current_offset_from_frame_pointer;

	return expression_handle == 0;
}

static b32 analyze_function(Program* program, Function* function, LocalVariableContext* local_variable_context)
{
	i64 variable_count = local_variable_context->count;

	StackInfo stack_info = { .current_local_variables = local_variable_context };

	b32 result = true;

	for (i64 i = 0; i < function->parameter_count; ++i)
	{
		FunctionParameter parameter = program->function_parameters.items[i + function->first_parameter];
		if (!add_parameter_variable(program, parameter.name, PrimitiveDatatype_I32, function->source_location, (i32)i, &stack_info, variable_count))
		{
			result = false;
		}
	}

	if (!analyze_top_level_expression(program, function->first_expression, &stack_info))
	{
		result = false;
	}

	function->stack_size = stack_info.stack_size;
	local_variable_context->count = variable_count;

	return result;
}

b32 analyze(Program* program)
{
	LocalVariableContext local_variable_context = { 0 };

	b32 result = true;

	for (i64 i = 0; i < program->functions.count; ++i)
	{
		local_variable_context.count = 0;

		b32 success = analyze_function(program, &program->functions.items[i], &local_variable_context);
		result &= success;
	}

	array_free(&local_variable_context);

	return result;
}
