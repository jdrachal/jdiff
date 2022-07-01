cmake_minimum_required(VERSION 3.16)
project(jdiff)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)

pkg_search_module(UUID REQUIRED uuid)
pkg_search_module(OPENSSL REQUIRED openssl)
pkg_search_module(CRYPTO REQUIRED libcrypto)

include_directories(
        ${PROJECT_SOURCE_DIR}/include/io
        ${PROJECT_SOURCE_DIR}/include/diff
        ${PROJECT_SOURCE_DIR}/include/hash
        ${PROJECT_SOURCE_DIR}/include/parser
        ${PROJECT_SOURCE_DIR}/include/test
        ${UUID_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIRS}
        ${CRYPTO_INCLUDE_DIRS})

add_library(filemanager STATIC src/file_reader.cpp src/diff.cpp src/file_writer.cpp)

add_executable(jdiff app/jdiff.cpp)
target_link_libraries(jdiff filemanager ${UUID_LIBRARIES} ${OPENSSL_LIBRARIES} ${CRYPTO_LIBRARIES})

add_executable(test src/test_diff.cpp)

Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v2.13.9 # or a later release
)

FetchContent_MakeAvailable(Catch2)

target_link_libraries(test Catch2::Catch2 filemanager ${UUID_LIBRARIES} ${OPENSSL_LIBRARIES} ${CRYPTO_LIBRARIES})