#include "file_reader.hpp"

#include <iostream>

namespace io {
    FileReader::FileReader() {
        max_frame_size_ = 0;
        rolled_out_ = 0;
    }

    FileReader::FileReader(const std::string &file_path, uint16_t block_size) {
        is_ = std::ifstream(file_path, std::ios_base::binary);
        if(!is_){
            throw std::invalid_argument(std::string("File " + file_path + " doesn't exist or broken!"));
        }
        if(block_size > 0) {
            max_frame_size_ = block_size;
        } else {
            max_frame_size_ = calculateBlockSize(std::filesystem::file_size(file_path));
        }
        rolled_out_ = 0;
    }

    FileReader::FileReader(const std::string &file_path) {
        is_ = std::ifstream(file_path, std::ios_base::binary);
        if(!is_){
            throw std::invalid_argument(std::string("File " + file_path + " doesn't exist or broken!"));
        }

        rolled_out_ = 0;
        max_frame_size_ = calculateBlockSize(std::filesystem::file_size(file_path));
    }

    FileReader::~FileReader() {
        is_.close();
    }

    bool FileReader::rollByte() {
        if (frame_.size() == max_frame_size_) {
            rolled_out_ = frame_.front();
            frame_.pop_front();
        }

        char c;
        if (is_.get(c)) {
            frame_.push_back(c);
            return true;
        } else {
            return false;
        }
    }

    std::vector<unsigned char> FileReader::getNextChunk() {
        std::vector<unsigned char> chunk(max_frame_size_);
        is_.read((char*)chunk.data(), static_cast<long>(chunk.size()));

        auto data_count = is_.gcount();
        if (max_frame_size_ > data_count) {
            chunk.resize(data_count);
            chunk.shrink_to_fit();
        }

        return chunk;
    }

    std::vector<unsigned char> FileReader::getCurrentFrame() {
        return std::vector<unsigned char>{frame_.begin(), frame_.end()};
    }

    char FileReader::getLatestByte() {
        return frame_.back();
    }

    char FileReader::getRolledOutByte() const {
        return rolled_out_;
    }

    std::vector<uint8_t> FileReader::getBuffer(){
        return std::vector<uint8_t>{(std::istreambuf_iterator<char>(is_)),
                                 std::istreambuf_iterator<char>()};
    }
}