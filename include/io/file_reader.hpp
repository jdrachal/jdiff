//
// Created by jdrachal on 28.06.2022.

#ifndef ROLLING_HASH_CRAWLER_HPP
#define ROLLING_HASH_CRAWLER_HPP

#include <iostream>
#include <vector>
#include <list>
#include <fstream>
#include <filesystem>
#include "rhash.hpp"

namespace io {

    class FileReader {
    public:
        FileReader();
        explicit FileReader(const std::string &file_path, uint16_t block_size);
        explicit FileReader(const std::string &file_path);
        virtual ~FileReader();

        virtual bool rollByte();
        virtual std::vector<unsigned char> getNextChunk();
        std::vector<unsigned char> getCurrentFrame();
        char getLatestByte();
        char getRolledOutByte() const;

        std::vector<uint8_t> getBuffer();
        const std::string & file_path() const { return file_path_; }
        const uint16_t & max_frame_size() const { return max_frame_size_; }

        static inline bool doesFileExist(const std::string &file_path) {
            std::ifstream is(file_path);
            return is.good();
        }

        static uint16_t calculateBlockSize(uintmax_t file_size){
            uint16_t block_size = s_max_block_size;

            while ((file_size / s_min_block_count) < block_size) {
                block_size /= 2;
            }

            return block_size;
        }

    protected:
        std::list<char> frame_;
        char rolled_out_;
        uint16_t max_frame_size_;

    private:
        static constexpr inline uint8_t s_min_block_count = 2;
        static constexpr inline uint16_t s_max_block_size = 1024*4;

        std::string file_path_;
        std::ifstream is_;
    };
}

#endif //ROLLING_HASH_CRAWLER_HPP
