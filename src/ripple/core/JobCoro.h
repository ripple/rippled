//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef RIPPLE_CORE_JOBCORO_H_INCLUDED
#define RIPPLE_CORE_JOBCORO_H_INCLUDED

#include <ripple/basics/win32_workaround.h>
#include <ripple/core/Job.h>
#include <ripple/basics/LocalValue.h>
#include <boost/coroutine/all.hpp>
#include <condition_variable>
#include <string>
#include <memory>
#include <mutex>

namespace ripple {

class JobQueue;

namespace detail {

struct JobCoro_create_t { };

} // detail

/** Coroutines must run to completion. */
class JobCoro : public std::enable_shared_from_this<JobCoro>
{
private:
    detail::LocalValues lvs_;
    JobQueue& jq_;
    JobType type_;
    std::string name_;
    bool running_;
    std::mutex mutex_;
    std::mutex mutex_run_;
    std::condition_variable cv_;
    boost::coroutines::asymmetric_coroutine<void>::pull_type coro_;
    boost::coroutines::asymmetric_coroutine<void>::push_type* yield_;
#ifndef NDEBUG
    bool finished_ = false;
#endif

public:
    // Private: Used in the implementation
    template <class F>
    JobCoro(detail::JobCoro_create_t, JobQueue&, JobType,
        std::string const&, F&&);

    // Not copy-constructible or assignable
    JobCoro(JobCoro const&) = delete;
    JobCoro& operator= (JobCoro const&) = delete;

    ~JobCoro();

    /** Suspend coroutine execution.
        Effects:
          The coroutine's stack is saved.
          The associated Job thread is released.
        Note:
          The associated Job function returns.
          Undefined behavior if called consecutively without a corresponding post.
    */
    void yield() const;

    /** Schedule coroutine execution.
        Effects:
          Returns immediately.
          A new job is scheduled to resume the execution of the coroutine.
          When the job runs, the coroutine's stack is restored and execution
            continues at the beginning of coroutine function or the statement
            after the previous call to yield.
        Undefined behavior if called consecutively without a corresponding yield.
    */
    void post();

    /** Waits until coroutine returns from the user function. */
    void join();
};

} // ripple

#endif
