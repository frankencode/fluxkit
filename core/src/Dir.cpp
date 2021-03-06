/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include <sys/stat.h> // mkdir
#include <flux/File>
#include <flux/FileStatus>
#include <flux/exceptions>
#include <flux/Dir>

namespace flux {

Ref<Dir> Dir::tryOpen(String path)
{
    DIR *dir = ::opendir(path);
    if (dir) return new Dir(path, dir);
    return 0;
}

Dir::Dir(String path, DIR *dir)
    : path_(path),
      dir_(dir)
{
    if (!dir_) {
        dir_ = ::opendir(path_);
        if (!dir_) FLUX_SYSTEM_RESOURCE_ERROR(errno, path);
    }
}

Dir::~Dir()
{
    if (::closedir(dir_) == -1)
        FLUX_SYSTEM_DEBUG_ERROR(errno);
}

String Dir::path() const { return path_; }

String Dir::path(String name) const
{
    if (path_ == ".") return name;
    if (path_ == "/") return "/" + name;
    return path_ + "/" + name;
}

bool Dir::read(String *name)
{
    struct dirent buf;
    struct dirent *result;
    memclr(&buf, sizeof(buf));
    int ret = ::readdir_r(dir_, &buf, &result);
    if (ret != 0) FLUX_SYSTEM_DEBUG_ERROR(ret);
    if (result)
        *name = buf.d_name;
    return result;
}

bool Dir::access(String path, int flags)
{
    return access(path, flags);
}

bool Dir::exists(String path)
{
    return File::exists(path) && (File::status(path)->type() == File::Directory);
}

int Dir::count(String path)
{
    Ref<Dir> dir = tryOpen(path);
    if (!dir) return 0;
    int n = 0;
    for (String name; dir->read(&name);) {
        if (name != "." && name != "..")
            ++n;
    }
    return n;
}

void Dir::create(String path, int mode)
{
    if (::mkdir(path, mode) == -1)
        FLUX_SYSTEM_RESOURCE_ERROR(errno, path);
}

void Dir::establish(String path, int mode)
{
    Ref<StringList> missingDirs = StringList::create();
    while ((path != "") && (path != "/")) {
        if (Dir::exists(path)) break;
        missingDirs->pushFront(path);
        path = path->reducePath();
    }
    while (missingDirs->count() > 0)
        Dir::create(missingDirs->popFront(), mode);
}

void Dir::unlink(String path)
{
    if (::rmdir(path) == -1)
        FLUX_SYSTEM_RESOURCE_ERROR(errno, path);
}

} // namespace flux
