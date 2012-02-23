/**
 * Copyright 2011 Matthew Endsley. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 *
 * Based on js0n implementation at:
 * https://github.com/quartzjer/js0n/blob/master/js0n.c
 */
#ifndef MY_JSON_H__
#define MY_JSON_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


enum myjson_token_type
{
	MYJSON_TOKEN_LITERAL,
	MYJSON_TOKEN_STRING,
	MYJSON_TOKEN_OBJECT,
	MYJSON_TOKEN_ARRAY,
};

struct myjson_token
{
	enum myjson_token_type type;
	uint16_t start;
	uint16_t length;
};

void myjson_init();
int myjson_parse( const char* json, int length, struct myjson_token *out, int max_tokens );
int myjson_get_integer( const char* json, const struct myjson_token* token );
long long myjson_get_integer64( const char* json, const struct myjson_token* token );

#ifdef __cplusplus
}
#endif

#endif // MY_JSON_H__
