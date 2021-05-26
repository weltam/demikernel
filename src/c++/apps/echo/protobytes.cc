// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "protobytes.hh"
#include "stress_bytes.pb.h" // protobuf
#include <iostream>
#include <dmtr/libos/mem.h>
#include <string>
#include <assert.h>
#include <boost/chrono.hpp>
#define FILL_CHAR 'a'
//#define DMTR_PROFILE

/*
 * Header Format (protobuf):
 * | total length | size of type | type     | size of msg | msg      |
 * | size_t       | size_t       | variable | size_t      | variable |
 *
 * */
inline stress_bytes::Msg5L five_level_bytes(const string& string_field) {
    stress_bytes::Msg5L msg;
    stress_bytes::Msg4L* left = msg.mutable_left();
    stress_bytes::Msg4L* right = msg.mutable_right();
    fill_in_four_level_bytes(left, string_field);
    fill_in_four_level_bytes(right, string_field);
    return msg;
}

inline void fill_in_four_level_bytes(stress_bytes::Msg4L* msg, const string& string_field)  {
    stress_bytes::Msg3L* left = msg->mutable_left();
    stress_bytes::Msg3L* right = msg->mutable_right();
    fill_in_three_level_bytes(left, string_field);
    fill_in_three_level_bytes(right, string_field);
}

inline stress_bytes::Msg4L four_level_bytes(const string& string_field) {
    stress_bytes::Msg4L msg;
    stress_bytes::Msg3L* left = msg.mutable_left();
    stress_bytes::Msg3L* right = msg.mutable_right();
    fill_in_three_level_bytes(left, string_field);
    fill_in_three_level_bytes(right, string_field);
    return msg; 
}

inline void fill_in_three_level_bytes(stress_bytes::Msg3L* msg, const string& string_field) {
    stress_bytes::Msg2L* left = msg->mutable_left();
    stress_bytes::Msg2L* right = msg->mutable_right();
    fill_in_two_level_bytes(left, string_field);
    fill_in_two_level_bytes(right, string_field);
}

stress_bytes::Msg3L three_level_bytes(const string& string_field) {
    stress_bytes::Msg3L msg;
    stress_bytes::Msg2L* left = msg.mutable_left();
    stress_bytes::Msg2L* right = msg.mutable_right();
    fill_in_two_level_bytes(left, string_field);
    fill_in_two_level_bytes(right, string_field);
    return msg;
}

inline void fill_in_two_level_bytes(stress_bytes::Msg2L* msg, const string& string_field) {
    stress_bytes::Msg1L* left = msg->mutable_left();
    stress_bytes::Msg1L* right = msg->mutable_right();
    fill_in_one_level_bytes(left, string_field);
    fill_in_one_level_bytes(right, string_field);
}

inline stress_bytes::Msg2L two_level_bytes(const string& string_field) {
    stress_bytes::Msg2L msg;
    stress_bytes::Msg1L* left = msg.mutable_left();
    stress_bytes::Msg1L* right = msg.mutable_right();
    fill_in_one_level_bytes(left, string_field);
    fill_in_one_level_bytes(right, string_field);
    return msg;
}

inline void fill_in_one_level_bytes(stress_bytes::Msg1L* msg, const string& string_field) {
    stress_bytes::GetMessage *get1 = msg->mutable_left();
    get1->set_key(string_field);
    stress_bytes::GetMessage *get2 = msg->mutable_left();
    get2->set_key(string_field);
}


inline stress_bytes::Msg1L one_level_bytes(const string& string_field) {
    stress_bytes::Msg1L msg;
    stress_bytes::GetMessage *get1 = msg.mutable_left();
    get1->set_key(string_field);
    stress_bytes::GetMessage *get2 = msg.mutable_left();
    get2->set_key(string_field);
    return msg;
}

#ifdef DMTR_PROFILE
inline stress_bytes::GetMessage get_message_bytes(const string& string_field, dmtr_latency_t *alloc_latency, dmtr_latency_t *copy_latency) {
#else
inline stress_bytes::GetMessage get_message_bytes(const string& string_field) {
#endif
#ifdef DMTR_PROFILE
    auto start_alloc = boost::chrono::steady_clock::now();
#endif
    stress_bytes::GetMessage get;
#ifdef DMTR_PROFILE
    auto end_alloc = boost::chrono::steady_clock::now();
#endif
    get.set_key(string_field);
#ifdef DMTR_PROFILE
    auto end_set = boost::chrono::steady_clock::now();
    dmtr_record_latency(alloc_latency, (end_alloc - start_alloc).count());
    dmtr_record_latency(copy_latency, (end_set - end_alloc).count());
#endif
    return get;
}




stress_bytes::PutMessage* put_message_bytes(const string& string_field) {
    stress_bytes::PutMessage* put = new stress_bytes::PutMessage;
    put->set_key(string_field);
    put->set_value(string_field);
    return put;
}

protobuf_bytes_echo::protobuf_bytes_echo(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::PROTOBUF, field_size, message_type),
#ifdef DMTR_PROFILE
    alloc_latency(NULL),
    copy_latency(NULL),
    encode_latency(NULL),
    decode_latency(NULL),
#endif
    string_field(generate_string(field_size))
{
#ifdef DMTR_PROFILE
    dmtr_new_latency(&alloc_latency, "Protobytes alloc count");
    dmtr_new_latency(&copy_latency, "Protobytes copy count");
    dmtr_new_latency(&encode_latency, "Encode count");
    dmtr_new_latency(&decode_latency, "Decode count");
#endif
}

void protobuf_bytes_echo::handle_message(void *buf, size_t len) {
#ifdef DMTR_PROFILE
    auto start =  boost::chrono::steady_clock::now();
#endif
    if (my_msg_enum == echo_message::msg_type::GET) {
        stress_bytes::GetMessage getMsg_deser;
        getMsg_deser.ParseFromArray(buf, len);
    } else if (my_msg_enum == echo_message::msg_type::PUT) {
        stress_bytes::PutMessage putMsg_deser;
        putMsg_deser.ParseFromArray(buf, len);
    } else if (my_msg_enum == echo_message::msg_type::MSG1L) {
        stress_bytes::Msg1L msg1L_deser;
        msg1L_deser.ParseFromArray(buf, len);
    } else if (my_msg_enum == echo_message::msg_type::MSG2L) {
        stress_bytes::Msg2L msg2L_deser;
        msg2L_deser.ParseFromArray(buf, len);
    } else if (my_msg_enum == echo_message::msg_type::MSG3L) {
        stress_bytes::Msg3L msg3L_deser;
        msg3L_deser.ParseFromArray(buf, len);
    } else if (my_msg_enum == echo_message::msg_type::MSG4L) {
        stress_bytes::Msg4L msg4L_deser;
        msg4L_deser.ParseFromArray(buf, len);
    } else if (my_msg_enum == echo_message::msg_type::MSG5L) {
        stress_bytes::Msg5L msg5L_deser;
        msg5L_deser.ParseFromArray(buf, len);
    }
#ifdef DMTR_PROFILE
    auto end = boost::chrono::steady_clock::now();
    dmtr_record_latency(decode_latency, (end - start).count());
#endif
}

void protobuf_bytes_echo::deserialize_message(dmtr_sgarray_t &sga) {
    assert(sga.sga_numsegs == 1);
    handle_message(sga.sga_segs[0].sgaseg_buf, sga.sga_segs[0].sgaseg_len);
}

void protobuf_bytes_echo::encode_msg(dmtr_sgarray_t &sga,
                                const Message& msg, void *context) {
    sga.sga_numsegs = 1;
    void *buf = NULL;
    dmtr_malloc(&buf, msg.ByteSizeLong());
    msg.SerializeToArray(buf, msg.ByteSizeLong());
    sga.sga_segs[0].sgaseg_len = msg.ByteSizeLong();
    sga.sga_segs[0].sgaseg_buf = buf;
}

void protobuf_bytes_echo::serialize_message(dmtr_sgarray_t &sga, void *context) {
    sga.sga_numsegs = 1;
    if (my_msg_enum == echo_message::msg_type::GET) {
#ifdef DMTR_PROFILE
        stress_bytes::GetMessage getMsg = get_message_bytes(string_field, alloc_latency, copy_latency);
        auto start_encode = boost::chrono::steady_clock::now();
#else
        stress_bytes::GetMessage getMsg = get_message_bytes(string_field);
#endif
        encode_msg(sga, getMsg, context);
#ifdef DMTR_PROFILE
        auto end_encode = boost::chrono::steady_clock::now();
        dmtr_record_latency(encode_latency, (end_encode - start_encode).count());
#endif
    } else if (my_msg_enum == echo_message::msg_type::PUT) {
        stress_bytes::PutMessage *putMsg = put_message_bytes(string_field);
        encode_msg(sga, *putMsg, context);
    } else if (my_msg_enum == echo_message::msg_type::MSG1L) {
        stress_bytes::Msg1L msg1L = one_level_bytes(string_field);
        encode_msg(sga, msg1L, context);
    } else if (my_msg_enum == echo_message::msg_type::MSG2L) {
        stress_bytes::Msg2L msg2L = two_level_bytes(string_field);
        encode_msg(sga, msg2L, context);
    } else if (my_msg_enum == echo_message::msg_type::MSG3L) {
        stress_bytes::Msg3L msg3L = three_level_bytes(string_field);
        encode_msg(sga, msg3L, context);
    } else if (my_msg_enum == echo_message::msg_type::MSG4L) {
        stress_bytes::Msg4L msg4L = four_level_bytes(string_field);
        encode_msg(sga, msg4L, context);
    } else if (my_msg_enum == echo_message::msg_type::MSG5L) {
        stress_bytes::Msg5L msg5L = five_level_bytes(string_field);
        encode_msg(sga, msg5L, context);
    } 
}

void protobuf_bytes_echo::print_counters() {
#ifdef DMTR_PROFILE
    dmtr_dump_latency(stderr, alloc_latency);
    dmtr_dump_latency(stderr, copy_latency);
    dmtr_dump_latency(stderr, encode_latency);
    dmtr_dump_latency(stderr, decode_latency);
#endif
}
