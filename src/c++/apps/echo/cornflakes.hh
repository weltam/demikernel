// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#ifndef ECHO_CORNFLAKES_H_
#define ECHO_CORNFLAKES_H_

#include "message.hh"
#include <dmtr/sga.h>
#include <sys/types.h>

using namespace std;

class cornflakes_echo : public echo_message
{
    private: void *mmap_addr;
    private: size_t mmap_len;
    private: size_t mmap_available_len;
    private: void *recv_payload;

    public: cornflakes_echo(uint32_t field_size, string message_type);
    public: ~cornflakes_echo();

    public: virtual void serialize_message(dmtr_sgarray_t &sga, void *context);
    public: virtual void deserialize_message(dmtr_sgarray_t &sga);

    public: void init_ext_mem();

    public: virtual void print_counters() {}

    public: void * get_mmap_addr();
    public: size_t get_mmap_len();
    public: void set_mmap_available_len(size_t len);
};

#endif /* ECHO_CORNFLAKES_H_ */
