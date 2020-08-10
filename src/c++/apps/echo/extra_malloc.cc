// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "extramalloc.hh"
#include <dmtr/libos/mem.h>
#include <assert.h>
#include <string>

malloc_baseline::malloc_baseline(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::MALLOCBASELINE, field_size, message_type),
    in_p(NULL),
    out_p(NULL),
    out_string("a", field_size)
{}

void malloc_baseline::serialize_message(dmtr_sgarray_t &sga) {
    if (out_p != NULL) {
        free(out_p);
    }
    // first, create a String from the buffer
    sga.sga_numsegs = 1;
    std::string buffer_str(out_string.c_str(), my_field_size);
    void *p = NULL;
    dmtr_malloc(&p, sga.sga_segs[0].sgaseg_len);
    assert(p != NULL);
    memcpy(p, buffer_str.c_str(), sga.sga_segs[0].sgaseg_len);
    out_p = p;
}

void malloc_baseline::deserialize_message(dmtr_sgarray_t &sga) {
    if (in_p != NULL) {
        free(in_p);
    }
    assert(sga.sga_numsegs == 1);
    std::string buffer_str((char *)sga.sga_segs[0].sgaseg_buf, sga.sga_segs[0].sgaseg_len);
    void *p = NULL;
    dmtr_malloc(&p, sga.sga_segs[0].sgaseg_len);
    assert(p != NULL);
    memcpy(p, buffer_str.c_str(), sga.sga_segs[0].sgaseg_len);
    in_p = p;
}
