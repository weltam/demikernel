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
//#define DMTR_PROFILE
using namespace std;

typedef google::protobuf::Message Message;

class protobuf_bytes_echo : public echo_message
{
#ifdef DMTR_PROFILE
    private: dmtr_latency_t *alloc_latency;
    private: dmtr_latency_t *copy_latency;
    private: dmtr_latency_t* encode_latency;
    private: dmtr_latency_t* decode_latency;
#endif
    
    private: string string_field;
    public: protobuf_bytes_echo(uint32_t field_size, string message_type);

    public: virtual void serialize_message(dmtr_sgarray_t &sga, void *context);
    public: virtual void deserialize_message(dmtr_sgarray_t &sga);

    public: void encode_msg(dmtr_sgarray_t &sga, const Message& msg, void *context);
    public: void handle_message(void *buf, size_t len);
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

#ifdef DMTR_PROFILE
stress_bytes::GetMessage get_message_bytes(const string& string_field, dmtr_latency_t *alloc_latency, dmtr_latency_t *copy_latency);
#else
stress_bytes::GetMessage get_message_bytes(const string& string_field);
#endif
stress_bytes::PutMessage* put_message_bytes(const string& string_field);

#endif 
