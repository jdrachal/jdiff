#include "file_writer.hpp"
#include <fstream>

namespace io {

    FileWriter::FileWriter(const std::string &file_path) {
        os_ = std::ofstream(file_path, std::ios_base::binary);
    }

    FileWriter::~FileWriter() {
        os_.close();
    }

    void FileWriter::append(const std::vector<unsigned char> &data) {
        os_.write((char*)&data[0], data.size());
    }
}