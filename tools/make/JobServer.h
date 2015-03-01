/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUXMAKE_JOBSERVER_H
#define FLUXMAKE_JOBSERVER_H

#include <flux/Thread>
#include <flux/Channel>
#include "Job.h"

namespace fluxmake {

class JobServer: public Thread
{
public:
    inline static Ref<JobServer> start(JobChannel *requestChannel, JobChannel *replyChannel, bool paranoid = false) {
        return new JobServer(requestChannel, replyChannel, paranoid);
    }

private:
    JobServer(JobChannel *requestChannel, JobChannel *replyChannel, bool paranoid);
    ~JobServer();
    virtual void run();

    Ref<JobChannel> requestChannel_;
    Ref<JobChannel> replyChannel_;
    bool paranoid_;
};

} // namespace fluxmake

#endif // FLUXMAKE_JOBSERVER_H
