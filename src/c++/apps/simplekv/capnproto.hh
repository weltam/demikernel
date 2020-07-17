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
    
    public: virtual void client_send_get(int req_id, string key, dmtr_sgarray_t &sga);
    public: virtual void client_send_put(int req_id, string key, string value, dmtr_sgarray_t &sga);
    public: virtual int client_handle_response(dmtr_sgarray_t &sga);
    public: virtual void server_handle_request(dmtr_sgarray_t &sga);
};

#endif
