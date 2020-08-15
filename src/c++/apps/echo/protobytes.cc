// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "protobytes.hh"
#include "stress_bytes.pb.h" // protobuf
#include <iostream>
#include <dmtr/libos/mem.h>
#include <string>
#include <assert.h>
#define FILL_CHAR 'a'
#define DMTR_PROFILE

/*
 * Header Format (protobuf):
 * | total length | size of type | type     | size of msg | msg      |
 * | size_t       | size_t       | variable | size_t      | variable |
 *
 * */


stress_bytes::Msg5L* five_level_bytes(uint32_t field_size) {
    stress_bytes::Msg5L* msg = new stress_bytes::Msg5L;
    stress_bytes::Msg4L* left = four_level_bytes(field_size/2);
    stress_bytes::Msg4L* right = four_level_bytes(field_size/2);
    msg->set_allocated_left(left);
    msg->set_allocated_right(right);
    return msg;
}

stress_bytes::Msg4L* four_level_bytes(uint32_t field_size) {
    stress_bytes::Msg4L* msg = new stress_bytes::Msg4L;
    stress_bytes::Msg3L* left = three_level_bytes(field_size/2);
    stress_bytes::Msg3L* right = three_level_bytes(field_size/2);
    msg->set_allocated_left(left);
    msg->set_allocated_right(right);
    return msg; 
}

stress_bytes::Msg3L* three_level_bytes(uint32_t field_size) {
    stress_bytes::Msg3L* msg = new stress_bytes::Msg3L;
    stress_bytes::Msg2L* left = two_level_bytes(field_size/2);
    stress_bytes::Msg2L* right = two_level_bytes(field_size/2);
    msg->set_allocated_left(left);
    msg->set_allocated_right(right);
    return msg; 
}

stress_bytes::Msg2L* two_level_bytes(uint32_t field_size) {
    stress_bytes::Msg2L* msg = new stress_bytes::Msg2L;
    stress_bytes::Msg1L* left = one_level_bytes(field_size/2);
    stress_bytes::Msg1L* right = one_level_bytes(field_size/2);
    msg->set_allocated_left(left);
    msg->set_allocated_right(right);
    return msg; 
}

stress_bytes::Msg1L* one_level_bytes(uint32_t field_size) {
    stress_bytes::Msg1L* msg = new stress_bytes::Msg1L;
    stress_bytes::GetMessage* get1 = get_message_bytes(field_size/2);
    stress_bytes::GetMessage* get2 = get_message_bytes(field_size/2);
    msg->set_allocated_left(get1);
    msg->set_allocated_right(get2);
    return msg;
}

stress_bytes::GetMessage* get_message_bytes(uint32_t field_size) {
    stress_bytes::GetMessage* get = new stress_bytes::GetMessage;
    get->set_key(generate_string(field_size));
    return get;
}

stress_bytes::PutMessage* put_message_bytes(uint32_t field_size) {
    stress_bytes::PutMessage* put = new stress_bytes::PutMessage;
    put->set_key(generate_string(field_size));
    put->set_value(generate_string(field_size));
    return put;
}

protobuf_bytes_echo::protobuf_bytes_echo(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::PROTOBUF, field_size, message_type),
    getMsg(),
    putMsg(),
    msg1L(),
    msg2L(),
    msg3L(),
    msg4L(),
    msg5L(),
    getMsg_deser(),
    putMsg_deser(),
    msg1L_deser(),
    msg2L_deser(),
    msg3L_deser(),
    msg4L_deser(),
    msg5L_deser(),
    serialize_latency(NULL),
    parse_latency(NULL),
    encode_malloc_latency(NULL),
    encode_memcpy_latency(NULL),
    decode_string_latency(NULL)
    
{
    switch (my_msg_enum) {
        case echo_message::msg_type::GET:
            getMsg = *get_message_bytes(field_size);
            break;
        case echo_message::msg_type::PUT:
            putMsg = *put_message_bytes(field_size);
            break;
        case echo_message::msg_type::MSG1L:
            msg1L = *one_level_bytes(field_size);
            break;
        case echo_message::msg_type::MSG2L:
            msg2L = *two_level_bytes(field_size);
            break;
        case echo_message::msg_type::MSG3L:
            msg3L = *three_level_bytes(field_size);
            break;
        case echo_message::msg_type::MSG4L:
            msg4L = *four_level_bytes(field_size);
            break;
        case echo_message::msg_type::MSG5L:
            msg5L = *five_level_bytes(field_size);
            break;
    }
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
    /*uint8_t *ptr = (uint8_t *)sga.sga_segs[0].sgaseg_buf;
    
    size_t totalLen = *(size_t *)ptr;
    ptr += sizeof(totalLen);
    
    size_t typeLen = *(size_t *)ptr;
    ptr += sizeof(typeLen);
    string type((char *)ptr, typeLen);
    ptr += typeLen;
    
    size_t dataLen = *(size_t *)ptr;
    ptr += sizeof(dataLen);
    string data((char *)ptr, dataLen);*/
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
    /*size_t typeLen = my_message_type.length();
    size_t totalLen = (typeLen + sizeof(typeLen) +
                        dataLen + sizeof(dataLen) +
                        sizeof(totalLen));

    dmtr_malloc(&p, totalLen);
    assert(p != NULL);
    sga.sga_segs[0].sgaseg_len = totalLen;
    sga.sga_segs[0].sgaseg_buf = p;
    char *buf = reinterpret_cast<char *>(p);
    char *ptr = buf;

    *((size_t *) ptr) = totalLen;
    ptr += sizeof(totalLen);
    assert((size_t)(ptr-buf) < totalLen);

    *((size_t *) ptr) = typeLen;
    ptr += sizeof(typeLen);
    assert((size_t)(ptr-buf) < totalLen);
    
    memcpy(ptr, my_message_type.c_str(), typeLen);
    ptr += typeLen;
    assert((size_t)(ptr-buf) < totalLen);

    *((size_t *) ptr) = dataLen;
    ptr += sizeof(dataLen);
    assert((size_t)(ptr-buf) < totalLen);

    memcpy(ptr, data.c_str(), dataLen);
    ptr += dataLen;
    assert((size_t)(ptr-buf) == totalLen);*/

}

void protobuf_bytes_echo::serialize_message(dmtr_sgarray_t &sga) {
    sga.sga_numsegs = 1;

    switch (my_msg_enum) {
        case echo_message::msg_type::GET:
            encode_msg(sga, getMsg);
            break;
        case echo_message::msg_type::PUT:
            encode_msg(sga, putMsg);
            break;
        case echo_message::msg_type::MSG1L:
            encode_msg(sga, msg1L);
            break;
        case echo_message::msg_type::MSG2L:
            encode_msg(sga, msg2L);
            break;
        case echo_message::msg_type::MSG3L:
            encode_msg(sga, msg3L);
            break;
        case echo_message::msg_type::MSG4L:
            encode_msg(sga, msg4L);
            break;
        case echo_message::msg_type::MSG5L:
            encode_msg(sga, msg5L);
            break;
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
