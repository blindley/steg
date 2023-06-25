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
