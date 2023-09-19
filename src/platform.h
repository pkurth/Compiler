#pragma once

#include "common.h"

void create_directory(String path);


String read_file(const char* filename);
void write_file(const char* filename, String s);
String path_get_parent(String path);
String path_get_filename(String path);
String path_get_stem(String path);
