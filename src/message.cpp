#include <stdexcept>

#include "declarations.h"

u8 const SIGNATURE[] = { 0x2F, 0x64, 0xA9 };
u8 const MAGIC_14[] = { 53, 219, 170, 213, 10, 183, 76, 85, 179, 82, 181, 170, 55, 85 };

// Reads 4 bytes, interpreting them as a big-endian u32
u32 u32_from_bytes_be(u8 const* bytes) {
    u32 result = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    return result;
}

// Writes a u32 in big-endian format
void u32_to_bytes_be(u32 value, u8* bytes_out) {
    bytes_out[0] = (value >> 24);
    bytes_out[1] = (value >> 16);
    bytes_out[2] = (value >> 8);
    bytes_out[3] = value;
}

// Given actual message size, calculates size after formatting for hiding
size_t calculate_formatted_message_size(size_t message_size) {
    size_t formatted_size = message_size + sizeof(SIGNATURE) + sizeof(u32);
    formatted_size = (formatted_size + 62) / 63 * 64;
    return formatted_size;
}

// Given a chunk count indicating number of complex chunks, calculates how
// large of a message can be stored after formatting
size_t calculate_message_capacity_from_chunk_count(size_t chunk_count) {
    size_t usable_chunks = (chunk_count - 2) / 8 * 8;
    size_t usable_bytes = usable_chunks / 8 * 63 - sizeof(SIGNATURE) - sizeof(u32);
    return usable_bytes;
}

// Conjugates a group of 8 chunks
//
// We do this in groups of 8 because the conjugation map is stored in the first byte of the first
// chunk for every group of 8
void conjugate_group(DataChunk* chunk_ptr) {
    u8 conj_map = 0;
    for (size_t i = 1; i < 8; i++) {
        conj_map <<= 1;
        auto complexity = chunk_ptr[i].measure_complexity();
        if (complexity < 0.5) {
            chunk_ptr[i].conjugate();
            conj_map |= 1;
        }
    }

    chunk_ptr[0].bytes[0] = conj_map;
    auto complexity = chunk_ptr[0].measure_complexity();
    if (complexity < 0.5) {
        chunk_ptr[0].conjugate();
    }
}

// Deconjugate a group of 8 chunks
void de_conjugate_group(DataChunk* chunk_ptr) {
    if ((chunk_ptr[0].bytes[0] & 0x80) == 0x80) {
        chunk_ptr[0].conjugate();
    }

    auto conj_map = chunk_ptr[0].bytes[0];

    for (size_t i = 1; i < 8; i++) {
        if (conj_map & (0x80 >> i)) {
            chunk_ptr[i].conjugate();
        }
    }
}

// Format a message for hiding
//
// Several things need to be done to a message in order that we can find it again. First, we need to
// know the size of the message. So a 32 bit prefix indicating the size is placed in front.
//
// Second, we want to be confident that we aren't just pulling some random bytes out of an image,
// and there actually is a message hidden here. So we insert three marker bytes which are just some
// random numbers generated one time. Note that this is now redundant, because we have a much more
// unique 14 byte magic number that precedes the message and is used for determining which bitplanes
// were actually used for hiding the message.
//
// Third, we need to make sure all of the message chunks are complex, otherwise the extractor won't
// know which chunks to pull out. So we conjugate all chunks with complexity < 0.5. However, we also
// need a way to know which chunks were conjugated. So an 8 bit conjugation map is placed as the
// first byte of every 8th chunk. For simplicity, the formatted message is extended to a multiple of
// 8 chunks. It's possible that this could cause a message that would otherwise be able to fit, to
// not fit, if its size is very close to the capacity of the cover image (within 63 bytes).
DataChunkArray format_message(std::vector<u8> const& message, u8 rmax, u8 gmax, u8 bmax, u8 amax) {
    // The message is prefixed with 3 chunks. The first chunk contains starts with the conjugation
    // map for the first group of 8 chunks. The last 4 bytes tell us the size of the message. The 3
    // bytes in between are another randomly generated magic number. This number is checked on
    // extraction for validation purposes. However, this is redundant due to the magic chunks that
    // follow. The primary purpose of the 3 magic bytes is to just fill up the first chunk to 8
    // bytes. The conjugation map is not counted as part of the size. The second and third chunks
    // are the magic chunks, explained elsewhere. So the 23 here is the 4 bytes for storing the
    // size, 3 magic bytes, and 16 bytes for the magic chunks.
    size_t formatted_size = 23 + message.size();

    // This is how many groups of 8 chunks in the formatted message. A group consists of 63 bytes of
    // the message (or meta data), plus 1 byte for the conjugation map. So this formula just rounds
    // the size up to the nearest multiple of 63, and divides by 63 to get the number of groups.
    size_t formatted_chunk_group_count = (formatted_size + 62) / 63;
    size_t formatted_chunk_count = formatted_chunk_group_count * 8;
    DataChunkArray formatted_data;
    formatted_data.chunks.resize(formatted_chunk_count);

    u32 message_size = message.size();
    u8* out_ptr = formatted_data.bytes_begin();

    std::memcpy(out_ptr + 1, SIGNATURE, 3);
    u32_to_bytes_be(message_size, out_ptr + 4);

    auto magic_chunks = generate_magic_chunks(rmax, gmax, bmax, amax);
    formatted_data.chunks[1] = magic_chunks[0];
    formatted_data.chunks[2] = magic_chunks[1];

    // start outputting the actual message at the 3rd chunk (24th byte)
    size_t out_index = 3 * 8;
    size_t in_index = 0;

    while (in_index < message.size()) {
        // skip over the conjugation byte
        if (out_index % 64 == 0) {
            out_index++;
        }

        out_ptr[out_index] = message[in_index];
        ++out_index;
        ++in_index;
    }

    if (formatted_data.chunks.size() % 8) {
        auto err = "chunks not multiple of 8, fix this";
        throw std::logic_error(err);
    }

    for (size_t i = 0; i < formatted_data.chunks.size(); i += 8) {
        conjugate_group(formatted_data.chunks.data() + i);
    }

    return formatted_data;
}

size_t parse_size_chunk(DataChunk size_chunk) {
    if ((size_chunk.bytes[0] & 0x80) == 0x80)
        size_chunk.conjugate();
    if (std::memcmp(size_chunk.bytes + 1, SIGNATURE, 3) != 0)
        throw std::runtime_error("invalid signature");
    return (size_t)u32_from_bytes_be(size_chunk.bytes + 4);
}

// Undoes what format_message(...) did.
//
// Unconjugates conjugated chunks, extracts size, checks signature, and returns message in its
// original form.
std::vector<u8> unformat_message(DataChunkArray formatted_data) {
    std::vector<u8> message;
    if (formatted_data.chunks.size() < 8) {
        return message;
    }

    size_t parsed_message_size = parse_size_chunk(formatted_data.chunks[0]);
    size_t num_chunk_groups = formatted_data.chunks.size() / 8;
    size_t formatted_message_size = num_chunk_groups * 63;
    size_t max_possible_message_size = formatted_message_size - 23;
    size_t actual_message_size = std::min(parsed_message_size, max_possible_message_size);

    for (size_t i = 0; i < num_chunk_groups; i++) {
        auto chunk_ptr = formatted_data.chunks.data() + i * 8;
        de_conjugate_group(chunk_ptr);
    }

    size_t formatted_data_index = 24;
    auto formatted_data_byte_ptr = formatted_data.bytes_begin();
    message.reserve(actual_message_size);
    for (size_t i = 0; i < actual_message_size; i++) {
        if (formatted_data_index % 64 == 0)
            ++formatted_data_index;
        message.push_back(formatted_data_byte_ptr[formatted_data_index]);
        ++formatted_data_index;
    }

    return message;
}

#ifdef STEG_TEST

#include <gtest/gtest.h>

TEST(message, message_formatting) {
    std::vector<u8> message;

    for (size_t i = 0; i < 4099; i++) {
        message.push_back(std::rand() >> 7);
    }

    auto formatted_message = format_message(message, 8, 8, 8, 8);
    auto recovered_message = unformat_message(formatted_message);
    ASSERT_EQ(message, recovered_message);
}

#endif // STEG_TEST
