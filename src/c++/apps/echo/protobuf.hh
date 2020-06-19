// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.


#ifndef ECHO_PROTOBUF_H_
#define ECHO_PROTOBUF_H_

#include "stress.pb.h"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>

using namespace std;
using namespace stress;

typedef google::protobuf::Message Message;

string generate_string(uint32_t field_size);

void init_protobuf(const string& type, uint32_t field_size);

Msg5L* five_level(uint32_t field_size);

Msg4L* four_level(uint32_t field_size);

Msg3L* three_level(uint32_t field_size);

Msg2L* two_level(uint32_t field_size);

Msg1L* one_level(uint32_t field_size);

GetMessage* get_message(uint32_t field_size);

PutMessage* put_message(uint32_t field_size);

void handle_message(const string& type, const string& msg);

void decode_serialized_msg(dmtr_sgarray_t &sga);

void generate_serialized_msg(const Message& msg, const string& type);

void generate_protobuf_packet(dmtr_sgarray_t &sga,
                                const string& type,
                                uint32_t field_size);

#endif 
