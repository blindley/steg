#include "utility.h"
#include "image.h"

#include <vector>
#include <cstdlib>
#include <algorithm>
#include <format>
#include <string>
#include <span>
#include <cassert>
#include <bit>

#include "bpcs.h"

size_t const bitplane_priority[] = {
    7, 15, 23, 31, 6, 14, 22, 30,
    5, 13, 21, 29, 4, 12, 20, 28,
    3, 11, 19, 27, 2, 10, 18, 26,
    1, 9, 17, 25, 0, 8, 16, 24
};

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
    size_t usable_width = chunks_in_width * 8;
    size_t usable_height = chunks_in_height * 8;

    binary_to_gray_code_inplace(img.pixel_data);

    DataChunkArray planed_data;
    planed_data.chunks.resize(chunks_in_width * chunks_in_height * 32);
    u8* planed_data_ptr = (u8*)planed_data.chunks.data();
    size_t pd_bit_index = 0;

    u8 const* pixel_data_ptr = img.pixel_data.data();

    for (size_t bp = 0; bp < 32; bp++) {
        size_t bitplane_index = bitplane_priority[bp];

        for (size_t chunk_y_index = 0; chunk_y_index < chunks_in_height; chunk_y_index++) {
            for (size_t chunk_x_index = 0; chunk_x_index < chunks_in_width; chunk_x_index++) {
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
    }

    return planed_data;
}

void de_chunkify(Image& img, DataChunkArray const& planed_data) {
    size_t chunks_in_width = img.width / 8;
    size_t chunks_in_height = img.height / 8;
    size_t usable_width = chunks_in_width * 8;
    size_t usable_height = chunks_in_height * 8;

    u8 const* planed_data_ptr = (u8 const*)planed_data.chunks.data();
    size_t pd_bit_index = 0;

    u8* pixel_data_ptr = img.pixel_data.data();

    for (size_t bp = 0; bp < 32; bp++) {
        size_t bitplane_index = bitplane_priority[bp];

        for (size_t chunk_y_index = 0; chunk_y_index < chunks_in_height; chunk_y_index++) {
            for (size_t chunk_x_index = 0; chunk_x_index < chunks_in_width; chunk_x_index++) {
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
    }

    gray_code_to_binary_inplace(img.pixel_data);
}

size_t count_bit_transitions(u8 byte) {
    u8 x = (byte << 1) | (byte & 1);
    u8 diff = x ^ byte;
    return std::popcount(diff);
}

size_t count_bit_differences(u8 a, u8 b) {
    u8 diff = a ^ b;
    return std::popcount(diff);
}

float measure_plane_chunk_complexity(DataChunk const& chunk) {
    size_t count = 0;
    for (size_t i = 0; i < 8; i++) {
        count += count_bit_transitions(chunk.bytes[i]);
    }

    for (size_t i = 0; i < 7; i++) {
        count += count_bit_differences(chunk.bytes[i], chunk.bytes[i+1]);
    }

    size_t const max_bit_transitions = 2 * 7 * 8;
    return (float)count / (float)max_bit_transitions;
}

void hide_raw_bytes(float threshold, DataChunkArray& cover, DataChunkArray const& formatted_message) {
    auto message_chunk_iter = formatted_message.begin();
    for (auto& cover_chunk : cover) {
        if (message_chunk_iter == formatted_message.end())
            break;
        auto complexity = measure_plane_chunk_complexity(cover_chunk);
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

DataChunkArray unhide_raw_bytes(float threshold, DataChunkArray const& cover) {
    DataChunkArray formatted_message;
    for (auto& cover_chunk : cover) {
        auto complexity = measure_plane_chunk_complexity(cover_chunk);
        if (complexity >= threshold) {
            formatted_message.chunks.push_back(cover_chunk);
        }
    }

    return formatted_message;
}

void conjugate(DataChunk& chunk) {
    for (size_t i = 0; i < 4; i++) {
        chunk.bytes[i * 2] ^= 0xAA;
        chunk.bytes[i * 2 + 1] ^= 0x55;
    }
}

void conjugate_data(float threshold, DataChunkArray& formatted_data) {
    for (auto& chunk : formatted_data) {
        auto complexity = measure_plane_chunk_complexity(chunk);
        if (complexity < threshold) {
            conjugate(chunk);
        }
    }
}

void de_conjugate_data(DataChunkArray& formatted_data) {
    for (auto& chunk : formatted_data) {
        if ((chunk.bytes[0] & 0x80) == 0x80) {
            conjugate(chunk);
        }
    }
}

DataChunkArray format_message_for_hiding(float threshold, std::vector<u8> const& message) {
    size_t formatted_size = (message.size() * 8 + 32 + 62) / 63 * 8;
    DataChunkArray formatted_data;
    formatted_data.chunks.resize(formatted_size / 8);

    size_t out_bit_index = 0;
    size_t in_bit_index = 0;

    u32 message_size = message.size();
    u8* out_ptr = formatted_data.bytes_begin();

    set_bit(out_ptr, out_bit_index++, 0);
    for (size_t i = 0; i < 32; i++) {
        u8 bit_value = (message_size >> (31 - i)) & 1;
        set_bit(out_ptr, out_bit_index++, bit_value);
    }

    for (size_t i = 0; i < message_size * 8; i++) {
        if (out_bit_index % 64 == 0) {
            set_bit(out_ptr, out_bit_index++, 0);
        }

        auto bit_value = get_bit(message.data(), in_bit_index++);
        set_bit(out_ptr, out_bit_index++, bit_value);
    }

    conjugate_data(threshold, formatted_data);

    return formatted_data;
}

// here we take formatted_data by value because we need a non-const copy of it
std::vector<u8> unformat_message(DataChunkArray formatted_data) {
    de_conjugate_data(formatted_data);

    size_t out_bit_index = 0;
    size_t in_bit_index = 1;

    u32 message_size = 0;
    u8 const* formatted_data_ptr = formatted_data.bytes_begin();

    for (size_t i = 0; i < 32; i++) {
        message_size <<= 1;
        auto bit_value = get_bit(formatted_data_ptr, in_bit_index++);
        message_size |= bit_value;
    }

    std::vector<u8> message;
    message.resize(message_size);

    while (out_bit_index < message_size * 8) {
        if (in_bit_index % 64 == 0) {
            in_bit_index++;
        } else {
            auto bit_value = get_bit(formatted_data_ptr, in_bit_index++);
            set_bit(message.data(), out_bit_index++, bit_value);
        }
    }

    return message;
}

void bpcs_hide_message(float threshold, Image& img, std::vector<u8> const& message) {
    auto formatted_data = format_message_for_hiding(threshold, message);
    auto planed_data = chunkify(img);
    hide_raw_bytes(threshold, planed_data, formatted_data);
    de_chunkify(img, planed_data);
}

std::vector<u8> bpcs_unhide_message(float threshold, Image& img) {
    auto planed_data = chunkify(img);
    auto formatted_data = unhide_raw_bytes(threshold, planed_data);
    auto message = unformat_message(formatted_data);
    return message;
}

size_t measure_capacity(float threshold, Image& img) {
    auto cover = chunkify(img);

    size_t complex_chunk_count = 0;
    for (auto& cover_chunk : cover) {
        auto complexity = measure_plane_chunk_complexity(cover_chunk);
        if (complexity >= threshold) {
            complex_chunk_count++;
        }
    }

    size_t bit_count = complex_chunk_count * 63 - 32;
    return bit_count / 8;
}
