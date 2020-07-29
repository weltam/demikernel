// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <dmtr/libos/timer_queue.hh>

#include <dmtr/annot.h>
#include <dmtr/libos/mem.h>
#include <iostream>
#include <boost/chrono.hpp>

dmtr::timer_queue::timer_queue(int qd) :
    io_queue(TIMER_Q, qd),
    my_timer(timer()),
    my_good_flag(true)
{
}

int dmtr::timer_queue::new_object(std::unique_ptr<io_queue> &q_out, int qd) {
    auto * const q = new timer_queue(qd);
    DMTR_NOTNULL(ENOMEM, q);
    q_out = std::unique_ptr<io_queue>(q);
    q->start_threads();
    return 0;
}

int
dmtr::timer_queue::push(dmtr_qtoken_t qt, const dmtr_sgarray_t &sga) {
    return 0;
}

int
dmtr::timer_queue::push_tick(dmtr_qtoken_t qt, const int expiry)
{
    DMTR_TRUE(EINVAL, good());
    DMTR_NOTNULL(EINVAL, my_push_thread);
    on = true;

    DMTR_OK(new_task(qt, DMTR_OPC_PUSH, expiry));
    my_push_thread->enqueue(qt);
    my_push_thread->service();
    return 0;
}

int dmtr::timer_queue::stop_timer(dmtr_qtoken_t qt) {
    on = false;
    return 0;
}

int dmtr::timer_queue::push_thread(task::thread_type::yield_type &yield, task::thread_type::queue_type &tq) {
    while (good())  {
        while (tq.empty()) {
            yield();
        }

        auto qt = tq.front();
        tq.pop();
        task *t;
        DMTR_OK(get_task(t, qt));

        const int *expiry = NULL;
        DMTR_TRUE(EINVAL, t->arg(expiry));

        my_timer.set_expiry(*expiry);
        DMTR_OK(t->complete(0));
    }

    return 0;
}

int dmtr::timer_queue::pop(dmtr_qtoken_t qt) {
    DMTR_TRUE(EINVAL, good());
    DMTR_NOTNULL(EINVAL, my_pop_thread);

    DMTR_OK(new_task(qt, DMTR_OPC_POP));
    my_pop_thread->enqueue(qt);
    return 0;
}

int dmtr::timer_queue::pop_thread(task::thread_type::yield_type &yield, task::thread_type::queue_type &tq) {
    while (good()) {
        while (tq.empty()) {
            yield();
        }


        auto qt = tq.front();
        tq.pop();
        task *t;
        DMTR_OK(get_task(t, qt));
       
        // yield when the timer hasn't expired
        // or timer is turned off 
        while (!(my_timer.has_expired() || !on)) {
            yield();
        }
    
        DMTR_OK(t->complete(0));
    }

    return 0;
}

int dmtr::timer_queue::poll(dmtr_qresult_t &qr_out, dmtr_qtoken_t qt) {
    DMTR_TRUE(EINVAL, good());

    task *t;
    DMTR_OK(get_task(t, qt));

    int ret;
    switch (t->opcode()) {
        default:
            return ENOTSUP;
        case DMTR_OPC_PUSH:
            ret = my_push_thread->service();
            break;
        case DMTR_OPC_POP:
            ret = my_pop_thread->service();
            break;
    }

    switch (ret) {
        default:
            std::cout << "ret: " << ret << ", on: " << on << ", expired: " << my_timer.has_expired() << std::endl;
            DMTR_FAIL(ret);
        case EAGAIN:
            break;
        case 0:
            DMTR_UNREACHABLE();
    }

    return t->poll(qr_out);
}

int dmtr::timer_queue::drop(dmtr_qtoken_t qt) {
    DMTR_TRUE(EINVAL, good());

    return io_queue::drop(qt);
}

int dmtr::timer_queue::close() {
    return 0;
}

void dmtr::timer_queue::start_threads() {
    my_push_thread.reset(new task::thread_type([=](task::thread_type::yield_type &yield, task::thread_type::queue_type &tq) {
        return push_thread(yield, tq);
    }));

    my_pop_thread.reset(new task::thread_type([=](task::thread_type::yield_type &yield, task::thread_type::queue_type &tq) {
        return pop_thread(yield, tq);
    }));
}



