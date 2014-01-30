/**
 * Copyright 2011 Matthew Endsley. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 *
 * Based on js0n implementation at:
 * https://github.com/quartzjer/js0n/blob/master/js0n.c
 */

#ifndef TINY_JSONP_H
#define TINY_JSONP_H

#include <string>

struct JsonKeyValue;

class JsonObject {
public:
	struct Impl;

	JsonObject();
	~JsonObject();

	static bool parse(const char* json, int length, JsonObject* object);

	bool isValid() const;
	bool isObject() const;
	bool isArray() const;

	std::string asString() const;
	void asCStr(const char** str, int* length) const;
	bool asBool() const;
	int asInt() const;
	long long asInt64() const;
	float asFloat() const;

	int numChildren() const;
	JsonObject getChild(const char* key) const;
	JsonObject getChild(int index) const;
	JsonKeyValue getObjectChild(int index) const;

private:
	bool shouldDelete;
	const Impl *impl;
};

struct JsonKeyValue {
	std::string key;
	JsonObject value;
};

#endif
