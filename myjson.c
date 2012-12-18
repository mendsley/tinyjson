/**
 * Copyright 2011 Matthew Endsley. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 *
 * Based on js0n implementation at:
 * https://github.com/quartzjer/js0n/blob/master/js0n.c
 */

#include <stdint.h>
#include <string.h>

#include "myjson.h"

enum opcode
{
	op_loop,
	op_bad,
	op_up,
	op_down,
	op_quote_up,
	op_quote_down,
	op_escape,
	op_unescape,
	op_bare,
	op_unbare,
	op_utf8_2,
	op_utf8_3,
	op_utf8_4,
	op_utf8_continue,
};

typedef uint8_t state[256];

static state st_struct;
static state st_bare;
static state st_string;
static state st_utf8cont;
static state st_escape;

void myjson_init()
{
	int ii;

	memset(st_struct, op_bad, sizeof(st_struct));
	memset(st_bare, op_bad, sizeof(st_bare));
	memset(st_string, op_bad, sizeof(st_string));
	memset(st_utf8cont, op_bad, sizeof(st_utf8cont));
	memset(st_escape, op_bad, sizeof(st_escape));

	st_struct['\t'] = st_struct[' '] = st_struct['\r'] = st_struct['\n'] = op_loop;
	st_struct[':'] = st_struct[','] = op_loop;
	st_struct['"'] = op_quote_up;
	st_struct['['] = st_struct['{'] = op_up;
	st_struct[']'] = st_struct['}'] = op_down;
	st_struct['-'] = op_bare;
	for (ii = '0'; ii <= '9'; ++ii) // 0-9
		st_struct[ii] = op_bare;
	st_struct['t'] = st_struct['f'] = st_struct['n'] = op_bare; // true, false, null

	for (ii = 32; ii <= 126; ++ii) // could be more pendantic/validation checking
		st_bare[ii] = op_loop;
	st_bare['\t'] = st_bare[' '] = st_bare['\r'] = st_bare['\n'] = op_unbare;
	st_bare[','] = st_bare[']'] = st_bare['}'] = op_unbare;

	for (ii = 32; ii <= 126; ++ii)
		st_string[ii] = op_loop;
	st_string['\\'] = op_escape;
	st_string['"'] = op_quote_down;
	for (ii = 192; ii <= 223; ++ii)
		st_string[ii] = op_utf8_2;
	for (ii = 224; ii <= 239; ++ii)
		st_string[ii] = op_utf8_3;
	for (ii = 240; ii <= 247; ++ii)
		st_string[ii] = op_utf8_4;

	for (ii = 128; ii <= 191; ++ii)
		st_utf8cont[ii] = op_utf8_continue;

	st_escape['"'] = st_escape['\\'] = st_escape['b'] = op_unescape;
	st_escape['f'] = st_escape['n'] = st_escape['r'] = op_unescape;
	st_escape['t'] = st_escape['u'] = op_unescape;
}

#define CHECK() if (out) { if (current == out_end) { return max_tokens; } } else { current = dummy; }
#define PUSH(i) CHECK(); if (depth == 1) current->start = (uint16_t)(((cur+i) - (const unsigned char*)json))
#define CAP(i) CHECK(); if (depth == 1) (ntokens++),(current++)->length = (uint16_t)((cur+i) - ((const unsigned char*)json + current->start) + 1)

int myjson_parse( const char* json, int length, struct myjson_token *out, int max_tokens )
{
	int err = myjson_parse_err(json, length, out, max_tokens);
	if (err == -1)
		err = 0;
	return err;
}

int myjson_parse_err( const char* json, int length, struct myjson_token *out, int max_tokens )
{
	const unsigned char* cur;
	const unsigned char* end;
	struct myjson_token* out_end  = out + max_tokens;
	uint8_t (*st)[256] = &st_struct;
	int utf8_remain = 0;
	int depth = 0;
	struct myjson_token dummy[2];
	struct myjson_token* current = out;
	int ntokens = 0;

	end = (unsigned char*)json+length;
	for (cur = (unsigned char*)json; cur < end; ++cur)
	{
		const enum opcode opcode = (*st)[*cur];
		switch (opcode)
		{
		case op_bad:
			return -1;

		case op_up:
			PUSH(0);
			if (depth == 1)
			{
				current->type = (*cur) == '[' ? MYJSON_TOKEN_ARRAY : MYJSON_TOKEN_OBJECT;
			}
			++depth;
			break;

		case op_down:
			--depth;
			CAP(0);
			break;

		case op_quote_up:
			PUSH(1);
			if (depth == 1)
			{
				current->type = MYJSON_TOKEN_STRING;
			}
			st = &st_string;
			break;

		case op_quote_down:
			st = &st_struct;
			CAP(-1);
			break;

		case op_escape:
			st = &st_escape;
			break;

		case op_unescape:
			st = &st_string;
			break;

		case op_bare:
			PUSH(0);
			if (depth == 1)
			{
				current->type = MYJSON_TOKEN_LITERAL;
			}
			st = &st_bare;
			break;

		case op_unbare:
			CAP(-1);
			--cur;
			st = &st_struct;
			break;

		case op_utf8_2:
			utf8_remain = 1;
			st = &st_utf8cont;
			break;

		case op_utf8_3:
			utf8_remain = 2;
			break;

		case op_utf8_4:
			utf8_remain = 3;
			break;

		case op_utf8_continue:
			if (--utf8_remain == 0)
				st = &st_struct;
			break;

		case op_loop:
			break;
		}
	}

	return (int)ntokens;
}

int myjson_get_integer( const char* json, const struct myjson_token* token )
{
	int result = 0;
	const char* value = json + token->start;
	const char* end = value + token->length;

	if (*value == '-')
		++value;

	while (value < end)
	{
		result *= 10;
		result += (*value) - '0';
		++value;
	}

	if (json[token->start] == '-')
		result = -result;

	return result;
}

long long myjson_get_integer64( const char* json, const struct myjson_token* token )
{
	long long result = 0;
	const char* value = json + token->start;
	const char* end = value + token->length;

	if (*value == '-')
		++value;

	while (value < end)
	{
		result *= 10;
		result += (*value) - '0';
		++value;
	}

	if (json[token->start] == '-')
		result = -result;

	return result;
}