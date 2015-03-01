/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include <flux/Format>
#include <flux/Variant>
#include <flux/Singleton>
#include <flux/String>

namespace flux {

String::String(): Super(Singleton<ByteArray>::instance()) {}

String::String(const char *data, int size)
{
    if (size < 0 && data) size = strlen(data);
    if (size <= 0) Super::set(Singleton<ByteArray>::instance());
    else Super::set(ByteArray::copy(data, size));
}

String::String(const Variant &b):
    Super(cast<ByteArray>(b))
{
    if (!Super::get()) Super::set(Singleton<ByteArray>::instance());
}

String::String(const Format &b)
{
    *this = *ByteArray::join(b);
}

String::String(Ref<StringList> parts) { *this = join(parts); }

} // namespace flux
