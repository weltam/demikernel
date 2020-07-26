// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "kv.hh"
#include "flatbuffers.hh"
#include <string>
#include <dmtr/sga.h>
#include <dmtr/libos/mem.h>
#include <sys/types.h>
#include <iostream>
#include <assert.h>

flatbuffers_kv::flatbuffers_kv() :
    simplekv(simplekv::library::FLATBUFFERS)
{}

void flatbuffers_kv::client_send_get(int req_id, simplekv::StringPointer key, dmtr_sgarray_t &sga, void *context) {
    flatbuffers::FlatBufferBuilder* builder = reinterpret_cast<flatbuffers::FlatBufferBuilder *>(context);
    GetMessageFBBuilder getMsg(*builder);
    auto msgKey = builder->CreateString((char *)key.ptr, key.len);
    getMsg.add_key(msgKey);
    getMsg.add_req_id(req_id);
    auto finalMsg = getMsg.Finish();
    builder->Finish(finalMsg);
    encode_msg(sga, builder->GetBufferPointer(), builder->GetSize(), simplekv::request::GET);
}

void flatbuffers_kv::client_send_put(int req_id, simplekv::StringPointer key, simplekv::StringPointer value, dmtr_sgarray_t &sga, void *context) {
    flatbuffers::FlatBufferBuilder* builder = reinterpret_cast<flatbuffers::FlatBufferBuilder *>(context);
    PutMessageFBBuilder putMsg(*builder);
    auto msgKey = builder->CreateString((char *)key.ptr, key.len);
    auto msgValue = builder->CreateString((char *)value.ptr, value.len);
    putMsg.add_key(msgKey);
    putMsg.add_value(msgValue);
    putMsg.add_req_id(req_id);
    auto finalMsg = putMsg.Finish();
    builder->Finish(finalMsg);
    encode_msg(sga, builder->GetBufferPointer(), builder->GetSize(), simplekv::request::PUT);
}

int flatbuffers_kv::client_handle_response(dmtr_sgarray_t &sga) {
    simplekv::request msg_type;
    uint8_t* data = decode_msg(sga, &msg_type);
    const ResponseFB* responseMsg;

    switch (msg_type) {
        case simplekv::request::RESPONSE:
            responseMsg = flatbuffers::GetRoot<ResponseFB>(data);
            return responseMsg->req_id();
        default:
            std::cerr << "Client received back non response message type: " << encode_enum(msg_type) << std::endl;
            exit(1);
    }

}

string flatbuffers_kv::client_check_response(dmtr_sgarray_t &sga) {
    simplekv::request msg_type;
    uint8_t* data = decode_msg(sga, &msg_type);
    const ResponseFB* responseMsg;

    switch (msg_type) {
        case simplekv::request::RESPONSE:
            responseMsg = flatbuffers::GetRoot<ResponseFB>(data);
            return string(responseMsg->value()->c_str());
        default:
            std::cerr << "Client received back non response message type: " << encode_enum(msg_type) << std::endl;
            exit(1);
    }
    
}

int flatbuffers_kv::server_handle_request(dmtr_sgarray_t &in_sga, dmtr_sgarray_t &out_sga, bool* free_in, bool* free_out, void *context) {
    *free_in = true;
    *free_out = true;
    simplekv::request msg_type;
    uint8_t* data = decode_msg(in_sga, &msg_type);
    
    const GetMessageFB* getMsg;
    const PutMessageFB* putMsg;
    flatbuffers::Offset<flatbuffers::String> msgValue;
    
    flatbuffers::FlatBufferBuilder* builder = reinterpret_cast<flatbuffers::FlatBufferBuilder *>(context);
    ResponseFBBuilder responseMsg(*builder);

    switch(msg_type) {
        case simplekv::request::GET:
            getMsg = flatbuffers::GetRoot<GetMessageFB>(data);
            msgValue = builder->CreateString(my_map.at(getMsg->key()->c_str()));
            responseMsg.add_value(msgValue);
            responseMsg.add_req_id(getMsg->req_id());
            break;
        case simplekv::request::PUT:
            putMsg = flatbuffers::GetRoot<PutMessageFB>(data);
            my_map[putMsg->key()->c_str()] = putMsg->value()->c_str();
            responseMsg.add_req_id(putMsg->req_id());
            break;
        default:
            std::cerr << "Server received msg of type not put or get." << std::endl;
            exit(1);
    }
    auto finalMsg = responseMsg.Finish();
    builder->Finish(finalMsg);
    encode_msg(out_sga, builder->GetBufferPointer(), builder->GetSize(), simplekv::request::RESPONSE);
    return 0;
}

void flatbuffers_kv::encode_msg(dmtr_sgarray_t &sga, uint8_t* data_buf, int size, simplekv::request msg_type) {
    int32_t type = encode_enum(msg_type);
    size_t totalLen = sizeof(type);
    void *p = NULL;
    dmtr_malloc(&p, totalLen);
    assert(p != NULL);
    
    // one for type, one for data
    sga.sga_numsegs = 2;
    sga.sga_segs[0].sgaseg_len = totalLen;
    sga.sga_segs[0].sgaseg_buf = p;
    char *buf = reinterpret_cast<char *>(p);
    char *ptr = buf;

    // encode type into first buffer
    *((int32_t *)ptr) = type;

    // encode data pointer into the next buffer
    sga.sga_segs[1].sgaseg_len = (size_t) size;
    sga.sga_segs[1].sgaseg_buf = (void*) data_buf;
}

uint8_t* flatbuffers_kv::decode_msg(dmtr_sgarray_t &sga, simplekv::request* msg_type) {
    assert(sga.sga_numsegs == 2);
    uint8_t *ptr = (uint8_t *)sga.sga_segs[0].sgaseg_buf;

    int32_t type = *(int32_t *)ptr;
    ptr += sizeof(type);
    *msg_type = decode_enum(type);

    return (uint8_t *)sga.sga_segs[1].sgaseg_buf;
}
