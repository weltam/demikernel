#ifndef ECHO_CAPNPROTO_H_
#define ECHO_CAPNPROTO_H_

#include "stress.capnp.h"
#include <capnp/message.h>
#include <capnp/orphan.h>
#include "message.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>

using namespace std;

class capnproto_echo: public echo_message
{
    private: capnp::MallocMessageBuilder malloc_builder;
    private: GetMessageCP::Builder getMsg;
    private: PutMessageCP::Builder putMsg;
    private: Msg1LCP::Builder msg1L;
    private: Msg2LCP::Builder msg2L;
    private: Msg3LCP::Builder msg3L;
    private: Msg4LCP::Builder msg4L;
    private: Msg5LCP::Builder msg5L; 

    public: capnproto_echo(uint32_t field_size, string message_type);

    public: virtual void serialize_message(dmtr_sgarray_t &sga);
    public: virtual void deserialize_message(dmtr_sgarray_t &sga);

    public: void encode_msg(dmtr_sgarray_t &sga, kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> segments);

    public: void decode_msg(dmtr_sgarray_t &sga, kj::ArrayPtr<const capnp::word>* segments);

    private: void build_get();
    private: void recursive_get(GetMessageCP::Builder builder, uint32_t field_size);
    private: void build_put();
    private: void build_msg1L();
    private: void recursive_msg1L(Msg1LCP::Builder builder, uint32_t field_size);
    private: void build_msg2L();
    private: void recursive_msg2L(Msg2LCP::Builder builder, uint32_t field_size);
    private: void build_msg3L();
    private: void recursive_msg3L(Msg3LCP::Builder builder, uint32_t field_size);
    private: void build_msg4L();
    private: void recursive_msg4L(Msg4LCP::Builder builder, uint32_t field_size);
    private: void build_msg5L();

};

#endif

