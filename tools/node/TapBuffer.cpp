/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include <flux/Thread>
#include <flux/str>
#include "TapBuffer.h"

namespace fluxnode {

Ref<TapBuffer> TapBuffer::open(Stream *stream, String prefix)
{
    return new TapBuffer(stream, prefix);
}

TapBuffer::TapBuffer(Stream *stream, String prefix):
    LineBuffer(stream),
    prefix_(prefix)
{}

String TapBuffer::prefix() const
{
    return "(" + inum(uint64_t(thread()->id()), 62) + ") " + prefix_;
}

void TapBuffer::write(const ByteArray *buf)
{
    Ref<StringList> parts;
    int i = 0, i0 = 0;
    for (int n = buf->count(); i < n; ++i) {
        uint8_t ch = buf->byteAt(i);
        if (ch <= 31 || 127 <= ch) {
            if (!parts) parts = StringList::create();
            if (i0 < i) parts->append(buf->copy(i0, i));
            i0 = i + 1;
            if (ch == 0x08) parts->append("\\b");
            else if (ch == 0x09) parts->append("\\t");
            else if (ch == 0x0A) parts->append("\n");
            else if (ch == 0x0D) parts->append("");
            else if (ch == 0x0C) parts->append("\\f");
            else {
                String s = "\\xXX";
                const char *hex = "0123456789ABCDEF";
                s->at(s->count() - 2) = hex[ch / 16];
                s->at(s->count() - 1) = hex[ch % 16];
                parts->append(s);
            }
        }
    }
    if (parts) {
        if (i0 < i) parts->append(buf->copy(i0, i));
        LineBuffer::write(parts->join());
    }
    else {
        LineBuffer::write(buf);
    }
}

} // namespace fluxnode
