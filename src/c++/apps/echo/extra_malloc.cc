// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "extramalloc.hh"
#include <dmtr/libos/mem.h>
#include <assert.h>
#include <string>

malloc_baseline::malloc_baseline(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::MALLOCBASELINE, field_size, message_type)
{}

void malloc_baseline::serialize_message(dmtr_sgarray_t &sga) {
    // first, create a String from the buffer
    std::string buffer_str((char *)sga.sga_segs[0].sgaseg_buf, sga.sga_segs[0].sgaseg_len);
    void *p = NULL;
    dmtr_malloc(&p, sga.sga_segs[0].sgaseg_len);
    assert(p != NULL);
    memcpy(p, buffer_str.c_str(), sga.sga_segs[0].sgaseg_len);
    free(p);
}

void malloc_baseline::deserialize_message(dmtr_sgarray_t &sga) {
    std::string buffer_str((char *)sga.sga_segs[0].sgaseg_buf, sga.sga_segs[0].sgaseg_len);
    void *p = NULL;
    dmtr_malloc(&p, sga.sga_segs[0].sgaseg_len);
    assert(p != NULL);
    memcpy(p, buffer_str.c_str(), sga.sga_segs[0].sgaseg_len);
    free(p);
}
