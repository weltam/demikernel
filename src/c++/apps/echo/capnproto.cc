// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "capnproto.hh"
#include "message.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>
#include <dmtr/libos/mem.h>
#include "assert.h"
#include <iostream>

capnproto_echo::capnproto_echo(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::CAPNPROTO, field_size, message_type),
    string_field(generate_string(field_size)) {}

void capnproto_echo::serialize_message(dmtr_sgarray_t &sga, void *context) {
    if (my_msg_enum == echo_message::msg_type::GET) {
        GetMessageCP::Builder getMsg = (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).initRoot<GetMessageCP>();
        build_get(getMsg);
        encode_msg(sga, (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).getSegmentsForOutput());
    } else if (my_msg_enum == echo_message::msg_type::PUT) {
        PutMessageCP::Builder putMsg = (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).initRoot<PutMessageCP>();
        build_put(putMsg);
        encode_msg(sga, (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).getSegmentsForOutput());
    } else if (my_msg_enum == echo_message::msg_type::MSG1L) {
        Msg1LCP::Builder msg1L = (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).initRoot<Msg1LCP>();
        build_msg1L(msg1L);
        encode_msg(sga, (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).getSegmentsForOutput());

    } else if (my_msg_enum == echo_message::msg_type::MSG2L) {
        Msg2LCP::Builder msg2L = (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).initRoot<Msg2LCP>();
        build_msg2L(msg2L);
        encode_msg(sga, (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).getSegmentsForOutput());
    
    } else if (my_msg_enum == echo_message::msg_type::MSG3L) {
        Msg3LCP::Builder msg3L = (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).initRoot<Msg3LCP>();
        build_msg3L(msg3L);
        encode_msg(sga, (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).getSegmentsForOutput());

    } else if (my_msg_enum == echo_message::msg_type::MSG4L) {
        Msg4LCP::Builder msg4L = (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).initRoot<Msg4LCP>();
        build_msg4L(msg4L);
        encode_msg(sga, (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).getSegmentsForOutput());
    } else if (my_msg_enum == echo_message::msg_type::MSG5L) {
        Msg5LCP::Builder msg5L = (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).initRoot<Msg5LCP>();
        build_msg5L(msg5L);
        encode_msg(sga, (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).getSegmentsForOutput());
    }
}

void capnproto_echo::deserialize_message(dmtr_sgarray_t &sga) {
    GetMessageCP::Reader local_get;
    PutMessageCP::Reader local_put;
    Msg1LCP::Reader local_msg1L; 
    Msg2LCP::Reader local_msg2L;
    Msg3LCP::Reader local_msg3L;
    Msg4LCP::Reader local_msg4L;
    Msg5LCP::Reader local_msg5L;
    auto segment_array = kj::heapArray<kj::ArrayPtr<const capnp::word>>(sga.sga_numsegs);
    decode_msg(sga, segment_array.begin());

    capnp::SegmentArrayMessageReader message(segment_array);
    switch (my_msg_enum) {
        case echo_message::msg_type::GET:
            local_get = message.getRoot<GetMessageCP>();
            break;
        case echo_message::msg_type::PUT:
            local_put = message.getRoot<PutMessageCP>();
            break;
        case echo_message::msg_type::MSG1L:
            local_msg1L = message.getRoot<Msg1LCP>();
            break;
        case echo_message::msg_type::MSG2L:
            local_msg2L = message.getRoot<Msg2LCP>();
            break;
        case echo_message::msg_type::MSG3L:
            local_msg3L = message.getRoot<Msg3LCP>();
            break;
        case echo_message::msg_type::MSG4L:
            local_msg4L = message.getRoot<Msg4LCP>();
            break;
        case echo_message::msg_type::MSG5L:
            local_msg5L = message.getRoot<Msg5LCP>();
            break;
    }
}

void capnproto_echo::encode_msg(dmtr_sgarray_t &sga, kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> segments) {
    // check if we can even do the correc tthings
    capnp::SegmentArrayMessageReader message(segments);
    //sga.sga_numsegs = segments.size() + 1;
    sga.sga_numsegs = segments.size();

    // write the data into the next buffers
    for (size_t i = 0; i < segments.size(); i++) {
        kj::ArrayPtr<const capnp::word> segment = (segments[i]);
        kj::ArrayPtr<const kj::byte> bytes = segment.asBytes();
        sga.sga_segs[i].sgaseg_len = bytes.size();
        sga.sga_segs[i].sgaseg_buf = (void *) bytes.begin();
    }

}

void capnproto_echo::decode_msg(dmtr_sgarray_t &sga, kj::ArrayPtr<const capnp::word>* segments) {
    auto current_segment = segments; 
    for (size_t i = 0; i < sga.sga_numsegs; i++) {
        unsigned char* segment = reinterpret_cast<unsigned char *>(sga.sga_segs[i].sgaseg_buf);
        const capnp::word* bytes = reinterpret_cast<const capnp::word*>(segment);
        size_t word_size = sga.sga_segs[i].sgaseg_len/sizeof(capnp::word);
        kj::ArrayPtr<const capnp::word> words(bytes, word_size);
        *current_segment = words;
        current_segment += 1;
    }
}

inline void capnproto_echo::build_get(GetMessageCP::Builder getMsg) {
    getMsg.setKey(string_field);
}

inline void capnproto_echo::recursive_get(GetMessageCP::Builder builder, uint32_t field_size) {
    builder.setKey(string_field);
}

inline void capnproto_echo::build_put(PutMessageCP::Builder putMsg) {
    putMsg.setKey(string_field);
    putMsg.setValue(generate_string(my_field_size/2));
}

inline void capnproto_echo::build_msg1L(Msg1LCP::Builder msg1L) {
    auto left = msg1L.initLeft();
    auto right = msg1L.initRight();
    recursive_get(left, my_field_size/2);
    recursive_get(right, my_field_size/2);
}

inline void capnproto_echo::recursive_msg1L(Msg1LCP::Builder builder, uint32_t field_size) {
    auto left = builder.initLeft();
    recursive_get(left, field_size/2);
    auto right = builder.initRight();
    recursive_get(right, field_size/2);
}

inline void capnproto_echo::build_msg2L(Msg2LCP::Builder msg2L) {
    auto left = msg2L.initLeft();
    recursive_msg1L(left, my_field_size/2);
    auto right = msg2L.initRight();
    recursive_msg1L(right, my_field_size/2);
}

inline void capnproto_echo::recursive_msg2L(Msg2LCP::Builder builder, uint32_t field_size) {
    auto left = builder.initLeft();
    recursive_msg1L(left, field_size/2);
    auto right = builder.initRight();
    recursive_msg1L(right, field_size/2);
}

inline void capnproto_echo::build_msg3L(Msg3LCP::Builder msg3L) {
    auto left = msg3L.initLeft();
    recursive_msg2L(left, my_field_size/2);
    auto right = msg3L.initRight();
    recursive_msg2L(right, my_field_size/2);
}

inline void capnproto_echo::recursive_msg3L(Msg3LCP::Builder builder, uint32_t field_size) {
    auto left = builder.initLeft();
    recursive_msg2L(left, field_size/2);
    auto right = builder.initRight();
    recursive_msg2L(right, field_size/2);
}

inline void capnproto_echo::build_msg4L(Msg4LCP::Builder msg4L) {
    auto left = msg4L.initLeft();
    recursive_msg3L(left, my_field_size/2);
    auto right = msg4L.initRight();
    recursive_msg3L(right, my_field_size/2);
}

inline void capnproto_echo::recursive_msg4L(Msg4LCP::Builder builder, uint32_t field_size) {
    auto left = builder.initLeft();
    recursive_msg3L(left, field_size/2);
    auto right = builder.initRight();
    recursive_msg3L(right, field_size/2);
}

inline void capnproto_echo::build_msg5L(Msg5LCP::Builder msg5L) {
    auto left = msg5L.initLeft();
    recursive_msg4L(left, my_field_size/2);
    auto right = msg5L.initRight();
    recursive_msg4L(right, my_field_size/2);
}

