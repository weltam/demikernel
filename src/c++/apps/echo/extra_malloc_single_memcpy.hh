// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.


#ifndef ECHO_MALLOCBASELINE_SINGLE_MEMCPY_H_
#define ECHO_MALLOCBASELINE_SINGLE_MEMCPY_H_

#include <dmtr/sga.h>
#include "message.hh"
#include <string>
#include <dmtr/latency.h>


class malloc_baseline_single_memcpy : public echo_message
{
    private: void * in_p;
    private: void * out_p;
    private: std::string allocated_string;
    private: dmtr_latency_t* encode_memcpy_latency;
    private: dmtr_latency_t* decode_memcpy_latency;


    public: malloc_baseline_single_memcpy(uint32_t field_size, string message_type);
    public: ~malloc_baseline_single_memcpy();
    public: virtual void serialize_message(dmtr_sgarray_t &sga, void *context);
    public: virtual void deserialize_message(dmtr_sgarray_t &sga);
    public: virtual void print_counters();
};

#endif
