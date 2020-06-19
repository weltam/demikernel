// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "protobuf.hh"
#include "stress.pb.h"
#include <iostream>
#include <dmtr/libos/mem.h>
#include <string>
#include <assert.h>
#define FILL_CHAR 'a'

/*
 * Header Format:
 * | total length | size of type | type     | size of msg | msg      |
 * | size_t       | size_t       | variable | size_t      | variable |
 *
 * */
GetMessage getMsg;
PutMessage putMsg;
Msg1L msg1L;
Msg2L msg2L;
Msg3L msg3L;
Msg4L msg4L;
Msg5L msg5L;

void init_protobuf(const string& type, uint32_t field_size) {
    if (type == getMsg.GetTypeName()) {
        getMsg = *get_message(field_size);
    } else if (type == putMsg.GetTypeName()) {
        putMsg = *put_message(field_size);
    } else if (type == msg1L.GetTypeName()) {
        msg1L = *one_level(field_size);
    } else if (type == msg2L.GetTypeName()) {
        msg2L = *two_level(field_size);
    } else if (type == msg3L.GetTypeName()) {
        msg3L = *three_level(field_size);
    } else if (type == msg4L.GetTypeName()) {
        msg4L = *four_level(field_size);
    } else if (type == msg5L.GetTypeName()) {
        msg5L = *five_level(field_size);
    }
}

string generate_string(uint32_t field_size) {
    return string(field_size, FILL_CHAR);
}

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

void handle_message(const string& type, const string& msg) {
    GetMessage getMessage_local;
    PutMessage putMessage_local;
    Msg1L msg1L_local;
    Msg2L msg2L_local;
    Msg3L msg3L_local;
    Msg4L msg4L_local;
    Msg5L msg5L_local;

    if (type == getMessage_local.GetTypeName()) {
        getMessage_local.ParseFromString(msg);
    } else if (type == putMessage_local.GetTypeName()) {
        putMessage_local.ParseFromString(msg);
    } else if (type == msg1L_local.GetTypeName()) {
        msg1L_local.ParseFromString(msg);
    } else if (type == msg2L_local.GetTypeName()) {
        msg2L_local.ParseFromString(msg);
    } else if (type == msg3L_local.GetTypeName()) {
        msg3L_local.ParseFromString(msg);
    } else if (type == msg4L.GetTypeName()) {
        msg4L_local.ParseFromString(msg);
    } else if (type == msg5L_local.GetTypeName()) {
        msg5L_local.ParseFromString(msg);
    } else {
        std::cerr << "Unknown type for " << type << std::endl;
        exit(1);
    }
}

void decode_serialized_msg(dmtr_sgarray_t &sga) {
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

    handle_message(type, data);
}

void generate_serialized_msg(dmtr_sgarray_t &sga,
                                const Message& msg,
                                const string& type) {
    void *p = NULL;
    
    string data = msg.SerializeAsString();
    size_t dataLen = data.length();
    size_t typeLen = type.length();
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
    
    memcpy(ptr, type.c_str(), typeLen);
    ptr += typeLen;
    assert((size_t)(ptr-buf) < totalLen);

    *((size_t *) ptr) = dataLen;
    ptr += sizeof(dataLen);
    assert((size_t)(ptr-buf) < totalLen);

    memcpy(ptr, data.c_str(), dataLen);
    ptr += dataLen;
    assert((size_t)(ptr-buf) == totalLen);

}

void generate_protobuf_packet(dmtr_sgarray_t &sga,
                                const string& type,
                                uint32_t field_size) {
    sga.sga_numsegs = 1;
    // GetMessage getMessage;
    // PutMessage putMessage;
    // Msg1L msg1L;
    // Msg2L msg2L;
    // Msg3L msg3L;
    // Msg4L msg4L;
    // Msg5L msg5L;

    if (type == getMsg.GetTypeName()) {
        //getMessage = *get_message(field_size);
        generate_serialized_msg(sga, getMsg, type);
    } else if (type == putMsg.GetTypeName()) {
        //putMessage = *put_message(field_size);
        generate_serialized_msg(sga, putMsg, type);
    } else if (type == msg1L.GetTypeName()) {
        //msg1L = *one_level(field_size);
        generate_serialized_msg(sga, msg1L, type);
    } else if (type == msg2L.GetTypeName()) {
        //msg2L = *two_level(field_size);
        generate_serialized_msg(sga, msg2L, type);
    } else if (type == msg3L.GetTypeName()) {
        //msg3L = *three_level(field_size);
        generate_serialized_msg(sga, msg3L, type);
    } else if (type == msg4L.GetTypeName()) {
        //msg4L = *four_level(field_size);
        generate_serialized_msg(sga, msg4L, type);
    } else if (type == msg5L.GetTypeName()) {
        //msg5L = *five_level(field_size;
        generate_serialized_msg(sga, msg5L, type);
    } else {
        std::cerr << "Unknown type for " << type << std::endl;
        exit(1);
    }

}
