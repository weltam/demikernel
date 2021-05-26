// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.


#ifndef ECHO_PROTOBUF_H_
#define ECHO_PROTOBUF_H_

#include "stress.pb.h"
#include "message.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>
#include <dmtr/latency.h>

using namespace std;
using namespace stress;

//#define DMTR_PROFILE

typedef google::protobuf::Message Message;

class protobuf_echo : public echo_message
{
    private: string string_field;
    // private: GetMessage getMsg;
    
#ifdef DMTR_PROFILE
    private: dmtr_latency_t *alloc_latency;
    private: dmtr_latency_t *copy_latency;
    private: dmtr_latency_t *encode_latency;
    private: dmtr_latency_t *decode_latency;
#endif
    public: protobuf_echo(uint32_t field_size, string message_type);

    public: virtual void serialize_message(dmtr_sgarray_t &sga, void *context);
    public: virtual void deserialize_message(dmtr_sgarray_t &sga);

    public: void encode_msg(dmtr_sgarray_t &sga, const Message& msg, void *context);
    public: void handle_message(void *buf, size_t len);
    public: void print_counters();
};


string generate_string();

Msg5L five_level(const string& string_field);

Msg4L four_level(const string& string_field);

void fill_in_four_level(Msg4L* msg, const string& string_field);

Msg3L three_level(const string& string_field);

void fill_in_three_level(Msg3L* msg, const string& string_field);

Msg2L two_level(const string& string_field);

void fill_in_two_level(Msg2L* msg, const string& string_field);

Msg1L one_level(const string& string_field);

void fill_in_one_level(Msg1L* msg, const string& string_field);

#ifdef DMTR_PROFILE
GetMessage get_message(const string& string_field, dmtr_latency_t *alloc_latency, dmtr_latency_t *copy_latency);
#else
GetMessage get_message(const string& string_field);
#endif

PutMessage* put_message(const string& string_field);

#endif 
