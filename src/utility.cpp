
#include "utility.h"

u8 get_bit(u8 const* data, size_t bit_index) {
    size_t byte_index = bit_index / 8;
    size_t shift = 7 - (bit_index % 8);
    return (data[byte_index] >> shift) & 1;
}

void set_bit(u8* data, size_t bit_index, u8 bit_value) {
    size_t byte_index = bit_index / 8;
    size_t shift = bit_index % 8;
    if (bit_value) {
        data[byte_index] |= (0x80 >> shift);
    } else {
        data[byte_index] &= ~(0x80 >> shift);
    }
}

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
