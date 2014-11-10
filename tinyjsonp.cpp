/**
 * Copyright 2011 Matthew Endsley. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 *
 * Based on js0n implementation at:
 * https://github.com/quartzjer/js0n/blob/master/js0n.c
 */

#include "tinyjsonp.h"
#include "tinyjson.h"

#include <string.h>
#include <map>
#include <vector>
#include <string>

namespace {
typedef JsonObject::Impl Impl;

struct InternalString {
	const char* str;
	int len;
};

static bool operator<(const InternalString a, const InternalString b) {
	const int size = a.len > b.len ? b.len : a.len;
	const int cmp = strncmp(a.str, b.str, size);
	return cmp < 0 || (cmp == 0 && b.len > a.len);
}
}

struct JsonObject::Impl {
	typedef std::map<InternalString, Impl> map;
	typedef std::vector<Impl> vector;

	tinyjson_token_type type;
	InternalString value;
	vector arrChildren;
	map objChildren;
};

static bool parseImpl(const char* json, int length, tinyjson_token_type type, Impl* impl);

static bool parseObject(const char* json, const tinyjson_token* tokens, int ntokens, Impl* impl) {
	if (ntokens % 2)
		return false;

	for (int ii = 0; ii < ntokens; ii += 2) {
		if (tokens[ii].type != MYJSON_TOKEN_STRING)
			return false;

		const InternalString key = {json + tokens[ii].start, tokens[ii].length};
		Impl& child = impl->objChildren[key];
		if (!parseImpl(json + tokens[ii + 1].start, tokens[ii + 1].length, tokens[ii + 1].type, &child))
			return false;
	}

	return true;
}

static bool parseArray(const char* json, const tinyjson_token* tokens, int ntokens, Impl* impl) {
	impl->arrChildren.resize(ntokens);
	for (int ii = 0; ii < ntokens; ++ii)
		if (!parseImpl(json + tokens[ii].start, tokens[ii].length, tokens[ii].type, &impl->arrChildren[ii]))
			return false;

	return true;
}

static bool parseImpl(const char* json, int length, tinyjson_token_type type, Impl* impl) {
	impl->type = type;

	if (type == MYJSON_TOKEN_STRING || type == MYJSON_TOKEN_LITERAL) {
		impl->value.str = json;
		impl->value.len = length;
		return true;
	}

	int ntokens = tinyjson_parse_err(json, length, 0, 0);
	if (ntokens == -1)
		return false;
	if (ntokens == 0)
		return true;

	std::vector<tinyjson_token> tokens(ntokens);
	tinyjson_parse(json, length, &tokens[0], ntokens);

	switch (type) {
	case MYJSON_TOKEN_OBJECT:
		return parseObject(json, &tokens[0], ntokens, impl);

	case MYJSON_TOKEN_ARRAY:
		return parseArray(json, &tokens[0], ntokens, impl);

	case MYJSON_TOKEN_STRING:
	case MYJSON_TOKEN_LITERAL:
		break;
	}

	return false;
}


JsonObject::JsonObject()
	: impl(0)
	, shouldDelete(false)
{
}

JsonObject::~JsonObject() {
	if (shouldDelete)
		delete impl;
}

bool JsonObject::parse(const char* json, int length, JsonObject* object) {
	if (length && json[0] != '{')
		return false;

	Impl *impl = new Impl;
	if (!parseImpl(json, length, MYJSON_TOKEN_OBJECT, impl)) {
		delete impl;
		return false;
	}

	if (object->impl && object->shouldDelete)
		delete object->impl;

	object->impl = impl;
	object->shouldDelete = true;
	return true;
}

bool JsonObject::isValid() const {
	return impl != 0;
}

bool JsonObject::isObject() const {
	return impl && impl->type == MYJSON_TOKEN_OBJECT;
}

bool JsonObject::isArray() const {
	return impl && impl->type == MYJSON_TOKEN_ARRAY;
}

std::string JsonObject::asString() const {
	if (!impl || impl->type != MYJSON_TOKEN_STRING)
		return std::string();

	return std::string(impl->value.str, impl->value.len);
}

void JsonObject::asCStr(const char** str, int* length) const {
	if (!impl || impl->type != MYJSON_TOKEN_STRING) {
		*str = 0;
		*length = 0;
	} else {
		*str = impl->value.str;
		*length = impl->value.len;
	}
}

bool JsonObject::asBool() const {
	if (!impl || impl->type != MYJSON_TOKEN_LITERAL || impl->value.len < 1)
		return false;

	return impl->value.str[0] == 't';
}

int JsonObject::asInt() const {
	if (!impl || impl->type != MYJSON_TOKEN_LITERAL)
		return 0;

	tinyjson_token token;
	token.start = 0;
	token.length = impl->value.len;
	return tinyjson_get_integer(impl->value.str, &token);
}

long long JsonObject::asInt64() const {
	if (!impl || impl->type != MYJSON_TOKEN_LITERAL)
		return 0;

	tinyjson_token token;
	token.start = 0;
	token.length = impl->value.len;
	return tinyjson_get_integer64(impl->value.str, &token);
}

float JsonObject::asFloat() const {
	if (!impl || impl->type != MYJSON_TOKEN_LITERAL)
		return 0;

	tinyjson_token token;
	token.start = 0;
	token.length = impl->value.len;
	return tinyjson_get_float(impl->value.str, &token);
}

int JsonObject::numChildren() const {
	if (isArray())
		return (int)impl->arrChildren.size();
	if (isObject())
		return (int)impl->objChildren.size();
	return 0;
}

JsonObject JsonObject::getChild(const char* key) const {
	if (!isObject())
		return JsonObject();

	InternalString str = {key, (int)strlen(key)};
	const Impl::map::const_iterator it = impl->objChildren.find(str);
	if (it == impl->objChildren.end())
		return JsonObject();

	JsonObject child;
	child.impl = &(*it).second;
	child.shouldDelete = false;
	return child;
}

JsonObject JsonObject::getChild(int index) const {
	if (!isArray() || index < 0 || index >= (int)impl->arrChildren.size())
		return JsonObject();

	JsonObject child;
	child.impl = &impl->arrChildren[index];
	child.shouldDelete = false;
	return child;
}

JsonKeyValue JsonObject::getObjectChild(int index) const {
	if (isArray() || index < 0 || index >= (int)impl->objChildren.size())
		return JsonKeyValue();

	Impl::map::const_iterator it = impl->objChildren.begin();
	for (int ii = 0; ii < index; ++ii)
		++it;

	JsonKeyValue kv;
	kv.key.assign((*it).first.str, (*it).first.len);
	kv.value.impl = &(*it).second;
	kv.value.shouldDelete = false;
	return kv;
}
