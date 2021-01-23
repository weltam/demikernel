// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#define BOOST_CHRONO_VERSION 2

#include "common.hh"
#include "capnproto.hh"
#include "flatbuffers.hh"
#include "protobuf.hh"
#include "protobytes.hh"
#include "stress_cornflakes.hh"
#include "cornflakes.hh"
#include "extra_malloc.hh"
#include "extra_malloc_no_str.hh"
#include "extra_malloc_no_malloc.hh"
#include "extra_malloc_single_memcpy.hh"
#include "message.hh"
#include <arpa/inet.h>
#include <boost/chrono.hpp>
#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <cstring>
#include <dmtr/annot.h>
#include <dmtr/latency.h>
#include <dmtr/libos.h>
#include <dmtr/libos/mem.h>
#include <dmtr/sga.h>
#include <dmtr/wait.h>
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <boost/chrono/chrono_io.hpp>
#define DMTR_PROFILE
#define LWIP

#ifdef LWIP
#include <rte_common.h>
#include <rte_mbuf.h>
#endif

namespace po = boost::program_options;
uint64_t sent = 0, recved = 0;
dmtr_latency_t *latency = NULL;
int qd;
dmtr_sgarray_t sga = {};
echo_message *echo = NULL;
bool free_buf = false;
boost::chrono::time_point<boost::chrono::system_clock> exp_start;
boost::chrono::time_point<boost::chrono::system_clock> exp_end;


//#define TRAILING_REQUESTS 
void finish() {
    exp_end = boost::chrono::system_clock::now();
    auto dt = exp_end - exp_start;
    using namespace boost::chrono;
    std::cout << time_fmt(boost::chrono::timezone::local, "%H:%M:%S") <<
    system_clock::now() << '\n';
    std::cerr << "Start: " << time_fmt(boost::chrono::timezone::local) << exp_start << "; End: " <<  time_fmt(boost::chrono::timezone::local) << exp_end << "; Total time taken: " << dt.count() << std::endl;
    std::cerr << "Sent: " << sent << "  Recved: " << recved << std::endl;
    if (free_buf) {
        dmtr_sgafree(&sga);
    }
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
    std::cout << "Num concurrent clients " << clients << std::endl;
    // SGAs will be allocated with this many segments
    int num_send_segments = sga_size;
    int num_recv_segments = 1;
    DMTR_OK(dmtr_init(argc, argv));

    DMTR_OK(dmtr_new_latency(&latency, "end-to-end"));
    if (boost::none != latency_log) {
        DMTR_OK(dmtr_add_of(latency, boost::get(latency_log).c_str()));
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


    // init serialization messages so variables aren't freed prematurely
    protobuf_echo protobuf_data(packet_size, message);
    protobuf_bytes_echo proto_bytes_data(packet_size, message);
    capnproto_echo capnproto_data(packet_size, message);
    flatbuffers_echo flatbuffers_data(packet_size, message);
    malloc_baseline malloc_baseline_echo(packet_size, message);
    malloc_baseline_no_str malloc_baseline_no_str(packet_size, message);
    malloc_baseline_no_malloc malloc_baseline_no_malloc(packet_size, message);
    malloc_baseline_single_memcpy malloc_baseline_single_memcpy(packet_size, message);
    cornflakes_echo corn_echo(packet_size, message);

    // context for serialize function
    capnp::MallocMessageBuilder message_builder;
    flatbuffers::FlatBufferBuilder fb_builder(packet_size);
    cornflakes::GetMessageCF cornflakes_get;

    void *context = NULL;
    if (zero_copy) {
        dmtr_set_zero_copy();
        sga.sga_numsegs = num_send_segments; // so it doesn't error out
        DMTR_OK(dmtr_init_mempools(packet_size, num_send_segments));
        printf("Done initializing mempools\n");
        free_buf = true;
    } else {
        // if not running serialization test, send normal "aaaaa";
        if (!run_protobuf_test) {
            fill_in_sga(sga, num_send_segments);
            free_buf = true;
        } else if (!std::strcmp(cereal_system.c_str(), "malloc_baseline")) {
            fill_in_sga(sga, num_send_segments);
            echo = &malloc_baseline_echo;
            echo->serialize_message(sga, context);
            free_buf = true;
        } else if (!std::strcmp(cereal_system.c_str(), "malloc_no_str")) {
            fill_in_sga(sga, num_send_segments);
            echo = &malloc_baseline_no_str;
            echo->serialize_message(sga, context);
            free_buf = true;
        } else if (!std::strcmp(cereal_system.c_str(), "memcpy")) {
            fill_in_sga(sga, num_send_segments);
            echo = &malloc_baseline_no_malloc;
            echo->serialize_message(sga, context);
            free_buf = true;
        } else if (!std::strcmp(cereal_system.c_str(), "single_memcpy")) {
            fill_in_sga(sga, num_send_segments);
            echo = &malloc_baseline_single_memcpy;
            echo->serialize_message(sga, context);
            free_buf = true;
        } else if (!std::strcmp(cereal_system.c_str(), "protobuf")) {
            allocate_segments(&sga, num_send_segments);
            echo = &protobuf_data;
            echo->serialize_message(sga, context);
            free_buf = true;
        } else if (!std::strcmp(cereal_system.c_str(), "protobytes")) {
            allocate_segments(&sga, num_send_segments);
            echo = &proto_bytes_data;
            echo->serialize_message(sga, context);
            free_buf = true;
        } else if (!std::strcmp(cereal_system.c_str(), "capnproto")) {
            allocate_segments(&sga, num_send_segments);
            echo = &capnproto_data;
            context = (void *)(&message_builder);
            echo->serialize_message(sga, context);
        } else if (!std::strcmp(cereal_system.c_str(), "flatbuffers")) {
            allocate_segments(&sga, num_send_segments);
            echo = &flatbuffers_data;
            context = (void *)(&fb_builder);
            echo->serialize_message(sga, context);
        } else if (!std::strcmp(cereal_system.c_str(), "cornflakes")) {
            allocate_segments(&sga, num_send_segments);
            echo = &corn_echo;
            // setup external memory in dpdk datapath
            corn_echo.init_ext_mem();
            uint16_t ext_mem_len = (uint16_t)corn_echo.get_mmap_len();
            void *mmap_addr = corn_echo.get_mmap_addr();
            DMTR_OK(dmtr_set_external_memory(mmap_addr, &ext_mem_len));
            corn_echo.set_mmap_available_len((size_t )ext_mem_len);
            context = (void *)(&cornflakes_get);
            echo->serialize_message(sga, context);
        } else {
            std::cerr << "Serialization cereal_system " << cereal_system << " unknown." << std::endl;
            exit(1);
        }
    }
    uint32_t last_sent = 0;
    exp_start = boost::chrono::system_clock::now();
    // run a simpler test with retries turned on
    if (retries) {
        // one more iteration: don't count first
        // iterations += 1;
        // set up timers for each client
        int timer_qds[clients];

        for (uint32_t c = 0; c < clients; c++) {
            DMTR_OK(dmtr_new_timer(&timer_qds[c]));
            //printf("Timer queue # %u: %d\n", c, timer_qds[c]);
        }

        // maps seq number of request to start time
        std::unordered_map<uint32_t, boost::chrono::time_point<boost::chrono::steady_clock>> start_times;
        // maps seq number to push client/timer ID
        std::unordered_map<uint32_t, uint32_t> seqno_to_client;
        std::unordered_map<uint32_t, uint32_t> client_to_seqno;

        int ret;
        int num_retries = 0;
        dmtr_qresult_t wait_out;
        allocate_segments(&wait_out.qr_value.sga, num_recv_segments);

        dmtr_qtoken_t push_tokens[clients];
        dmtr_qtoken_t pop_tokens[clients*2];
        dmtr_qtoken_t timer_q_push[clients];
        int timeout = 800000;
        
        // set up our signal handlers
        if (signal(SIGINT, sig_handler) == SIG_ERR)
            std::cout << "\ncan't catch SIGINT\n";

        // start all the clients
        for (uint32_t c = 0; c < clients; c++) {
            // last sent packet
            last_sent += 1;
            seqno_to_client.insert(std::make_pair(last_sent, c));
            client_to_seqno.insert(std::make_pair(c, last_sent));
            sga.id = last_sent;

            // first push and pop
            DMTR_OK(dmtr_push(&push_tokens[c], qd, &sga));
            sent++;
            DMTR_OK(dmtr_pop(&pop_tokens[2*c], qd));
            
            // record start time
            start_times.insert(std::make_pair(last_sent, boost::chrono::steady_clock::now()));
            
            // start timer for this client
            DMTR_OK(dmtr_push_tick(&timer_q_push[c], timer_qds[c], timeout));
            DMTR_OK(dmtr_pop(&pop_tokens[2*c+1], timer_qds[c]));
            //printf("Sent seqno %u with pop token %lu\n", last_sent, pop_tokens[2*c]);
        } 

        int idx = 0;

        do {
            // wait for a returned value
            ret =  dmtr_wait_any(&wait_out, &idx, pop_tokens, clients*2);

            // one of clients timed out
            if (idx % 2 != 0) {
                uint32_t client_idx = (idx - 1)/2;
                uint32_t seqno = client_to_seqno.at(client_idx);

                // first packet takes longer
                if (recved != 0) {
                    // actual retry
                    num_retries++;

                    auto sent_dt = boost::chrono::steady_clock::now() - start_times.at(seqno);
                    std::cout << "Idx " << idx << " fired after " << sent_dt.count() << " time, since sent: " << sent_dt.count() << " time, recvd so far: " << recved << ", pkt id: " << seqno <<  std::endl;
                    // drop the push token from this echo
                    if (push_tokens[client_idx] != 0) {
                        DMTR_OK(dmtr_drop(push_tokens[client_idx]));
                        push_tokens[client_idx] = 0;
                    }
                    sga.id = seqno; // make sure pushed packet has correct ID
                
                    DMTR_OK(dmtr_push(&push_tokens[client_idx], qd, &sga));
                    printf("Resent seq # %u\n", seqno);
                }
                
                // reset the timeout
                if (timer_q_push[client_idx] != 0) {
                    DMTR_OK(dmtr_drop(timer_q_push[client_idx]));
                    timer_q_push[client_idx] = 0;
                }
                
                DMTR_OK(dmtr_push_tick(&timer_q_push[client_idx], timer_qds[client_idx], timeout));
                DMTR_OK(dmtr_pop(&pop_tokens[idx], timer_qds[client_idx]));
                continue;
            } else {
                // received a packet
                auto recved_time = boost::chrono::steady_clock::now();
                uint32_t pop_idx = idx/2;
                uint32_t seqno = wait_out.qr_value.sga.id;

                // if seqno is not in current outstanding packets, must be old
                if (seqno_to_client.find(seqno) == seqno_to_client.end()) {
                    printf("Received packet with old id: %u; not in outstanding seqno map; sent: %lu\n", seqno, sent);
                    // add another pop, as this "took place" of packet that was
                    // supposed to be received
                    DMTR_OK(dmtr_pop(&pop_tokens[pop_idx * 2], qd));
                    continue;
                }

                uint32_t client_idx = seqno_to_client.at(seqno);
                if (client_to_seqno.find(client_idx) == client_to_seqno.end() ||
                        client_to_seqno.at(client_idx) != seqno) {
                    printf("Received seqno %u for client %u but client %u's entry in client_to_seqno does not exist or is different\n", seqno, client_idx, client_idx);
                    exit(1);
                }
                
                //printf("Received seqno %u on pop idx %d\n", seqno, pop_idx);
                // calculate start time
                auto dt = recved_time - start_times.at(seqno);

                // remove entries from maps
                client_to_seqno.erase(client_to_seqno.find(client_idx));
                seqno_to_client.erase(seqno_to_client.find(seqno));
                start_times.erase(start_times.find(seqno));

                last_sent += 1;
                seqno = last_sent;
                sga.id = seqno;

                seqno_to_client.insert(std::make_pair(seqno, client_idx));
                client_to_seqno.insert(std::make_pair(client_idx, seqno));
                
                DMTR_OK(dmtr_record_latency(latency, dt.count()));
                recved++;
                iterations--;

                if (push_tokens[client_idx] != 0) {
                    DMTR_OK(dmtr_drop(push_tokens[client_idx]));
                    push_tokens[client_idx] = 0;
                }
                // drop timer token from this echo
                if (timer_q_push[client_idx] != 0) {
                    DMTR_OK(dmtr_drop(timer_q_push[client_idx]));
                    timer_q_push[client_idx] = 0;
                }

                // free the recv buffer
                DMTR_OK(dmtr_sgafree(&wait_out.qr_value.sga));
                if (wait_out.qr_value.sga.recv_segments != NULL) {
#ifdef LWIP
                    rte_pktmbuf_free((struct rte_mbuf*) wait_out.qr_value.sga.recv_segments);
#endif
                }

                // send a new packet on the client_idx
                DMTR_OK(dmtr_push(&push_tokens[client_idx], qd, &sga));
                sent++;
                // pop on this popped token
                DMTR_OK(dmtr_pop(&pop_tokens[pop_idx*2], qd));
                //printf("Sent seqno %u with pop token %lu\n", seqno, pop_tokens[pop_idx]);
                start_times.insert(std::make_pair(seqno, boost::chrono::steady_clock::now()));
                
                // reset the timer corresponding to this pushed packet
                DMTR_OK(dmtr_push_tick(&timer_q_push[client_idx], timer_qds[client_idx], timeout));
            }   
        } while (iterations > 0 && ret == 0);
        std::cout << "Final num retries: " << num_retries << std::endl;
        finish();
        exit(0);

        // free the wait out segment buffers
        DMTR_OK(dmtr_sgafree_segments(&wait_out.qr_value.sga));
    } 

    std::cerr << "Number of clients: " << clients << std::endl;

    dmtr_qtoken_t push_tokens[clients];
    dmtr_qtoken_t pop_tokens[clients];
    std::unordered_map<uint32_t, boost::chrono::time_point<boost::chrono::steady_clock>> start_times; 
    std::unordered_map<uint32_t, uint32_t> seqno_to_client;

    // set up our signal handlers
    if (signal(SIGINT, sig_handler) == SIG_ERR)
        std::cout << "\ncan't catch SIGINT\n";
 
    // start all the clients
    for (uint32_t c = 0; c < clients; c++) {
        last_sent++;
        seqno_to_client.insert(std::make_pair(last_sent, c));
        sga.id = last_sent;
        
        printf("Sga size: %u\n", sga.sga_numsegs);
        // push message to server
        DMTR_OK(dmtr_push(&push_tokens[c], qd, &sga));
        sent++;
        // async pop
        DMTR_OK(dmtr_pop(&pop_tokens[c], qd));
        // record start time
        start_times.insert(std::make_pair(last_sent, boost::chrono::steady_clock::now()));
    }
    
    int ret;
    dmtr_qresult_t wait_out;
    allocate_segments(&wait_out.qr_value.sga, num_recv_segments);
    
    do {
        int idx = 0;
        // wait for a returned value
        ret =  dmtr_wait_any(&wait_out, &idx, pop_tokens, clients);
        uint32_t seqno = wait_out.qr_value.sga.id;
        uint32_t client_idx = seqno_to_client.at(seqno);
        
        auto dt = boost::chrono::steady_clock::now() - start_times.at(seqno);
        DMTR_OK(dmtr_record_latency(latency, dt.count()));
        
        recved++;
        seqno_to_client.erase(seqno_to_client.find(seqno));
        start_times.erase(start_times.find(seqno));
        
        // free the allocated sga
        DMTR_OK(dmtr_sgafree(&wait_out.qr_value.sga));
        if (wait_out.qr_value.sga.recv_segments != NULL) {
#ifdef LWIP
            rte_pktmbuf_free((struct rte_mbuf*) wait_out.qr_value.sga.recv_segments);
#endif
        }

        // count the iteration
        iterations--;
        // drop the push token from this echo
        if (push_tokens[client_idx] != 0) {
            DMTR_OK(dmtr_drop(push_tokens[client_idx]));
            push_tokens[client_idx] = 0;
        }

#ifndef TRAILING_REQUESTS        
        // if there are fewer than clients left, then we just wait for the responses
        if (iterations < clients) {
            pop_tokens[idx] = 0;
            continue;
        }
#endif
        last_sent++;
        seqno_to_client.insert(std::make_pair(last_sent, client_idx));
        sga.id = last_sent;
        // start again
        // push back to the server
        DMTR_OK(dmtr_push(&push_tokens[client_idx], qd, &sga));
        sent++;
        // async pop
        DMTR_OK(dmtr_pop(&pop_tokens[idx], qd));
        // restart the clock
        start_times.insert(std::make_pair(last_sent, boost::chrono::steady_clock::now()));
    } while (iterations > 0 && ret == 0);

    finish();
    exit(0);
    
    // free the wait out segment buffers
    DMTR_OK(dmtr_sgafree_segments(&wait_out.qr_value.sga));
}
