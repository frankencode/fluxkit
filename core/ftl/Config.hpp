 /*
  * Copyright (C) 2007-2013 Frank Mertens.
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version
  * 2 of the License, or (at your option) any later version.
  */
#ifndef FTL_CONFIG_HPP
#define FTL_CONFIG_HPP

#include "Wire.hpp"

namespace ftl
{

FTL_STD_EXCEPTION(ConfigException);

class Config: public WireObject
{
public:
	static hook<Config> create();

	void read(string path);
	void read(int argc, char **argv);

	StringList *options() const;
	StringList *arguments() const;
	string path() const;

	inline bool flag(const char *name) {
		bool h = false;
		lookup(name, &h);
		return h;
	}

private:
	Config() {}

	hook<WireObject> object_;
	hook<StringList> options_;
	hook<StringList> arguments_;
	string path_;
};

} // namespace ftl

#endif // FTL_CONFIG_HPP