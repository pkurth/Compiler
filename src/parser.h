#pragma once

#include "types.h"

Program parse(TokenStream stream);
void free_program(Program* program);
void print_program(Program program);
