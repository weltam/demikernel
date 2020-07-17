// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.


#ifndef KV_FLATBUFFERS_H_
#define KV_FLATBUFFERS_H_

#include "stress_generated.h"
#include "kv.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>

using namespace std;
using namespace stress;

class flatbuffers_kv : public simplekv
{
    public: flatbuffers_kv();

    public: virtual void client_send_get(int req_id, string key, dmtr_sgarray_t &sga);
    public: virtual void client_send_put(int req_id, string key, string value, dmtr_sgarray_t &sga);
    public: virtual int client_handle_response(dmtr_sgarray_t &sga);
    public: virtual void server_handle_request(dmtr_sgarray_t &sga);

    private: void encode_msg(dmtr_sgarray_t &sga, uint8_t* data_buf, int size, simplekv::request msg_type);

    private: uint8_t* decode_msg(dmtr_sgarray_t &sga, simplekv::request* msg_type);


};

#endif
