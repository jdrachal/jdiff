//
// Created by jdrachal on 28.06.2022.

#ifndef ROLLING_HASH_DIFF_HPP
#define ROLLING_HASH_DIFF_HPP

#include <iostream>
#include <unordered_map>
#include <vector>
#include <map>
#include <fstream>
#include <openssl/sha.h>
#include "file_reader.hpp"
#include "file_writer.hpp"

namespace diff{

    typedef unsigned char ubyte_t;
    typedef std::vector<ubyte_t> sha256_t;

    template<typename T>
    void generic_push_back(std::vector<ubyte_t> &buff, const T& t) {
        for(int i = sizeof(T) - 1; i >= 0; i--) {
            buff.push_back(t >> (i*8));
        }
    }


    template<typename T>
    void generic_push_front(std::vector<ubyte_t> &buff, const T& t) {
        for(int i = 0; i < sizeof(T); i++) {
            buff.insert(buff.begin(), t >> (i*8));
        }
    }

    template<typename T>
    void generic_read_var_offset(std::vector<ubyte_t> &buff, size_t offset, T& t) {
        for(int i = 0; i < sizeof(T); i++) {
            t = t | (static_cast<T>(buff[offset+i]) << ((sizeof(T)-i-1)*8));
        }
    }

    struct Delta {
        sha256_t sha;
        uint16_t block_size;
        std::map<uint32_t, std::vector<ubyte_t>> inserts;
        std::map<uint32_t, uint32_t> deletes;

        Delta() : block_size(0) {}

        void clear();
        std::vector<ubyte_t> serialize();
        void deserialize(std::vector<ubyte_t> buff);
    };

    struct Signature {
        std::vector<ubyte_t> sha;
        uint16_t block_size;
        std::unordered_map<uint32_t, std::unordered_map<uint64_t, uint32_t>> signatures;

        Signature() : block_size(0){}

        void addSignature(uint32_t rhash, uint64_t xxhash, uint32_t index);
        uint64_t countSignatures();
        std::vector<ubyte_t> serialize();
        void deserialize(std::vector<ubyte_t> buff);
        void clear();
    };

    class Diff {
    public:
        Diff() = default;
        Diff(const Diff &diff) = delete;

        void prepareSignatures(io::FileReader &reader, bool sha=false);
        void prepareDelta(const Signature &s, io::FileReader &reader, bool sha=false);

        void generateSignatureFile(const std::string &file_path);
        void getSignatureFromFile(const std::string &file_path);

        void generateDeltaFile(const std::string &file_path);
        void getDeltaFromFile(const std::string &file_path);

        static void patchFile(const Delta &delta, io::FileReader &r_base_file,
                              io::FileWriter &w_new_file, bool checkSha=false, const sha256_t& checksum={});
        static sha256_t calculateFileSha256(const std::string &file_path);
        static bool compareSha(const sha256_t &hash1, const sha256_t &hash2);

        const Signature & signature() const { return signature_; }
        const Delta & delta() const { return delta_; }


    private:
        static constexpr std::size_t s_block_size_4k = (1 << 12);

        Signature signature_;
        Delta delta_;
    };
}

#endif //ROLLING_HASH_DIFF_HPP
