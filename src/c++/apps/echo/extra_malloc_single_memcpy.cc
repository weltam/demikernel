// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "extra_malloc_single_memcpy.hh"
#include <dmtr/libos/mem.h>
#include <assert.h>
#include <string>
#include <iostream>
#define DMTR_PROFILE
malloc_baseline_single_memcpy::malloc_baseline_single_memcpy(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::MALLOCBASELINE, field_size, message_type),
    in_p(NULL),
    out_p(NULL),
    allocated_string("a", field_size)
{
#ifdef DMTR_PROFILE
    dmtr_new_latency(&encode_memcpy_latency, "encode Memcpy latency");
    dmtr_new_latency(&decode_memcpy_latency, "decode Memcpy latency");
#endif
    dmtr_malloc(&in_p, field_size);
    assert(in_p != NULL);

    dmtr_malloc(&out_p, field_size);
    assert(out_p != NULL);
}
malloc_baseline_single_memcpy::~malloc_baseline_single_memcpy() {
    if (in_p != NULL) {
        free(in_p);
    }

    if (out_p != NULL) {
        free(out_p);
    }
}

void malloc_baseline_single_memcpy::serialize_message(dmtr_sgarray_t &sga, void *context) {
    sga.sga_numsegs = 1;
#ifdef DMTR_PROFILE
    auto start_memcpy = rdtsc();
#endif
    memcpy(out_p, allocated_string.c_str(), sga.sga_segs[0].sgaseg_len);
#ifdef DMTR_PROFILE
    dmtr_record_latency(encode_memcpy_latency, rdtsc() - start_memcpy);
#endif
}

void malloc_baseline_single_memcpy::deserialize_message(dmtr_sgarray_t &sga) {
    assert(sga.sga_numsegs == 1);
#ifdef DMTR_PROFILE
    auto start_memcpy = rdtsc();
#endif
    memcpy(in_p, allocated_string.c_str(), sga.sga_segs[0].sgaseg_len);
#ifdef DMTR_PROFILE
    dmtr_record_latency(decode_memcpy_latency, rdtsc() - start_memcpy);
#endif
}

void malloc_baseline_single_memcpy::print_counters() {
#ifdef DMTR_PROFILE
    dmtr_dump_latency(stderr, encode_memcpy_latency);
    dmtr_dump_latency(stderr, decode_memcpy_latency);
#endif
}
