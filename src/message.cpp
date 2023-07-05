
#include "bpcs.h"
#include "utility.h"

u8 const SIGNATURE[] = { 0x2F, 0x64, 0xA9 };

void conjugate(DataChunk& chunk) {
    for (size_t i = 0; i < 4; i++) {
        chunk.bytes[i * 2] ^= 0xAA;
        chunk.bytes[i * 2 + 1] ^= 0x55;
    }
}

void conjugate_data(float threshold, DataChunkArray& formatted_data) {
    for (auto& chunk : formatted_data) {
        auto complexity = measure_complexity(chunk);
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

DataChunkArray format_message_v2(float threshold, std::vector<u8> const& message) {
    size_t formatted_size = message.size() + sizeof(SIGNATURE) + sizeof(u32);
    formatted_size = (formatted_size + 62) / 63 * 64;
    DataChunkArray formatted_data;
    formatted_data.chunks.resize(formatted_size / 8);

    size_t out_index = 0;
    size_t in_index = 0;

    u32 message_size = message.size();
    u8* out_ptr = formatted_data.bytes_begin();

    out_ptr[out_index++] = 0;
    for (size_t i = 0; i < sizeof(SIGNATURE); i++) {
        out_ptr[out_index++] = SIGNATURE[i];
    }

    for (size_t i = 0; i < sizeof(message_size); i++) {
        size_t shift = 24 - 8 * i;
        out_ptr[out_index++] = message_size >> shift;
    }

    for (size_t i = 0; i < message.size(); i++) {
        if (out_index % 64 == 0) {
            out_ptr[out_index++] = 0;
        }

        out_ptr[out_index++] = message.data()[in_index++];
    }

    u8 conj_map = 0;
    for (size_t i = 0; i < formatted_data.chunks.size(); i++) {
        size_t chunk_index = i + 1;
        if (chunk_index % 8 == 0) {
            chunk_index -= 8;
            formatted_data.chunks[chunk_index].bytes[0] = conj_map;
            auto complexity = measure_complexity(formatted_data.chunks[chunk_index]);
            if (complexity < 0.5) {
                conjugate(formatted_data.chunks[chunk_index]);
            }
        } else {
            conj_map <<= 1;
            auto complexity = measure_complexity(formatted_data.chunks[chunk_index]);
            if (complexity < 0.5) {
                conjugate(formatted_data.chunks[chunk_index]);
                conj_map |= 1;
            }
        }
    }

    return formatted_data;
}

DataChunkArray format_message_for_hiding(float threshold, std::vector<u8> const& message) {
    size_t formatted_size =
        ((message.size() + sizeof(SIGNATURE) + sizeof(u32)) * 8 + 62) / 63 * 8;
    DataChunkArray formatted_data;
    formatted_data.chunks.resize(formatted_size / 8);

    size_t out_bit_index = 0;
    size_t in_bit_index = 0;

    u32 message_size = message.size();
    u8* out_ptr = formatted_data.bytes_begin();

    set_bit(out_ptr, out_bit_index++, 0);

    for (size_t i = 0; i < sizeof(SIGNATURE) * 8; i++) {
        u8 bit_value = get_bit(SIGNATURE, i);
        set_bit(out_ptr, out_bit_index++, bit_value);
    }

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
    u8 const* formatted_data_ptr = formatted_data.bytes_begin();

    u8 signature[sizeof(SIGNATURE)];
    for (size_t i = 0; i < sizeof(SIGNATURE) * 8; i++) {
        auto bit_value = get_bit(formatted_data_ptr, in_bit_index++);
        set_bit(signature, i, bit_value);
    }

    if (std::memcmp(signature, SIGNATURE, sizeof(SIGNATURE)) != 0) {
        throw "invalid signature";
    }

    u32 message_size = 0;

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
