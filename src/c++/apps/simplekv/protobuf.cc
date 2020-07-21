// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "kv.hh"
#include "protobuf.hh"
#include "stress.pb.h" // protobuf
#include <iostream>
#include <dmtr/libos/mem.h>
#include <string>
#include <assert.h>

/*
 * Header Format (protobuf):
 * | type enum    | size of msg | msg       |
 * | int32_t      | size_t      |  variable |
 *
 * */

protobuf_kv::protobuf_kv() :
    simplekv(simplekv::library::PROTOBUF)
{}

void protobuf_kv::client_send_get(int req_id, simplekv::StringPointer key, dmtr_sgarray_t &sga) {
    GetMessageProto* get = new GetMessageProto;
    get->set_key(key.string());
    get->set_req_id(req_id);
    encode_msg(sga, *get, simplekv::request::GET);
}

void protobuf_kv::client_send_put(int req_id, simplekv::StringPointer key, simplekv::StringPointer value, dmtr_sgarray_t &sga) {
    PutMessageProto* put = new PutMessageProto;
    put->set_key(key.string());
    put->set_req_id(req_id);
    put->set_value(value.string());
    encode_msg(sga, *put, simplekv::request::PUT);
}

int protobuf_kv::client_handle_response(dmtr_sgarray_t &sga) {
    simplekv::request msg_type;
    string data = decode_msg(sga, &msg_type);
    ResponseProto response;

    switch(msg_type) {
        case simplekv::request::RESPONSE:
            response.ParseFromString(data);
            return response.req_id();
        default:
            std::cerr << "Client received back non response message type of " << std::endl;
            exit(1);
    }

}

string protobuf_kv::client_check_response(dmtr_sgarray_t &sga) {
    simplekv::request msg_type;
    string data = decode_msg(sga, &msg_type);
    ResponseProto response;

    switch(msg_type) {
        case simplekv::request::RESPONSE:
            return response.value();
        default:
            std::cerr << "Client received back non response message type of " << std::endl;
            exit(1);
    }
    
}

int protobuf_kv::server_handle_request(dmtr_sgarray_t &in_sga, dmtr_sgarray_t &out_sga, bool* free_in, bool* free_out) {
    simplekv::request msg_type;
    string data = decode_msg(in_sga, &msg_type);

    GetMessageProto get;
    PutMessageProto put;
    ResponseProto* response = new ResponseProto;
    *free_in = true;
    *free_out = true;

    switch(msg_type) {
        case simplekv::request::GET:
            get.ParseFromString(data);
            response->set_value(my_map.at(get.key()));
            response->set_req_id(get.req_id());
            encode_msg(out_sga, *response, simplekv::request::RESPONSE);
            break;
        case simplekv::request::PUT:
            put.ParseFromString(data);
            my_map[put.key()] = put.value();
            response->set_req_id(put.req_id());
            encode_msg(out_sga, *response, simplekv::request::RESPONSE);
            break;
        default:
            std::cerr << "Server received msg of type not put or get." << std::endl;
            exit(1);
    }
    return 0;
}

void protobuf_kv::encode_msg(dmtr_sgarray_t &sga, const Message& msg, simplekv::request msg_type) {
    string data = msg.SerializeAsString();
    size_t dataLen = data.length();
    int32_t type = encode_enum(msg_type);
    size_t totalLen = sizeof(dataLen) + sizeof(type) + dataLen;

    void *p = NULL;
    dmtr_malloc(&p, totalLen);
    assert(p != NULL);
    sga.sga_numsegs = 1;
    sga.sga_segs[0].sgaseg_len = totalLen;
    sga.sga_segs[0].sgaseg_buf = p;
    char *buf = reinterpret_cast<char *>(p);
    char *ptr = buf;

    *((int32_t *) ptr) = type;
    ptr += sizeof(type);
    assert((size_t)(ptr - buf) < totalLen);

    *((size_t *) ptr) = dataLen;
    ptr += sizeof(dataLen);
    assert((size_t)(ptr - buf) < totalLen);

    memcpy(ptr, data.c_str(), dataLen);
    ptr += dataLen;
    assert((size_t)(ptr-buf) == totalLen);

}

string protobuf_kv::decode_msg(dmtr_sgarray_t &sga, simplekv::request* msg_type) {
    assert(sga.sga_numsegs == 1);
    uint8_t *ptr = (uint8_t *)sga.sga_segs[0].sgaseg_buf;

    int32_t type = *(int32_t *)ptr;
    ptr += sizeof(type);
    *msg_type = decode_enum(type);

    size_t dataLen = *(size_t *)ptr;
    ptr += sizeof(dataLen);

    string data((char *)ptr, dataLen);
    return data;
}

