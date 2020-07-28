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

uint16_t port = 12345;
boost::optional<std::string> server_ip_addr;
boost::optional<std::string> file;
std::string cereal_system = std::string("handcrafted");
std::string config_path;
std::string kv_load;
std::string kv_access;
uint64_t packet_size = 1000;
bool check = false;
uint32_t client_id = 0;
uint32_t timeout_value = 2500000; // in nanoseconds
#define FILL_CHAR 'a'

using namespace boost::program_options;

void parse_args(int argc, char **argv, bool server)
{
    std::string config_path;
    options_description desc{"echo experiment options"};
    desc.add_options()
        ("help", "produce help message")
        ("ip", value<std::string>(), "server ip address")
        ("port", value<uint16_t>(&port)->default_value(12345), "server port")
        ("config-path,r", value<std::string>(&config_path)->default_value("./config.yaml"), "specify configuration file")
        ("system", value<std::string>(&cereal_system)->default_value("none"), "serialization method to test")
        ("loads", value<std::string>(), "kv load file")
        ("access", value<std::string>(), "kv access file")
        ("check", "Check that the puts and gets did the correct thing")
        ("id", value<uint32_t>(&client_id)->default_value(0), "Client id")
        ("timeout", value<uint32_t>(&timeout_value)->default_value(2500000), "Timeout value for retries.")
        ("file", value<std::string>(), "log file");

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

    if (vm.count("loads")) {
        kv_load = vm["loads"].as<std::string>();
        if (access(kv_load.c_str(), R_OK) == -1) {
            std::cerr << "Unable to find kv loads file at `" << kv_load << "`." << std::endl;
        }
    }

    if (vm.count("access")) {
        kv_access = vm["access"].as<std::string>();
        if (access(kv_access.c_str(), R_OK) == -1) {
            std::cerr << "Unable to find kv access file at `" << kv_access << "`." << std::endl;
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

    if (vm.count("id")) {
        client_id = vm["id"].as<uint32_t>();
    }

    if (vm.count("timeout")) {
        timeout_value = vm["timeout"].as<uint32_t>();
    }

    if (vm.count("system")) {
        cereal_system = vm["system"].as<std::string>();
    }

    if (vm.count("file")) {
        file = vm["file"].as<std::string>();
    }

    if (vm.count("check")) {
        check = true;
    }

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

void read_packet(dmtr_sgarray_t &sga, uint32_t field_size) {
    std::string data((char*)sga.sga_segs[0].sgaseg_buf, sga.sga_segs[0].sgaseg_len);
}
#endif
