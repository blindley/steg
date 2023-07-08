

#include <vector>
#include <cstdlib>
#include <algorithm>
#include <format>
#include <string>
#include <span>
#include <cassert>
#include <bit>
#include <random>

#include "bpcs.h"
#include "message.h"
#include "utility.h"
#include "image.h"
#include "datachunk.h"

size_t const bitplane_priority[] = {
    7, 15, 23, 31, 6, 14, 22, 30,
    5, 13, 21, 29, 4, 12, 20, 28,
    3, 11, 19, 27, 2, 10, 18, 26,
    1, 9, 17, 25, 0, 8, 16, 24
};

u8 binary_to_gray_code(u8 binary) {
    return (binary >> 1) ^ binary;
}

u8 gray_code_to_binary(u8 gray_code) {
    u8 temp = gray_code ^ (gray_code >> 4);
    temp ^= (temp >> 2);
    temp ^= (temp >> 1);
    return temp;
}

void binary_to_gray_code_inplace(std::vector<u8>& vec) {
    std::transform(vec.begin(), vec.end(), vec.begin(), binary_to_gray_code);
}

void gray_code_to_binary_inplace(std::vector<u8>& vec) {
    std::transform(vec.begin(), vec.end(), vec.begin(), gray_code_to_binary);
}

DataChunkArray chunkify(Image& img) {
    size_t chunks_in_width = img.width / 8;
    size_t chunks_in_height = img.height / 8;
    size_t chunks_per_bitplane = chunks_in_width * chunks_in_height;

    binary_to_gray_code_inplace(img.pixel_data);

    DataChunkArray planed_data;
    planed_data.chunks.resize(chunks_in_width * chunks_in_height * 32);
    u8* planed_data_ptr = (u8*)planed_data.chunks.data();
    size_t pd_bit_index = 0;

    u8 const* pixel_data_ptr = img.pixel_data.data();

    std::mt19937_64 gen(img.width * 1000003 + img.height);
    std::vector<size_t> chunk_priority;
    chunk_priority.resize(chunks_per_bitplane);
    for (size_t i = 0; i < chunks_per_bitplane; i++)
        chunk_priority[i] = i;
    std::shuffle(chunk_priority.begin(), chunk_priority.end(), gen);

    for (size_t bp = 0; bp < 32; bp++) {
        size_t bitplane_index = bitplane_priority[bp];

        for (size_t _chunk_index = 0; _chunk_index < chunks_per_bitplane; _chunk_index++) {
            size_t chunk_index = chunk_priority[_chunk_index];
            size_t chunk_x_index = chunk_index % chunks_in_width;
            size_t chunk_y_index = chunk_index / chunks_in_width;

            for (size_t row_in_chunk = 0; row_in_chunk < 8; row_in_chunk++) {
                for (size_t col_in_chunk = 0; col_in_chunk < 8; col_in_chunk++) {
                    size_t pixel_x = chunk_x_index * 8 + col_in_chunk;
                    size_t pixel_y = chunk_y_index * 8 + row_in_chunk;
                    size_t pixel_index = pixel_y * img.width + pixel_x;
                    size_t byte_index = pixel_index * 4;
                    size_t bit_index = byte_index * 8 + bitplane_index;
                    auto bit_value = get_bit(pixel_data_ptr, bit_index);
                    set_bit(planed_data_ptr, pd_bit_index++, bit_value);
                }
            }
        }
    }

    return planed_data;
}

void de_chunkify(Image& img, DataChunkArray const& planed_data) {
    size_t chunks_in_width = img.width / 8;
    size_t chunks_in_height = img.height / 8;
    size_t chunks_per_bitplane = chunks_in_width * chunks_in_height;

    u8 const* planed_data_ptr = (u8 const*)planed_data.chunks.data();
    size_t pd_bit_index = 0;

    u8* pixel_data_ptr = img.pixel_data.data();

    std::mt19937_64 gen(img.width * 1000003 + img.height);
    std::vector<size_t> chunk_priority;
    chunk_priority.resize(chunks_per_bitplane);
    for (size_t i = 0; i < chunks_per_bitplane; i++)
        chunk_priority[i] = i;
    std::shuffle(chunk_priority.begin(), chunk_priority.end(), gen);

    for (size_t bp = 0; bp < 32; bp++) {
        size_t bitplane_index = bitplane_priority[bp];

        for (size_t _chunk_index = 0; _chunk_index < chunks_per_bitplane; _chunk_index++) {
            size_t chunk_index = chunk_priority[_chunk_index];
            size_t chunk_x_index = chunk_index % chunks_in_width;
            size_t chunk_y_index = chunk_index / chunks_in_width;

            for (size_t row_in_chunk = 0; row_in_chunk < 8; row_in_chunk++) {
                for (size_t col_in_chunk = 0; col_in_chunk < 8; col_in_chunk++) {
                    size_t pixel_x = chunk_x_index * 8 + col_in_chunk;
                    size_t pixel_y = chunk_y_index * 8 + row_in_chunk;
                    size_t pixel_index = pixel_y * img.width + pixel_x;
                    size_t byte_index = pixel_index * 4;
                    size_t bit_index = byte_index * 8 + bitplane_index;
                    auto bit_value = get_bit(planed_data_ptr, pd_bit_index++);
                    set_bit(pixel_data_ptr, bit_index, bit_value);
                }
            }
        }
    }

    gray_code_to_binary_inplace(img.pixel_data);
}

void hide_formatted_message(float threshold, DataChunkArray& cover, DataChunkArray const& formatted_message) {
    auto message_chunk_iter = formatted_message.begin();
    for (auto& cover_chunk : cover) {
        if (message_chunk_iter == formatted_message.end())
            break;
        auto complexity = cover_chunk.measure_complexity();
        if (complexity >= threshold) {
            cover_chunk = *message_chunk_iter;
            ++message_chunk_iter;
        }
    }

    if (message_chunk_iter != formatted_message.end()) {
        auto chunks_hidden = message_chunk_iter - formatted_message.begin();
        auto bits_hidden = chunks_hidden * 63 - 32;
        auto bytes_hidden = bits_hidden / 8;
        throw std::format("max hiding capacity ({} bytes) exceeded", bytes_hidden);
    }
}

DataChunkArray unhide_formatted_message(float threshold, DataChunkArray const& cover) {
    DataChunkArray formatted_message;
    for (auto& cover_chunk : cover) {
        auto complexity = cover_chunk.measure_complexity();
        if (complexity >= threshold) {
            formatted_message.chunks.push_back(cover_chunk);
        }
    }

    return formatted_message;
}

void bpcs_hide_message(float threshold, Image& img, std::vector<u8> const& message) {
    auto formatted_data = format_message(message);
    auto planed_data = chunkify(img);
    hide_formatted_message(threshold, planed_data, formatted_data);
    de_chunkify(img, planed_data);
}

std::vector<u8> bpcs_unhide_message(float threshold, Image& img) {
    auto planed_data = chunkify(img);
    auto formatted_data = unhide_formatted_message(threshold, planed_data);
    auto message = unformat_message(formatted_data);
    return message;
}

Measurements measure_capacity(float threshold, Image& img) {
    Measurements meas = {};
    auto cover = chunkify(img);

    size_t chunks_per_bitplane = cover.chunks.size() / 32;
    size_t chunk_index = 0;
    size_t complex_chunk_count = 0;
    for (size_t _bitplane_index = 0; _bitplane_index < 32; _bitplane_index++) {
        size_t bitplane_index = bitplane_priority[_bitplane_index];
        for (size_t _chunk_index = 0; _chunk_index < chunks_per_bitplane; _chunk_index++) {
            auto complexity = cover.chunks[chunk_index].measure_complexity();
            if (complexity >= threshold) {
                meas.available_chunks_per_bitplane[bitplane_index]++;
                complex_chunk_count++;
            }
            chunk_index++;
        }
    }

    meas.total_message_capacity = calculate_message_capacity_from_chunk_count(complex_chunk_count);

    return meas;
}

#ifdef STEG_TEST

#include <gtest/gtest.h>

size_t num_bits_diff(u8 a, u8 b) {
    return std::popcount((u8)(a ^ b));
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


#endif // STEG_TEST
