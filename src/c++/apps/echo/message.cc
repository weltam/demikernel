// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include <string>
#include <stdio.h>
#include <cstring>
#include "message.hh"
#define FILL_CHAR 'a'

uint64_t rdtsc() {
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

string generate_string(uint32_t field_size) {
    return string(field_size, FILL_CHAR);
}


echo_message::echo_message(enum library cereal_type,
                            uint32_t field_size,
                            string message_type)
{
    my_cereal_type = cereal_type;
    my_field_size  = field_size;
    my_message_type = message_type;
                  
    if (!strcmp(message_type.c_str(), "Get")) {
        my_msg_enum = echo_message::msg_type::GET;
    } else if (!strcmp(message_type.c_str(),"Put")) {
        my_msg_enum = echo_message::msg_type::PUT;
    } else if (!strcmp(message_type.c_str(), "Msg1L")) {
        my_msg_enum = echo_message::msg_type::MSG1L;
    } else if (!strcmp(message_type.c_str(), "Msg2L")) {
        my_msg_enum = echo_message::msg_type::MSG2L;
    } else if (!strcmp(message_type.c_str(), "Msg3L")) {
        my_msg_enum = echo_message::msg_type::MSG3L;
    } else if (!strcmp(message_type.c_str(), "Msg4L")) {
        my_msg_enum = echo_message::msg_type::MSG4L;
    } else if (!strcmp(message_type.c_str(), "Msg5L")) {
        my_msg_enum = echo_message::msg_type::MSG5L;
    } else {
    }
}
