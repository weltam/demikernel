// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "stress_generated.h"
#include "flatbuffers.hh"
#include "message.hh"
#include <string>
#include <dmtr/sga.h>
#include <dmtr/libos/mem.h>
#include <sys/types.h>
#include <iostream>

flatbuffers_echo::flatbuffers_echo(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::FLATBUFFERS, field_size, message_type),
    string_field(generate_string(field_size)) {}

void flatbuffers_echo::serialize_message(dmtr_sgarray_t &sga, void *context) {
    flatbuffers::FlatBufferBuilder* builder = reinterpret_cast<flatbuffers::FlatBufferBuilder *>(context);

    if (my_msg_enum == echo_message::msg_type::GET) {
        auto msgKey = builder->CreateString(string_field);
        GetMessageFBBuilder getMsg(*builder);
        getMsg.add_key(msgKey);
        auto final_get = getMsg.Finish();
        builder->Finish(final_get);
    } else if (my_msg_enum == echo_message::msg_type::PUT) {
        auto msgKey = builder->CreateString(string_field);
        auto msgValue = builder->CreateString(string_field);
        PutMessageFBBuilder putMsg(*builder);
        putMsg.add_key(msgKey);
        putMsg.add_value(msgValue);
        auto final_put = putMsg.Finish();
        builder->Finish(final_put);
    } else if (my_msg_enum == echo_message::msg_type::MSG1L) {
        Msg1LFBBuilder msg1L(*builder);
        build_msg1L(msg1L, builder);

    } else if (my_msg_enum == echo_message::msg_type::MSG2L) {
        Msg2LFBBuilder msg2L(*builder);
        build_msg2L(msg2L, builder);

    } else if (my_msg_enum == echo_message::msg_type::MSG3L) {
        Msg3LFBBuilder msg3L(*builder);
        build_msg3L(msg3L, builder);

    } else if (my_msg_enum == echo_message::msg_type::MSG4L) {
        Msg4LFBBuilder msg4L(*builder);
        build_msg4L(msg4L, builder);

    } else if (my_msg_enum == echo_message::msg_type::MSG5L) {
        Msg5LFBBuilder msg5L(*builder);
        build_msg5L(msg5L, builder);
    }
    encode_msg(sga, builder->GetBufferPointer(), builder->GetSize());
}

void flatbuffers_echo::deserialize_message(dmtr_sgarray_t &sga) {
    assert(sga.sga_numsegs == 1);
    uint8_t *data_ptr = (uint8_t *)(sga.sga_segs[0].sgaseg_buf);
    handle_msg(data_ptr);
}

void flatbuffers_echo::encode_msg(dmtr_sgarray_t &sga, uint8_t* data_buf, int size) {
    // write the data into the 2nd buf ptr
    sga.sga_numsegs = 1;
    size_t dataLen = (size_t) size;
    sga.sga_segs[0].sgaseg_len = dataLen;
    sga.sga_segs[0].sgaseg_buf = (void*) data_buf;  // maybe needs a cast to void*?
}

template<class T>
inline void read_data(uint8_t* buf, const T** ptr) {
    *ptr = flatbuffers::GetRoot<T>(buf);
}

void flatbuffers_echo::handle_msg(uint8_t* buf) {
    const GetMessageFB* local_get;
    const PutMessageFB* local_put;
    const Msg1LFB* local_msg1L;
    const Msg2LFB* local_msg2L;
    const Msg3LFB* local_msg3L;
    const Msg4LFB* local_msg4L;
    const Msg5LFB* local_msg5L;
    switch (my_msg_enum) {
        case echo_message::msg_type::GET:
            read_data<GetMessageFB>(buf, &local_get);
            break;
        case echo_message::msg_type::PUT:
            read_data<PutMessageFB>(buf, &local_put);
            break;
        case echo_message::msg_type::MSG1L:
            read_data<Msg1LFB>(buf, &local_msg1L);
            break;
        case echo_message::msg_type::MSG2L:
            read_data<Msg2LFB>(buf, &local_msg2L);
            break;
        case echo_message::msg_type::MSG3L:
            read_data<Msg3LFB>(buf, &local_msg3L);
            break;
        case echo_message::msg_type::MSG4L:
            read_data<Msg4LFB>(buf, &local_msg4L);
            break;
        case echo_message::msg_type::MSG5L:
            read_data<Msg5LFB>(buf, &local_msg5L);
            break;
    }
}

// TODO: remove these functions so they're not nested.
void flatbuffers_echo::build_msg1L(Msg1LFBBuilder msg1L, flatbuffers::FlatBufferBuilder *msg1LBuilder) {
    auto string_field = generate_string(my_field_size);
    auto leaf = msg1LBuilder->CreateString(string_field);
    auto left = CreateGetMessageFB(*msg1LBuilder, leaf);
    auto right = CreateGetMessageFB(*msg1LBuilder, leaf);
    msg1L.add_left(left);
    msg1L.add_right(right);
    auto final_msg = msg1L.Finish();
    msg1LBuilder->Finish(final_msg);
}

void flatbuffers_echo::build_msg2L(Msg2LFBBuilder msg2L, flatbuffers::FlatBufferBuilder *msg2LBuilder) {
    auto string_field = generate_string(my_field_size);
    auto leaf = msg2LBuilder->CreateString(string_field);
    auto getLeaf = CreateGetMessageFB(*msg2LBuilder, leaf);
    auto left = CreateMsg1LFB(*msg2LBuilder, getLeaf, getLeaf);
    auto right = CreateMsg1LFB(*msg2LBuilder, getLeaf, getLeaf);
    msg2L.add_left(left);
    msg2L.add_right(right);
    auto final_msg = msg2L.Finish();
    msg2LBuilder->Finish(final_msg);

}

void flatbuffers_echo::build_msg3L(Msg3LFBBuilder msg3L, flatbuffers::FlatBufferBuilder *msg3LBuilder) {
    auto string_field = generate_string(my_field_size);
    auto leaf = msg3LBuilder->CreateString(string_field);
    auto getLeaf = CreateGetMessageFB(*msg3LBuilder, leaf);
    auto msg1LLeaf = CreateMsg1LFB(*msg3LBuilder, getLeaf, getLeaf);
    auto left = CreateMsg2LFB(*msg3LBuilder, msg1LLeaf, msg1LLeaf);
    auto right = CreateMsg2LFB(*msg3LBuilder, msg1LLeaf, msg1LLeaf);
    msg3L.add_left(left);
    msg3L.add_right(right);
    auto final_msg = msg3L.Finish();
    msg3LBuilder->Finish(final_msg);

}

void flatbuffers_echo::build_msg4L(Msg4LFBBuilder msg4L, flatbuffers::FlatBufferBuilder *msg4LBuilder) {
    auto string_field = generate_string(my_field_size);
    auto leaf = msg4LBuilder->CreateString(string_field);
    auto getLeaf = CreateGetMessageFB(*msg4LBuilder, leaf);
    auto msg1LLeaf = CreateMsg1LFB(*msg4LBuilder, getLeaf, getLeaf);
    auto msg2LLeaf = CreateMsg2LFB(*msg4LBuilder, msg1LLeaf, msg1LLeaf);
    auto left = CreateMsg3LFB(*msg4LBuilder, msg2LLeaf, msg2LLeaf);
    auto right = CreateMsg3LFB(*msg4LBuilder, msg2LLeaf, msg2LLeaf);
    msg4L.add_left(left);
    msg4L.add_right(right);
    auto final_msg = msg4L.Finish();
    msg4LBuilder->Finish(final_msg);
}

void flatbuffers_echo::build_msg5L(Msg5LFBBuilder msg5L, flatbuffers::FlatBufferBuilder *msg5LBuilder) {
    auto string_field = generate_string(my_field_size);
    auto leaf = msg5LBuilder->CreateString(string_field);
    auto getLeaf = CreateGetMessageFB(*msg5LBuilder, leaf);
    auto msg1LLeaf = CreateMsg1LFB(*msg5LBuilder, getLeaf, getLeaf);
    auto msg2LLeaf = CreateMsg2LFB(*msg5LBuilder, msg1LLeaf, msg1LLeaf);
    auto msg3LLeaf = CreateMsg3LFB(*msg5LBuilder, msg2LLeaf, msg2LLeaf);
    auto left = CreateMsg4LFB(*msg5LBuilder, msg3LLeaf, msg3LLeaf);
    auto right = CreateMsg4LFB(*msg5LBuilder, msg3LLeaf, msg3LLeaf);
    msg5L.add_left(left);
    msg5L.add_right(right);
    auto final_msg = msg5L.Finish();
    msg5LBuilder->Finish(final_msg);

}

