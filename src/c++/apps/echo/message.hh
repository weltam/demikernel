// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#ifndef ECHO_MESSAGE_H_
#define ECHO_MESSAGE_H_

#include <string>
#include <stdio.h>
#include <dmtr/sga.h>
#include <sys/types.h>
#include <cstring>
#define FILL_CHAR 'a'
using namespace std;

uint64_t rdtsc();

string generate_string(uint32_t field_size);


class echo_message
{
    public: enum library {
        PROTOBUF,
        CAPNPROTO,
        FLATBUFFERS,  
        MALLOCBASELINE,
        PROTOARENA,
    };

    public: enum msg_type {
        GET,
        PUT,
        MSG1L,
        MSG2L,
        MSG3L,
        MSG4L,
        MSG5L,
    };

    protected: enum library my_cereal_type; // which library it is
    protected: uint32_t my_field_size; // size of message being sent back and forth
    protected: enum msg_type my_msg_enum; // which message is being sent back and forth
    protected: string my_message_type; // string version of the same message type 
    
    protected: echo_message(enum library cereal_type,
                            uint32_t field_size,
                            string message_type);
    public: ~echo_message() {}

    public: uint32_t field_size() const {
                return my_field_size;
            }

    public: string message_type() const {
                return my_message_type;
            }

    // encode and serialize message
    // context could be a flatbuffers builder, or nothing at all
    public: virtual void serialize_message(dmtr_sgarray_t &sga, void *context) = 0;
    // decode and deserialize message
    public: virtual void deserialize_message(dmtr_sgarray_t &sga) = 0;
    // print extra counters out
    public: virtual void print_counters() = 0;
};

#endif
