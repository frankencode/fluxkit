/*
 * Copyright (C) 2014 Frank Mertens.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef FLUX_ARGUMENTS_H
#define FLUX_ARGUMENTS_H

#include "String.h"

namespace flux
{

class YasonObject;

class Arguments: public Object
{
public:
	static Ref<Arguments> parse(int argc, char **argv);
	Ref<YasonObject> read(YasonObject *prototype) const;

	inline StringList *options() const { return options_; }
	inline StringList *items() const { return items_; }
	inline String execPath() const { return execPath_; }
	inline String toolName() const { return execPath_->fileName(); }

private:
	Arguments(int argc, char **argv);

	Ref<StringList> options_;
	Ref<StringList> items_;
	String execPath_;
};

} // namespace flux

#endif // FLUX_ARGUMENTS_H