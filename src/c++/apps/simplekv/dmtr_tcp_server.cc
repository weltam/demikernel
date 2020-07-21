// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "common.hh"
#include "capnproto.hh"
#include "kv.hh"
#include "flatbuffers.hh"
#include "handcrafted.hh"
#include "protobuf.hh"
#include <arpa/inet.h>
#include <boost/chrono.hpp>
#include <boost/optional.hpp>
#include <cassert>
#include <cstring>
#include <dmtr/annot.h>
#include <dmtr/latency.h>
#include <dmtr/libos.h>
#include <dmtr/libos/mem.h>
#include <dmtr/sga.h>
#include <dmtr/wait.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <stdio.h>
#include <string.h>


//#define DMTR_PROFILE
// #define OPEN2
// general file descriptors
int lqd = 0;
int fqd = 0;
uint64_t sent = 0;
uint64_t recved = 0;
simplekv *kv = NULL;

void sig_handler(int signo)
{
    dmtr_close(lqd);
    if (0 != fqd) dmtr_close(fqd);
    std::cerr << "Sent: " << sent << "  Recved: " << recved << std::endl;
    exit(0);
}

int main(int argc, char *argv[])
{
    // grab commandline args
    parse_args(argc, argv, true);

    // set up server socket address
    struct sockaddr_in saddr = {};
    saddr.sin_family = AF_INET;
    if (boost::none == server_ip_addr) {
        std::cerr << "Listening on `*:" << port << "`..." << std::endl;
        saddr.sin_addr.s_addr = INADDR_ANY;
    } else {
        const char *s = boost::get(server_ip_addr).c_str();
        std::cerr << "Listening on `" << s << ":" << port << "`..." << std::endl;
        if (inet_pton(AF_INET, s, &saddr.sin_addr) != 1) {
            std::cerr << "Unable to parse IP address." << std::endl;
            return -1;
        }
    }
    saddr.sin_port = htons(port);

    DMTR_OK(dmtr_init(argc, argv));

    std::vector<dmtr_qtoken_t> tokens;
    dmtr_qtoken_t push_tokens[256];
    std::pair<dmtr_sgarray_t, bool> popped_buffers[256];
    std::pair<dmtr_sgarray_t, bool> pushed_buffers[256];
    memset(push_tokens, 0, 256 * sizeof(dmtr_qtoken_t));
    dmtr_qtoken_t qtemp;

    // open listening socket
    DMTR_OK(dmtr_socket(&lqd, AF_INET, SOCK_STREAM, 0));
    std::cout << "listen qd: " << lqd << std::endl;

    DMTR_OK(dmtr_bind(lqd, reinterpret_cast<struct sockaddr *>(&saddr), sizeof(saddr)));

    DMTR_OK(dmtr_listen(lqd, 10));

    // our accept is asynchronous
    DMTR_OK(dmtr_accept(&qtemp, lqd));
    // add the accept as the first token
    tokens.push_back(qtemp);

    // set up our signal handlers
    if (signal(SIGINT, sig_handler) == SIG_ERR)
        std::cout << "\ncan't catch SIGINT\n";

    // set up and initialize server kv
    handcrafted_kv handcrafted_kv_server;
    protobuf_kv protobuf_kv_server;
    capnproto_kv capnproto_kv_server;
    flatbuffers_kv flatbuffers_kv_server;
    
    if (!std::strcmp(cereal_system.c_str(), "none")) {
        kv = reinterpret_cast <simplekv*>(&handcrafted_kv_server);
    } else if (!std::strcmp(cereal_system.c_str(), "protobuf")) {
        kv = reinterpret_cast <simplekv*>(&protobuf_kv_server);
    } else if (!std::strcmp(cereal_system.c_str(), "capnproto")) {
        kv = reinterpret_cast <simplekv*>(&capnproto_kv_server);
    } else if (!std::strcmp(cereal_system.c_str(), "flatbuffers")) {
        kv = reinterpret_cast <simplekv*>(&flatbuffers_kv_server);
    } else {
        std::cerr << "Serialization cereal_system " << cereal_system  << " unknown." << std::endl;
        exit(1);
    }

    if (kv->init(kv_load) != 0) {
        std::cerr << "Error initializing key value store with load file: " << kv_load << std::endl;
        exit(1);
    }
    
    dmtr_qresult_t wait_out;
    int idx = 0;
    while (1) {
        int status = dmtr_wait_any(&wait_out, &idx, tokens.data(), tokens.size());

        // if we got an EOK back from wait
        if (status == 0) {
            //std::cout << "Found something: qd=" << wait_out.qr_qd;

            // check if it's the listening socket
            if (wait_out.qr_qd == lqd) {
                DMTR_TRUE(EINVAL, idx == 0);
                std::cerr << "connection accepted (qid = " << wait_out.qr_value.ares.qd << ")." << std::endl;
                // do a pop on the incoming connection 
                DMTR_OK(dmtr_pop(&qtemp, wait_out.qr_value.ares.qd));
                // add the token to the token list
                tokens.push_back(qtemp);
                DMTR_OK(dmtr_accept(&tokens[0], lqd));
            } else {
                recved++;
                // free last pushed and popped buffers if necessary
                if (popped_buffers[idx].second) {
                    DMTR_OK(kv->free_sga(&popped_buffers[idx].first, true));
                }
                popped_buffers[idx] = std::make_pair(wait_out.qr_value.sga, false);
                
                // process the request
                dmtr_sgarray_t out_sga;
                bool free_in;
                bool free_out;
                DMTR_OK(kv->server_handle_request(wait_out.qr_value.sga, out_sga, &free_in, &free_out));

                if (pushed_buffers[idx].second) {
                    DMTR_OK(kv->free_sga(&pushed_buffers[idx].first, false));
                }

                pushed_buffers[idx] = std::make_pair(out_sga, free_out);
                popped_buffers[idx].second = free_in;

                // remove last push token
                if (push_tokens[idx] != 0) {
                    // should be done by now if we already got a response
                    DMTR_OK(dmtr_drop(push_tokens[idx]));
                }

                push_tokens[idx] = 0;
                
                // reserialize the message to send back into the array
                DMTR_OK(dmtr_push(&push_tokens[idx], wait_out.qr_qd, &out_sga));
                sent++;
                // async pop to get next message
                DMTR_OK(dmtr_pop(&tokens[idx], wait_out.qr_qd));
            }
        } else {
            assert(status == ECONNRESET || status == ECONNABORTED);
            fprintf(stderr, "closing connection");
            dmtr_close(wait_out.qr_qd);
            tokens.erase(tokens.begin()+idx);
        }
    }
}


