#include <gtest/gtest.h>
#include <cstdlib>

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

    // TODO: add tests for images with more than one chunk
}

TEST(bcps, bitplane_bytes) {
    std::vector<u8> data(32);

    insert_bitplane_byte(data.data(), 31, 0x55);

    // 0x55 = 0b01010101
    std::vector<u8> expected_data = {
        0, 0, 0, 0,     0, 0, 0, 1,
        0, 0, 0, 0,     0, 0, 0, 1,
        0, 0, 0, 0,     0, 0, 0, 1,
        0, 0, 0, 0,     0, 0, 0, 1,
    };

    ASSERT_EQ(data, expected_data);

    insert_bitplane_byte(data.data(), 0, 0xAA);

    expected_data = {
        0x80, 0, 0, 0,     0, 0, 0, 1,
        0x80, 0, 0, 0,     0, 0, 0, 1,
        0x80, 0, 0, 0,     0, 0, 0, 1,
        0x80, 0, 0, 0,     0, 0, 0, 1,
    };

    ASSERT_EQ(data, expected_data);

    u8 bp_31 = extract_bitplane_byte(data.data(), 31);
    ASSERT_EQ(bp_31, 0x55);

    u8 bp_0 = extract_bitplane_byte(data.data(), 0);
    ASSERT_EQ(bp_0, 0xAA);

    for (int i = 0; i < 32; i++) {
        size_t bitplane_index = i;
        u8 value_to_insert = std::rand() >> 7;
        insert_bitplane_byte(data.data(), bitplane_index, value_to_insert);
        expected_data[bitplane_index] = value_to_insert;
    }

    for (int i = 0; i < 32; i++) {
        size_t bitplane_index = i;
        u8 value = extract_bitplane_byte(data.data(), bitplane_index);
        u8 expected_value = expected_data[bitplane_index];
        ASSERT_EQ(value, expected_value);
    }
}

TEST(bpcs, planify) {
    std::vector<u8> data;
    for (int i = 0; i < 4096; i++) {
        data.push_back(std::rand() >> 7);
    }

    auto planed_data = planify(data);

    ASSERT_NE(data, planed_data);

    auto de_planed_data = de_planify(planed_data);

    ASSERT_EQ(data, de_planed_data);
}

TEST(bcps, message_formatting) {
    std::vector<u8> message;
    for (size_t i = 0; i < 256; i++) {
        message.push_back(i);
    }

    auto formatted_message = format_message_for_hiding(0.0, message);
    ASSERT_EQ(formatted_message.size(), 272);
    for (size_t i = 0; i < 7; i++) {
        ASSERT_EQ(formatted_message[57 + i], 52 + i);
    }

    auto recovered_message = unformat_message(formatted_message);
    ASSERT_EQ(message, recovered_message);

    message.clear();

    for (size_t i = 0; i < 4099; i++) {
        message.push_back(std::rand() >> 7);
    }

    formatted_message = format_message_for_hiding(0.49, message);
    recovered_message = unformat_message(formatted_message);
    ASSERT_EQ(message, recovered_message);
}

TEST(bcps, message_hiding) {
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

    hide_message(0.3, img, message);
    auto extracted_message = unhide_message(0.3, img);

    ASSERT_EQ(message, extracted_message);
}