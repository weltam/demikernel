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


Msg5L* five_level(uint32_t field_size) {
    Msg5L* msg = new Msg5L;
    Msg4L* left = four_level(field_size/2);
    Msg4L* right = four_level(field_size/2);
    msg->set_allocated_left(left);
    msg->set_allocated_right(right);
    return msg;
}

Msg4L* four_level(uint32_t field_size) {
    Msg4L* msg = new Msg4L;
    Msg3L* left = three_level(field_size/2);
    Msg3L* right = three_level(field_size/2);
    msg->set_allocated_left(left);
    msg->set_allocated_right(right);
    return msg; 
}

Msg3L* three_level(uint32_t field_size) {
    Msg3L* msg = new Msg3L;
    Msg2L* left = two_level(field_size/2);
    Msg2L* right = two_level(field_size/2);
    msg->set_allocated_left(left);
    msg->set_allocated_right(right);
    return msg; 
}

Msg2L* two_level(uint32_t field_size) {
    Msg2L* msg = new Msg2L;
    Msg1L* left = one_level(field_size/2);
    Msg1L* right = one_level(field_size/2);
    msg->set_allocated_left(left);
    msg->set_allocated_right(right);
    return msg; 
}

Msg1L* one_level(uint32_t field_size) {
    Msg1L* msg = new Msg1L;
    GetMessage* get1 = get_message(field_size/2);
    GetMessage* get2 = get_message(field_size/2);
    msg->set_allocated_left(get1);
    msg->set_allocated_right(get2);
    return msg;
}

GetMessage* get_message(uint32_t field_size) {
    GetMessage* get = new GetMessage;
    get->set_key(generate_string(field_size));
    return get;
}

PutMessage* put_message(uint32_t field_size) {
    PutMessage* put = new PutMessage;
    put->set_key(generate_string(field_size));
    put->set_value(generate_string(field_size));
    return put;
}

protobuf_echo::protobuf_echo(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::PROTOBUF, field_size, message_type),
    getMsg(),
    putMsg(),
    msg1L(),
    msg2L(),
    msg3L(),
    msg4L(),
    msg5L()
{
    switch (my_msg_enum) {
        case echo_message::msg_type::GET:
            getMsg = *get_message(field_size);
            break;
        case echo_message::msg_type::PUT:
            putMsg = *put_message(field_size);
            break;
        case echo_message::msg_type::MSG1L:
            msg1L = *one_level(field_size);
            break;
        case echo_message::msg_type::MSG2L:
            msg2L = *two_level(field_size);
            break;
        case echo_message::msg_type::MSG3L:
            msg3L = *three_level(field_size);
            break;
        case echo_message::msg_type::MSG4L:
            msg4L = *four_level(field_size);
            break;
        case echo_message::msg_type::MSG5L:
            msg5L = *five_level(field_size);
            break;
    }

}

void protobuf_echo::handle_message(const string& msg) {
    GetMessage getMessage_local;
    PutMessage putMessage_local;
    Msg1L msg1L_local;
    Msg2L msg2L_local;
    Msg3L msg3L_local;
    Msg4L msg4L_local;
    Msg5L msg5L_local;

    switch (my_msg_enum) {
        case echo_message::msg_type::GET:
            getMessage_local.ParseFromString(msg);
            break;
        case echo_message::msg_type::PUT:
            putMessage_local.ParseFromString(msg);
            break;
        case echo_message::msg_type::MSG1L:
            msg1L_local.ParseFromString(msg);
            break;
        case echo_message::msg_type::MSG2L:
            msg2L_local.ParseFromString(msg);
            break;
        case echo_message::msg_type::MSG3L:
            msg3L_local.ParseFromString(msg);
            break;
        case echo_message::msg_type::MSG4L:
            msg4L_local.ParseFromString(msg);
            break;
        case echo_message::msg_type::MSG5L:
            msg5L_local.ParseFromString(msg);
            break;
    }
}

void protobuf_echo::deserialize_message(dmtr_sgarray_t &sga) {
    assert(sga.sga_numsegs == 1);
    uint8_t *ptr = (uint8_t *)sga.sga_segs[0].sgaseg_buf;
    
    size_t totalLen = *(size_t *)ptr;
    ptr += sizeof(totalLen);
    
    size_t typeLen = *(size_t *)ptr;
    ptr += sizeof(typeLen);
    string type((char *)ptr, typeLen);
    ptr += typeLen;
    
    size_t dataLen = *(size_t *)ptr;
    ptr += sizeof(dataLen);
    string data((char *)ptr, dataLen);
    ptr += dataLen;
    handle_message(data);
}

void protobuf_echo::encode_msg(dmtr_sgarray_t &sga,
                                const Message& msg) {
    void *p = NULL;
    
    string data = msg.SerializeAsString();
    size_t dataLen = data.length();
    size_t typeLen = my_message_type.length();
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
    assert((size_t)(ptr-buf) == totalLen);

}

void protobuf_echo::serialize_message(dmtr_sgarray_t &sga) {
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

