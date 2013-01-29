 /*
  * Copyright (C) 2007-2013 Frank Mertens.
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version
  * 2 of the License, or (at your option) any later version.
  */
#ifndef FTL_JSON_HPP
#define FTL_JSON_HPP

#include "AbnfCoreSyntax.hpp"
#include "Singleton.hpp"
#include "variant.hpp"
#include "Map.hpp"

namespace ftl
{

FTL_EXCEPTION(JsonException, Exception);

typedef Map<string, variant> JsonObject;

class Json: public AbnfCoreSyntax, public Singleton<Json>
{
public:
	variant parse(string text);

private:
	friend class Singleton<Json>;

	Json();

	variant parseText(ByteArray *text, Token *token);
	variant parseObject(ByteArray *text, Token *token);
	variant parseArray(ByteArray *text, Token *token);
	Pair<string, variant> parseMember(ByteArray *text, Token *token);
	variant parseValue(ByteArray *text, Token *token);
	string parseString(ByteArray *text, Token *token);
	double parseNumber(ByteArray *text, Token *token);

	int int_;
	int frac_;
	int exp_;
	int number_;
	int string_;
	int value_;
	int member_;
	int object_;
	int list_;
	int text_;

	int false_, true_, null_;
};

inline Json *json() { return Json::instance(); }

} // namespace ftl

#endif // FTL_JSON_HPP