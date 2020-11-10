// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "protobytes.hh"
#include "stress_bytes.pb.h" // protobuf
#include <iostream>
#include <dmtr/libos/mem.h>
#include <string>
#include <assert.h>
#define FILL_CHAR 'a'
// #define DMTR_PROFILE

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

inline stress_bytes::GetMessage get_message_bytes(const string& string_field) {
    stress_bytes::GetMessage get;
    get.set_key(string_field);
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
    string_field(generate_string(field_size)),
    serialize_latency(NULL),
    parse_latency(NULL),
    encode_malloc_latency(NULL),
    encode_memcpy_latency(NULL),
    decode_string_latency(NULL)
    
{
#ifdef DMTR_PROFILE
    dmtr_new_latency(&serialize_latency, "Protobytes serialize count");
    dmtr_new_latency(&parse_latency, "Protobytes parse count");
    dmtr_new_latency(&encode_malloc_latency, "Encode malloc count");
    dmtr_new_latency(&encode_memcpy_latency, "Encode memcpy count");
    dmtr_new_latency(&decode_string_latency, "Decode string count");
#endif

}

void protobuf_bytes_echo::handle_message(const string& msg) {
#ifdef DMTR_PROFILE
    auto start = rdtsc();
#endif
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
#ifdef DMTR_PROFILE
    dmtr_record_latency(parse_latency, rdtsc() - start);
#endif
}

void protobuf_bytes_echo::deserialize_message(dmtr_sgarray_t &sga) {
    assert(sga.sga_numsegs == 1);
#ifdef DMTR_PROFILE
    auto start = rdtsc();
#endif
    string data((char *)sga.sga_segs[0].sgaseg_buf, sga.sga_segs[0].sgaseg_len);
#ifdef DMTR_PROFILE
    dmtr_record_latency(decode_string_latency, rdtsc() - start);
#endif
    //ptr += dataLen;
    handle_message(data);
}

void protobuf_bytes_echo::encode_msg(dmtr_sgarray_t &sga,
                                const Message& msg) {
    sga.sga_numsegs = 1;
    void *p = NULL;
#ifdef DMTR_PROFILE
    auto start_serialize = rdtsc();
#endif
    string data = msg.SerializeAsString();
#ifdef DMTR_PROFILE
    dmtr_record_latency(serialize_latency, rdtsc() - start_serialize);
#endif
    size_t dataLen = data.length();

    sga.sga_segs[0].sgaseg_len = dataLen;
#ifdef DMTR_PROFILE
    auto start_malloc = rdtsc();
#endif
    dmtr_malloc(&p, dataLen);
#ifdef DMTR_PROFILE
    dmtr_record_latency(encode_malloc_latency, rdtsc() - start_malloc);
#endif
    assert(p != NULL);
    sga.sga_segs[0].sgaseg_buf = p;
#ifdef DMTR_PROFILE
    auto start_memcpy = rdtsc();
#endif
    memcpy(p, (void *)data.c_str(), dataLen);
#ifdef DMTR_PROFILE
    dmtr_record_latency(encode_memcpy_latency, rdtsc() - start_memcpy);
#endif
}

void protobuf_bytes_echo::serialize_message(dmtr_sgarray_t &sga, void *context) {
    sga.sga_numsegs = 1;
    if (my_msg_enum == echo_message::msg_type::GET) {
        stress_bytes::GetMessage getMsg = get_message_bytes(string_field);
        encode_msg(sga, getMsg);
    } else if (my_msg_enum == echo_message::msg_type::PUT) {
        stress_bytes::PutMessage *putMsg = put_message_bytes(string_field);
        encode_msg(sga, *putMsg);
    } else if (my_msg_enum == echo_message::msg_type::MSG1L) {
        stress_bytes::Msg1L msg1L = one_level_bytes(string_field);
        encode_msg(sga, msg1L);
    } else if (my_msg_enum == echo_message::msg_type::MSG2L) {
        stress_bytes::Msg2L msg2L = two_level_bytes(string_field);
        encode_msg(sga, msg2L);
    } else if (my_msg_enum == echo_message::msg_type::MSG3L) {
        stress_bytes::Msg3L msg3L = three_level_bytes(string_field);
        encode_msg(sga, msg3L);
    } else if (my_msg_enum == echo_message::msg_type::MSG4L) {
        stress_bytes::Msg4L msg4L = four_level_bytes(string_field);
        encode_msg(sga, msg4L);
    } else if (my_msg_enum == echo_message::msg_type::MSG5L) {
        stress_bytes::Msg5L msg5L = five_level_bytes(string_field);
        encode_msg(sga, msg5L);
    } 
}

void protobuf_bytes_echo::print_counters() {
#ifdef DMTR_PROFILE
    dmtr_dump_latency(stderr, serialize_latency);
    dmtr_dump_latency(stderr, parse_latency);
    dmtr_dump_latency(stderr, encode_malloc_latency);
    dmtr_dump_latency(stderr, encode_memcpy_latency);
    dmtr_dump_latency(stderr, decode_string_latency);
#endif
}
