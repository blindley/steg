#include <gtest/gtest.h>

#include "../src/utility.h"
#include "../src/image.h"

#include "bcps_test.h"

size_t count_bits_set(u8 n) {
    size_t count = 0;
    while (n) {
        count += n & 1;
        n >>= 1;
    }
    return count;
}

size_t num_bits_diff(u8 a, u8 b) {
    return count_bits_set(a ^ b);
}

TEST(bcps, gray_code_conversions) {
    for (int i = 0; i <= 256; i++) {
        u8 a = i;
        u8 b = i + 1;
        u8 ga = binary_to_gray_code(a);
        u8 gb = binary_to_gray_code(b);
        size_t diff = num_bits_diff(ga, gb);
        ASSERT_EQ(diff, 1);
    }

    for (int i = 0; i < 256; i++) {
        u8 a = i;
        u8 ga = binary_to_gray_code(a);
        u8 ba = gray_code_to_binary(ga);
        ASSERT_EQ(a, ba);
    }
}

TEST(bcps, chunkify) {
    Image img;
    img.width = 9;
    img.height = 9;
    img.pixel_data.resize(9 * 9 * 4);

    std::vector<u8> chunked_data;
    for (int i = 0; i < 256; i++) {
        chunked_data.push_back(i);
    }

    de_chunkify(img, chunked_data);

    for (int y = 0; y < 9; y++) {
        size_t offset = y * 9 * 4;
        if (y < 8) {
            u8 first_value = y * 8 * 4;
            for (int i = 0; i < 8 * 4; i++) {
                ASSERT_EQ(img.pixel_data[offset + i], first_value + i);
            }

            offset += 8 * 4;

            for (int i = 0; i < 4; i++) {
                ASSERT_EQ(img.pixel_data[offset + i], 0);
            }
        } else { // y == 8
            for (int i = 0; i < 9 * 4; i++) {
                ASSERT_EQ(img.pixel_data[offset + i], 0);
            }
        }
    }

    auto rechunked_data = chunkify(img);

    ASSERT_EQ(chunked_data, rechunked_data);
}
