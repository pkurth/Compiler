#pragma once

#include "types.h"

TokenStream tokenize(String source_code);
void free_token_stream(TokenStream* tokens);
void print_tokens(TokenStream* tokens);

