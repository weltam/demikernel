// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#include "kv.hh"
#include <fstream>
#include <iostream>
#include <string>
#include<vector>
#include <boost/algorithm/string.hpp>

using namespace std;

int32_t encode_enum(simplekv::request req) {
    switch (req) {
        case simplekv::GET:
            return 0;
        case simplekv::PUT:
            return 1;
        case simplekv::RESPONSE:
            return 2;
        default:
            std::cerr << "Unknown enum value to encode" << std::endl;
            exit(1);
    }
}

simplekv::request decode_enum(int32_t val) {
    if (val == 0) {
        return simplekv::GET;
    } else if (val == 1) {
        return simplekv::PUT;
    } else if (val == 2) {
        return simplekv::RESPONSE;
    }
    std::cerr << "Unknown enum val: " << val << std::endl;
    exit(1);
}

int initialize_map(string load_file, unordered_map<string, string> &my_map) {
    ifstream f(load_file.c_str());

    if (!f) {
        std::cerr << "Error opening: " << load_file << std::endl;
        return 1;
    }

    string line;
    while(getline(f, line)) {
        vector<string> request;
        boost::split(request, line, [](char c){return c == ' ';});
        if (request.size() != 4) {
            return 1;
        }

        if (strcmp(request[1].c_str(), "UPDATE") != 0) {
            return 1;
        }
        my_map.insert(make_pair(request[2], request[3]));
    }

    f.close();
    return 0;
}

simplekv::simplekv(enum library cereal_type) :
            my_cereal_type(cereal_type)
{}

// initializes the kv store from ycsb load file
int simplekv::init(string load_file) {
    if (initialize_map(load_file, my_map) != 0) {
        return 1;
    }
    // for handcrafted serialization, need to initialize the bytes dictionary
    switch (my_cereal_type) {
        case simplekv::library::HANDCRAFTED:
            for (auto &it: my_map) {
                simplekv::StringPointer key((char *)it.first.c_str(), it.first.length());
                simplekv::StringPointer value((char *)it.second.c_str(), it.second.length());
                my_bytes_map.insert(make_pair(key, value));
            }
        default:
            break;
    }
    return 0;
}

// depending on if this is a recv buffer or send buffer for kv server of client
// frees the entire sga or just the pointer at front
// Ptr at front describes message type and req_id
// For capnproto and flatbuffers, the pointers after idx 0 go to the capnproto
// or flatbuffer builder in memory
// So we don't want to free that by accident
int simplekv::free_sga(dmtr_sgarray_t* sga, bool is_in) {
    switch (my_cereal_type) {
        case simplekv::library::PROTOBUF:
            return dmtr_sgafree(sga);
        case simplekv::library::FLATBUFFERS:
            if (is_in) {
                return dmtr_sgafree(sga);
            } else {
                return dmtr_sgafree_seg(sga, 0);
            }
        case simplekv::library::CAPNPROTO:
            if (is_in) {
                return dmtr_sgafree(sga);
            } else {
                return dmtr_sgafree_seg(sga, 0);
            }
        case simplekv::library::HANDCRAFTED:
            if (is_in) {
                return dmtr_sgafree(sga);
            } else {
                return dmtr_sgafree_seg(sga, 0);
            }
    }
    return 0;
}
