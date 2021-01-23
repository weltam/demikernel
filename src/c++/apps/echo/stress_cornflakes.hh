// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.


#ifndef CORNFLAKES_GENERATED_STRESS_H_
#define CORNFLAKES_GENERATED_STRESS_H_

#include "assert.h"
#include <dmtr/sga.h>
#include <iostream>
#include <dmtr/libos/mem.h>
#include "endian.h"

// This file contains fake "generated code" to test how a pointer based
// serialization library would work, for a simple GetMessage with a single blob

// TODO: test if this actually compiles and doesn't segfault
// in client binary, just test different sgas to make sure using this works
// (or add a new binary)
// Then write the wrapper binary that actually calls this to "serialize" and
// "deserialize" stuff
namespace cornflakes {

struct CFPtr {
    char *ptr;
    size_t len;
};

class GetMessageCF {

    private:
        static const size_t HEADER_SIZE = 8; // 4 for length and 4 for pointer
        static const size_t SIZE_FIELD = 4;
        static const size_t POINTER_FIELD = 8;
        static const size_t KEY_HEADER_OFF = 0;
        
        char header[HEADER_SIZE];
        void *key_ptr; // TODO; do we even maintain separate pointers to the key

        uint32_t get_key_len() {
            uint32_t *key_len = (uint32_t *)(&header[KEY_HEADER_OFF]);
            return  *key_len;
        }
        
        void set_key_len(size_t key_len) {
            uint32_t *key_len_ptr = (uint32_t *)(&header[KEY_HEADER_OFF]);
            *key_len_ptr = (uint32_t)key_len;
        }

        void set_key_offset(uint32_t offset) {
            uint32_t *offset_ptr = (uint32_t *)(&header[KEY_HEADER_OFF + SIZE_FIELD]);
            *offset_ptr = offset;
        }

    public:
        GetMessageCF() :
        header(),
        key_ptr(NULL)
        {
            set_key_len(0);
            set_key_offset(0);
            key_ptr = NULL;
        }

        bool has_key() {
            if (get_key_len() == 0) {
                return false;
            } else {
                return true;
            }
        }

        void set_key(char *key, size_t len) {
            set_key_len(len);
            key_ptr = (void *)key;
        }

        CFPtr get_key() {
            CFPtr cf;
            cf.ptr = (char *)key_ptr;
            cf.len = (size_t)get_key_len();
            return cf;
        }

        void serialize(dmtr_sgarray_t &sga) {
            size_t field_offset = HEADER_SIZE;
            // set offsets in header for each field
            if (has_key()) {
                // do this before changing network ordering of length
                sga.sga_segs[1].sgaseg_len = (size_t)get_key_len();
                // modify header to reflect network ordering
                set_key_len(htobe32((uint32_t)get_key_len()));
                set_key_offset(htobe32((uint32_t)field_offset));
                field_offset += POINTER_FIELD;
            }

            // fill in SGA
            sga.sga_numsegs = 1;
            sga.sga_segs[0].sgaseg_len = HEADER_SIZE;
            sga.sga_segs[0].sgaseg_buf = (void *)(&header);
        
            // if has_key(), add SGA entry for key
            if (has_key()) {
                sga.sga_numsegs += 1;
                sga.sga_segs[1].sgaseg_buf = key_ptr;
            }

        }

        // for now, assume that sga comes back as a single contiguous buffer
        void deserialize(dmtr_sgarray_t &sga) {
            assert(sga.sga_numsegs == 1);
            char *payload = (char *)sga.sga_segs[0].sgaseg_buf;
            assert(sga.sga_segs[0].sgaseg_len >= HEADER_SIZE);
            // for each field, check if has field and set pointer
            uint32_t key_len = be32toh(*((uint32_t *)(&payload[KEY_HEADER_OFF])));
            if (key_len != 0) {
                set_key_len((size_t)key_len);
                size_t key_off = be32toh(*((uint32_t *)(&payload[KEY_HEADER_OFF + SIZE_FIELD])));
                key_ptr = (void *)(&payload[key_off]);
            }
        }
};

} // namespace cornflakes

#endif

