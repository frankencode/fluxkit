/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include <flux/File>
#include <flux/toki/Document>

namespace flux {
namespace toki {

Ref<Document> Document::load(String path)
{
    return Document::create(File::open(path)->map(), path);
}

void Document::save()
{
    File::save(path_, text_);
}

Document::Document(String text, String path)
    : path_(path),
      text_(text),
      spans_(Spans::create())
{}

}} // namespace flux::toki
