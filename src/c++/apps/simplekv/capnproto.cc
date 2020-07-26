// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "capnproto.hh"
#include "assert.h"
#include "stress.capnp.h"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>
#include <dmtr/libos/mem.h>
#include <iostream>

capnproto_kv::capnproto_kv() :
    simplekv(simplekv::library::CAPNPROTO)
{}

void capnproto_kv::client_send_get(int req_id, simplekv::StringPointer key, dmtr_sgarray_t &sga, void *context) {  
    GetMessageCP::Builder getMsg = (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).initRoot<GetMessageCP>();
    getMsg.setKey(string((char *)key.ptr, key.len));
    getMsg.setReqId(req_id);
    //std::cout << "Trying to send get: " << key.string().c_str() << "; req id: " << req_id << std::endl;
    kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> segments = (reinterpret_cast<capnp::MallocMessageBuilder*>(context))->getSegmentsForOutput();
    encode_msg(sga, simplekv::request::GET, segments);
}

void capnproto_kv::client_send_put(int req_id, simplekv::StringPointer key, simplekv::StringPointer value, dmtr_sgarray_t &sga, void* context) {
    PutMessageCP::Builder putMsg = (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).initRoot<PutMessageCP>();
    putMsg.setKey(string((char *)key.ptr, key.len));
    putMsg.setValue(string((char *)value.ptr, value.len));
    putMsg.setReqId(req_id);
    //std::cout << "Trying to send put: " << key.string().c_str() << "->" << value.string().c_str() << "; req id: " << req_id << std::endl;
    kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> segments = (reinterpret_cast<capnp::MallocMessageBuilder*>(context))->getSegmentsForOutput();
    encode_msg(sga, simplekv::request::PUT, segments);
}

int capnproto_kv::client_handle_response(dmtr_sgarray_t &sga) {
    simplekv::request req_type;
    auto segment_array = kj::heapArray<kj::ArrayPtr<const capnp::word>>(sga.sga_numsegs - 1);
    decode_msg(sga, &req_type, segment_array.begin());
    capnp::SegmentArrayMessageReader message(segment_array);
    ResponseCP::Reader response;
    switch(req_type) {
        case simplekv::request::RESPONSE:
            response = message.getRoot<ResponseCP>();
            return response.getReqId();
        default:
            std::cerr << "Client received back non response type: " << encode_enum(req_type) << std::endl;
            exit(1);
    }
}

string capnproto_kv::client_check_response(dmtr_sgarray_t &sga) {
    simplekv::request req_type;
    auto segment_array = kj::heapArray<kj::ArrayPtr<const capnp::word>>(sga.sga_numsegs - 1);
    decode_msg(sga, &req_type, segment_array.begin());
    capnp::SegmentArrayMessageReader message(segment_array);
    ResponseCP::Reader response;
    switch(req_type) {
        case simplekv::request::RESPONSE:
            response = message.getRoot<ResponseCP>();
            return string(response.getValue().cStr());
        default:
            std::cerr << "Client received back non response type: " << encode_enum(req_type) << std::endl;
            exit(1);

    }
}

int capnproto_kv::server_handle_request(dmtr_sgarray_t &in_sga, dmtr_sgarray_t &out_sga, bool* free_in, bool* free_out, void* context) {
    *free_in = true;
    *free_out = true;

    simplekv::request req_type;
    auto segment_array = kj::heapArray<kj::ArrayPtr<const capnp::word>>(in_sga.sga_numsegs - 1);
    decode_msg(in_sga, &req_type, segment_array.begin());
    capnp::SegmentArrayMessageReader message(segment_array);

    GetMessageCP::Reader local_get;
    PutMessageCP::Reader local_put;
    capnp::MallocMessageBuilder malloc_builder;
    ResponseCP::Builder responseMsg = (*(reinterpret_cast<capnp::MallocMessageBuilder*>(context))).initRoot<ResponseCP>();

    switch (req_type) {
        case simplekv::request::GET:
            local_get = message.getRoot<GetMessageCP>();
            //std::cout << "Req id: " << local_get.getReqId() << std::endl;
            //std::cout << "Trying to get " << local_get.getKey().cStr() << std::endl;
            responseMsg.setValue(my_map[local_get.getKey().cStr()].c_str());
            responseMsg.setReqId(local_get.getReqId());
            break;
        case simplekv::request::PUT:
            local_put = message.getRoot<PutMessageCP>();
            //std::cout << "Req id: " << local_put.getReqId() << std::endl;
            //std::cout << "Trying to put: " << local_put.getKey().cStr() << "->" << local_put.getValue().cStr() << std::endl;
            my_map[local_put.getKey().cStr()] = local_put.getValue().cStr();
            responseMsg.setReqId(local_put.getReqId());
            break;
        default:
            std::cerr << "Server received non get or put request type: " << encode_enum(req_type) << std::endl;
            return -1;
    }

    kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> response_segments = (reinterpret_cast<capnp::MallocMessageBuilder*>(context))->getSegmentsForOutput();
    encode_msg(out_sga, simplekv::request::RESPONSE, response_segments);
    return 0;
}

void capnproto_kv::encode_msg(dmtr_sgarray_t &sga, simplekv::request req_type, kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> segments) {
    // number of captain proto segments + 1
    sga.sga_numsegs = segments.size() + 1;
    // encode the type
    int32_t type = encode_enum(req_type);
    size_t total_len = sizeof(type);
    void *p = NULL;
    dmtr_malloc(&p, total_len);
    assert(p != NULL);

    sga.sga_segs[0].sgaseg_len = total_len;
    sga.sga_segs[0].sgaseg_buf = p;
    char *buf = reinterpret_cast<char *>(p);
    char *ptr = buf;

    *((int32_t *) ptr) = type;

    // encode the message into the rest of the segments
    for (size_t i = 0; i < segments.size(); i++) {
        kj::ArrayPtr<const capnp::word> segment = segments[i];
        kj::ArrayPtr<const kj::byte> bytes = segment.asBytes();
        sga.sga_segs[i+1].sgaseg_len = bytes.size();
        sga.sga_segs[i+1].sgaseg_buf = (void *) bytes.begin();
    } 

}

void capnproto_kv::decode_msg(dmtr_sgarray_t &sga, simplekv::request* req_type, kj::ArrayPtr<const capnp::word>* segments) {
    // first segment contains the message type
    uint8_t *ptr = (uint8_t *)sga.sga_segs[0].sgaseg_buf;
    int32_t type = *(int32_t *)ptr;
    *req_type = decode_enum(type);

    auto current_segment = segments;
    for (size_t i = 1; i < sga.sga_numsegs; i++) {
        unsigned char* segment = reinterpret_cast<unsigned char *>(sga.sga_segs[i].sgaseg_buf);
        const capnp::word* bytes = reinterpret_cast<const capnp::word*>(segment);
        size_t word_size = sga.sga_segs[i].sgaseg_len/sizeof(capnp::word);
        kj::ArrayPtr<const capnp::word> words(bytes, word_size);
        *current_segment = words;
        current_segment += 1;
    }
}


