// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "extra_malloc_no_str.hh"
#include <dmtr/libos/mem.h>
#include <assert.h>
#include <string>
#include <iostream>
#define DMTR_PROFILE
malloc_baseline_no_str::malloc_baseline_no_str(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::MALLOCBASELINE, field_size, message_type),
    in_p(NULL),
    out_p(NULL),
    out_string("a", field_size),
    allocated_out_string("a", field_size),
    allocated_in_string("a", field_size)
{
#ifdef DMTR_PROFILE
    dmtr_new_latency(&encode_string_latency, "encode String copy latency");
    dmtr_new_latency(&encode_malloc_latency, "encode Malloc latency");
    dmtr_new_latency(&encode_memcpy_latency, "encode Memcpy latency");
    dmtr_new_latency(&decode_string_latency, "decode String copy latency");
    dmtr_new_latency(&decode_malloc_latency, "decode Malloc latency");
    dmtr_new_latency(&decode_memcpy_latency, "decode Memcpy latency");
#endif
}

void malloc_baseline_no_str::serialize_message(dmtr_sgarray_t &sga) {
    if (out_p != NULL) {
        free(out_p);
    }
    // first, create a String from the buffer
    sga.sga_numsegs = 1;
#ifdef DMTR_PROFILE
    auto start = rdtsc();
#endif
    memcpy((char *)allocated_out_string.c_str(), out_string.c_str(), my_field_size);
#ifdef DMTR_PROFILE
    dmtr_record_latency(encode_string_latency, rdtsc() - start);
#endif
    void *p = NULL;
#ifdef DMTR_PROFILE
    auto start_malloc = rdtsc();
#endif
    dmtr_malloc(&p, sga.sga_segs[0].sgaseg_len);
#ifdef DMTR_PROFILE
    dmtr_record_latency(encode_malloc_latency, rdtsc() - start_malloc);
#endif
    assert(p != NULL);
#ifdef DMTR_PROFILE
    auto start_memcpy = rdtsc();
#endif
    memcpy(p, allocated_out_string.c_str(), sga.sga_segs[0].sgaseg_len);
#ifdef DMTR_PROFILE
    dmtr_record_latency(encode_memcpy_latency, rdtsc() - start_memcpy);
#endif
    out_p = p;
}

void malloc_baseline_no_str::deserialize_message(dmtr_sgarray_t &sga) {
    if (in_p != NULL) {
        free(in_p);
    }
    assert(sga.sga_numsegs == 1);
#ifdef DMTR_PROFILE
    auto start = rdtsc();
#endif
    memcpy((char *)allocated_in_string.c_str(), (char *)sga.sga_segs[0].sgaseg_buf, sga.sga_segs[0].sgaseg_len);
#ifdef DMTR_PROFILE
    dmtr_record_latency(decode_string_latency, rdtsc() - start);
#endif
    void *p = NULL;
#ifdef DMTR_PROFILE
    auto start_malloc = rdtsc();
#endif
    dmtr_malloc(&p, sga.sga_segs[0].sgaseg_len);
#ifdef DMTR_PROFILE
    dmtr_record_latency(decode_malloc_latency, rdtsc() - start_malloc);
#endif
    assert(p != NULL);
#ifdef DMTR_PROFILE
    auto start_memcpy = rdtsc();
#endif
    memcpy(p, allocated_in_string.c_str(), sga.sga_segs[0].sgaseg_len);
#ifdef DMTR_PROFILE
    dmtr_record_latency(decode_memcpy_latency, rdtsc() - start_memcpy);
#endif
    in_p = p;
}

void malloc_baseline_no_str::print_counters() {
    std::cerr << "In print counters" << std::endl;
#ifdef DMTR_PROFILE
    std::cerr << "In print counters" << std::endl;
    dmtr_dump_latency(stderr, encode_string_latency);
    dmtr_dump_latency(stderr, encode_malloc_latency);
    dmtr_dump_latency(stderr, encode_memcpy_latency);
    dmtr_dump_latency(stderr, decode_string_latency);
    dmtr_dump_latency(stderr, decode_malloc_latency);
    dmtr_dump_latency(stderr, decode_memcpy_latency);
#endif
}
