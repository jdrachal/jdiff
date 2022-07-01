//
// Created by jdrachal on 29.06.2022.
// Rhash implementation is based on TR-CS-96-05 "The rsync algorithm" publication
// https://www.andrew.cmu.edu/course/15-749/READINGS/required/cas/tridgell96.pdf
//

#ifndef JDIFF_RHASH_HPP
#define JDIFF_RHASH_HPP

#include <cstdint>
#include <vector>

class RHash {
public:

    RHash() = delete;

    explicit RHash(uint16_t block_size) {
        block_size_ = block_size;
        a_ = 0;
        sum_ = 0;
    }

    void roll(char oldest_byte, char next_byte) {
        if (counter_ < block_size_) {
            counter_++;
        } else {
            a_ -= oldest_byte;
            sum_ -= (block_size_*oldest_byte);
        }
        a_ += (uint16_t)next_byte;
        sum_ += a_;
        moduloM(a_);
        moduloM(sum_);
    }

    uint32_t hash() const {
        return a_ | (sum_ << 16);
    }

    static uint32_t hashBuffer(std::vector<unsigned char> buffer) {
        uint16_t a = 0;
        uint16_t sum = 0;

        for (const auto& byte : buffer){
            a += (uint16_t)byte;
            sum += a;
            moduloM(a);
            moduloM(sum);
        }

        return a | (sum << 16);
    }

    static inline void moduloM(uint16_t &val){
        val %= M;
    }

    static constexpr inline uint16_t M = 65521;

private:
    uint16_t a_;
    uint16_t sum_;

    uint16_t block_size_;
    uint16_t counter_;
};

#endif //JDIFF_RHASH_HPP
