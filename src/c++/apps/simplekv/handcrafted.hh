// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#ifndef KV_HANDCRAFTED_H_
#define KV_HANDCRAFTED_H_

#include <dmtr/sga.h>
#include "kv.hh"
#include <string>

using namespace std;
class handcrafted_kv: public simplekv
{
    public: handcrafted_kv();
    
    public: virtual void client_send_put(int req_id, simplekv::StringPointer key, simplekv::StringPointer value, dmtr_sgarray_t &sga, void *context);

    public: virtual void client_send_get(int req_id, simplekv::StringPointer key, dmtr_sgarray_t &sga, void *context);

    public: virtual int client_handle_response(dmtr_sgarray_t &sga);

    public: virtual string client_check_response(dmtr_sgarray_t &sga);

    public: virtual int server_handle_request(dmtr_sgarray_t &in_sga, dmtr_sgarray_t &out_sga, bool* free_in, bool* free_out, void *context);

    private: void encode_type(dmtr_sgarray_t &in_sga, dmtr_sgarray_t &out_sga, simplekv::request req_type, int req_id, bool opt);

    private: int decode_type(dmtr_sgarray_t &sga, simplekv::request* req_type);

    private: int server_handle_get(dmtr_sgarray_t &in_sga, dmtr_sgarray_t &out_sga, int req_id);
    private: int server_handle_put(dmtr_sgarray_t &in_sga, dmtr_sgarray_t &out_sga, int req_id);
};
#endif
