// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef ECHO_COMMON_H_
#define ECHO_COMMON_H_

#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <dmtr/annot.h>
#include <dmtr/types.h>
#include <iostream>
#include <dmtr/libos/mem.h>
#include <string.h>
#include <yaml-cpp/yaml.h>
#include <cstring>
#include <stdio.h>

#define DEFAULT_SGASIZE_SER_TEST 4
//#define DMTR_ALLOCATE_SEGMENTS

uint16_t port = 12345;
boost::optional<std::string> server_ip_addr;
uint32_t packet_size = 64;
uint32_t iterations = 10;
uint32_t clients = 1;
bool retries = false;
const char FILL_CHAR = 'a';
boost::optional<std::string> file;
std::string cereal_system = std::string("none");
std::string message = std::string("none");
boost::optional<std::string> latency_log;
bool run_protobuf_test = false;
uint32_t sga_size = 1;
bool zero_copy = false;

using namespace boost::program_options;

void parse_args(int argc, char **argv, bool server)
{
    std::string config_path;
    options_description desc{"echo experiment options"};
    desc.add_options()
        ("help", "produce help message")
        ("ip", value<std::string>(), "server ip address")
        ("port", value<uint16_t>(&port)->default_value(12345), "server port")
        ("size,s", value<uint32_t>(&packet_size)->default_value(64), "packet payload size")
        ("iterations,i", value<uint32_t>(&iterations)->default_value(10), "test iterations")
        ("clients,c", value<uint32_t>(&clients)->default_value(1), "clients")
        ("config-path,r", value<std::string>(&config_path)->default_value("./config.yaml"), "specify configuration file")
        ("file", value<std::string>(), "log file")
        ("latlog", value<std::string>(), "log file for latencies")
        ("system", value<std::string>(&cereal_system)->default_value("none"), "serialization method to test")
        ("message", value<std::string>(), "message type to test")
        ("sgasize", value<uint32_t>(&sga_size)->default_value(1), "Number of buffers in sga")
        ("retry", "run client with retries")
        ("zero-copy", "zero copy mode");


    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    // print help
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        exit(0);
    }

    if (!server) {
        server_ip_addr = "127.0.0.1";
    }

    if (access(config_path.c_str(), R_OK) == -1) {
        std::cerr << "Unable to find config file at `" << config_path << "`." << std::endl;
    } else {
        YAML::Node config = YAML::LoadFile(config_path);
        if (server) {
            YAML::Node node = config["server"]["bind"]["host"];
            if (YAML::NodeType::Scalar == node.Type()) {
                server_ip_addr = node.as<std::string>();
            }

            node = config["server"]["bind"]["port"];
            if (YAML::NodeType::Scalar == node.Type()) {
                port = node.as<uint16_t>();
            }
        } else {
            YAML::Node node = config["client"]["connect_to"]["host"];
            if (YAML::NodeType::Scalar == node.Type()) {
                server_ip_addr = node.as<std::string>();
            }

            node = config["client"]["connect_to"]["port"];
            if (YAML::NodeType::Scalar == node.Type()) {
                port = node.as<uint16_t>();
            }
        }
    }

    if (vm.count("ip")) {
        server_ip_addr = vm["ip"].as<std::string>();
        //std::cout << "Setting server IP to: " << ip << std::endl;
    }

    if (vm.count("port")) {
        port = vm["port"].as<uint16_t>();
        //std::cout << "Setting server port to: " << port << std::endl;
    }

    if (vm.count("iterations")) {
        iterations = vm["iterations"].as<uint32_t>();
        //std::cout << "Setting iterations to: " << iterations << std::endl;
    }

    if (vm.count("size")) {
        packet_size = vm["size"].as<uint32_t>();
        //std::cout << "Setting packet size to: " << packet_size << " bytes." << std::endl;
    }

    if (vm.count("file")) {
        file = vm["file"].as<std::string>();
    }

    if (vm.count("latlog")) {
        latency_log = vm["latlog"].as<std::string>();
    }

    if (vm.count("system")) {
        cereal_system = vm["system"].as<std::string>();
        if (std::strcmp(cereal_system.c_str(), "none")) {
            std::cout << "Setting run_protobuf_test true" << std::endl;
            run_protobuf_test = true;
        }
    }

    if (vm.count("message")) {
        message = vm["message"].as<std::string>();
    }

    if (vm.count("retry")) {
        std::cout << "Running with retries turned on; iterations are " << iterations << std::endl;
        retries = true;
    }

    if (vm.count("zero-copy")) {
        zero_copy = true;
    }

    if (vm.count("sgasize")) {
        sga_size = vm["sgasize"].as<uint32_t>();
        if (run_protobuf_test && sga_size == 1) {
            // for the non baseline tests, baseline number of segments used for
            // now
            sga_size = DEFAULT_SGASIZE_SER_TEST;
        }
    }

    std::cout << "system: " << cereal_system << ", message: " << message << std::endl;
};

void* generate_packet()
{
    void *p = NULL;
    dmtr_malloc(&p, packet_size);
    char *s = reinterpret_cast<char *>(p);
    memset(s, FILL_CHAR, packet_size);
    s[packet_size - 1] = '\0';
    return p;
};

void* fill_packet(uint32_t size) {
    void * p = NULL;
    dmtr_malloc(&p, size);
    char *s = reinterpret_cast<char *>(p);
    memset(s, FILL_CHAR, size);
    s[size - 1] = '\0';
    return p;
}

void read_packet(dmtr_sgarray_t &sga, uint32_t field_size) {
    std::string data((char*)sga.sga_segs[0].sgaseg_buf, sga.sga_segs[0].sgaseg_len);
}

void allocate_segments(dmtr_sgarray_t* sga, uint32_t num_segments) {
#ifdef DMTR_ALLOCATE_SEGMENTS
    // allocate the segments
    void* segments = malloc(sizeof(dmtr_sgaseg) * num_segments);
    assert(segments != NULL);
    sga->sga_segs = (dmtr_sgaseg*)segments;
#endif
}

void fill_in_sga(dmtr_sgarray_t &sga, uint32_t num_segments) {
    allocate_segments(&sga, num_segments);
    sga.sga_numsegs = num_segments;
    uint32_t segment_size = packet_size / num_segments;
    for (uint32_t i = 0; i < num_segments; i++) {
        uint32_t size = segment_size;
        if (i == num_segments - 1) {
            size = packet_size - (segment_size * (num_segments - 1));
        } 
        sga.sga_segs[i].sgaseg_len = size;
        sga.sga_segs[i].sgaseg_buf = fill_packet(size);
    }
}

void fill_in_sga_no_alloc(dmtr_sgarray_t &sga, uint32_t num_segments) {
    allocate_segments(&sga, num_segments);
    sga.sga_numsegs = num_segments;
    uint32_t segment_size = packet_size / num_segments;
    for (uint32_t i = 0; i < num_segments; i++) {
        uint32_t size = segment_size;
        if (i == num_segments - 1) {
            size = packet_size - (segment_size * (num_segments - 1));
        }
        sga.sga_segs[i].sgaseg_len = size;
    }
}
#endif
