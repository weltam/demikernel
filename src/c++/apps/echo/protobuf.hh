// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.


#ifndef ECHO_PROTOBUF_H_
#define ECHO_PROTOBUF_H_

#include "stress.pb.h"
#include "message.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>

using namespace std;
using namespace stress;

typedef google::protobuf::Message Message;

class protobuf_echo : public echo_message
{
    private: GetMessage getMsg;
    private: PutMessage putMsg;
    private: Msg1L msg1L;
    private: Msg2L msg2L;
    private: Msg3L msg3L;
    private: Msg4L msg4L;
    private: Msg5L msg5L;

    public: protobuf_echo(uint32_t field_size, string message_type);

    public: virtual void serialize_message(dmtr_sgarray_t &sga);
    public: virtual void deserialize_message(dmtr_sgarray_t &sga);

    public: void encode_msg(dmtr_sgarray_t &sga, const Message& msg);
    public: void handle_message(const string& msg);
};


string generate_string(uint32_t field_size);

Msg5L* five_level(uint32_t field_size);

Msg4L* four_level(uint32_t field_size);

Msg3L* three_level(uint32_t field_size);

Msg2L* two_level(uint32_t field_size);

Msg1L* one_level(uint32_t field_size);

GetMessage* get_message(uint32_t field_size);

PutMessage* put_message(uint32_t field_size);

#endif 
