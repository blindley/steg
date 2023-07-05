#include <gtest/gtest.h>
#include <cstdlib>
#include <array>
#include <random>

#include "../src/utility.h"
#include "../src/image.h"
#include "../src/message.h"

#include "bpcs_test.h"

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

TEST(bpcs, gray_code_conversions) {
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

TEST(bpcs, measure_complexity) {
    DataChunk chunk = {};
    ASSERT_EQ(measure_complexity(chunk), 0.0f);

    std::memset(chunk.bytes, 0xFF, 8);
    ASSERT_EQ(measure_complexity(chunk), 0.0f);

    std::memset(chunk.bytes, 0xAA, 8);
    ASSERT_EQ(measure_complexity(chunk), 0.5f);

    std::memset(chunk.bytes, 0x55, 8);
    ASSERT_EQ(measure_complexity(chunk), 0.5f);

    std::memset(chunk.bytes, 0xCC, 8);
    ASSERT_EQ(measure_complexity(chunk), 24.0f/112.0f);

    std::memset(chunk.bytes, 0x33, 8);
    ASSERT_EQ(measure_complexity(chunk), 24.0f/112.0f);

    auto alt_fill = [&](u8 a, u8 b) {
        for (size_t i = 0; i < 4; i++) {
            size_t index = i * 2;
            chunk.bytes[index] = a;
            chunk.bytes[index + 1] = b;
        }
    };

    alt_fill(0x55, 0xAA);
    ASSERT_EQ(measure_complexity(chunk), 1.0f);

    alt_fill(0xAA, 0x55);
    ASSERT_EQ(measure_complexity(chunk), 1.0f);

    alt_fill(0, 0xFF);
    ASSERT_EQ(measure_complexity(chunk), 0.5f);

    alt_fill(0xFF, 0);
    ASSERT_EQ(measure_complexity(chunk), 0.5f);
}

TEST(bpcs, set_bit) {
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

void randomize_chunk(std::mt19937_64& rng, DataChunk& chunk) {
    for (size_t i = 0; i < 8; i++) {
        auto value = rng();
        std::memcpy(chunk.bytes, &value, 8);
    }
}

TEST(bpcs, conjugate_complexity) {
    // test if the complexity of a chunk plus the complexity of its conjugate == 1

    std::random_device rd;
    auto seed = ((u64)rd()) ^ ((u64)std::time(nullptr));
    std::mt19937_64 gen64(seed);

    DataChunk chunk;
    for (int i = 0; i < 2000; i++) {
        randomize_chunk(gen64, chunk);
        auto complexity = measure_complexity(chunk);
        conjugate(chunk);
        auto conj_complexity = measure_complexity(chunk);
        auto complexity_sum = complexity + conj_complexity;
        ASSERT_FLOAT_EQ(complexity_sum, 1.0f);
    }
}

TEST(bpcs, message_formatting) {
    std::vector<u8> message;

    for (size_t i = 0; i < 4099; i++) {
        message.push_back(std::rand() >> 7);
    }

    auto formatted_message = format_message(message);
    auto recovered_message = unformat_message(formatted_message);
    ASSERT_EQ(message, recovered_message);
}

TEST(bpcs, message_hiding) {
    std::vector<u8> message;
    for (size_t i = 0; i < 511; i++) {
        message.push_back(std::rand() >> 7);
    }

    Image img = {};
    img.width = 257;
    img.height = 135;
    img.pixel_data.resize(img.width * img.height * 4);
    for (size_t chunk_start_y = 0; chunk_start_y + 8 <= img.height; chunk_start_y += 8) {
        for (size_t chunk_start_x = 0; chunk_start_x + 8 <= img.width; chunk_start_x += 8) {
            float p = (float)rand() / (float)RAND_MAX;
            if (p < 0.5) {
                for (size_t y_off = 0; y_off < 8; y_off++) {
                    for (size_t x_off = 0; x_off < 8; x_off++) {
                        size_t pixel_data_offset = (chunk_start_y + y_off) * img.width * 4
                            + x_off * 4;
                        for (size_t i = 0; i < 4; i++) {
                            size_t offset = pixel_data_offset + i;
                            img.pixel_data[offset] = std::rand() >> 7;
                        }
                    }
                }
            }
        }
    }

    bpcs_hide_message(0.3, img, message);
    auto extracted_message = bpcs_unhide_message(0.3, img);

    ASSERT_EQ(message, extracted_message);
}
