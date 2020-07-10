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

    private: flatbuffers::FlatBufferBuilder getBuilder;
    private: flatbuffers::FlatBufferBuilder putBuilder;
    private: flatbuffers::FlatBufferBuilder msg1LBuilder;
    private: flatbuffers::FlatBufferBuilder msg2LBuilder;
    private: flatbuffers::FlatBufferBuilder msg3LBuilder;
    private: flatbuffers::FlatBufferBuilder msg4LBuilder;
    private: flatbuffers::FlatBufferBuilder msg5LBuilder;
    
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
};

#endif
