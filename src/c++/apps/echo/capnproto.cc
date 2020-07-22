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
    malloc_builder(),
    getMsg(nullptr),
    putMsg(nullptr),
    msg1L(nullptr),
    msg2L(nullptr),
    msg3L(nullptr),
    msg4L(nullptr),
    msg5L(nullptr)
{
    switch (my_msg_enum) {
        case echo_message::msg_type::GET:
            build_get();
            break;
        case echo_message::msg_type::PUT:
            build_put();
            break;
        case echo_message::msg_type::MSG1L:
            build_msg1L();
            break;
        case echo_message::msg_type::MSG2L:
            build_msg2L();
            break;
        case echo_message::msg_type::MSG3L:
            build_msg3L();
            break;
        case echo_message::msg_type::MSG4L:
            build_msg4L();
            break;
        case echo_message::msg_type::MSG5L:
            build_msg5L();
            break;
    }
}

void capnproto_echo::serialize_message(dmtr_sgarray_t &sga) {
    switch (my_msg_enum) {
        case echo_message::msg_type::GET:
            encode_msg(sga, malloc_builder.getSegmentsForOutput());
            break;
        case echo_message::msg_type::PUT:
            encode_msg(sga, malloc_builder.getSegmentsForOutput());
            break;
        case echo_message::msg_type::MSG1L:
            encode_msg(sga, malloc_builder.getSegmentsForOutput());
            break;
        case echo_message::msg_type::MSG2L:
            encode_msg(sga, malloc_builder.getSegmentsForOutput());
            break;
        case echo_message::msg_type::MSG3L:
            encode_msg(sga, malloc_builder.getSegmentsForOutput());
            break;
        case echo_message::msg_type::MSG4L:
            encode_msg(sga, malloc_builder.getSegmentsForOutput());
            break;
        case echo_message::msg_type::MSG5L:
            encode_msg(sga, malloc_builder.getSegmentsForOutput());
            break;
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
    auto segment_array = kj::heapArray<kj::ArrayPtr<const capnp::word>>(sga.sga_numsegs - 1);
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
    sga.sga_numsegs = segments.size() + 1;
    void *p = NULL;
    size_t typeLen = my_message_type.length();
    size_t totalLen = (typeLen + sizeof(typeLen));
    dmtr_malloc(&p, totalLen);
    assert(p != NULL);

    sga.sga_segs[0].sgaseg_len = totalLen;
    sga.sga_segs[0].sgaseg_buf = p;
    char *buf = reinterpret_cast<char *>(p);
    char *ptr = buf;
    *((size_t *) ptr) = typeLen;
    ptr += sizeof(typeLen);
    assert((size_t)(ptr-buf) < totalLen);
    
    memcpy(ptr, my_message_type.c_str(), typeLen);
    ptr += typeLen;
    assert((size_t)(ptr-buf) < totalLen);

    // write the data into the next buffers
    for (size_t i = 0; i < segments.size(); i++) {
        kj::ArrayPtr<const capnp::word> segment = (segments[i]);
        kj::ArrayPtr<const kj::byte> bytes = segment.asBytes();
        sga.sga_segs[i+1].sgaseg_len = bytes.size();
        sga.sga_segs[i+1].sgaseg_buf = (void *) bytes.begin();
    }

    deserialize_message(sga);
}

void capnproto_echo::decode_msg(dmtr_sgarray_t &sga, kj::ArrayPtr<const capnp::word>* segments) {
    auto current_segment = segments; 
    for (size_t i = 1; i < sga.sga_numsegs; i++) {
        unsigned char* segment = reinterpret_cast<unsigned char *>(sga.sga_segs[i].sgaseg_buf);
        const capnp::word* bytes = reinterpret_cast<const capnp::word*>(segment);
        size_t word_size = sga.sga_segs[i].sgaseg_len/sizeof(capnp::word);
        kj::ArrayPtr<const capnp::word> words(bytes, word_size);
        *current_segment = words;
        current_segment += 1;
    }
}

void capnproto_echo::build_get() {
    getMsg = malloc_builder.initRoot<GetMessageCP>();
    getMsg.setKey(generate_string(my_field_size));
}

void capnproto_echo::recursive_get(GetMessageCP::Builder builder, uint32_t field_size) {
    builder.setKey(generate_string(field_size));
}

void capnproto_echo::build_put() {
    putMsg = malloc_builder.initRoot<PutMessageCP>();
    putMsg.setKey(generate_string(my_field_size/2));
    putMsg.setValue(generate_string(my_field_size/2));

}

void capnproto_echo::build_msg1L() {
    msg1L = malloc_builder.initRoot<Msg1LCP>();
    auto left = msg1L.initLeft();
    auto right = msg1L.initRight();
    recursive_get(left, my_field_size/2);
    recursive_get(right, my_field_size/2);
}

void capnproto_echo::recursive_msg1L(Msg1LCP::Builder builder, uint32_t field_size) {
    auto left = builder.initLeft();
    recursive_get(left, field_size/2);
    auto right = builder.initRight();
    recursive_get(right, field_size/2);
}

void capnproto_echo::build_msg2L() {
    msg2L = malloc_builder.initRoot<Msg2LCP>();
    
    auto left = msg2L.initLeft();
    recursive_msg1L(left, my_field_size/2);
    auto right = msg2L.initRight();
    recursive_msg1L(right, my_field_size/2);
}

void capnproto_echo::recursive_msg2L(Msg2LCP::Builder builder, uint32_t field_size) {
    auto left = builder.initLeft();
    recursive_msg1L(left, field_size/2);
    auto right = builder.initRight();
    recursive_msg1L(right, field_size/2);
}

void capnproto_echo::build_msg3L() {
    msg3L = malloc_builder.initRoot<Msg3LCP>();

    auto left = msg3L.initLeft();
    recursive_msg2L(left, my_field_size/2);
    auto right = msg3L.initRight();
    recursive_msg2L(right, my_field_size/2);
}

void capnproto_echo::recursive_msg3L(Msg3LCP::Builder builder, uint32_t field_size) {
    auto left = builder.initLeft();
    recursive_msg2L(left, field_size/2);
    auto right = builder.initRight();
    recursive_msg2L(right, field_size/2);
}

void capnproto_echo::build_msg4L() {
    msg4L = malloc_builder.initRoot<Msg4LCP>();

    auto left = msg4L.initLeft();
    recursive_msg3L(left, my_field_size/2);
    auto right = msg4L.initRight();
    recursive_msg3L(right, my_field_size/2);
}

void capnproto_echo::recursive_msg4L(Msg4LCP::Builder builder, uint32_t field_size) {
    auto left = builder.initLeft();
    recursive_msg3L(left, field_size/2);
    auto right = builder.initRight();
    recursive_msg3L(right, field_size/2);
}

void capnproto_echo::build_msg5L() {
    msg5L = malloc_builder.initRoot<Msg5LCP>();

    auto left = msg5L.initLeft();
    recursive_msg4L(left, my_field_size/2);
    auto right = msg5L.initRight();
    recursive_msg4L(right, my_field_size/2);
}

