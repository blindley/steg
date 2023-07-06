
#include "utility.h"

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
