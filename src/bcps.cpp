#include <gtest/gtest.h>

#include "utility.h"
#include "image.h"

#include <vector>
#include <cstdlib>
#include <algorithm>

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

std::vector<u8> planify(std::vector<u8> const& data) {
    std::vector<u8> planed_data(data.size());

    size_t bytes_per_plane = data.size() / 32;

    for (size_t bitplane_index = 0; bitplane_index < 32; bitplane_index++) {
        u8* planed_data_ptr = planed_data.data() + bitplane_index * bytes_per_plane;
        extract_bitplane(data.data(), bitplane_index, planed_data_ptr, bytes_per_plane);
    }

    return planed_data;
}

std::vector<u8> de_planify(std::vector<u8> const& planed_data) {
    std::vector<u8> data(planed_data.size());

    size_t bytes_per_plane = data.size() / 32;

    for (size_t bitplane_index = 0; bitplane_index < 32; bitplane_index++) {
        u8 const* planed_data_ptr = planed_data.data() + bitplane_index * bytes_per_plane;
        insert_bitplane(data.data(), bitplane_index, planed_data_ptr, bytes_per_plane);;
    }

    return data;
}
