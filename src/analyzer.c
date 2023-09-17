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


static NumericDatatype unary_operation_result_datatype(NumericDatatype rhs, ExpressionType expression_type)
{
	if (rhs == NumericDatatype_Unknown)
	{
		return NumericDatatype_Unknown;
	}

	NumericDatatype result = NumericDatatype_Unknown;

	switch (expression_type)
	{
		case ExpressionType_Negate: 
			result = min(rhs, NumericDatatype_I32);
			break;
		case ExpressionType_BitwiseNot:
			result = numeric_is_integral(rhs) ? rhs : NumericDatatype_Unknown;
			break;
		case ExpressionType_Not: 
			result = numeric_converts_to_b32(rhs) ? NumericDatatype_B32 : NumericDatatype_Unknown;
			break;
	}

	return result;
}

static NumericDatatype binary_operation_result_datatype(NumericDatatype lhs, NumericDatatype rhs, ExpressionType expression_type)
{
	if (lhs == NumericDatatype_Unknown || rhs == NumericDatatype_Unknown)
	{
		return NumericDatatype_Unknown;
	}

	NumericDatatype result = NumericDatatype_Unknown;

	switch (expression_type)
	{
		case ExpressionType_LogicalOr:
		case ExpressionType_LogicalAnd:
			result = (numeric_converts_to_b32(lhs) && numeric_converts_to_b32(rhs)) ? NumericDatatype_B32 : NumericDatatype_Unknown;
			break;

		case ExpressionType_BitwiseOr:
		case ExpressionType_BitwiseXor:
		case ExpressionType_BitwiseAnd:
			result = (numeric_converts_to_b32(lhs) && numeric_converts_to_b32(rhs)) ? max(lhs, rhs) : NumericDatatype_Unknown;
			break;

		case ExpressionType_Equal:
		case ExpressionType_NotEqual:
		case ExpressionType_Less:
		case ExpressionType_Greater:
		case ExpressionType_LessEqual:
		case ExpressionType_GreaterEqual:
			result = NumericDatatype_B32;
			break;

		case ExpressionType_LeftShift:
		case ExpressionType_RightShift:
			result = (numeric_is_integral(lhs) && numeric_is_integral(rhs)) ? max(lhs, rhs) : NumericDatatype_Unknown;
			break;

		case ExpressionType_Addition:
		case ExpressionType_Subtraction:
		case ExpressionType_Multiplication:
		case ExpressionType_Division:
			result = max(lhs, rhs);
			break;

		case ExpressionType_Modulo:
			result = (numeric_is_integral(lhs) && numeric_is_integral(rhs)) ? max(lhs, rhs) : NumericDatatype_Unknown;
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

static b32 add_local_variable(Program* program, String identifier, NumericDatatype data_type, SourceLocation source_location,
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

static b32 add_parameter_variable(Program* program, String identifier, NumericDatatype data_type, SourceLocation source_location,
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

		//expression->result_data_type = var->data_type;
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

		//expression->result_data_type = binary_operation_result_datatype(lhs->result_data_type, rhs->result_data_type, expression->type);
	}
	else if (expression_is_unary_operation(expression->type))
	{
		UnaryExpression e = expression->unary;

		if (!analyze_expression(program, e.rhs, stack_info))
		{
			return false;
		}

		Expression* rhs = program_get_expression(program, e.rhs);

		//expression->result_data_type = unary_operation_result_datatype(rhs->result_data_type, expression->type);
	}
	else if (expression->type == ExpressionType_NumericLiteral)
	{
		//expression->result_data_type = expression->literal.type;
	}
	else if (expression->type == ExpressionType_StringLiteral)
	{
		//expression->result_data_type = expression->literal.type;
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

		//expression->result_data_type = var->data_type;
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
						fprintf(stderr, "LINE %d: ", called_function->source_location.line);
						print_line(program->source_code, called_function->source_location);
						error_printed = true;
					}
					fprintf(stderr, "LINE %d: ", function->source_location.line);
					print_line(program->source_code, function->source_location);
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

static b32 analyze_statements(Program* program, i32 first_statement, i32 statement_count, StackInfo* stack_info)
{
	i64 first_local_variable_in_current_block = stack_info->current_local_variables->count;

	i32 current_offset_from_frame_pointer = stack_info->current_offset_from_frame_pointer;
	i64 current_local_variable_count = stack_info->current_local_variables->count;

	for (i32 i = 0; i < statement_count; ++i)
	{
		i32 statement_index = first_statement + i;

		Statement* statement = program_get_statement(program, statement_index);

		if (statement->type == StatementType_Simple)
		{
			if (!analyze_expression(program, statement->simple.expression, stack_info)) { return false; }
		}
		else if (statement->type == StatementType_Declaration)
		{
			DeclarationStatement e = statement->declaration;

			Expression* lhs = program_get_expression(program, e.lhs);
			assert(lhs->type == ExpressionType_Identifier); // Temporary.

			String identifier = lhs->identifier.name;

			if (!add_local_variable(program, identifier, e.data_type, statement->source_location, stack_info, first_local_variable_in_current_block)) { return false; }
			if (!analyze_expression(program, e.lhs, stack_info)) { return false; }
		}
		else if (statement->type == StatementType_DeclarationAssignment)
		{
			DeclarationAssignmentStatement e = statement->declaration_assignment;

			if (!analyze_expression(program, e.rhs, stack_info)) { return false; }

			Expression* rhs = program_get_expression(program, e.rhs);
			Expression* lhs = program_get_expression(program, e.lhs);
			assert(lhs->type == ExpressionType_Identifier); // Temporary.

			NumericDatatype data_type = e.data_type;// (e.data_type == NumericDatatype_Unknown) ? rhs->result_data_type : e.data_type;

			String identifier = lhs->identifier.name;
			if (!add_local_variable(program, identifier, data_type, statement->source_location, stack_info, first_local_variable_in_current_block)) { return false; }
			if (!analyze_expression(program, e.lhs, stack_info)) { return false; }
		}
		else if (statement->type == StatementType_Return)
		{
			if (!analyze_expression(program, statement->ret.rhs, stack_info)) { return false; }
		}
		else if (statement->type == StatementType_Block)
		{
			if (!analyze_statements(program, statement_index + 1, statement->block.statement_count, stack_info)) { return false; }
			i += statement->block.statement_count;
		}
		else if (statement->type == StatementType_Branch)
		{
			BranchStatement e = statement->branch;

			if (!analyze_expression(program, e.condition, stack_info)) { return false; }

			if (!analyze_statements(program, statement_index + 1, e.then_statement_count, stack_info)) { return false; }
			if (!analyze_statements(program, statement_index + e.then_statement_count + 1, e.else_statement_count, stack_info)) { return false; }
			i += e.then_statement_count + e.else_statement_count;
		}
		else if (statement->type == StatementType_Loop)
		{
			LoopStatement e = statement->loop;

			if (!analyze_expression(program, e.condition, stack_info)) { return false; }

			if (!analyze_statements(program, statement_index + 1, e.then_statement_count, stack_info)) { return false; }
			i += e.then_statement_count;
		}
		else
		{
			assert(false);
		}
	}

	stack_info->current_local_variables->count = current_local_variable_count;
	stack_info->current_offset_from_frame_pointer = current_offset_from_frame_pointer;

	return true;
}

static b32 analyze_function(Program* program, Function* function, LocalVariableContext* local_variable_context)
{
	i64 variable_count = local_variable_context->count;

	StackInfo stack_info = { .current_local_variables = local_variable_context };

	b32 result = true;

	for (i64 i = 0; i < function->parameter_count; ++i)
	{
		FunctionParameter parameter = program->function_parameters.items[i + function->first_parameter];
		if (!add_parameter_variable(program, parameter.name, NumericDatatype_I32, function->source_location, (i32)i, &stack_info, variable_count))
		{
			result = false;
		}
	}

	if (!analyze_statements(program, function->body_first_statement, function->body_statement_count, &stack_info))
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
