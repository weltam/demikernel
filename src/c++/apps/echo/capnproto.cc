// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "capnproto.hh"
#include "message.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>

capnproto_echo::capnproto_echo(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::CAPNPROTO, field_size, message_type),
    getMsg(nullptr),
    putMsg(nullptr),
    msg1L(nullptr),
    msg2L(nullptr),
    msg3L(nullptr),
    msg4L(nullptr),
    msg5L(nullptr)
{
    // TODO: initialize the correct message
}

void capnproto_echo::serialize_message(dmtr_sgarray_t &sga) {

}

void capnproto_echo::deserialize_message(dmtr_sgarray_t &sga) {

}

