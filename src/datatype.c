#include "datatype.h"

#include <stdio.h>
#include <inttypes.h>

const char* serialize_numeric_literal(NumericLiteral data)
{
	static char result[128];

	switch (data.type)
	{
		case NumericDatatype_B32:
			snprintf(result, sizeof(result), data.data_b32 ? "1" : "0");
			break;
		case NumericDatatype_I32:
			snprintf(result, sizeof(result), "%" PRIi32, data.data_i32);
			break;
		case NumericDatatype_U32:
			snprintf(result, sizeof(result), "%" PRIu32, data.data_u32);
			break;
		case NumericDatatype_F32:
			snprintf(result, sizeof(result), "%f", data.data_f32);
			break;
	}

	return result;
}

const char* numeric_to_string(NumericDatatype type)
{
	switch (type)
	{
		case NumericDatatype_B32:
			return "b32";
		case NumericDatatype_I32:
			return "i32";
		case NumericDatatype_U32:
			return "u32";
		case NumericDatatype_F32:
			return "f32";
	}
	return "Unknown datatype";
}

