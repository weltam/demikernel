// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#ifndef KV_H_
#define KV_H_

#include <dmtr/sga.h>
#include <unordered_map>
#include <string>
#include <sys/types.h>


using namespace std;

class simplekv
{
    public: enum library {
        PROTOBUF,
        CAPNPROTO,
        FLATBUFFERS,
        HANDCRAFTED,
    };

    public: enum request {
        GET,
        PUT,
        RESPONSE,
    };

    protected: enum library my_cereal_type;
    protected: unordered_map<string, string> my_map;


    protected: simplekv(enum library cereal_type);
    public: virtual ~simplekv() {}

    public: int init(string load_file);

    public: virtual void client_send_put(int req_id, string key, string value, dmtr_sgarray_t &sga) = 0;
    public: virtual void client_send_get(int req_id, string key, dmtr_sgarray_t &sga) = 0;
    public: virtual int client_handle_response(dmtr_sgarray_t &sga) = 0;

    public: virtual void server_handle_request(dmtr_sgarray_t &sga) = 0;
};

int32_t encode_enum(simplekv::request req);

simplekv::request decode_enum(int32_t val);

#endif

