/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include <flux/System>
#include "JobServer.h"
#include "JobScheduler.h"

namespace fluxmake {

Ref<JobScheduler> JobScheduler::create(int concurrency, bool paranoid)
{
    return new JobScheduler(concurrency, paranoid);
}

JobScheduler::JobScheduler(int concurrency, bool paranoid)
    : concurrency_((concurrency > 0) ? concurrency : System::concurrency()),
      paranoid_(paranoid),
      requestChannel_(JobChannel::create()),
      replyChannel_(JobChannel::create()),
      started_(false),
      status_(0),
      totalCount_(0),
      finishCount_(0)
{}

void JobScheduler::start()
{
    if (started_) return;
    started_ = true;
    serverPool_ = ServerPool::create();
    for (int i = 0; i < concurrency_; ++i)
        serverPool_->pushBack(JobServer::start(requestChannel_, replyChannel_, paranoid_));
}

void JobScheduler::schedule(Job *job)
{
    requestChannel_->pushBack(job);
    ++totalCount_;
}

bool JobScheduler::collect(Ref<Job> *completedJob)
{
    start();
    if ((finishCount_ == totalCount_) || !serverPool_) {
        *completedJob = 0;
        return false;
    }

    Ref<Job> job = replyChannel_->popFront();
    *completedJob = job;
    if (job->status() != 0) {
        status_ = job->status();
        serverPool_ = 0;
    }
    ++finishCount_;

    return true;
}

} // namespace fluxmake
