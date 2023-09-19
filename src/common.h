#pragma once

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef u32 b32;

#define arraysize(arr) (i64)(sizeof(arr) / sizeof((arr)[0]))


struct String
{
	char* str;
	i64 len;
};
typedef struct String String;

#define string_from_cstr(cstr) (String){ .str = cstr, .len = sizeof(cstr) - 1 }

static void string_free(String* s)
{
	free(s->str);
	s->str = 0;
	s->len = 0;
}

static b32 string_equal(String s1, String s2)
{
	return s1.len == s2.len && strncmp(s1.str, s2.str, s1.len) == 0;
}

static void string_push(String* string, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	string->len += vsprintf(string->str + string->len, format, args);
	va_end(args);
}


#define DynamicArray(ItemType)														\
	struct																			\
	{																				\
		ItemType* items;															\
		i64 count;																	\
		i64 capacity;																\
	}

#define array_push(a, item)															\
	do																				\
	{																				\
		if ((a)->count == (a)->capacity)											\
		{																			\
			(a)->capacity = max((a)->capacity * 2, 16);								\
			(a)->items = realloc((a)->items, sizeof(*(a)->items) * (a)->capacity);	\
		}																			\
		(a)->items[(a)->count++] = item;											\
	} while (0)

#define array_free(a)																\
	do																				\
	{																				\
		free((a)->items);															\
		(a)->items = 0;																\
		(a)->capacity = 0;															\
		(a)->count = 0;																\
	} while (0)


#if 0
#define HashMap(KeyType, ValueType, capacity)										\
	struct																			\
	{																				\
		i64 (*hash_function)(KeyType);												\
		struct KeyIndexPair															\
		{																			\
			KeyType key;															\
			i64 index;																\
		} keys[capacity];															\
		ValueType values[capacity];													\
		i64 count;																	\
	}

#define hashmap_insert(map, key, value)												\
	do																				\
	{																				\
		i64 hash = (map)->hash_function(key);										\
		i64 capacity = arraysize((map)->keys);										\
		i64 key_index = hash % capacity;											\
		i64 value_index = (map)->count++;											\
		(map)->values[value_index] = value;											\
		(map)->keys[key_index].key = key;											\
		(map)->keys[key_index].index = value_index;									\
	} while (0)

//#define hashmap_lookup(map, key) \
//	(map)->values[(map)->keys[(map)->hash_function(key) % arraysize((map)->keys)].index]

static i64 hash_string(String str)
{
	return str.len * str.str[0];
}

static void test()
{
	typedef HashMap(String, int, 512) StringToIntMap;

	StringToIntMap map = { &hash_string };

	String key = string_from_cstr("Hallo Welt");
	hashmap_insert(&map, key, 12);

	int key = hashmap_lookup(&map, key);

	int a = 0;

	int index = (int i = 3, i);
}
#endif


struct SourceLocation
{
	i32 line;
	i32 global_character_index;
};
typedef struct SourceLocation SourceLocation;

