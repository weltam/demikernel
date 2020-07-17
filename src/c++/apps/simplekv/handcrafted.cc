// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include <dmtr/sga.h>
#include "handcrafted.hh"
#include <string>
#include <string_view>

using namespace std;

handcrafted_kv::handcrafted_kv() :
    simplekv(simplekv::library::HANDCRAFTED)
{}

void handcrafted_kv::client_send_put(int req_id, string key, string value, dmtr_sgarray_t &sga) {
    // put message has 1 pointer to encoded type & req_id
    // 1 pointer to key
    // 1 pointer to value
    sga.sga_numsegs = 3;
    
    // encode type & req_id
    encode_type(sga, simplekv::request::PUT, req_id);

    // encode the key
    sga.sga_segs[1].sgaseg_len = key.length();
    sga.sga_segs[1].sgaseg_buf = (void *)&key.c_str();

    // encode the value
    sga.sga_segs[2].sgaseg_len = value.length();
    sga.sga_segs[2].sgaseg_buf = (void *)&value.c_str();
}

void handcrafted_kv::client_send_get(int req_id, string key, dmtr_sgarray_t &sga) {
    sga.sga_numsegs = 2;

    // encode type & req_id
    encode_type(sga, simplekv::request::GET, req_id);

    // encode the key
    sga.sga_segs[1].sgaseg_len = key.length();
    sga.sga_segs[1].sgaseg_buf = (void *)&key.c_str();
}

int handcrafted_kv::client_handle_response(dmtr_sgarray_t &sga) {
    simplekv::request msg_type;
    int req_id = decode_type(sga, &msg_type);
    return req_id;
}

void handcrafted_kv::server_handle_request(dmtr_sgarray_t &sga) {
    simplekv::request msg_type;
    int req_id = decode_type(sga, &msg_type);
    
    switch (msg_type) {
        case simplekv::response::GET:
            server_handle_get(sga, req_id);
            break;
        case simplekv::response::PUT:
            server_handle_put(sga, req_id);
            break;
        default:
            std::cerr << "Server received non put or get message type: " << encode_enum(msg_type) << std::endl;
            exit(1);
    }

}

void handcrafted_kv::server_handle_get(dmtr_sgarray_t &sga, int req_id) {
    assert(sga.sga_numsegs == 2);
    // read the value
    std::string_view key((char *)sga.sga_segs[1].sgaseg_buf, sga.sga_segs[1].sgaseg_len);
    std::string_view value(my_map.at(key));
    
    encode_type(sga, simplekv::request::RESPONSE, req_id);
    sga.sga_segs[1].sgaseg_len = value.length();
    sga.sga_segs[1].sgaseg_buf = (void *)(&my_map.at(key));
}

void handcrafted_kv::server_handle_put(dmtr_sgarray_t &sga, int req_id) {
    assert(sga.sga_numsegs == 3);

    // read key and value
    std::string_view key((char *)sga.sga_segs[1].sgaseg_buf, sga.sga_segs[1].sgaseg_len);
    std::string_view value((char *)sga.sga_segs[2].sgaseg_buf, sga.sga_segs[2].sgaseg_len);

    sga.sga_numsegs = 2;
    my_map[key] = value;
    encode_type(sga, simplekv::request::RESPONSE, req_id);
}

void handcrafted_kv::encode_type(dmtr_sgarray_t &sga, simplekv::request req_type, int req_id) {
    int32_t type = encode_enum(msg_type);
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
    *msg_type = decode_enum(type);

    int req_id = *(int *)ptr;
    return req_id;
}

