/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUX_STRING_H
#define FLUX_STRING_H

#include <flux/strings>
#include <flux/ByteArray>
#include <flux/List>
#ifdef QT_CORE_LIB
#include <QString>
#include <QUrl>
#endif

namespace flux {

class Format;
class Variant;

/** \brief String convenience type: make references to ByteArray behave
  *        like fundamental types
  */
class String: public Ref<ByteArray>
{
public:
    typedef Ref<ByteArray> Super;

    String();
    explicit String(int size): Super(ByteArray::create(size)) {}
    String(int size, char zero): Super(ByteArray::create(size, zero)) {
        FLUX_ASSERT(0 <= zero);
    }
    String(const Ref<ByteArray> &b): Super(b) {}
    String(ByteArray *b): Super(b) {}
    String(const ByteArray *b): Super(const_cast<ByteArray *>(b)) {}
    String(const String &b): Super(b.Super::get()) {}
    String(const Format &b);
    explicit String(const Variant &b);
    String(Ref<StringList> parts);

    String(const char *data, int size = -1);

    inline static String join(const StringList *parts, String sep = "") { return ByteArray::join(parts, sep); }

    inline String &operator=(const String &b) {
        Super::set(b.Super::get());
        return *this;
    }

    inline String &operator=(const char *data) {
        String b(data);
        Super::set(b.Super::get());
        return *this;
    }

    inline String &operator+=(const String &b) {
        Ref<StringList> l = StringList::create();
        l->append(*this);
        l->append(b);
        *this = l->join();
        return *this;
    }

    inline String &operator+=(const char *b) {
        return *this += String(b);
    }

    inline operator char *() const { return Super::get()->chars(); }
    inline ByteArray &operator *() const { return *Super::get(); }

#ifdef QT_CORE_LIB
    inline operator QString() const {
        return QString::fromUtf8(Super::get()->chars(), Super::get()->count());
    }

    inline operator QUrl() const {
        return QUrl::fromLocalFile(QString::fromUtf8(Super::get()->chars(), Super::get()->count()));
    }
#endif

private:
    friend class ByteArray;
};

inline Ref<StringList> operator+(const String &a, const String &b) {
    Ref<StringList> l = StringList::create();
    l->append(a);
    l->append(b);
    return l;
}
inline Ref<StringList> operator+(Ref<StringList> &a, const String &b) { a->append(b); return a; }

inline bool operator==(const String &a, const String &b) { return a->count() == b->count() && strcmp(a->chars(), b->chars()) == 0; }
inline bool operator!=(const String &a, const String &b) { return a->count() != b->count() || strcmp(a->chars(), b->chars()) != 0; }
inline bool operator< (const String &a, const String &b) { return strcmp(a->chars(), b->chars()) <  0; }
inline bool operator> (const String &a, const String &b) { return strcmp(a->chars(), b->chars()) >  0; }
inline bool operator<=(const String &a, const String &b) { return strcmp(a->chars(), b->chars()) <= 0; }
inline bool operator>=(const String &a, const String &b) { return strcmp(a->chars(), b->chars()) >= 0; }

inline bool operator==(const char *a, const String &b) { return strcmp(a, b->chars()) == 0; }
inline bool operator!=(const char *a, const String &b) { return strcmp(a, b->chars()) != 0; }
inline bool operator< (const char *a, const String &b) { return strcmp(a, b->chars()) <  0; }
inline bool operator> (const char *a, const String &b) { return strcmp(a, b->chars()) >  0; }
inline bool operator<=(const char *a, const String &b) { return strcmp(a, b->chars()) <= 0; }
inline bool operator>=(const char *a, const String &b) { return strcmp(a, b->chars()) >= 0; }

inline bool operator==(char *a, const String &b) { return strcmp(a, b->chars()) == 0; }
inline bool operator!=(char *a, const String &b) { return strcmp(a, b->chars()) != 0; }
inline bool operator< (char *a, const String &b) { return strcmp(a, b->chars()) <  0; }
inline bool operator> (char *a, const String &b) { return strcmp(a, b->chars()) >  0; }
inline bool operator<=(char *a, const String &b) { return strcmp(a, b->chars()) <= 0; }
inline bool operator>=(char *a, const String &b) { return strcmp(a, b->chars()) >= 0; }

inline bool operator==(const String &a, const char *b) { return strcmp(a->chars(), b) == 0; }
inline bool operator!=(const String &a, const char *b) { return strcmp(a->chars(), b) != 0; }
inline bool operator< (const String &a, const char *b) { return strcmp(a->chars(), b) <  0; }
inline bool operator> (const String &a, const char *b) { return strcmp(a->chars(), b) >  0; }
inline bool operator<=(const String &a, const char *b) { return strcmp(a->chars(), b) <= 0; }
inline bool operator>=(const String &a, const char *b) { return strcmp(a->chars(), b) >= 0; }

inline bool operator==(const String &a, char *b) { return strcmp(a->chars(), b) == 0; }
inline bool operator!=(const String &a, char *b) { return strcmp(a->chars(), b) != 0; }
inline bool operator< (const String &a, char *b) { return strcmp(a->chars(), b) <  0; }
inline bool operator> (const String &a, char *b) { return strcmp(a->chars(), b) >  0; }
inline bool operator<=(const String &a, char *b) { return strcmp(a->chars(), b) <= 0; }
inline bool operator>=(const String &a, char *b) { return strcmp(a->chars(), b) >= 0; }

} // namespace flux

#endif // FLUX_STRING_H
