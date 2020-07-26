// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "common.hh"
#include "capnproto.hh"
#include <capnp/message.h>
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

    simplekv::library library;
    
    if (!std::strcmp(cereal_system.c_str(), "handcrafted")) {
        kv = reinterpret_cast <simplekv*>(&handcrafted_kv_client);
        library = simplekv::library::HANDCRAFTED;
    } else if (!std::strcmp(cereal_system.c_str(), "protobuf")) {
        kv = reinterpret_cast <simplekv*>(&protobuf_kv_client);
        library = simplekv::library::PROTOBUF;
    } else if (!std::strcmp(cereal_system.c_str(), "capnproto")) {
        kv = reinterpret_cast <simplekv*>(&capnproto_kv_client);
        library = simplekv::library::CAPNPROTO;
    } else if (!std::strcmp(cereal_system.c_str(), "flatbuffers")) {
        kv = reinterpret_cast <simplekv*>(&flatbuffers_kv_client);
        library = simplekv::library::FLATBUFFERS;
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
    
    // run the same trace, but check the kv is doing the right thing
    if (check) {
        std::unordered_map<string, string> client_map;
        DMTR_OK(initialize_map(kv_load, client_map));
        
        std::string line;
        ifstream access(kv_access.c_str());
        if (!access) {
            std::cerr << "Error loading access file " << kv_access.c_str() << std::endl;
        }

        std::vector<std::string> request;
        int current_request = 0;
        dmtr_qresult_t wait_out;
        dmtr_qtoken_t push_token;
        dmtr_qtoken_t pop_token;
        bool is_get = false;
        while (getline(access, line)) {
            boost::split(request, line, [](char c){ return c == ' '; });
            uint32_t request_client = std::stoi(request[0]);
            // only do the work for this client
            if (client_id != request_client) {
                continue;
            }
            current_request ++;
            if (strcmp(request[1].c_str(), "GET") == 0) {
                is_get = true;
            }
            simplekv::StringPointer key((char *)request[2].c_str(), request[2].length());
            void *context = NULL;
            capnp::MallocMessageBuilder capnp_builder;
            flatbuffers::FlatBufferBuilder fb_builder(128);
            switch (library) {
                case simplekv::library::CAPNPROTO:
                    context = (void *)(&capnp_builder);
                    break;
                case simplekv::library::FLATBUFFERS:
                    context = (void *)(&fb_builder);
                    break;
                default:
                    break;
            }

            if (is_get) {
                kv->client_send_get(current_request, key, sga, context);
            } else {
            simplekv::StringPointer value((char *)request[3].c_str(), request[3].length());
                kv->client_send_put(current_request, key, value, sga, context);
                client_map[request[2]] = request[3];
            }
            
            DMTR_OK(dmtr_push(&push_token, qd, &sga));
            sent++;
            DMTR_OK(dmtr_pop(&pop_token, qd));

            int ret = dmtr_wait(&wait_out, pop_token);
            if (ret != 0) {
                std::cerr << "Error on pop for req " << current_request << std::endl;
            }
            recved++;
            if (is_get) {
                std::string actual_value = client_map[request[2]];
                std::string network_value = kv->client_check_response(wait_out.qr_value.sga);
                assert(actual_value == network_value);
            } else {
                // free the out sga and in sga
                DMTR_OK(kv->free_sga(&sga, false));
                DMTR_OK(kv->free_sga(&wait_out.qr_value.sga, true));

                if (push_token != 0) {
                    DMTR_OK(dmtr_drop(push_token));
                    push_token = 0;
                }

                capnp::MallocMessageBuilder capnp_check_builder;
                flatbuffers::FlatBufferBuilder fb_check_builder(128);
                switch (library) {
                    case simplekv::library::CAPNPROTO:
                        context = (void *)&capnp_check_builder;
                        break;
                    case simplekv::library::FLATBUFFERS:
                        context = (void *)(&fb_check_builder);
                    default:
                        break;
                }

                kv->client_send_get(current_request, key, sga, context);
                DMTR_OK(dmtr_push(&push_token, qd, &sga));
                DMTR_OK(dmtr_pop(&pop_token, qd));

                int ret = dmtr_wait(&wait_out, pop_token);
                if (ret != 0) {
                    std::cerr << "Error on pop for req " << current_request << std::endl;
                }
                std::string network_value = kv->client_check_response(wait_out.qr_value.sga);
                std::string actual_value = client_map[request[2]];
                assert(actual_value == network_value); 
            }

            // free the out sga
            DMTR_OK(kv->free_sga(&sga, false));
            // free the in sga
            DMTR_OK(kv->free_sga(&wait_out.qr_value.sga, true));
            if (push_token != 0) {
                DMTR_OK(dmtr_drop(push_token));
                push_token = 0;
            }
        }

        finish();
        exit(0);
    }


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
    std::vector<std::string> request;
    while (getline(f, line)) {
        boost::split(request, line, [](char c){return c == ' ';});
        uint32_t request_client = std::stoi(request[0]);
        // only do the work for this client
        if (client_id != request_client) {                
            continue;
        }
        current_request++;
        simplekv::StringPointer key((char *)request[2].c_str(), request[2].length());

        // define context for building message
        void *context = NULL;
        flatbuffers::FlatBufferBuilder fb_builder(128);
        capnp::MallocMessageBuilder capnp_builder;
            switch (library) {
                case simplekv::library::CAPNPROTO:
                    context = (void *)&capnp_builder;
                    break;
                case simplekv::library::FLATBUFFERS:
                    context = (void *)(&fb_builder);
                default:
                    break;
            }


        // fill in the request sga with the next request to send out
        if (strcmp(request[1].c_str(), "GET") == 0) {
            kv->client_send_get(current_request, key, sga, context);

        } else {
            simplekv::StringPointer value((char *)request[3].c_str(), request[3].length());
            kv->client_send_put(current_request, key, value, sga, context);
        }
    
        DMTR_OK(dmtr_push(&push_token, qd, &sga));
        sent++;
        DMTR_OK(dmtr_pop(&pop_tokens[0], qd));
        start_time = boost::chrono::steady_clock::now();

        /*if (recved != 0 && timer_q_push != 0) {
            DMTR_OK(dmtr_drop(timer_q_push));
            timer_q_push = 0;
        }*/
        if (recved == 0) {
            DMTR_OK(dmtr_push_tick(&timer_q_push, timer_qd, timeout));
            // only need to pop timer for the first packet
            DMTR_OK(dmtr_pop(&pop_tokens[1], timer_qd));
        }

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
                // free in sga
                DMTR_OK(kv->free_sga(&wait_out.qr_value.sga, true));
                if (req_id != current_request) {
                    // receiving something from an old retry
                    std::cout << "Received old message of Req: " << req_id << "; current: " << current_request << std::endl;
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
                DMTR_OK(dmtr_push_tick(&timer_q_push, timer_qd, timeout));
                /*if (pop_tokens[1] != 0) {
                    DMTR_OK(dmtr_drop(pop_tokens[1]));
                    pop_tokens[1] = 0;
                }*/

            } else {
                // need to retry
                if (recved != 0) {
                    retries++;
                    auto sent_dt = boost::chrono::steady_clock::now() - start_time;
                    std::cout << "Req " << current_request << " retry after: " << sent_dt.count() << std::endl;

                    if (push_token != 0) {
                        DMTR_OK(dmtr_drop(push_token));
                        push_token = 0;
                    }

                    // push the data again
                    DMTR_OK(dmtr_push(&push_token, qd, &sga));
                    std::cout << "Finished pushing the data again for retry." << std::endl;
                }

                if (timer_q_push != 0) {
                    DMTR_OK(dmtr_drop(timer_q_push));
                    timer_q_push = 0;
                }

                // reset the timer
                DMTR_OK(dmtr_push_tick(&timer_q_push, timer_qd, timeout));
                std::cout << "Reset on timer push" << std::endl;
                DMTR_OK(dmtr_pop(&pop_tokens[1], timer_qd));
                std::cout << "Reset on timer pop" << std::endl;
            }
        } while (!finished_request);
        request.clear();
        // free out sga
        DMTR_OK(kv->free_sga(&sga, false));
    }


    finish();
    exit(0);
}
