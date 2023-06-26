#include "utility.h"
#include "image.h"

#include <vector>
#include <cstdlib>
#include <algorithm>
#include <format>
#include <string>

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

std::vector<u8> chunk_and_planify(Image const& img) {
    auto chunked_data = chunkify(img);
    binary_to_gray_code_inplace(chunked_data);
    return planify(chunked_data);
}

void de_chunk_and_planify(Image& img, std::vector<u8> const& planed_data) {
    auto chunked_data = de_planify(planed_data);
    gray_code_to_binary_inplace(chunked_data);
    de_chunkify(img, chunked_data);
}

size_t count_bit_transitions(u8 byte) {
    size_t count = 0;
    u8 prev_bit = byte & 1;
    for (size_t i = 0; i < 7; i++) {
        byte >>= 1;
        u8 this_bit = byte & 1;
        count += (this_bit ^ prev_bit);
        prev_bit = this_bit;
    }
    return count;
}

u8 extract_vertical_byte(u8 const* planed_data_chunk, size_t bit_index) {
    u8 byte = 0;
    size_t shift = 7 - bit_index;
    for (size_t i = 0; i < 8; i++) {
        byte <<= 1;
        byte |= ((planed_data_chunk[i] >> shift) & 1);
    }
    return byte;
}

size_t count_vertical_bit_transitions(u8 const* planed_data_chunk, size_t bit_index) {
    u8 byte = extract_vertical_byte(planed_data_chunk, bit_index);
    return count_bit_transitions(byte);
}

size_t count_horizontal_bit_transitions(u8 const* planed_data_chunk) {
    size_t count = 0;
    for (size_t i = 0; i < 8; i++) {
        count += count_bit_transitions(planed_data_chunk[i]);
    }
    return count;
}

size_t count_vertical_bit_transitions(u8 const* planed_data_chunk) {
    size_t count = 0;
    for (size_t i = 0; i < 8; i++) {
        count += count_vertical_bit_transitions(planed_data_chunk, i);
    }
    return count;
}

float measure_plane_chunk_complexity(u8 const* planed_data_chunk) {
    size_t total_bit_transitions =
        count_horizontal_bit_transitions(planed_data_chunk) +
        count_vertical_bit_transitions(planed_data_chunk);
    /* There are 7 possible transitions in a sequence of 8 bits. A plane chunk
        has 8 rows of 8 bits. So there are 7 * 8 possible horizontal transitions,
        and 7 * 8 possible vertical transitions
    */
    size_t const max_bit_transitions = 2 * 7 * 8;
    return (float)total_bit_transitions / (float)max_bit_transitions;
}

void hide_raw_bytes(float threshold, std::vector<u8>& planed_data, std::vector<u8> const& raw_bytes) {
    size_t rb_offset = 0;
    for (size_t pd_offset = 0; pd_offset < planed_data.size(); pd_offset += 8) {
        if (rb_offset >= raw_bytes.size())
            break;
        auto pd_ptr = planed_data.data() + pd_offset;
        auto complexity = measure_plane_chunk_complexity(pd_ptr);
        if (complexity >= threshold) {
            auto rb_ptr = raw_bytes.data() + rb_offset;
            std::memcpy(pd_ptr, rb_ptr, 8);
            rb_offset += 8;
        }
    }

    if (rb_offset < raw_bytes.size()) {
        throw std::format("max hiding capacity ({} bytes) exceeded", rb_offset);
    }
}

std::vector<u8> unhide_raw_bytes(float threshold, std::vector<u8> const& planed_data) {
    std::vector<u8> raw_bytes;
    for (size_t pd_offset = 0; pd_offset < planed_data.size(); pd_offset += 8) {
        auto pd_ptr = planed_data.data() + pd_offset;
        auto complexity = measure_plane_chunk_complexity(pd_ptr);
        if (complexity >= threshold) {
            raw_bytes.insert(raw_bytes.end(), pd_ptr, pd_ptr + 8);
        }
    }

    return raw_bytes;
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

void invert_plane_chunk(u8* data) {
    for (size_t i = 0; i < 4; i++) {
        data[i * 2] ^= 0xAA;
        data[i * 2 + 1] ^= 0x55;
    }
}

void make_all_chunks_complex(float threshold, std::vector<u8>& formatted_data) {
    for (size_t i = 0; i < formatted_data.size(); i += 8) {
        auto fdata_ptr = formatted_data.data() + i;
        auto complexity = measure_plane_chunk_complexity(fdata_ptr);
        if (complexity < threshold) {
            invert_plane_chunk(fdata_ptr);
        }
    }
}

void restore_complexified_chunks(std::vector<u8>& formatted_data) {
    for (size_t i = 0; i < formatted_data.size(); i += 8) {
        auto fdata_ptr = formatted_data.data() + i;
        if ((fdata_ptr[0] & 0x80) == 0x80) {
            invert_plane_chunk(fdata_ptr);
        }
    }
}

std::vector<u8> format_message_for_hiding(float threshold, std::vector<u8> const& message) {
    size_t formatted_size = (message.size() * 8 + 32 + 62) / 63 * 8;
    std::vector<u8> formatted_data(formatted_size);

    size_t out_bit_index = 0;
    size_t in_bit_index = 0;

    u32 message_size = message.size();

    set_bit(formatted_data.data(), out_bit_index++, 0);
    for (size_t i = 0; i < 32; i++) {
        u8 bit_value = (message_size >> (31 - i)) & 1;
        set_bit(formatted_data.data(), out_bit_index++, bit_value);
    }

    for (size_t i = 0; i < message_size * 8; i++) {
        if (out_bit_index % 64 == 0) {
            set_bit(formatted_data.data(), out_bit_index++, 0);
        }

        auto bit_value = get_bit(message.data(), in_bit_index++);
        set_bit(formatted_data.data(), out_bit_index++, bit_value);
    }

    make_all_chunks_complex(threshold, formatted_data);

    return formatted_data;
}

// here we take formatted_data by value because we need a non-const copy of it
std::vector<u8> unformat_message(std::vector<u8> formatted_data) {
    restore_complexified_chunks(formatted_data);

    size_t out_bit_index = 0;
    size_t in_bit_index = 1;

    u32 message_size = 0;

    for (size_t i = 0; i < 32; i++) {
        message_size <<= 1;
        auto bit_value = get_bit(formatted_data.data(), in_bit_index++);
        message_size |= bit_value;
    }

    std::vector<u8> message;
    message.resize(message_size);

    while (out_bit_index < message_size * 8) {
        if (in_bit_index % 64 == 0) {
            in_bit_index++;
        } else {
            auto bit_value = get_bit(formatted_data.data(), in_bit_index++);
            set_bit(message.data(), out_bit_index++, bit_value);
        }
    }

    return message;
}

void hide_message(float threshold, Image& img, std::vector<u8> const& message) {
    auto formatted_data = format_message_for_hiding(threshold, message);
    auto planed_data = chunk_and_planify(img);
    hide_raw_bytes(threshold, planed_data, formatted_data);
    de_chunk_and_planify(img, planed_data);
}

std::vector<u8> unhide_message(float threshold, Image const& img) {
    auto planed_data = chunk_and_planify(img);
    auto formatted_data = unhide_raw_bytes(threshold, planed_data);
    auto message = unformat_message(formatted_data);
    return message;
}
