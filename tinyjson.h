/**
 * Copyright 2011 Matthew Endsley. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 *
 * Based on js0n implementation at:
 * https://github.com/quartzjer/js0n/blob/master/js0n.c
 */

#ifndef TINY_JSON_H
#define TINY_JSON_H

#ifdef __cplusplus
extern "C" {
#endif


enum tinyjson_token_type {
	MYJSON_TOKEN_LITERAL,
	MYJSON_TOKEN_STRING,
	MYJSON_TOKEN_OBJECT,
	MYJSON_TOKEN_ARRAY,
};

struct tinyjson_token {
	enum tinyjson_token_type type;
	unsigned int start;
	unsigned int length;
};

void tinyjson_init();
int tinyjson_parse( const char* json, int length, struct tinyjson_token *out, int max_tokens );
int tinyjson_parse_err( const char* json, int length, struct tinyjson_token *out, int max_tokens );
int tinyjson_get_integer( const char* json, const struct tinyjson_token* token );
long long tinyjson_get_integer64( const char* json, const struct tinyjson_token* token );
float tinyjson_get_float( const char* json, const struct tinyjson_token* token );

#ifdef __cplusplus
}
#endif

#endif
