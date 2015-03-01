/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUX_IOMONITOR_H
#define FLUX_IOMONITOR_H

#include <poll.h>
#include <flux/SystemStream>
#include <flux/Array>
#include <flux/Map>

namespace flux {

/** \brief I/O event
  */
class IoEvent: public Object
{
public:
    enum Activity {
        ReadyRead = POLLIN,
        ReadyWrite = POLLOUT,
        ReadyAccept = ReadyRead
    };

    inline SystemStream *stream() const { return stream_; }
    inline int activity() const { return activity_; }

private:
    friend class IoMonitor;

    inline static Ref<IoEvent> create(int index, SystemStream *stream, int activity) {
        return new IoEvent(index, stream, activity);
    }

    IoEvent(int index, SystemStream *stream, int activity):
        index_(index),
        stream_(stream),
        activity_(activity)
    {}

    int index_;
    Ref<SystemStream> stream_;
    int activity_;
};

typedef Array<IoEvent *> IoActivity;

/** \brief Register and wait for I/O events
  */
class IoMonitor: public Object
{
public:
    static Ref<IoMonitor> create(int maxCount = 0);

    IoEvent *addEvent(SystemStream *stream, int activity);
    void removeEvent(IoEvent *event);

    Ref<IoActivity> wait(double timeout = -1);

private:
    typedef struct pollfd PollFd;
    typedef Array<PollFd> Fds;
    typedef Map<int, Ref<IoEvent> > Events;

    IoMonitor(int maxCount);

    Ref<Fds> fds_;
    Ref<Events> events_;
};

} // namespace flux

#endif // FLUX_IOMONITOR_H
