/*
 * Copyright (C) 2007-2014 Frank Mertens.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef FLUX_FLOATSYNTAX_H
#define FLUX_FLOATSYNTAX_H

#include "SyntaxDefinition.h"

namespace flux
{

template<class SubClass> class Singleton;

class FloatSyntax: public SyntaxDefinition
{
public:
	void read(float64_t *value, const ByteArray *text, Token *token) const;
	Ref<Token> read(float64_t *value, const ByteArray *text, int i = -1) const;

	inline int literal() const { return literal_; }

private:
	friend class Singleton<FloatSyntax>;

	FloatSyntax();

	int sign_;
	int integerPart_;
	int fractionPart_;
	int exponentSign_;
	int exponent_;
	int nan_;
	int infinite_;
	int literal_;
};

const FloatSyntax *floatSyntax();

} // namespace flux

#endif // FLUX_FLOATSYNTAX_H