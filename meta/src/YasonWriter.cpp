/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include <flux/Format>
#include <flux/meta/YasonWriter>

namespace flux {
namespace meta {

Ref<YasonWriter> YasonWriter::create(Format format, String indent) { return new YasonWriter(format, indent); }

YasonWriter::YasonWriter(Format format, String indent):
    format_(format),
    indent_(indent)
{}

void YasonWriter::write(Variant value)
{
    writeValue(value, 0);
    format_ << nl;
}

void YasonWriter::writeValue(Variant value, int depth)
{
    if ( type(value) == Variant::IntType ||
         type(value) == Variant::BoolType ||
         type(value) == Variant::FloatType )
    {
        format_ << value;
    }
    else if (type(value) == Variant::StringType) {
        String s = value;
        if (s->contains("\""))
            s = s->replace("\"", "\\\"");
        s = s->escape();
        format_ << "\"" << s << "\"";
    }
    else if (type(value) == Variant::ListType) {
        writeList(value, depth);
    }
    else if (type(value) == Variant::ObjectType) {
        writeObject(value, depth);
    }
}

void YasonWriter::writeList(Variant value, int depth)
{
    if (itemType(value) == Variant::IntType)
        writeTypedList<int>(value, depth);
    else if (itemType(value) == Variant::BoolType)
        writeTypedList<bool>(value, depth);
    else if (itemType(value) == Variant::FloatType)
        writeTypedList<float>(value, depth);
    else if (itemType(value) == Variant::StringType)
        writeTypedList<String>(value, depth);
    else
        writeTypedList<Variant>(value, depth);
}

bool YasonWriter::isIdentifier(String name) const
{
    for (int i = 0, n = name->count(); i < n; ++i) {
        char ch = name->at(i);
        if (!(
            (('a' <= ch) && (ch <= 'z')) ||
            (('A' <= ch) && (ch <= 'Z')) ||
            (ch == '_') ||
            (ch == '-') ||
            (('0' <= ch) && (ch <= '9'))
        ))
            return false;
    }
    return true;
}

void YasonWriter::writeObject(Variant value, int depth)
{
    Ref<MetaObject> object = cast<MetaObject>(value);
    if (!object) {
        format_ << "null";
        return;
    }
    if (object->className() != "") {
        format_ << object->className();
        format_ << " ";
    }
    if (object->count() == 0 && !object->hasChildren()) {
        format_ << "{}";
        return;
    }
    format_ << "{\n";
    writeIndent(depth + 1); // FIXME: having an "IndentStream" would be nice
    for (int i = 0; i < object->count(); ++i) {
        String memberName = object->keyAt(i);
        Variant memberValue = object->valueAt(i);
        if (isIdentifier(memberName))
            format_ << memberName << ": ";
        else
            format_ << "\"" << memberName << "\": ";
        writeValue(memberValue, depth + 1);
        format_ << "\n";
        if (i < object->count() - 1)
            writeIndent(depth + 1);
    }
    if (object->hasChildren()) {
        if (object->count() > 0) writeIndent(depth + 1);
        MetaObjectList *children = object->children();
        for (int i = 0; i < children->count(); ++i) {
            writeObject(children->at(i), depth + 1);
            format_ << "\n";
            if (i < children->count() -1)
                writeIndent(depth + 1);
        }
    }
    writeIndent(depth);
    format_ << "}";
}

void YasonWriter::writeIndent(int depth)
{
    for (int i = 0; i < depth; ++i) format_ << indent_;
}

template<class T>
void YasonWriter::writeTypedList(Variant value, int depth)
{
    List<T> *list = cast< List<T> >(value);
    if (list->count() == 0) {
        format_ << "[]";
        return;
    }
    format_ << "[ ";
    for (int i = 0; i < list->count(); ++i) {
        writeValue(list->at(i), depth);
        if (i < list->count() - 1)
            format_ << ", ";
    }
    format_ << " ]";
}

}} // namespace flux::meta
