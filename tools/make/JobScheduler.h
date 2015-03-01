/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUXMAKE_JOBSCHEDULER_H
#define FLUXMAKE_JOBSCHEDULER_H

#include <flux/Thread>
#include "Job.h"

namespace fluxmake {

using namespace flux;

class JobScheduler: public Object
{
public:
    static Ref<JobScheduler> create(int concurrency = -1, bool paranoid = false);

    inline int concurrency() const { return concurrency_; }

    void start();
    void schedule(Job *job);
    bool collect(Ref<Job> *completedJob);

    inline int status() const { return status_; }
    inline int totalCount() const { return totalCount_; }
    inline int finishCount() const { return finishCount_; }

private:
    JobScheduler(int concurrency, bool paranoid);

    int concurrency_;
    bool paranoid_;

    Ref<JobChannel> requestChannel_;
    Ref<JobChannel> replyChannel_;

    typedef Queue< Ref<JobServer> > ServerPool;
    Ref<ServerPool> serverPool_;

    bool started_;
    int status_;
    int totalCount_;
    int finishCount_;
};

} // namespace fluxmake

#endif // FLUXMAKE_JOBSCHEDULER_H
