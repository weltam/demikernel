// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.

#ifndef KV_H_
#define KV_H_

#include <dmtr/sga.h>
#include <unordered_map>
#include <string>
#include <experimental/string_view>
#include <sys/types.h>


using namespace std;

class simplekv
{
    public: enum library {
        PROTOBUF,
        CAPNPROTO,
        FLATBUFFERS,
        HANDCRAFTED,
    };

    public: enum request {
        GET,
        PUT,
        RESPONSE,
    };

    struct StringPointer {
        char* ptr;
        size_t len;
        dmtr_sgarray_t sga;

        // constructor
        StringPointer(char* bytes, size_t bytes_len) :
            ptr(bytes),
            len(bytes_len),
            sga({})
        {}

        StringPointer(char* bytes, size_t bytes_len, dmtr_sgarray_t &sga) :
            ptr(bytes),
            len(bytes_len),
            sga(sga)
        {}


        StringPointer(const StringPointer& sp) :
            ptr(sp.ptr),
            len(sp.len),
            sga{}
        {}

        // equality
        bool operator == (const StringPointer &other) const {
            std::experimental::string_view me(ptr, len);
            std::experimental::string_view other_str(other.ptr, other.len);
            return me == other_str;
        }

        std::string string() {
            return std::string((char *)ptr, len);
        }
    };

    struct HashStringPointer
    {
        std::size_t operator() (const StringPointer &stringptr) const
        {
            std::experimental::string_view me(stringptr.ptr, stringptr.len);
            return std::hash<std::experimental::string_view>()(me);
        }
    };

    protected: enum library my_cereal_type;
    protected: unordered_map<string, string> my_map;
    // for handcrafted serialization, store map as raw pointers
    protected: unordered_map<StringPointer, StringPointer, HashStringPointer> my_bytes_map;


    protected: simplekv(enum library cereal_type);
    public: virtual ~simplekv() {}

    public: int init(string load_file);
    public: int free_sga(dmtr_sgarray_t* sga, bool is_in);

    public: virtual void client_send_put(int req_id, StringPointer key, StringPointer value, dmtr_sgarray_t &sga, void *context) = 0;
    public: virtual void client_send_get(int req_id, StringPointer key, dmtr_sgarray_t &sga, void *context) = 0;
    public: virtual int client_handle_response(dmtr_sgarray_t &sga) = 0;
    public: virtual string client_check_response(dmtr_sgarray_t &sga) = 0;

    public: virtual int server_handle_request(dmtr_sgarray_t &in_sga, dmtr_sgarray_t &out_sga, bool* free_in, bool* free_out, void *context) = 0;
};

int32_t encode_enum(simplekv::request req);

simplekv::request decode_enum(int32_t val);

int initialize_map(string load_file, unordered_map<string, string> &my_map);


#endif

