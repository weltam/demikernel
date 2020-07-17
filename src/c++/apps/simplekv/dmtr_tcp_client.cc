// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "common.hh"
#include "capnproto.hh"
#include "flatbuffers.hh"
#include "protobuf.hh"
#include "kv.hh"
#include "handcrafted.hh"
#include <arpa/inet.h>
#include <boost/chrono.hpp>
#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/algorithm/string.hpp>
#include <cstring>
#include <dmtr/annot.h>
#include <dmtr/latency.h>
#include <dmtr/libos.h>
#include <dmtr/libos/mem.h>
#include <dmtr/sga.h>
#include <dmtr/wait.h>
#include <iostream>
#include <fstream>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

//#define DMTR_PROFILE

namespace po = boost::program_options;
uint64_t sent = 0, recved = 0;
dmtr_latency_t *latency = NULL;
int qd;
dmtr_sgarray_t sga = {};
simplekv *kv = NULL;
int retries = 0;

void finish() {
    std::cerr << "Sent: " << sent << "  Recved: " << recved << std::endl;
    std::cerr << "Final num retries: " << retries << std::endl;
    dmtr_sgafree(&sga);
    dmtr_close(qd);
    dmtr_dump_latency(stderr, latency);
}

void sig_handler(int signo)
{
    finish();
    exit(0);
}


int main(int argc, char *argv[]) {
    parse_args(argc, argv, false);
    DMTR_OK(dmtr_init(argc, argv));

    DMTR_OK(dmtr_new_latency(&latency, "end-to-end"));

    // set up signal handler
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        std::cout << "\nCan't catch SIGINT\n";
    }

    // set up and initialize client kv
    handcrafted_kv handcrafted_kv_client;
    protobuf_kv protobuf_kv_client;
    capnproto_kv capnproto_kv_client;
    flatbuffers_kv flatbuffers_kv_client;
    
    if (!std::strcmp(cereal_system.c_str(), "none")) {
        kv = reinterpret_cast <simplekv*>(&handcrafted_kv_client);
    } else if (!std::strcmp(cereal_system.c_str(), "protobuf")) {
        kv = reinterpret_cast <simplekv*>(&protobuf_kv_client);
    } else if (!std::strcmp(cereal_system.c_str(), "capnproto")) {
        kv = reinterpret_cast <simplekv*>(&capnproto_kv_client);
    } else if (!std::strcmp(cereal_system.c_str(), "flatbuffers")) {
        kv = reinterpret_cast <simplekv*>(&flatbuffers_kv_client);
    } else {
        std::cerr << "Serialization cereal_system " << cereal_system  << " unknown." << std::endl;
        exit(1);
    }

    DMTR_OK(dmtr_socket(&qd, AF_INET, SOCK_STREAM, 0));
    printf("client qd:\t%d\n", qd);
 
    struct sockaddr_in saddr = {};
    saddr.sin_family = AF_INET;
    const char *server_ip = boost::get(server_ip_addr).c_str();
    if (inet_pton(AF_INET, server_ip, &saddr.sin_addr) != 1) {
        std::cerr << "Unable to parse IP address." << std::endl;
        return -1;
    }
    saddr.sin_port = htons(port);

    std::cerr << "Attempting to connect to `" << boost::get(server_ip_addr) << ":" << port << "`..." << std::endl;
    dmtr_qtoken_t q;
    DMTR_OK(dmtr_connect(&q, qd, reinterpret_cast<struct sockaddr *>(&saddr), sizeof(saddr)));

    dmtr_qresult_t qr = {};
    DMTR_OK(dmtr_wait(&qr, q));
    std::cerr << "Connected." << std::endl;


    // load the request file
    std::string line;
    ifstream f(kv_access.c_str());
    if (!f) {
        std::cerr << "Error loading access file" << std::endl;
    }


    int current_request = 0;
    int timer_qd;
    int ret;
    dmtr_qresult_t wait_out;
    dmtr_qtoken_t push_token;
    dmtr_qtoken_t pop_tokens[2];
    dmtr_qtoken_t timer_q_push;
    boost::chrono::time_point<boost::chrono::steady_clock> start_time;

    // initialize timer_qd
    DMTR_OK(dmtr_new_timer(&timer_qd));

    boost::chrono::nanoseconds timeout { 1000000 };
    while (getline(f, line)) {
        std::vector<std::string> request;
        boost::split(request, line, [](char c){return c == ' ';});

        // fill in the request sga with the next request to send out
        if (strcmp(request[1].c_str(), "GET") == 0) {
            kv->client_send_get(current_request, request[2], sga);

        } else {
            // get request
            kv->client_send_put(current_request, request[2], request[3], sga);
        }
    
        std::vector<dmtr_qtoken_t> tokens;
        DMTR_OK(dmtr_push(&push_token, qd, &sga));
        sent++;
        DMTR_OK(dmtr_pop(&pop_tokens[0], qd));
        start_time = boost::chrono::steady_clock::now();

        DMTR_OK(dmtr_push_tick(&timer_q_push, timer_qd, timeout));
        DMTR_OK(dmtr_pop(&pop_tokens[1], timer_qd));

        int idx = 0;
        bool finished_request = false;
        do {
            // wait for a returned value or for the timer to pop
            ret = dmtr_wait_any(&wait_out, &idx, pop_tokens, 2);
            if (ret != 0) {
                std::cerr << "Error on waiting for request" << std::endl;
                exit(1);
            }
            if (idx == 0) {
                // client request came back
                auto dt = boost::chrono::steady_clock::now() - start_time;
                // check if the response is the same current request ID
                int req_id = kv->client_handle_response(wait_out.qr_value.sga);
                if (req_id != current_request) {
                    // receiving something from an old retry
                    continue;
                }

                // record that a new request came back
                recved++;
                DMTR_OK(dmtr_record_latency(latency, dt.count()));
                finished_request = true;
                
                // drop the push token from past push
                if (push_token != 0) {
                    DMTR_OK(dmtr_drop(push_token));
                    push_token = 0;
                }

                if (timer_q_push != 0) {
                    DMTR_OK(dmtr_drop(timer_q_push));
                    timer_q_push = 0;
                }

                // drop the past timer pop?
                if (pop_tokens[1] != 0) {
                    DMTR_OK(dmtr_drop(pop_tokens[1]));
                    pop_tokens[1] = 0;

                }
            } else {
                // need to retry
                if (recved != 0) {
                    retries++;
                    auto sent_dt = boost::chrono::steady_clock::now() - start_time;
                    std::cout << "Req " << current_request << " retry after: " << sent_dt.count() << std::endl;
                    // push the data again
                    DMTR_OK(dmtr_push(&push_token, qd, &sga));
                }

                if (timer_q_push != 0) {
                    DMTR_OK(dmtr_drop(timer_q_push));
                    timer_q_push = 0;
                }

                // reset the timer
                DMTR_OK(dmtr_push_tick(&timer_q_push, timer_qd, timeout));
                DMTR_OK(dmtr_pop(&pop_tokens[1], timer_qd));
            }
        } while (!finished_request);
    }


    finish();
    exit(0);
}
