// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "extra_malloc.hh"
#include <dmtr/libos/mem.h>
#include <assert.h>
#include <string>
#include <iostream>
#define DMTR_PROFILE
malloc_baseline::malloc_baseline(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::MALLOCBASELINE, field_size, message_type),
    in_p(NULL),
    out_p(NULL),
    out_string("a", field_size)
{
#ifdef DMTR_PROFILE
    dmtr_new_latency(&encode_string_latency, "encode String allocation latency");
    dmtr_new_latency(&encode_malloc_latency, "encode Malloc latency");
    dmtr_new_latency(&encode_memcpy_latency, "encode Memcpy latency");
    dmtr_new_latency(&decode_string_latency, "decode String allocation latency");
    dmtr_new_latency(&decode_malloc_latency, "decode Malloc latency");
    dmtr_new_latency(&decode_memcpy_latency, "decode Memcpy latency");
#endif
}

// encode has 2 memcpys and a malloc, or a string allocation, a memcpy (and
// implicitly a second memcpy) and a malloc
// can we build this up/down?
//  1. Take away the string allocation, on both sides
//  2. Take away the mallocs, leaving just the memcpys (have a buffer
//  pre-allocated, that's freed in the constructor): this should be pretty fast
void malloc_baseline::serialize_message(dmtr_sgarray_t &sga, void *context) {
    if (out_p != NULL) {
        free(out_p);
    }
    // first, create a String from the buffer
    sga.sga_numsegs = 1;
#ifdef DMTR_PROFILE
    auto start = rdtsc();
#endif
    std::string allocated_out_string(out_string.c_str(), my_field_size);
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

void malloc_baseline::deserialize_message(dmtr_sgarray_t &sga) {
    if (in_p != NULL) {
        free(in_p);
    }
    assert(sga.sga_numsegs == 1);
#ifdef DMTR_PROFILE
    auto start = rdtsc();
#endif
    std::string allocated_in_string((char *)sga.sga_segs[0].sgaseg_buf, sga.sga_segs[0].sgaseg_len);
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

void malloc_baseline::print_counters() {
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
