#ifndef ECHO_CAPNPROTO_H_
#define ECHO_CAPNPROTO_H_

#include "stress.capnp.h"
#include <capnp/message.h>
#include <capnp/orphan.h>
#include "message.hh"
#include <string>
#include <dmtr/sga.h>
#include <dmtr/latency.h>
#include <sys/types.h>

using namespace std;

//#define DMTR_PROFILE

class capnproto_echo: public echo_message
{
#ifdef DMTR_PROFILE
    private: dmtr_latency_t *cast_latency;
    private: dmtr_latency_t *copy_latency;
    private: dmtr_latency_t *encode_latency;
    private: dmtr_latency_t *decode_latency;
#endif
    private: string string_field;
    public: capnproto_echo(uint32_t field_size, string message_type);

    public: virtual void serialize_message(dmtr_sgarray_t &sga, void *context);
    public: virtual void deserialize_message(dmtr_sgarray_t &sga);


    public: void encode_msg(dmtr_sgarray_t &sga, kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> segments);

    public: void decode_msg(dmtr_sgarray_t &sga, kj::ArrayPtr<const capnp::word>* segments);
    public: void print_counters();

    private: void build_get(GetMessageCP::Builder getMsg);
    private: void recursive_get(GetMessageCP::Builder builder, uint32_t field_size);
    private: void build_put(PutMessageCP::Builder putMsg);
    private: void build_msg1L(Msg1LCP::Builder msg1L);
    private: void recursive_msg1L(Msg1LCP::Builder builder, uint32_t field_size);
    private: void build_msg2L(Msg2LCP::Builder msg2L);
    private: void recursive_msg2L(Msg2LCP::Builder builder, uint32_t field_size);
    private: void build_msg3L(Msg3LCP::Builder msg3L);
    private: void recursive_msg3L(Msg3LCP::Builder builder, uint32_t field_size);
    private: void build_msg4L(Msg4LCP::Builder msg4L);
    private: void recursive_msg4L(Msg4LCP::Builder builder, uint32_t field_size);
    private: void build_msg5L(Msg5LCP::Builder msg5L);

};

#endif

