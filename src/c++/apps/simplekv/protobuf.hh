// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.


#ifndef KV_PROTOBUF_H_
#define KV_PROTOBUF_H_

#include "stress.pb.h"
#include "kv.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>

using namespace std;
using namespace stress;

typedef google::protobuf::Message Message;

class protobuf_kv : public simplekv
{
    public: protobuf_kv();
    
    public: virtual void client_send_get(int req_id, simplekv::StringPointer key, dmtr_sgarray_t &sga);
    public: virtual void client_send_put(int req_id, simplekv::StringPointer key, simplekv::StringPointer value, dmtr_sgarray_t &sga);
    public: virtual int client_handle_response(dmtr_sgarray_t &sga);
    public: virtual string client_check_response(dmtr_sgarray_t &sga);
    public: virtual int server_handle_request(dmtr_sgarray_t &in_sga, dmtr_sgarray_t &out_sga, bool* free_in, bool* free_out);

    private: void encode_msg(dmtr_sgarray_t &sga, const Message& msg, simplekv::request msg_type);

    private: string decode_msg(dmtr_sgarray_t &sga, simplekv::request* msg_type);

};

#endif
