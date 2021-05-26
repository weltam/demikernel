// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "protobuf.hh"
#include "stress.pb.h" // protobuf
#include <iostream>
#include <dmtr/libos/mem.h>
#include <string>
#include <assert.h>
#include <dmtr/latency.h>
#include <boost/chrono.hpp>
#define FILL_CHAR 'a'
//#define DMTR_PROFILE

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

#ifdef DMTR_PROFILE
inline GetMessage get_message(const string& string_field, dmtr_latency_t *alloc_latency, dmtr_latency_t *copy_latency) {
#else
inline GetMessage get_message(const string& string_field) {
#endif
#ifdef DMTR_PROFILE
    auto start_alloc = boost::chrono::steady_clock::now();
#endif
    GetMessage get;
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


PutMessage* put_message(const string& string_field) {
    PutMessage* put = new PutMessage;
    put->set_key(string_field);
    put->set_value(string_field);
    return put;
}

protobuf_echo::protobuf_echo(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::PROTOBUF, field_size, message_type),
#ifdef DMTR_PROFILE
    alloc_latency(NULL),
    copy_latency(NULL),
    encode_latency(NULL),
    decode_latency(NULL),
#endif
    string_field(generate_string(field_size)) {
#ifdef DMTR_PROFILE
        dmtr_new_latency(&alloc_latency, "Protobuf serialize alloc");
        dmtr_new_latency(&copy_latency, "Protobuf serialize copy");
        dmtr_new_latency(&encode_latency, "Protobuf serialize encode");
        dmtr_new_latency(&decode_latency, "Protobuf serialize decode");
#endif
    }

void protobuf_echo::handle_message(void *buf, size_t len) {
    if (my_msg_enum == echo_message::msg_type::GET) {
        GetMessage getMsg_deser;
        getMsg_deser.ParseFromArray(buf, len);
    } else if (my_msg_enum == echo_message::msg_type::PUT) {
        PutMessage putMsg_deser;
        putMsg_deser.ParseFromArray(buf, len);
    } else if (my_msg_enum == echo_message::msg_type::MSG1L) {
        Msg1L msg1L_deser;
        msg1L_deser.ParseFromArray(buf, len);
    } else if (my_msg_enum == echo_message::msg_type::MSG2L) {
        Msg2L msg2L_deser;
        msg2L_deser.ParseFromArray(buf, len);
    } else if (my_msg_enum == echo_message::msg_type::MSG3L) {
        Msg3L msg3L_deser;
        msg3L_deser.ParseFromArray(buf, len);
    } else if (my_msg_enum == echo_message::msg_type::MSG4L) {
        Msg4L msg4L_deser;
        msg4L_deser.ParseFromArray(buf, len);
    } else if (my_msg_enum == echo_message::msg_type::MSG5L) {
        Msg5L msg5L_deser;
        msg5L_deser.ParseFromArray(buf, len);
    }
}

void protobuf_echo::deserialize_message(dmtr_sgarray_t &sga) {
#ifdef DMTR_PROFILE
    auto start_decode = boost::chrono::steady_clock::now();
#endif
    assert(sga.sga_numsegs == 1);
    handle_message(sga.sga_segs[0].sgaseg_buf, sga.sga_segs[0].sgaseg_len);
#ifdef DMTR_PROFILE
    auto end_decode = boost::chrono::steady_clock::now();
    dmtr_record_latency(decode_latency, (end_decode - start_decode).count());
#endif
}

void protobuf_echo::encode_msg(dmtr_sgarray_t &sga,
                                const Message& msg, void *context) {
    sga.sga_numsegs = 1;
    void *buf = NULL;
    dmtr_malloc(&buf, msg.ByteSizeLong());
    msg.SerializeToArray(buf, msg.ByteSizeLong());

    sga.sga_segs[0].sgaseg_len = msg.ByteSizeLong();
    sga.sga_segs[0].sgaseg_buf = buf;
}

void protobuf_echo::serialize_message(dmtr_sgarray_t &sga, void *context) {
    sga.sga_numsegs = 1;
    if (my_msg_enum == echo_message::msg_type::GET) {
#ifdef DMTR_PROFILE
        GetMessage getMsg = get_message(string_field, alloc_latency, copy_latency);
        auto start_encode = boost::chrono::steady_clock::now();
#else
        GetMessage getMsg = get_message(string_field);
#endif
        encode_msg(sga, getMsg, context);
#ifdef DMTR_PROFILE
        auto end_encode = boost::chrono::steady_clock::now();
        dmtr_record_latency(encode_latency, (end_encode - start_encode).count());
#endif
    } else if (my_msg_enum == echo_message::msg_type::PUT) {
        PutMessage *putMsg = put_message(string_field);
        encode_msg(sga, *putMsg, context);
    } else if (my_msg_enum == echo_message::msg_type::MSG1L) {
        Msg1L msg1L = one_level(string_field);
        encode_msg(sga, msg1L, context);
    } else if (my_msg_enum == echo_message::msg_type::MSG2L) {
        Msg2L msg2L = two_level(string_field);
        encode_msg(sga, msg2L, context);
    } else if (my_msg_enum == echo_message::msg_type::MSG3L) {
        Msg3L msg3L = three_level(string_field);
        encode_msg(sga, msg3L, context);
    } else if (my_msg_enum == echo_message::msg_type::MSG4L) {
        Msg4L msg4L = four_level(string_field);
        encode_msg(sga, msg4L, context);
    } else if (my_msg_enum == echo_message::msg_type::MSG5L) {
        Msg5L msg5L = five_level(string_field);
        encode_msg(sga, msg5L, context);
    } 
}

void protobuf_echo::print_counters() {
#ifdef DMTR_PROFILE
    dmtr_dump_latency(stderr, alloc_latency);
    dmtr_dump_latency(stderr, copy_latency);
    dmtr_dump_latency(stderr, encode_latency);
    dmtr_dump_latency(stderr, decode_latency);
#endif
}

