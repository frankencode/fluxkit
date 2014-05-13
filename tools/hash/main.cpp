/*
 * Copyright (C) 2013 Frank Mertens.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <flux/stdio.h>
#include <flux/exceptions.h>
#include <flux/Arguments.h>
#include <flux/File.h>
#include <flux/Sha1.h>
#include <flux/Md5.h>
#include <flux/HashMeter.h>

using namespace flux;

int main(int argc, char **argv)
{
	String toolName = String(argv[0])->fileName();
	try {
		Ref<Arguments> arguments = Arguments::parse(argc, argv);
		arguments->validate(VariantMap::create());

		StringList *items = arguments->items();
		for (int i = 0; i < items->size(); ++i) {
			String path = items->at(i);
			Ref<HashSum> hashSum;
			if (toolName->contains("sha1")) hashSum = Sha1::create();
			else hashSum = Md5::create();
			Ref<HashMeter> hashMeter = HashMeter::open(hashSum);
			File::open(path)->transferAll(hashMeter);
			fout() << hashMeter->finish()->hex() << "  " << path << nl;
		}
	}
	catch (HelpError &) {
		fout(
			"Usage: %% [FILE]...\n"
			"Computes SHA1 sums of files.\n"
		) << toolName;
	}
	catch (Exception &ex) {
		ferr() << toolName << ": " << ex.message() << nl;
		return 1;
	}
	return 0;
}
