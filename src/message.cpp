
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

DataChunkArray format_message(float threshold, std::vector<u8> const& message) {
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

void conjugate_group(DataChunk* chunk_ptr) {
    u8 conj_map = 0;
    for (size_t i = 1; i < 8; i++) {
        conj_map <<= 1;
        auto complexity = measure_complexity(chunk_ptr[i]);
        if (complexity < 0.5) {
            conjugate(chunk_ptr[i]);
            conj_map |= 1;
        }
    }

    chunk_ptr[0].bytes[0] = conj_map;
    auto complexity = measure_complexity(chunk_ptr[0]);
    if (complexity < 0.5) {
        conjugate(chunk_ptr[0]);
    }
}

DataChunkArray format_message_v2(float threshold, std::vector<u8> const& message) {
    size_t formatted_size = message.size() + sizeof(SIGNATURE) + sizeof(u32);
    formatted_size = (formatted_size + 62) / 63 * 64;
    DataChunkArray formatted_data;
    formatted_data.chunks.resize(formatted_size / 8);

    size_t out_index = 1;
    size_t in_index = 0;

    u32 message_size = message.size();
    u8* out_ptr = formatted_data.bytes_begin();

    std::memcpy(out_ptr + out_index, SIGNATURE, sizeof(SIGNATURE));
    out_index += sizeof(SIGNATURE);
    u32_to_bytes_be(message_size, out_ptr + out_index);
    out_index += sizeof(u32);

    size_t n = std::min(64 - out_index, message.size());
    std::memcpy(out_ptr + out_index, message.data(), n);
    out_index += n;
    in_index += n;

    while (in_index < message.size()) {
        out_index++;
        size_t remaining = message.size() - in_index;
        size_t n = std::min((size_t)63, remaining);
        std::memcpy(out_ptr + out_index, message.data() + in_index, n);
        out_index += n;
        in_index += n;
    }

    for (size_t i = 0; i < formatted_data.chunks.size(); i += 8) {
        conjugate_group(formatted_data.chunks.data() + i);
    }

    return formatted_data;
}

void de_conjugate_group(DataChunk* chunk_ptr) {
    if ((chunk_ptr[0].bytes[0] & 0x80) == 0x80) {
        conjugate(chunk_ptr[0]);
    }

    auto conj_map = chunk_ptr[0].bytes[0];

    for (size_t i = 1; i < 8; i++) {
        if (conj_map & (0x80 >> i)) {
            conjugate(chunk_ptr[i]);
        }
    }
}

std::vector<u8> unformat_message_v2(DataChunkArray formatted_data) {
    u8* byte_ptr = formatted_data.bytes_begin();
    DataChunk* chunk_ptr = formatted_data.begin();
    de_conjugate_group(chunk_ptr);
    chunk_ptr += 8;

    size_t in_index = 1;
    if (std::memcmp(byte_ptr + in_index, SIGNATURE, sizeof(SIGNATURE)) != 0) {
        std::runtime_error e("invalid signature");
        throw e;
    }
    in_index += sizeof(SIGNATURE);

    auto message_size = u32_from_bytes_be(byte_ptr + in_index);
    in_index += sizeof(u32);

    size_t formatted_size = message_size + sizeof(SIGNATURE) + sizeof(u32);
    formatted_size = (formatted_size + 62) / 63 * 64;

    auto chunk_count = formatted_size / 8;
    auto group_count = chunk_count / 8;

    for (size_t i = 1; i < group_count; i++) {
        de_conjugate_group(chunk_ptr);
        chunk_ptr += 8;
    }

    std::vector<u8> message;
    message.reserve(message_size);
    
    for (size_t i = 0; i < message_size; i++) {
        if (in_index % 64 == 0) {
            in_index++;
        }

        message.push_back(byte_ptr[in_index++]);
    }

    return message;
}
