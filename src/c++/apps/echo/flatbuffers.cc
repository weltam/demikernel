// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "flatbuffers.hh"
#include "message.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>

flatbuffers_echo::flatbuffers_echo(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::FLATBUFFERS, field_size, message_type),
    getBuilder(field_size),
    putBuilder(field_size),
    msg1LBuilder(field_size),
    msg2LBuilder(field_size),
    msg3LBuilder(field_size),
    msg4LBuilder(field_size),
    msg5LBuilder(field_size),
    getMsg(getBuilder),
    putMsg(putBuilder),
    msg1L(msg1LBuilder),
    msg2L(msg2LBuilder),
    msg3L(msg3LBuilder),
    msg4L(msg4LBuilder),
    msg5L(msg5LBuilder)
{
    // TODO: initialize the correct message
}

void flatbuffers_echo::serialize_message(dmtr_sgarray_t &sga) {

}

void flatbuffers_echo::deserialize_message(dmtr_sgarray_t &sga) {

}

void flatbuffers_echo::encode_msg(dmtr_sgarray_t &sga, uint8_t* buf, int size) {

}

void flatbuffers_echo::handle_msg(uint8_t* buf) {

}
