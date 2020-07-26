#ifndef KV_CAPNPROTO_H_
#define KV_CAPNPROTO_H_

#include "stress.capnp.h"
#include <capnp/message.h>
#include <capnp/orphan.h>
#include "kv.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>

using namespace std;

class capnproto_kv: public simplekv
{

    public: capnproto_kv();
    
    public: virtual void client_send_get(int req_id, simplekv::StringPointer key, dmtr_sgarray_t &sga, void* context);
    
    public: virtual void client_send_put(int req_id, simplekv::StringPointer key, simplekv::StringPointer value, dmtr_sgarray_t &sga, void* context);
    
    public: virtual int client_handle_response(dmtr_sgarray_t &sga);
    
    public: virtual string client_check_response(dmtr_sgarray_t &sga);
    
    public: virtual int server_handle_request(dmtr_sgarray_t &in_sga, dmtr_sgarray_t &out_sga, bool* free_in, bool* free_out, void* context);
    
    private: void encode_msg(dmtr_sgarray_t &sga, simplekv::request req_type, kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> segments);
    
    private: void decode_msg(dmtr_sgarray_t &sga, simplekv::request* req_type, kj::ArrayPtr<const capnp::word>* segments);
};

#endif
