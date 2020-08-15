// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.


#ifndef ECHO_FLATBUFFERS_H_
#define ECHO_FLATBUFFERS_H_

#include "stress_generated.h"
#include "message.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>

using namespace std;
using namespace stress;

class flatbuffers_echo : public echo_message
{

    private: std::string string_field;
    private: flatbuffers::FlatBufferBuilder getBuilder;
    private: flatbuffers::FlatBufferBuilder putBuilder;
    private: flatbuffers::FlatBufferBuilder msg1LBuilder;
    private: flatbuffers::FlatBufferBuilder msg2LBuilder;
    private: flatbuffers::FlatBufferBuilder msg3LBuilder;
    private: flatbuffers::FlatBufferBuilder msg4LBuilder;
    private: flatbuffers::FlatBufferBuilder msg5LBuilder;
    private: flatbuffers::Offset<flatbuffers::String> string_offset;
    
    private: GetMessageFBBuilder getMsg;
    private: PutMessageFBBuilder putMsg;
    private: Msg1LFBBuilder msg1L;
    private: Msg2LFBBuilder msg2L;
    private: Msg3LFBBuilder msg3L;
    private: Msg4LFBBuilder msg4L;
    private: Msg5LFBBuilder msg5L;

    public: flatbuffers_echo(uint32_t field_size, string message_type);

    public: virtual void serialize_message(dmtr_sgarray_t &sga);
    public: virtual void deserialize_message(dmtr_sgarray_t &sga);

    public: void encode_msg(dmtr_sgarray_t &sga, uint8_t* buf, int size);
    public: void handle_msg(uint8_t* buf);
    public: void print_counters() {}

    private: void build_get();
    private: void build_put();
    private: void build_msg1L();
    private: void build_msg2L();
    private: void build_msg3L();
    private: void build_msg4L();
    private: void build_msg5L();

};

template<class T>
void read_data(uint8_t* buf, const T** ptr);

#endif
