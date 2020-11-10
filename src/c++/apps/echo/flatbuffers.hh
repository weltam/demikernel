// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.


#ifndef ECHO_FLATBUFFERS_H_
#define ECHO_FLATBUFFERS_H_

#include "stress_generated.h"
#include "flatbuffers.hh"
#include "message.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>

using namespace std;
using namespace stress;

class flatbuffers_echo : public echo_message
{

    private: std::string string_field;

    public: flatbuffers_echo(uint32_t field_size, string message_type);

    public: virtual void serialize_message(dmtr_sgarray_t &sga, void *context);
    public: virtual void deserialize_message(dmtr_sgarray_t &sga);

    public: void encode_msg(dmtr_sgarray_t &sga, uint8_t* buf, int size);
    public: void handle_msg(uint8_t* buf);
    public: void print_counters() {}

    // TODO: remove these functions so flatbuffers doesn't complain about
    // nesting.
    private: void build_msg1L(Msg1LFBBuilder msg1L, flatbuffers::FlatBufferBuilder *msg1LBuilder);
    private: void build_msg2L(Msg2LFBBuilder msg2L, flatbuffers::FlatBufferBuilder *msg2LBuilder);
    private: void build_msg3L(Msg3LFBBuilder msg3L, flatbuffers::FlatBufferBuilder *msg3LBuilder);
    private: void build_msg4L(Msg4LFBBuilder msg4L, flatbuffers::FlatBufferBuilder *msg4LBuilder);
    private: void build_msg5L(Msg5LFBBuilder msg5L, flatbuffers::FlatBufferBuilder *msg5LBuilder);

};

template<class T>
void read_data(uint8_t* buf, const T** ptr);

#endif
