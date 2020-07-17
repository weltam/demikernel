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
simplekv::simplekv(enum library cereal_type) :
            my_cereal_type(cereal_type)
{}

// initializes the kv store from ycsb load file
int simplekv::init(string load_file) {
    ifstream f(load_file.c_str());

    if (!f) {
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
