// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "protobuf.hh"
#include "stress.pb.h" // protobuf
#include <iostream>
#include <dmtr/libos/mem.h>
#include <string>
#include <assert.h>
#define FILL_CHAR 'a'

/*
 * Header Format (protobuf):
 * | total length | size of type | type     | size of msg | msg      |
 * | size_t       | size_t       | variable | size_t      | variable |
 *
 * */


inline Msg5L five_level(const string& string_field) {
    Msg5L msg;
    Msg4L* left = msg.mutable_left();
    Msg4L* right = msg.mutable_right();
    fill_in_four_level(left, string_field);
    fill_in_four_level(right, string_field);
    return msg;
}

inline void fill_in_four_level(Msg4L* msg, const string& string_field)  {
    Msg3L* left = msg->mutable_left();
    Msg3L* right = msg->mutable_right();
    fill_in_three_level(left, string_field);
    fill_in_three_level(right, string_field);
}

inline Msg4L four_level(const string& string_field) {
    Msg4L msg;
    Msg3L* left = msg.mutable_left();
    Msg3L* right = msg.mutable_right();
    fill_in_three_level(left, string_field);
    fill_in_three_level(right, string_field);
    return msg; 
}

inline void fill_in_three_level(Msg3L* msg, const string& string_field) {
    Msg2L* left = msg->mutable_left();
    Msg2L* right = msg->mutable_right();
    fill_in_two_level(left, string_field);
    fill_in_two_level(right, string_field);
}

Msg3L three_level(const string& string_field) {
    Msg3L msg;
    Msg2L* left = msg.mutable_left();
    Msg2L* right = msg.mutable_right();
    fill_in_two_level(left, string_field);
    fill_in_two_level(right, string_field);
    return msg;
}

inline void fill_in_two_level(Msg2L* msg, const string& string_field) {
    Msg1L* left = msg->mutable_left();
    Msg1L* right = msg->mutable_right();
    fill_in_one_level(left, string_field);
    fill_in_one_level(right, string_field);
}

inline Msg2L two_level(const string& string_field) {
    Msg2L msg;
    Msg1L* left = msg.mutable_left();
    Msg1L* right = msg.mutable_right();
    fill_in_one_level(left, string_field);
    fill_in_one_level(right, string_field);
    return msg;
}

inline void fill_in_one_level(Msg1L* msg, const string& string_field) {
    GetMessage *get1 = msg->mutable_left();
    get1->set_key(string_field);
    GetMessage *get2 = msg->mutable_left();
    get2->set_key(string_field);
}


inline Msg1L one_level(const string& string_field) {
    Msg1L msg;
    GetMessage *get1 = msg.mutable_left();
    get1->set_key(string_field);
    GetMessage *get2 = msg.mutable_left();
    get2->set_key(string_field);
    return msg;
}

inline GetMessage get_message(const string& string_field) {
    GetMessage get;
    get.set_key(string_field);
    return get;
}


PutMessage* put_message(const string& string_field) {
    PutMessage* put = new PutMessage;
    put->set_key(string_field);
    put->set_value(string_field);
    return put;
}

protobuf_echo::protobuf_echo(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::PROTOBUF, field_size, message_type),
    string_field(generate_string(field_size)) {}

void protobuf_echo::handle_message(const string& msg) {
    switch (my_msg_enum) {
        case echo_message::msg_type::GET:
            getMsg_deser.ParseFromString(msg);
            break;
        case echo_message::msg_type::PUT:
            putMsg_deser.ParseFromString(msg);
            break;
        case echo_message::msg_type::MSG1L:
            msg1L_deser.ParseFromString(msg);
            break;
        case echo_message::msg_type::MSG2L:
            msg2L_deser.ParseFromString(msg);
            break;
        case echo_message::msg_type::MSG3L:
            msg3L_deser.ParseFromString(msg);
            break;
        case echo_message::msg_type::MSG4L:
            msg4L_deser.ParseFromString(msg);
            break;
        case echo_message::msg_type::MSG5L:
            msg5L_deser.ParseFromString(msg);
            break;
    }
}

void protobuf_echo::deserialize_message(dmtr_sgarray_t &sga) {
    assert(sga.sga_numsegs == 1);
    string data((char *)sga.sga_segs[0].sgaseg_buf, sga.sga_segs[0].sgaseg_len);
    handle_message(data);
}

void protobuf_echo::encode_msg(dmtr_sgarray_t &sga,
                                const Message& msg) {
    sga.sga_numsegs = 1;
    void *p = NULL;
    
    string data = msg.SerializeAsString();
    size_t dataLen = data.length();

    sga.sga_segs[0].sgaseg_len = dataLen;
    dmtr_malloc(&p, dataLen);
    // printf("Allocating protobuf data at: %p, length: %u\n", p, (unsigned)dataLen);
    assert(p != NULL);
    sga.sga_segs[0].sgaseg_buf = p;
    memcpy(p, (void *)data.c_str(), dataLen);
}

void protobuf_echo::serialize_message(dmtr_sgarray_t &sga, void *context) {
    sga.sga_numsegs = 1;
    if (my_msg_enum == echo_message::msg_type::GET) {
        GetMessage getMsg = get_message(string_field);
        encode_msg(sga, getMsg);
    } else if (my_msg_enum == echo_message::msg_type::PUT) {
        PutMessage *putMsg = put_message(string_field);
        encode_msg(sga, *putMsg);
    } else if (my_msg_enum == echo_message::msg_type::MSG1L) {
        Msg1L msg1L = one_level(string_field);
        encode_msg(sga, msg1L);
    } else if (my_msg_enum == echo_message::msg_type::MSG2L) {
        Msg2L msg2L = two_level(string_field);
        encode_msg(sga, msg2L);
    } else if (my_msg_enum == echo_message::msg_type::MSG3L) {
        Msg3L msg3L = three_level(string_field);
        encode_msg(sga, msg3L);
    } else if (my_msg_enum == echo_message::msg_type::MSG4L) {
        Msg4L msg4L = four_level(string_field);
        encode_msg(sga, msg4L);
    } else if (my_msg_enum == echo_message::msg_type::MSG5L) {
        Msg5L msg5L = five_level(string_field);
        encode_msg(sga, msg5L);
    } 
}

