// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef DMTR_LIBOS_TIMER_QUEUE_HH_IS_INCLUDED
#define DMTR_LIBOS_BASIC_QUEUE_HH_IS_INCLUDED

#include "io_queue.hh"

#include <dmtr/types.h>
#include <memory>
#include <queue>
#include <unordered_map>
#include <boost/chrono.hpp>

namespace dmtr {

class timer_queue : public io_queue
{
    private: dmtr::io_queue::timer my_timer;
    private: std::unique_ptr<task::thread_type> my_push_thread;
    private: std::unique_ptr<task::thread_type> my_pop_thread;
    private: bool my_good_flag;
    private: bool on;

    private: timer_queue(int qd);
    public: static int new_object(std::unique_ptr<io_queue> &q_out, int qd);

    // will service the push thread when push is called
    public: virtual int push(dmtr_qtoken_t qt, const dmtr_sgarray_t &sga);
    public: virtual int push_tick(dmtr_qtoken_t qt, const boost::chrono::nanoseconds expiry);
    public: virtual int pop(dmtr_qtoken_t qt);
    public: virtual int poll(dmtr_qresult_t &qr_out, dmtr_qtoken_t qt);
    public: virtual int drop(dmtr_qtoken_t qt);
    public: virtual int close();
    
    private: bool good() const {
        return my_good_flag;
    }

    private: void start_threads();
    private: int push_thread(task::thread_type::yield_type &yield, task::thread_type::queue_type &tq);
    private: int pop_thread(task::thread_type::yield_type &yield, task::thread_type::queue_type &tq);
    
};

} // namespace dmtr

#endif /* DMTR_LIBOS_TIMER_QUEUE_HH_IS_INCLUDED */
