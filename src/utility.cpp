// Benjamin Lindley, Vanessa Martinez
//
// utility.cpp
//
// General purpose functions that don't really belong to any specific module.

#include <fstream>
#include <stdexcept>
#include <random>
#include <sstream>

#include "declarations.h"

// Treats an array of bytes as an array of bits, and retrieves a bit by its index
//
// Bit index 0 is the MSB of the first byte. Bit index 7 is the LSB of the first byte. Bit 8 is
// the MSB of the second byte. Bit 15 is the LSB of the second byte. etc...
u8 get_bit(u8 const* data, size_t bit_index) {
    size_t byte_index = bit_index / 8;
    size_t shift = 7 - (bit_index % 8);
    return (data[byte_index] >> shift) & 1;
}

// Treats an array of bytes as an array of bits, and sets a bit by its index
//
// <bit_value> is just treated as a boolean, either false (zero) or true (non-zero).
void set_bit(u8* data, size_t bit_index, u8 bit_value) {
    size_t byte_index = bit_index / 8;
    size_t shift = bit_index % 8;
    if (bit_value) {
        data[byte_index] |= (0x80 >> shift);
    } else {
        data[byte_index] &= ~(0x80 >> shift);
    }
}

// Gets the file extension of a filename (the png in some_image.png)
std::string get_file_extension(std::string const& filename) {
    size_t i = filename.size();
    while (i > 0) {
        --i;
        if (filename[i] == '.') {
            return filename.substr(i + 1);
        }

        if (filename[i] == '/' || filename[i] == '\\') {
            return "";
        }
    }

    return "";
}

// Saves a byte array to a file
void save_file(std::string const& filename, u8 const* data, size_t len) {
    std::ofstream ofstr(filename, std::ios::binary);
    if (!ofstr) {
        std::ostringstream oss;
        oss << "unable to open " << filename << " for writing";
        auto err = oss.str();
        throw std::runtime_error(err);
    }

    ofstr.write((char const*)data, len);

    if (!ofstr) {
        std::ostringstream oss;
        oss << "error writing to " << filename;
        auto err = oss.str();
        throw std::runtime_error(err);
    }
}

// Saves a vector of bytes to a file
void save_file(std::string const& filename, std::vector<u8> const& data) {
    save_file(filename, data.data(), data.size());
}

// Loads a file to a vector of bytes
std::vector<u8> load_file(std::string const& filename) {
    std::ifstream ifstr(filename, std::ios::binary);

    if (!ifstr) {
        std::ostringstream oss;
        oss << "unable to open " << filename;
        auto err = oss.str();
        throw std::runtime_error(err);
    }

    ifstr.seekg(0, std::ios::end);
    auto size = ifstr.tellg();
    ifstr.seekg(0, std::ios::beg);

    if (!ifstr) {
        std::ostringstream oss;
        oss << "error reading " << filename;
        auto err = oss.str();
        throw std::runtime_error(err);
    }

    std::vector<u8> data;
    data.resize(size);

    ifstr.read((char*)data.data(), size);

    if (!ifstr) {
        std::ostringstream oss;
        oss << "error reading " << filename;
        auto err = oss.str();
        throw std::runtime_error(err);
    }

    return data;
}

// Creates a vector of random bytes
std::vector<u8> random_bytes(size_t size) {
    std::mt19937_64 gen;
    std::vector<u8> v(size);
    for (auto& e : v) {
        e = (u8)gen();
    }
    return v;
}

#ifdef STEG_TEST

#include <gtest/gtest.h>
#include <array>

TEST(utility, set_bit) {
    using array3 = std::array<u8, 3>;

    array3 a = {};
    for (size_t i = 0; i < 24; i++) {
        ASSERT_EQ(get_bit(a.data(), i), 0);
    }

    a = { 0xFF, 0xFF, 0xFF };
    for (size_t i = 0; i < 24; i++) {
        ASSERT_EQ(get_bit(a.data(), i), 1);
    }

    for (size_t i = 0; i < 24; i++) {
        a = {0, 0, 0};
        set_bit(a.data(), i, 1);
        for (size_t j = 0; j < 24; j++) {
            if (i == j) {
                ASSERT_EQ(get_bit(a.data(), j), 1);
            } else {
                ASSERT_EQ(get_bit(a.data(), j), 0);
            }
        }
    }

    for (size_t i = 0; i < 24; i++) {
        a = { 0xFF, 0xFF, 0xFF };
        set_bit(a.data(), i, 0);
        for (size_t j = 0; j < 24; j++) {
            if (i == j) {
                ASSERT_EQ(get_bit(a.data(), j), 0);
            } else {
                ASSERT_EQ(get_bit(a.data(), j), 1);
            }
        }
    }
}

#endif // STEG_TEST
