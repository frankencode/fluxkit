/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include <flux/assert>
#include <flux/exceptions>
#include <flux/IoMonitor>

namespace flux {

Ref<IoMonitor> IoMonitor::create(int maxCount) { return new IoMonitor(maxCount); }

IoMonitor::IoMonitor(int maxCount):
    fds_(Fds::create(maxCount)),
    events_(Events::create())
{}

IoEvent *IoMonitor::addEvent(SystemStream *stream, int activity)
{
    FLUX_ASSERT(events_->count() < fds_->count());
    Ref<IoEvent> event = IoEvent::create(events_->count(), stream, activity);
    events_->insert(event->index_, event);
    PollFd *p = &fds_->at(event->index_);
    p->fd = stream->fd();
    p->events = activity;
    return event;
}

void IoMonitor::removeEvent(IoEvent *event)
{
    int i = event->index_;
    int n = events_->count();

    if (i != n - 1) {
        IoEvent *h = events_->value(n - 1);
        h->index_ = i;
        events_->establish(i, h);
        fds_->at(i) = fds_->at(n - 1);
    }

    events_->remove(n - 1);
}

Ref<IoActivity> IoMonitor::wait(double timeout)
{
    PollFd *fds = 0;
    if (events_->count() > 0) fds = fds_->data();
    int n = ::poll(fds, events_->count(), (timeout > 0) ? timeout * 1000 : timeout);
    if (n < 0) FLUX_SYSTEM_DEBUG_ERROR(errno);

    FLUX_ASSERT(n <= events_->count());

    Ref<IoActivity> activity = IoActivity::create(n);
    int j = 0;
    for (int i = 0; i < events_->count(); ++i) {
        if (fds_->at(i).revents != 0) {
            FLUX_ASSERT(j < n);
            activity->at(j) = events_->value(i);
            ++j;
        }
    }

    FLUX_ASSERT(j == n);

    return activity;
}

} // namespace flux
