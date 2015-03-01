/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include <flux/ProcessFactory>
#include <flux/IoMonitor>
#include "JobServer.h"

namespace fluxmake {

JobServer::JobServer(JobChannel *requestChannel, JobChannel *replyChannel, bool paranoid):
    requestChannel_(requestChannel),
    replyChannel_(replyChannel),
    paranoid_(paranoid)
{
    Thread::start();
}

JobServer::~JobServer()
{
    requestChannel_->pushFront(0);
    wait();
}

void JobServer::run()
{
    Ref<ProcessFactory> factory = ProcessFactory::create();
    factory->setIoPolicy(Process::CloseInput|Process::ForwardOutput|Process::ForwardError);

    for (Ref<Job> job; requestChannel_->popFront(&job);) {
        factory->setCommand(job->command_);
        Ref<Process> process = factory->produce();
        Ref<StringList> outputList = StringList::create();
        Ref<ByteArray> buffer = ByteArray::create(0x1000);
        Ref<IoMonitor> monitor = IoMonitor::create(2);
        Ref<IoEvent> outReady = monitor->addEvent(process->out(), IoEvent::ReadyRead);
        Ref<IoEvent> errReady = monitor->addEvent(process->err(), IoEvent::ReadyRead);
        bool abort = false;
        for (Ref<IoActivity> activity; activity = monitor->wait();) {
            for (int i = 0; i < activity->count(); ++i) {
                int fill = activity->at(i)->stream()->read(buffer);
                outputList->append(ByteRange(buffer, 0, fill));
                if (paranoid_ && activity->at(i) == errReady)
                    abort = true;
            }
        }
        job->outputText_ = outputList->join();
        job->status_ = process->wait();
        replyChannel_->pushBack(job);
        if (abort) {
            job->status_ = -1;
            break;
        }
    }
}

} // namespace fluxmake
