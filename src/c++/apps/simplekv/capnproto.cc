// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "capnproto.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>

capnproto_kv::capnproto_kv() :
    simplekv(simplekv::library::CAPNPROTO)
{}

void capnproto_kv::client_send_get(int req_id, simplekv::StringPointer key, dmtr_sgarray_t &sga) {

}

void capnproto_kv::client_send_put(int req_id, simplekv::StringPointer key, simplekv::StringPointer value, dmtr_sgarray_t &sga) {

}

int capnproto_kv::client_handle_response(dmtr_sgarray_t &sga) {
    return 0;
}

string capnproto_kv::client_check_response(dmtr_sgarray_t &sga) {
    return string("");
}

int capnproto_kv::server_handle_request(dmtr_sgarray_t &in_sga, dmtr_sgarray_t &out_sga, bool* free_in, bool* free_out) {
    return 0;
}


