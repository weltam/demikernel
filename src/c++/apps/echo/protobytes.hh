// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.


#ifndef ECHO_PROTOBUF_BYTES_H_
#define ECHO_PROTOBUF_BYTES_H_

#include "stress_bytes.pb.h"
#include "message.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>
#include <dmtr/latency.h>

using namespace std;

typedef google::protobuf::Message Message;

class protobuf_bytes_echo : public echo_message
{
    private: string string_field;
    
    private: stress_bytes::GetMessage getMsg_deser;
    private: stress_bytes::PutMessage putMsg_deser;
    private: stress_bytes::Msg1L msg1L_deser;
    private: stress_bytes::Msg2L msg2L_deser;
    private: stress_bytes::Msg3L msg3L_deser;
    private: stress_bytes::Msg4L msg4L_deser;
    private: stress_bytes::Msg5L msg5L_deser;

    private: dmtr_latency_t* serialize_latency;
    private: dmtr_latency_t* parse_latency;
    private: dmtr_latency_t* encode_malloc_latency;
    private: dmtr_latency_t* encode_memcpy_latency;
    private: dmtr_latency_t* decode_string_latency;

    public: protobuf_bytes_echo(uint32_t field_size, string message_type);

    public: virtual void serialize_message(dmtr_sgarray_t &sga, void *context);
    public: virtual void deserialize_message(dmtr_sgarray_t &sga);

    public: void encode_msg(dmtr_sgarray_t &sga, const Message& msg);
    public: void handle_message(const string& msg);
    public: void print_counters();
};



stress_bytes::Msg5L five_level_bytes(const string& string_field);

void fill_in_four_level_bytes(stress_bytes::Msg4L* msg, const string& string_field);

stress_bytes::Msg4L four_level_bytes(const string& string_field);

void fill_in_three_level_bytes(stress_bytes::Msg3L* msg, const string& string_field);

stress_bytes::Msg3L three_level_bytes(const string& string_field);

void fill_in_two_level_bytes(stress_bytes::Msg2L* msg, const string& string_field);

stress_bytes::Msg2L two_level_bytes(const string& string_field);

void fill_in_one_level_bytes(stress_bytes::Msg1L* msg, const string& string_field);

stress_bytes::Msg1L one_level_bytes(const string& string_field);

stress_bytes::GetMessage get_message_bytes(const string& string_field);

stress_bytes::PutMessage* put_message_bytes(const string& string_field);

#endif 
