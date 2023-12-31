// Benjamin Lindley, Vanessa Martinez
//
// datachunk.cpp
//
// A DataChunk is just an 8 byte array with some convenient functions. This is the fundamental unit
// of data hiding in BPCS. The DataChunk is used in two separate places. In bpcs.cpp, the cover
// image is broken into 8x8 bitplane chunks. These bits are extracted into a DataChunk. Also, in
// message.cpp, the message is formatted into DataChunks. Then, formatted message datachunks can
// just be copied over cover image datachunks, instead of working at the byte level.

#include <algorithm>
#include <bit>
#include <map>
#include <vector>
#include <stdexcept>

#include "declarations.h"

// Counts all the bit transitions, from 1 to 0, or from 0 to 1, in a byte
size_t count_bit_transitions(u8 byte) {
    u8 x = (byte ^ (byte << 1)) & 0b11111110;
    return std::popcount(x);
}

// Counts the bit differences at each corresponding position of 2 bytes
size_t count_bit_differences(u8 a, u8 b) {
    u8 diff = a ^ b;
    return std::popcount(diff);
}

// Measure the complexity of a chunk
//
// Complexity is measured by counting the vertical and horizontal bit transitions in a chunk. Then
// dividing by 112, which is the maximum possible number of transitions in an 8x8 chunk.
float DataChunk::measure_complexity() const {
    size_t count = 0;
    for (size_t i = 0; i < 8; i++) {
        count += count_bit_transitions(bytes[i]);
    }

    for (size_t i = 0; i < 7; i++) {
        count += count_bit_differences(bytes[i], bytes[i+1]);
    }

    size_t const max_bit_transitions = 2 * 7 * 8;
    return (float)count / (float)max_bit_transitions;
}

// Makes a non complex chunk complex, or vice versa
//
// - 0xAA = 10101010
// - 0x55 = 01010101
//
// Repeat that 4 times and you have an 8x8 checkerboard pattern, where every bit is the opposite of
// its neighbors in all 4 directions. This gives a total of 112 bit transitions, which is the
// maximum possible in an 8x8 chunk. With the method by which we measure complexity, this also has
// the maximum possible complexity C = 1. Xoring a chunk with the checkerboard pattern causes all of
// the bit transitions to flip. That is, everywhere that there was a bit transition, now there
// isn't. And everywhere that there wasn't a bit transition, now there is. This causes the
// complexity to go from C to 1 - C.
//
// Conjucating a previously conjugated chunk gives back the original chunk.
void DataChunk::conjugate() {
    for (size_t i = 0; i < 4; i++) {
        bytes[i * 2] ^= 0xAA;
        bytes[i * 2 + 1] ^= 0x55;
    }
}

// Used for calculating the complexity threshold
//
// Based on the idea of a Cumulative Distribution Function, can be queried for a complexity
// threshold T, and returns how many chunks in the supplied DataChunkArray have complexity >= T
struct CDF {
    CDF(DataChunkArray const& chunks, std::vector<size_t> const& bitplane_priority);
    size_t query(float threshold) const;
    float max_threshold_to_store(size_t chunk_count) const;

    std::vector<std::pair<float, size_t>> inner;
};

// Generate the cumulative distribution of complex chunks
//
// First generate a histogram of all of the different complexity levels. Then use this to generate a
// cumulative distribution function (cdf). A cdf is like a histogram, but where histogram[x] = the
// number of elements which are equal to x, cdf[x] = the number of elements which are greater than
// or equal to x.
CDF::CDF(DataChunkArray const& chunks, std::vector<size_t> const& bitplane_priority) {
    size_t chunks_per_bitplane = chunks.chunks.size() / 32;

    std::map<float, size_t> hist;

    for (size_t bp = 0; bp < bitplane_priority.size(); bp++) {
        size_t bitplane_index = bitplane_priority[bp];

        for (size_t ci = 0; ci < chunks_per_bitplane; ci++) {
            size_t chunk_index = bitplane_index * chunks_per_bitplane + ci;

            auto& chunk = chunks.chunks[chunk_index];
            auto complexity = chunk.measure_complexity();
            hist[complexity]++;
        }
    }

    size_t cumulative = 0;
    for (auto i_hist = hist.rbegin(); i_hist != hist.rend(); ++i_hist) {
        cumulative += i_hist->second;
        inner.emplace_back(i_hist->first, cumulative);
    }

    std::reverse(inner.begin(), inner.end());
}

// returns the count of chunks which have complexity >= threshold
size_t CDF::query(float threshold) const {
    auto compare_to_first = [](auto&& p, float v) { return p.first < v; };
    auto it = std::lower_bound(inner.begin(), inner.end(), threshold, compare_to_first);
    if (it == inner.end())
        return 0;
    return it->second;
}


// Returns the maximum complexity threshold that can be used if you need to store the specified
// number of chunks.
//
// A negative value indicates that many chunks can not fit at any threshold
float CDF::max_threshold_to_store(size_t chunk_count) const {
    size_t granularity = 512;
    for (size_t i = 0; i <= granularity; i++) {
        float threshold = (float)(granularity - i) / (float)granularity;
        if (this->query(threshold) >= chunk_count)
            return threshold;
    }
    return -1.0f;
}

// Calculates the maximum threshold that can be used to store the given number of chunks in the
// given cover, using the specified bitplanes
float calculate_max_threshold(size_t message_chunk_count, DataChunkArray const& cover,
    std::vector<size_t> const& bitplane_priority)
{
    CDF cdf(cover, bitplane_priority);
    float threshold = cdf.max_threshold_to_store(message_chunk_count);
    threshold = std::min(0.5f, threshold);

    // CDF::max_threshold_to_store returns a negative number if the message can't fit, but we will
    // just clamp it to 0 and store a partial message in that case
    threshold = std::max(0.0f, threshold);

    return threshold;
}

#ifdef STEG_TEST

#include <gtest/gtest.h>
#include <random>

void randomize_chunk(std::mt19937_64& rng, DataChunk& chunk) {
    for (size_t i = 0; i < 8; i++) {
        auto value = rng();
        std::memcpy(chunk.bytes, &value, 8);
    }
}

TEST(datachunk, conjugate_complexity) {
    // test if the complexity of a chunk plus the complexity of its conjugate == 1

    std::random_device rd;
    auto seed = ((u64)rd()) ^ ((u64)std::time(nullptr));
    std::mt19937_64 gen64(seed);

    DataChunk chunk;
    for (int i = 0; i < 2000; i++) {
        randomize_chunk(gen64, chunk);
        auto complexity = chunk.measure_complexity();
        chunk.conjugate();
        auto conj_complexity = chunk.measure_complexity();
        auto complexity_sum = complexity + conj_complexity;
        ASSERT_FLOAT_EQ(complexity_sum, 1.0f);
    }
}

TEST(datachunk, measure_complexity) {
    DataChunk chunk = {};
    ASSERT_EQ(chunk.measure_complexity(), 0.0f);

    std::memset(chunk.bytes, 0xFF, 8);
    ASSERT_EQ(chunk.measure_complexity(), 0.0f);

    std::memset(chunk.bytes, 0xAA, 8);
    ASSERT_EQ(chunk.measure_complexity(), 0.5f);

    std::memset(chunk.bytes, 0x55, 8);
    ASSERT_EQ(chunk.measure_complexity(), 0.5f);

    std::memset(chunk.bytes, 0xCC, 8);
    ASSERT_EQ(chunk.measure_complexity(), 24.0f/112.0f);

    std::memset(chunk.bytes, 0x33, 8);
    ASSERT_EQ(chunk.measure_complexity(), 24.0f/112.0f);

    auto alt_fill = [&](u8 a, u8 b) {
        for (size_t i = 0; i < 4; i++) {
            size_t index = i * 2;
            chunk.bytes[index] = a;
            chunk.bytes[index + 1] = b;
        }
    };

    alt_fill(0x55, 0xAA);
    ASSERT_EQ(chunk.measure_complexity(), 1.0f);

    alt_fill(0xAA, 0x55);
    ASSERT_EQ(chunk.measure_complexity(), 1.0f);

    alt_fill(0, 0xFF);
    ASSERT_EQ(chunk.measure_complexity(), 0.5f);

    alt_fill(0xFF, 0);
    ASSERT_EQ(chunk.measure_complexity(), 0.5f);
}

TEST(datachunk, CDF) {
    DataChunkArray chunks;
    chunks.chunks.resize(17*32);
    chunks.chunks[0] = {};
    chunks.chunks[1] = { 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
    chunks.chunks[2] = { 0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, };
    chunks.chunks[3] = { 0x00, 0x07, 0x00, 0x07, 0x00, 0x07, 0x00, 0x00, };
    chunks.chunks[4] = { 0x00, 0xe7, 0x00, 0x07, 0x00, 0x07, 0x00, 0x00, };
    chunks.chunks[5] = { 0x00, 0xe7, 0x00, 0xe7, 0x00, 0x07, 0x00, 0x00, };
    chunks.chunks[6] = { 0x00, 0xe7, 0x00, 0xe7, 0x00, 0xe7, 0x00, 0x00, };
    chunks.chunks[7] = { 0x00, 0xe7, 0x00, 0xe7, 0x00, 0xe7, 0x00, 0x3F, };

    chunks.chunks[8] = { 0x55, 0xAA, 0x55, 0xAA, 0x00, 0x00, 0x00, 0x00, };

    for (size_t i = 0; i < 8; i++) {
        chunks.chunks[9 + i] = chunks.chunks[7 - i];
        chunks.chunks[9 + i].conjugate();
    }

    for (size_t i = 0; i < 17; i++) {
        float expected_complexity = (i * 7.0f) / 112.0f;
        float actual_complexity = chunks.chunks[i].measure_complexity();
        ASSERT_EQ(actual_complexity, expected_complexity);
    }

    std::vector<size_t> bitplane_priority(1, 0);
    CDF cdf(chunks, bitplane_priority);

    for (size_t i = 1; i <= 17; i++) {
        float expected_threshold = 1.0f - 0.0625f * (i - 1);
        float actual_threshold = cdf.max_threshold_to_store(i);
        ASSERT_EQ(expected_threshold, actual_threshold);
    }

    ASSERT_EQ(cdf.max_threshold_to_store(0), 1.0f);
    ASSERT_LT(cdf.max_threshold_to_store(18), 0.0f);
}

#endif // STEG_TEST
