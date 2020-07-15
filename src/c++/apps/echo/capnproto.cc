// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "capnproto.hh"
#include "message.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>

capnproto_echo::capnproto_echo(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::CAPNPROTO, field_size, message_type),
    builder(),
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
            break;
        case echo_message::msg_type::PUT:
            break;
        case echo_message::msg_type::MSG1L:
            break;
        case echo_message::msg_type::MSG2L:
            break;
        case echo_message::msg_type::MSG3L:
            break;
        case echo_message::msg_type::MSG4L:
            break;
        case echo_message::msg_type::MSG5L:
            break;
    }

}

void capnproto_echo::deserialize_message(dmtr_sgarray_t &sga) {
    switch (my_msg_enum) {
        case echo_message::msg_type::GET:
            break;
        case echo_message::msg_type::PUT:
            break;
        case echo_message::msg_type::MSG1L:
            break;
        case echo_message::msg_type::MSG2L:
            break;
        case echo_message::msg_type::MSG3L:
            break;
        case echo_message::msg_type::MSG4L:
            break;
        case echo_message::msg_type::MSG5L:
            break;
    }

}

void capnproto_echo::build_get() {
    getMsg = builder.initRoot<GetMessageCP>();
    getMsg.setKey(generate_string(my_field_size));
}

void capnproto_echo::build_put() {
    ::capnp::MallocMessageBuilder message;
    putMsg = message.initRoot<PutMessageCP>();
    putMsg.setKey(generate_string(my_field_size/2));
    putMsg.setValue(generate_string(my_field_size/2));

}

void capnproto_echo::build_msg1L() {
    ::capnp::MallocMessageBuilder message;
    msg1L = message.initRoot<Msg1LCP>();
    auto left = msg1L.initLeft();
    auto right = msg1L.initRight();
    
}

void capnproto_echo::build_msg2L() {

}

void capnproto_echo::build_msg3L() {

}

void capnproto_echo::build_msg4L() {

}

void capnproto_echo::build_msg5L() {

}

