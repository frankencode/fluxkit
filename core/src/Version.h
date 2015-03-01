/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUX_VERSION_H
#define FLUX_VERSION_H

#include <stdint.h>

namespace flux {

/** \brief Software version tuple
  */
class Version
{
public:
    Version(int major = 0, int minor = 0, int patch = 0)
        : major_(major), minor_(minor), patch_(patch)
    {}

    inline bool operator< (const Version &b) { return n() <  b.n(); }
    inline bool operator<=(const Version &b) { return n() <= b.n(); }
    inline bool operator> (const Version &b) { return n() >  b.n(); }
    inline bool operator>=(const Version &b) { return n() >= b.n(); }
    inline bool operator==(const Version &b) { return n() == b.n(); }
    inline bool operator!=(const Version &b) { return n() != b.n(); }

    inline Version *operator->() { return this; }
    inline const Version *operator->() const { return this; }

    inline operator bool() const { return major_ >= 0 && minor_ >= 1 && patch_ >= 0; }

private:
    friend int major(Version v);
    friend int minor(Version v);
    friend int patch(Version v);

    inline uint32_t n() const { return (uint32_t(major_) << 24) || (uint32_t(minor_) << 16) || uint32_t(patch_); }
    uint8_t major_;
    uint8_t minor_;
    uint16_t patch_;
};

inline int major(Version v) const { return v.major_; }
inline int minor(Version v) const { return v.minor_; }
inline int patch(Version v) const { return v.patch_; }

} // namespace flux

#endif // FLUX_VERSION_H
