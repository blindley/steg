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

#include "bcps.h"

std::vector<u8> chunkify(Image const& img) {
    // taking advantage of integer division truncation
    size_t chunks_in_width = img.width / 8;
    size_t chunks_in_height = img.height / 8;
    size_t usable_width = chunks_in_width * 8;
    size_t usable_height = chunks_in_height * 8;

    std::vector<u8> chunked_data;
    chunked_data.reserve(usable_width * usable_height * 4);

    for (size_t chunk_index_y = 0; chunk_index_y < chunks_in_height; chunk_index_y++) {
        size_t pixel_index_y_start = chunk_index_y * 8;
        for (size_t chunk_index_x = 0; chunk_index_x < chunks_in_width; chunk_index_x++) {
            size_t pixel_index_x = chunk_index_x * 8;
            for (size_t pixel_index_y_offset = 0; pixel_index_y_offset < 8; pixel_index_y_offset++) {
                size_t pixel_index_y = pixel_index_y_start + pixel_index_y_offset;
                size_t pixel_data_offset = 4 * (pixel_index_y * img.width + pixel_index_x);

                for (size_t i = 0; i < 32; i++) {
                    chunked_data.push_back(img.pixel_data[pixel_data_offset + i]);
                }
            }
        }
    }

    return chunked_data;
}

void de_chunkify(Image& img, std::vector<u8> const& chunked_data) {
    // taking advantage of integer division truncation
    size_t chunks_in_width = img.width / 8;
    size_t chunks_in_height = img.height / 8;
    size_t usable_width = chunks_in_width * 8;
    size_t usable_height = chunks_in_height * 8;

    // assert(chunked_data.size == usable_width * usable_height * 4);

    size_t chunked_data_offset = 0;
    for (size_t chunk_index_y = 0; chunk_index_y < chunks_in_height; chunk_index_y++) {
        size_t pixel_index_y_start = chunk_index_y * 8;
        for (size_t chunk_index_x = 0; chunk_index_x < chunks_in_width; chunk_index_x++) {
            size_t pixel_index_x = chunk_index_x * 8;
            for (size_t pixel_index_y_offset = 0; pixel_index_y_offset < 8; pixel_index_y_offset++) {
                size_t pixel_index_y = pixel_index_y_start + pixel_index_y_offset;
                size_t pixel_data_offset = 4 * (pixel_index_y * img.width + pixel_index_x);

                for (size_t i = 0; i < 32; i++) {
                    img.pixel_data[pixel_data_offset + i] = chunked_data[chunked_data_offset];
                    chunked_data_offset++;
                }
            }
        }
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

u8 extract_bitplane_byte(u8 const* byte_ptr, size_t bitplane_index) {
    u8 extracted_byte = 0;
    byte_ptr += bitplane_index / 8;
    size_t shift = 7 - (bitplane_index % 8);
    for (size_t i = 0; i < 8; i++) {
        u8 bit_value = (*byte_ptr >> shift) & 1;
        extracted_byte = (extracted_byte << 1) | bit_value;
        byte_ptr += 4;
    }

    return extracted_byte;
}

void insert_bitplane_byte(u8* byte_ptr, size_t bitplane_index, u8 inserted_byte) {
    byte_ptr += bitplane_index / 8;
    size_t shift = bitplane_index % 8;
    for (size_t i = 0; i < 8; i++) {
        u8 bit_value = inserted_byte & (0x80 >> i);
        if (bit_value) {
            *byte_ptr = *byte_ptr | (0x80 >> shift);
        } else {
            *byte_ptr &= ~(0x80 >> shift);
        }
        byte_ptr += 4;
    }
}

void extract_bitplane(u8 const* data_ptr, size_t bitplane_index, u8* planed_data_ptr,
    size_t planed_data_byte_count)
{
    for (size_t i = 0; i < planed_data_byte_count; i++) {
        *planed_data_ptr = extract_bitplane_byte(data_ptr, bitplane_index);
        planed_data_ptr++;
        data_ptr += 32;
    }
}

void insert_bitplane(u8* data_ptr, size_t bitplane_index, u8 const* planed_data_ptr,
    size_t planed_data_byte_count)
{
    for (size_t i = 0; i < planed_data_byte_count; i++) {
        insert_bitplane_byte(data_ptr, bitplane_index, *planed_data_ptr);
        planed_data_ptr++;
        data_ptr += 32;
    }
}

size_t const bitplane_priority[] = {
    7, 15, 23, 31, 6, 14, 22, 30,
    5, 13, 21, 29, 4, 12, 20, 28,
    3, 11, 19, 27, 2, 10, 18, 26,
    1, 9, 17, 25, 0, 8, 16, 24
};

DataChunkArray planify(std::vector<u8> const& data) {
    DataChunkArray planed_data;
    planed_data.chunks.resize(data.size() / 8);

    size_t bytes_per_plane = data.size() / 32;
    auto pd_ptr = planed_data.bytes_begin();

    for (size_t i = 0; i < 32; i++) {
        auto bitplane_index = bitplane_priority[i];
        extract_bitplane(data.data(), bitplane_index, pd_ptr, bytes_per_plane);
        pd_ptr += bytes_per_plane;
    }

    return planed_data;
}

std::vector<u8> de_planify(DataChunkArray const& planed_data) {
    std::vector<u8> data(planed_data.chunks.size() * 8);

    size_t bytes_per_plane = data.size() / 32;
    auto pd_ptr = planed_data.bytes_begin();

    for (size_t i = 0; i < 32; i++) {
        auto bitplane_index = bitplane_priority[i];
        insert_bitplane(data.data(), bitplane_index, pd_ptr, bytes_per_plane);
        pd_ptr += bytes_per_plane;
    }

    return data;
}

DataChunkArray chunk_and_planify(Image const& img) {
    auto chunked_data = chunkify(img);
    binary_to_gray_code_inplace(chunked_data);
    return planify(chunked_data);
}

void de_chunk_and_planify(Image& img, DataChunkArray const& planed_data) {
    auto chunked_data = de_planify(planed_data);
    gray_code_to_binary_inplace(chunked_data);
    de_chunkify(img, chunked_data);
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

void bcps_hide_message(float threshold, Image& img, std::vector<u8> const& message) {
    auto formatted_data = format_message_for_hiding(threshold, message);
    auto planed_image = chunk_and_planify(img);
    hide_raw_bytes(threshold, planed_image, formatted_data);
    de_chunk_and_planify(img, planed_image);
}

std::vector<u8> bcps_unhide_message(float threshold, Image const& img) {
    auto planed_image = chunk_and_planify(img);
    auto formatted_data = unhide_raw_bytes(threshold, planed_image);
    auto message = unformat_message(formatted_data);
    return message;
}
