// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "capnproto.hh"
#include <string>
#include <dmtr/sga.h>
#include <sys/types.h>

capnproto_kv::capnproto_kv() :
    simplekv(simplekv::library::CAPNPROTO)
{}

void capnproto_kv::client_send_get(int req_id, string key, dmtr_sgarray_t &sga) {

}

void capnproto_kv::client_send_put(int req_id, string key, string value, dmtr_sgarray_t &sga) {

}

int capnproto_kv::client_handle_response(dmtr_sgarray_t &sga) {
    return 0;
}

void capnproto_kv::server_handle_request(dmtr_sgarray_t &sga) {

}


