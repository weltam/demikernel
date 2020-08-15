// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "flatbuffers.hh"
#include "message.hh"
#include <string>
#include <dmtr/sga.h>
#include <dmtr/libos/mem.h>
#include <sys/types.h>
#include <iostream>

flatbuffers_echo::flatbuffers_echo(uint32_t field_size, string message_type) :
    echo_message(echo_message::library::FLATBUFFERS, field_size, message_type),
    string_field(generate_string(field_size)),
    getBuilder(field_size),
    putBuilder(field_size),
    msg1LBuilder(field_size),
    msg2LBuilder(field_size),
    msg3LBuilder(field_size),
    msg4LBuilder(field_size),
    msg5LBuilder(field_size),
    string_offset(getBuilder.CreateString(string_field)),
    getMsg(getBuilder),
    putMsg(putBuilder),
    msg1L(msg1LBuilder),
    msg2L(msg2LBuilder),
    msg3L(msg3LBuilder),
    msg4L(msg4LBuilder),
    msg5L(msg5LBuilder)
{
    switch (my_msg_enum) {
        case echo_message::msg_type::GET:
            build_get();
            break;
        case echo_message::msg_type::PUT:
            build_put();
            break;
        case echo_message::msg_type::MSG1L:
            build_msg1L();
            break;
        case echo_message::msg_type::MSG2L:
            build_msg2L();
            break;
        case echo_message::msg_type::MSG3L:
            build_msg3L();
            break;
        case echo_message::msg_type::MSG4L:
            build_msg4L();
            break;
        case echo_message::msg_type::MSG5L:
            build_msg5L();
            break;
    }
}

void flatbuffers_echo::serialize_message(dmtr_sgarray_t &sga) {
    switch (my_msg_enum) {
        case echo_message::msg_type::GET:
            encode_msg(sga, getBuilder.GetBufferPointer(), getBuilder.GetSize());
            break;
        case echo_message::msg_type::PUT:
            encode_msg(sga, putBuilder.GetBufferPointer(), putBuilder.GetSize());
            break;
        case echo_message::msg_type::MSG1L:
            encode_msg(sga, msg1LBuilder.GetBufferPointer(), msg1LBuilder.GetSize());
            break;
        case echo_message::msg_type::MSG2L:
            encode_msg(sga, msg2LBuilder.GetBufferPointer(), msg2LBuilder.GetSize());
            break;
        case echo_message::msg_type::MSG3L:
            encode_msg(sga, msg3LBuilder.GetBufferPointer(), msg3LBuilder.GetSize());
            break;
        case echo_message::msg_type::MSG4L:
            encode_msg(sga, msg4LBuilder.GetBufferPointer(), msg4LBuilder.GetSize());
            break;
        case echo_message::msg_type::MSG5L:
            encode_msg(sga, msg5LBuilder.GetBufferPointer(), msg5LBuilder.GetSize());
            break;
    }
}

void flatbuffers_echo::deserialize_message(dmtr_sgarray_t &sga) {
    assert(sga.sga_numsegs == 1);
    /*uint8_t *ptr = (uint8_t *)sga.sga_segs[0].sgaseg_buf;
    
    size_t totalLen = *(size_t *)ptr;
    ptr += sizeof(totalLen);
    
    size_t typeLen = *(size_t *)ptr;
    ptr += sizeof(typeLen);
    string type((char *)ptr, typeLen);
    ptr += typeLen;
    
    size_t dataLen = *(size_t *)ptr;
    ptr += sizeof(dataLen);*/
    
    uint8_t *data_ptr = (uint8_t *)(sga.sga_segs[0].sgaseg_buf);
    handle_msg(data_ptr);
}

void flatbuffers_echo::encode_msg(dmtr_sgarray_t &sga, uint8_t* data_buf, int size) {
    /*void *p = NULL;
    size_t dataLen = (size_t) size;
    size_t typeLen = my_message_type.length();
    // everything without the actual data
    size_t totalLen = (typeLen + sizeof(typeLen) +
                        sizeof(dataLen) +
                        sizeof(totalLen));
    dmtr_malloc(&p, totalLen);
    assert(p != NULL);
    sga.sga_numsegs = 2;
    sga.sga_segs[0].sgaseg_len = totalLen;
    sga.sga_segs[0].sgaseg_buf = p;

    char *buf = reinterpret_cast<char *>(p);
    char *ptr = buf;

    *((size_t *) ptr) = totalLen;
    ptr += sizeof(totalLen);
    assert((size_t)(ptr-buf) < totalLen);

    *((size_t *) ptr) = typeLen;
    ptr += sizeof(typeLen);
    assert((size_t)(ptr-buf) < totalLen);
    
    memcpy(ptr, my_message_type.c_str(), typeLen);
    ptr += typeLen;
    assert((size_t)(ptr-buf) < totalLen);

    *((size_t *) ptr) = dataLen;
    ptr += sizeof(dataLen);
    assert((size_t)(ptr-buf) < totalLen);*/

    // write the data into the 2nd buf ptr
    sga.sga_numsegs = 1;
    size_t dataLen = (size_t) size;
    sga.sga_segs[0].sgaseg_len = dataLen;
    sga.sga_segs[0].sgaseg_buf = (void*) data_buf;  // maybe needs a cast to void*?
}

template<class T>
void read_data(uint8_t* buf, const T** ptr) {
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

void flatbuffers_echo::build_get() {
    getMsg.add_key(string_offset);
    auto final_get = getMsg.Finish();
    getBuilder.Finish(final_get);
    std::cout << "Finished get" << std::endl;
}

void flatbuffers_echo::build_put() {
    auto string_field = generate_string(my_field_size);
    auto key = putBuilder.CreateString(string_field);
    putMsg.add_key(key);
    putMsg.add_value(key);
    auto final_put = putMsg.Finish();
    putBuilder.Finish(final_put);
}

void flatbuffers_echo::build_msg1L() {
    auto string_field = generate_string(my_field_size);
    auto leaf = msg1LBuilder.CreateString(string_field);
    auto left = CreateGetMessageFB(msg1LBuilder, leaf);
    auto right = CreateGetMessageFB(msg1LBuilder, leaf);
    msg1L.add_left(left);
    msg1L.add_right(right);
    auto final_msg = msg1L.Finish();
    msg1LBuilder.Finish(final_msg);
}

void flatbuffers_echo::build_msg2L() {
    auto string_field = generate_string(my_field_size);
    auto leaf = msg2LBuilder.CreateString(string_field);
    auto getLeaf = CreateGetMessageFB(msg2LBuilder, leaf);
    auto left = CreateMsg1LFB(msg2LBuilder, getLeaf, getLeaf);
    auto right = CreateMsg1LFB(msg2LBuilder, getLeaf, getLeaf);
    msg2L.add_left(left);
    msg2L.add_right(right);
    auto final_msg = msg2L.Finish();
    msg2LBuilder.Finish(final_msg);

}

void flatbuffers_echo::build_msg3L() {
    auto string_field = generate_string(my_field_size);
    auto leaf = msg3LBuilder.CreateString(string_field);
    auto getLeaf = CreateGetMessageFB(msg3LBuilder, leaf);
    auto msg1LLeaf = CreateMsg1LFB(msg3LBuilder, getLeaf, getLeaf);
    auto left = CreateMsg2LFB(msg3LBuilder, msg1LLeaf, msg1LLeaf);
    auto right = CreateMsg2LFB(msg3LBuilder, msg1LLeaf, msg1LLeaf);
    msg3L.add_left(left);
    msg3L.add_right(right);
    auto final_msg = msg3L.Finish();
    msg3LBuilder.Finish(final_msg);

}

void flatbuffers_echo::build_msg4L() {
    auto string_field = generate_string(my_field_size);
    auto leaf = msg4LBuilder.CreateString(string_field);
    auto getLeaf = CreateGetMessageFB(msg4LBuilder, leaf);
    auto msg1LLeaf = CreateMsg1LFB(msg4LBuilder, getLeaf, getLeaf);
    auto msg2LLeaf = CreateMsg2LFB(msg4LBuilder, msg1LLeaf, msg1LLeaf);
    auto left = CreateMsg3LFB(msg4LBuilder, msg2LLeaf, msg2LLeaf);
    auto right = CreateMsg3LFB(msg4LBuilder, msg2LLeaf, msg2LLeaf);
    msg4L.add_left(left);
    msg4L.add_right(right);
    auto final_msg = msg4L.Finish();
    msg4LBuilder.Finish(final_msg);

}

void flatbuffers_echo::build_msg5L() {
    auto string_field = generate_string(my_field_size);
    auto leaf = msg5LBuilder.CreateString(string_field);
    auto getLeaf = CreateGetMessageFB(msg5LBuilder, leaf);
    auto msg1LLeaf = CreateMsg1LFB(msg5LBuilder, getLeaf, getLeaf);
    auto msg2LLeaf = CreateMsg2LFB(msg5LBuilder, msg1LLeaf, msg1LLeaf);
    auto msg3LLeaf = CreateMsg3LFB(msg5LBuilder, msg2LLeaf, msg2LLeaf);
    auto left = CreateMsg4LFB(msg5LBuilder, msg3LLeaf, msg3LLeaf);
    auto right = CreateMsg4LFB(msg5LBuilder, msg3LLeaf, msg3LLeaf);
    msg5L.add_left(left);
    msg5L.add_right(right);
    auto final_msg = msg5L.Finish();
    msg5LBuilder.Finish(final_msg);

}

