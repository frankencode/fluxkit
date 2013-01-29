 /*
  * Copyright (C) 2007-2013 Frank Mertens.
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version
  * 2 of the License, or (at your option) any later version.
  */
#ifndef FTL_MEMORY_HPP
#define FTL_MEMORY_HPP

#include "ThreadLocalSingleton.hpp"

namespace ftl
{

class Memory: public Instance, public ThreadLocalSingleton<Memory>
{
public:
	static void *allocate(size_t size);
	static void free(void *data);

	static size_t pageSize();

	void *operator new(size_t size);
	void operator delete(void *data, size_t size);

private:
	friend class ThreadLocalSingleton<Memory>;

	Memory();

	class BucketHeader;

	size_t pageSize_;
	BucketHeader *bucket_;
};

inline Memory *memory() { return Memory::instance(); }

} // namespace ftl

#endif // FTL_MEMORY_HPP