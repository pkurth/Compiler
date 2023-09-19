#pragma once

#include "common.h"

enum NumericDatatype
{
	NumericDatatype_Unknown,

	NumericDatatype_B32,
	NumericDatatype_U32,
	NumericDatatype_I32,
	NumericDatatype_F32,

	NumericDatatype_Count,
};
typedef enum NumericDatatype NumericDatatype;

static b32 numeric_is_integral(NumericDatatype type)
{
	return type == NumericDatatype_I32 || type == NumericDatatype_U32;
}

static b32 numeric_converts_to_b32(NumericDatatype type)
{
	return type == NumericDatatype_B32 || numeric_is_integral(type);
}


struct NumericLiteral
{
	NumericDatatype type;

	union
	{
		b32 data_b32;
		u32 data_u32;
		i32 data_i32;
		f32 data_f32;
	};
};
typedef struct NumericLiteral NumericLiteral;

const char* numeric_to_string(NumericDatatype type);
const char* serialize_numeric_literal(NumericLiteral literal);
