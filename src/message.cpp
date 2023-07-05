
#include "bpcs.h"
#include "utility.h"

u8 const SIGNATURE[] = { 0x2F, 0x64, 0xA9 };

size_t calculate_formatted_message_size(size_t message_size) {
    size_t formatted_size = message_size + sizeof(SIGNATURE) + sizeof(u32);
    formatted_size = (formatted_size + 62) / 63 * 64;
    return formatted_size;
}

size_t calculate_message_capacity_from_chunk_count(size_t chunk_count) {
    size_t usable_chunks = chunk_count / 8 * 8;
    size_t usable_bytes = usable_chunks / 8 * 63 - sizeof(SIGNATURE) - sizeof(u32);
    return usable_bytes;
}

void conjugate(DataChunk& chunk) {
    for (size_t i = 0; i < 4; i++) {
        chunk.bytes[i * 2] ^= 0xAA;
        chunk.bytes[i * 2 + 1] ^= 0x55;
    }
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

DataChunkArray format_message(float threshold, std::vector<u8> const& message) {
    return format_message_v2(threshold, message);
}

std::vector<u8> unformat_message(DataChunkArray formatted_data) {
    return unformat_message_v2(formatted_data);
}
