#pragma once

#include "types.h"

TokenStream tokenize(String contents);
void free_token_stream(TokenStream* tokens);
void print_tokens(TokenStream* tokens);

