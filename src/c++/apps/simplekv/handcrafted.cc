// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include <dmtr/sga.h>
#include "handcrafted.hh"
#include <string>
#include <experimental/string_view>
#include <iostream>
#include <dmtr/libos/mem.h>
#include <sys/types.h>
#include <assert.h>
#include <dmtr/annot.h>

using namespace std;

handcrafted_kv::handcrafted_kv() :
    simplekv(simplekv::library::HANDCRAFTED)
{}

void handcrafted_kv::client_send_put(int req_id, simplekv::StringPointer key, simplekv::StringPointer value, dmtr_sgarray_t &sga) {
    // put message has 1 pointer to encoded type & req_id
    // 1 pointer to key
    // 1 pointer to value
    sga.sga_numsegs = 3;
    
    // encode type & req_id
    encode_type(sga, simplekv::request::PUT, req_id);

    // encode the key and value
    sga.sga_segs[1].sgaseg_len = key.len;
    sga.sga_segs[1].sgaseg_buf = (void *)key.ptr;
    
    sga.sga_segs[2].sgaseg_len = value.len;
    sga.sga_segs[2].sgaseg_buf = (void *)value.ptr;

}

void handcrafted_kv::client_send_get(int req_id, simplekv::StringPointer key, dmtr_sgarray_t &sga) {
    sga.sga_numsegs = 2;

    // encode type & req_id
    encode_type(sga, simplekv::request::GET, req_id);

    // encode the key
    sga.sga_segs[1].sgaseg_len = key.len;
    sga.sga_segs[1].sgaseg_buf = (void *)key.ptr;
}

int handcrafted_kv::client_handle_response(dmtr_sgarray_t &sga) {
    simplekv::request msg_type;
    int req_id = decode_type(sga, &msg_type);
    return req_id;
}

string handcrafted_kv::client_check_response(dmtr_sgarray_t &sga) {
    if (sga.sga_numsegs == 2) {
        return string((char *)sga.sga_segs[1].sgaseg_buf, sga.sga_segs[1].sgaseg_len);
    } else {
        return string("");
    }
}

int handcrafted_kv::server_handle_request(dmtr_sgarray_t &in_sga, dmtr_sgarray_t &out_sga, bool* free_in, bool* free_out) {
    simplekv::request msg_type;
    int req_id = decode_type(in_sga, &msg_type);
    
    switch (msg_type) {
        case simplekv::request::GET:
            *free_in = true; // free inward buffer when possible
            *free_out = true; // do not free outward buffer. ever
            return server_handle_get(in_sga, out_sga, req_id);
            
            break;
        case simplekv::request::PUT:
            *free_in = false; // don't free inward buffer; we will.
            *free_out = true; // don't free outward buffer.
            return server_handle_put(in_sga, out_sga, req_id);
            break;
        default:
            std::cerr << "Server received non put or get message type: " << encode_enum(msg_type) << std::endl;
            exit(1);
    }

}

int handcrafted_kv::server_handle_get(dmtr_sgarray_t &in_sga, dmtr_sgarray_t &out_sga, int req_id) {
    assert(in_sga.sga_numsegs == 2);
    simplekv::StringPointer key_ptr((char *)(in_sga.sga_segs[1].sgaseg_buf), in_sga.sga_segs[1].sgaseg_len);
    simplekv::StringPointer value_ptr = my_bytes_map.at(key_ptr);

    encode_type(out_sga, simplekv::request::RESPONSE, req_id);
    out_sga.sga_numsegs = 2;
    out_sga.sga_segs[1].sgaseg_len = value_ptr.len;
    out_sga.sga_segs[1].sgaseg_buf = (void *)(value_ptr.ptr);
    return 0;
}

int handcrafted_kv::server_handle_put(dmtr_sgarray_t &in_sga, dmtr_sgarray_t &out_sga, int req_id) {
    assert(in_sga.sga_numsegs == 3);

    simplekv::StringPointer key_ptr((char *)(in_sga.sga_segs[1].sgaseg_buf), in_sga.sga_segs[1].sgaseg_len);
    simplekv::StringPointer value_ptr((char *)(in_sga.sga_segs[2].sgaseg_buf), in_sga.sga_segs[2].sgaseg_len, in_sga);

    // read key and value
    auto it = my_bytes_map.find(key_ptr);
    if (it != my_bytes_map.end()) {
        dmtr_sgarray_t current_sga = it->second.sga;;
        it->second = value_ptr;
        DMTR_OK(dmtr_sgafree(&current_sga));
    } else {
        std::cout << "Todo: implement inserts" << std::endl;
    }

    out_sga.sga_numsegs = 1;
    encode_type(out_sga, simplekv::request::RESPONSE, req_id);
    return 0;
}

void handcrafted_kv::encode_type(dmtr_sgarray_t &sga, simplekv::request req_type, int req_id) {
    int32_t type = encode_enum(req_type);
    size_t totalLen = sizeof(type) + sizeof(int);

    void *p = NULL;
    dmtr_malloc(&p, totalLen);
    assert(p != NULL);

    sga.sga_segs[0].sgaseg_len = totalLen;
    sga.sga_segs[0].sgaseg_buf = p;
    char *buf = reinterpret_cast<char *>(p);
    char *ptr = buf;

    // encode type
    *((int32_t *)ptr) = type;
    ptr += sizeof(type);
    // encode req id into next
    *((int *)ptr) = req_id;
}

int handcrafted_kv::decode_type(dmtr_sgarray_t &sga, simplekv::request* req_type) {
    uint8_t *ptr = (uint8_t *)sga.sga_segs[0].sgaseg_buf;

    int32_t type = *(int32_t *)ptr;
    ptr += sizeof(type);
    *req_type = decode_enum(type);

    int req_id = *(int *)ptr;
    return req_id;
}

