
#include <array>
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

DataChunkArray chunkify(Image const& img, std::vector<size_t> const& bitplane_priority);
void de_chunkify(Image& img, DataChunkArray const& planed_data,
    std::vector<size_t> const& bitplane_priority);

// Converts a single byte to gray code
//
// The reason for this operation is explained here:
//      http://datahide.org/BPCSe/pbc-vs-cgc-e.html
u8 binary_to_gray_code(u8 binary) {
    return (binary >> 1) ^ binary;
}

// Converts a gray code byte back to plain binary
u8 gray_code_to_binary(u8 gray_code) {
    u8 temp = gray_code ^ (gray_code >> 4);
    temp ^= (temp >> 2);
    temp ^= (temp >> 1);
    return temp;
}

// Converts an array of bytes to gray code
void binary_to_gray_code_inplace(std::vector<u8>& vec) {
    std::transform(vec.begin(), vec.end(), vec.begin(), binary_to_gray_code);
}

// Converts an array of gray code bytes back to plain binary
void gray_code_to_binary_inplace(std::vector<u8>& vec) {
    std::transform(vec.begin(), vec.end(), vec.begin(), gray_code_to_binary);
}

// Generates the signature chunks
//
// SIG14 is a const array consisting of 14 random bytes generated from random.org
// These 14 bytes are split into 2 groups of 7, and then an 8th byte is attached
// to each. These appended bytes actually contain 4 numbers, taking 4 bits each,
// indicating how many bitplanes of each of the channels is used for hiding the
// message.
//
// The two 8 byte groups become the signature chunks, which are used to replace
// the first usable chunk in the first usable bitplane, according to the users
// specification. Because these chunks contain 14 randomly generated bytes, or
// 112 bits, the probability of these bytes appearing by chance in an image is
// astronomically low. So the presence of these signature chunks does two things.
// First, it identifies the image as almost certainly containing a hidden
// message which was inserted by this program. And second, it allows the
// extraction algorithm determine which bitplanes were used for hiding the
// message.
std::array<DataChunk, 2> generate_signature_chunks(u8 rmax, u8 gmax, u8 bmax, u8 amax) {
    std::array<DataChunk, 2> chunks;
    std::memcpy(chunks[0].bytes, SIG14, 7);
    std::memcpy(chunks[1].bytes, SIG14 + 7, 7);
    chunks[0].bytes[7] = (rmax << 4) | gmax;
    chunks[1].bytes[7] = (bmax << 4) | amax;

    return chunks;
}

// Returns an array containing which specific bitplanes to use, and in what
// order, based on the number of bitplanes to use per channel.
//
// The bitplanes are numbered, as far as this program is concerned, as follows,
// 0-7   = Red (0 is MSB, 7 is LSB)
// 8-15  = Green
// 16-23 = Blue
// 24-31 = Alpha
//
// The default order of the bitplanes, if all bitplanes are allowed to be used,
// is to start with red LSB (7), then proceed to the LSB of green (15),
// blue (23) and alpha (31). We keep rotating through rgba like that, going up
// and up to more significant bitplanes. Any one of the channels can be capped
// with a max number of bitplanes, where 0 means no bitplanes of that channel
// are used, and 8 means all of them are used. The least significant bitplanes
// of any particular channel are always used first. So an rmax value of 6, for
// example, would mean that the 2 most significant bitplanes of the red channel
// are not used.
std::vector<size_t> generate_bitplane_priority(u8 rmax, u8 gmax, u8 bmax, u8 amax) {
    std::vector<size_t> bitplane_priority;

    for (size_t i = 0; i < 8; i++) {
        if (i < rmax)
            bitplane_priority.push_back(7 - i);
        if (i < gmax)
            bitplane_priority.push_back(15 - i);
        if (i < bmax)
            bitplane_priority.push_back(23 - i);
        if (i < amax)
            bitplane_priority.push_back(31 - i);
    }

    return bitplane_priority;
}

// Determine the bitplane priority of an image with a hidden message
//
// Finds the signature chunks, and uses them to determine rmax, gmax, bmax and
// amax. (see generate_signature_chunks(...))
//
// From those values, determine bitplane priority
// (see generate_bitplane_priority(...))
std::vector<size_t> find_bitplane_priority(Image const& img) {
    // TODO: Chunifying the image twice, could probably improve this
    auto bitplane_priority = generate_bitplane_priority(8, 8, 8, 8);
    auto chunks = chunkify(img, bitplane_priority);
    size_t signature_chunks_found = 0;
    auto blank_signature_chunks = generate_signature_chunks(0, 0, 0, 0);
    for (auto& e : chunks) {
        if (std::memcmp(blank_signature_chunks[signature_chunks_found].bytes, e.bytes, 7) == 0) {
            blank_signature_chunks[signature_chunks_found].bytes[7] = e.bytes[7];
            signature_chunks_found++;
            if (signature_chunks_found == 2) {
                u8 max0 = blank_signature_chunks[0].bytes[7];
                u8 max1 = blank_signature_chunks[1].bytes[7];
                u8 rmax = (max0 >> 4) & 0x0F;
                u8 gmax = max0 & 0x0F;
                u8 bmax = (max1 >> 4) & 0x0F;
                u8 amax = max1 & 0x0F;
                return generate_bitplane_priority(rmax, gmax, bmax, amax);
            }
        }
    }

    throw std::runtime_error("signature chunks not found");
}

// Returns an array of DataChunks from the bitplanes of the image
//
// A DataChunk is an 8x8 bit chunk of a single bitplane of the image. The chunks
// are pulled out from one bitplane at a time, based on the bitplane priority.
// The order in which the chunks are pulled out in each bitplane is randomized
// using a reproducible random number generator with a fixed seed. For images
// in which the width or height is not divisible by 8, the excess pixels on the
// right side or bottom are simply skipped over, so there is no problem in
// handling images of any sizes.
//
// This requires a lot of intricate work shuffling around bits. But this
// process is greatly simplified by the get_bit(...) and set_bit(...) functions,
// which essentially treat an array of bytes as an array of bits. This makes
// the process of copying bits about as simple as it is to copy bytes.
// 
// After calling this function, the hiding and extraction algorithms no longer
// need to to think in terms a 2 dimensional image, or bitplanes. They just have
// a one dimensional array of 8 byte chunks which they can work with in the
// order which they appear.
DataChunkArray chunkify(Image const& img, std::vector<size_t> const& bitplane_priority) {
    size_t chunks_in_width = img.width / 8;
    size_t chunks_in_height = img.height / 8;
    size_t chunks_per_bitplane = chunks_in_width * chunks_in_height;

    size_t num_bitplanes = bitplane_priority.size();

    DataChunkArray planed_data;
    planed_data.chunks.resize(chunks_in_width * chunks_in_height * num_bitplanes);
    u8* planed_data_ptr = (u8*)planed_data.chunks.data();
    size_t pd_bit_index = 0;

    u8 const* pixel_data_ptr = img.pixel_data.data();

    std::mt19937_64 gen(img.width * 1000003 + img.height);
    std::vector<size_t> chunk_priority;
    chunk_priority.resize(chunks_per_bitplane);
    for (size_t i = 0; i < chunks_per_bitplane; i++)
        chunk_priority[i] = i;
    std::shuffle(chunk_priority.begin(), chunk_priority.end(), gen);

    for (size_t bp = 0; bp < num_bitplanes; bp++) {
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

// Inserts an array of DataChunks back into an image.
//
// Simply reverses the process of chunkify(...), breaking the chunks
// apart into their component bits and placing the bits them back into the
// appropriate place in the image. The same random number generator with the
// same seed is used as was used in chunkify(...).
//
// The structure of the loops is identical to that of chunkify(...), with the
// only difference being which array we call get_bit(...) and set_bit(...) on.
void de_chunkify(Image& img, DataChunkArray const& planed_data,
    std::vector<size_t> const& bitplane_priority)
{
    size_t chunks_in_width = img.width / 8;
    size_t chunks_in_height = img.height / 8;
    size_t chunks_per_bitplane = chunks_in_width * chunks_in_height;

    size_t num_bitplanes = bitplane_priority.size();

    u8 const* planed_data_ptr = (u8 const*)planed_data.chunks.data();
    size_t pd_bit_index = 0;

    u8* pixel_data_ptr = img.pixel_data.data();

    std::mt19937_64 gen(img.width * 1000003 + img.height);
    std::vector<size_t> chunk_priority;
    chunk_priority.resize(chunks_per_bitplane);
    for (size_t i = 0; i < chunks_per_bitplane; i++)
        chunk_priority[i] = i;
    std::shuffle(chunk_priority.begin(), chunk_priority.end(), gen);

    for (size_t bp = 0; bp < num_bitplanes; bp++) {
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
}

// Hides an already formatted message in a DataChunkArray returned by chunkify(...)
//
// chunkify(...) already did most of the hard work, giving us the data chunks
// from the bitplanes all in the correct order. Now we just have to iterate
// over them, checking their complexity against the threshold, and inserting
// the chunks from the formatted message at those locations. Note that the
// first two available chunks are used to store the signature chunks
// (see generate_signature_chunks(...))
void hide_formatted_message(float threshold, DataChunkArray& cover,
    DataChunkArray const& formatted_message,
    u8 rmax, u8 gmax, u8 bmax, u8 amax)
{
    auto signature_chunks = generate_signature_chunks(rmax, gmax, bmax, amax);

    size_t signature_chunk_index = 0;
    auto message_chunk_iter = formatted_message.begin();
    for (auto& cover_chunk : cover) {
        if (message_chunk_iter == formatted_message.end())
            break;
        auto complexity = cover_chunk.measure_complexity();
        if (complexity >= threshold) {
            if (signature_chunk_index < 2) {
                cover_chunk = signature_chunks[signature_chunk_index++];
            } else {
                cover_chunk = *message_chunk_iter;
                ++message_chunk_iter;
            }
        }
    }

    // reached end of cover data before full message was hidden
    if (message_chunk_iter != formatted_message.end()) {
        auto chunks_hidden = message_chunk_iter - formatted_message.begin();
        auto full_chunk_groups_hidden = chunks_hidden / 8;
        auto capacity = full_chunk_groups_hidden * 63 - 7;
        auto err = std::format("max hiding capacity ({} bytes) exceeded", capacity);
        throw std::runtime_error(err);
    }
}

// Extract a hidden formatted message from a DataChunkArray
//
// Just reverses the process of hide_formatted_message(...)
DataChunkArray unhide_formatted_message(DataChunkArray const& cover) {
    DataChunkArray formatted_message;
    size_t signature_chunk_index = 0;
    for (auto& cover_chunk : cover) {
        auto complexity = cover_chunk.measure_complexity();
        if (complexity >= 0.5) {
            if (signature_chunk_index < 2) {
                // No need to check signature chunks, they would have already
                // been checked by find_bitplane_priority(...)
                signature_chunk_index++;
            } else {
                formatted_message.chunks.push_back(cover_chunk);
            }
        }
    }

    return formatted_message;
}

// Hides a message in an image
//
// This is the high level function that ties everything together for the hiding
// algorithm.
float bpcs_hide_message(Image& img, std::vector<u8> const& message,
    u8 rmax, u8 gmax, u8 bmax, u8 amax)
{
    auto formatted_data = format_message(message);
    binary_to_gray_code_inplace(img.pixel_data);
    auto bitplane_priority = generate_bitplane_priority(rmax, gmax, bmax, amax);
    auto planed_data = chunkify(img, bitplane_priority);
    float threshold = calculate_max_threshold(formatted_data.chunks.size() + 2, planed_data);
    hide_formatted_message(threshold, planed_data, formatted_data, rmax, gmax, bmax, amax);
    de_chunkify(img, planed_data, bitplane_priority);
    gray_code_to_binary_inplace(img.pixel_data);
    return threshold;
}

// Extracts a message hidden in an image
//
// The high level function that ties everything together for the extracting
// algorithm.
std::vector<u8> bpcs_unhide_message(Image& img) {
    binary_to_gray_code_inplace(img.pixel_data);

    auto bitplane_priority = find_bitplane_priority(img);

    auto planed_data = chunkify(img, bitplane_priority);
    auto formatted_data = unhide_formatted_message(planed_data);
    auto message = unformat_message(formatted_data);
    return message;
}

// Given an image and a complexity threshold, determines the image's hiding
// capacity at that threshold.
Measurements measure_capacity(float threshold, Image& img) {
    auto bitplane_priority = generate_bitplane_priority(8, 8, 8, 8);

    Measurements meas = {};
    auto cover = chunkify(img, bitplane_priority);

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

Image generate_random_image(size_t width, size_t height) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    Image img = {};
    img.width = width;
    img.height = height;
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
                            img.pixel_data[offset] = gen();
                        }
                    }
                }
            }
        }
    }

    return img;
}

TEST(bpcs, message_hiding) {
    std::random_device rd;
    std::mt19937_64 gen(rd());

    std::vector<u8> message;
    for (size_t i = 0; i < 511; i++) {
        message.push_back(gen());
    }

    auto img = generate_random_image(257, 135);
    auto img_original = img;

    bpcs_hide_message(img, message, 8, 8, 8, 8);
    auto extracted_message = bpcs_unhide_message(img);

    ASSERT_EQ(message, extracted_message);

    img = img_original;

    bpcs_hide_message(img, message, 7, 6, 5, 4);
    auto extracted_message2 = bpcs_unhide_message(img);

    ASSERT_EQ(message, extracted_message2);
}

TEST(bpcs, generate_bitplane_priority) {
    auto bitplane_priority = generate_bitplane_priority(0, 0, 0, 0);
    ASSERT_EQ(bitplane_priority.empty(), true);

    bitplane_priority = generate_bitplane_priority(8, 8, 8, 8);
    std::vector<size_t> expected_bitplane_priority = {
        7, 15, 23, 31, 6, 14, 22, 30,
        5, 13, 21, 29, 4, 12, 20, 28,
        3, 11, 19, 27, 2, 10, 18, 26,
        1, 9, 17, 25, 0, 8, 16, 24,
    };

    ASSERT_EQ(bitplane_priority, expected_bitplane_priority);

    expected_bitplane_priority = {
        7, 15, 23, 31,
           14, 22, 30,
               21, 29,
                   28,
    };
    bitplane_priority = generate_bitplane_priority(1, 2, 3, 4);

    ASSERT_EQ(bitplane_priority, expected_bitplane_priority);
}

#endif // STEG_TEST
