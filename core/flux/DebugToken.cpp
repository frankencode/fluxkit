/*
 * Copyright (C) 2014 Frank Mertens.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include "Format.h"
#include "DebugToken.h"

namespace flux
{

void DebugToken::printTo(Stream *stream, ByteArray *text, int depth, int defaultScope)
{
	if (defaultScope == -1) defaultScope = scope();

	String indent(depth * 2, ' ');

	if (firstChild()) {
		Format(stream) << indent << ruleName() << "(" << i0() << ".." << i1() << ")=[" << nl;
	}
	else {
		Format format(stream);
		format << indent;
		if (scope() != defaultScope) format << scopeName() << "::";
		format << ruleName() << "=[" << text->copy(this) << "]" << nl;
	}

	for (Token *token = firstChild(); token; token = token->nextSibling())
		cast<DebugToken>(token)->printTo(stream, text, depth + 1, defaultScope);

	if (firstChild()) Format(stream) << indent << "]" << nl;
}

} // namespace flux