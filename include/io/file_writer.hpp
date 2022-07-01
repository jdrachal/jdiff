//
// Created by jdrachal on 28.06.2022.

#ifndef JDIFF_FILEMANAGER_HPP
#define JDIFF_FILEMANAGER_HPP

#include <iostream>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <iterator>

namespace io {

    class FileWriter {
    private:
        std::ofstream os_;

    public:
        FileWriter() = default;
        explicit FileWriter(const std::string &file_path);
        FileWriter(const FileWriter &fileManager) = delete;
        virtual ~FileWriter();

        virtual void append(const std::vector<unsigned char> &data);
    };
}

#endif //JDIFF_FILEMANAGER_HPP
