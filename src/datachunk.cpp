#include <algorithm>
#include <map>
#include <vector>

#include "datachunk.h"

static size_t count_bit_transitions(u8 byte) {
    u8 x = (byte ^ (byte << 1)) & 0b11111110;
    return std::popcount(x);
}

static size_t count_bit_differences(u8 a, u8 b) {
    u8 diff = a ^ b;
    return std::popcount(diff);
}

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

void DataChunk::conjugate() {
    for (size_t i = 0; i < 4; i++) {
        bytes[i * 2] ^= 0xAA;
        bytes[i * 2 + 1] ^= 0x55;
    }
}

CDF::CDF(DataChunkArray const& chunks) {
    std::map<float, size_t> hist;
    for (auto& e : chunks) {
        auto complexity = e.measure_complexity();
        hist[complexity]++;
    }

    size_t cumulative = 0;
    for (auto i_hist = hist.rbegin(); i_hist != hist.rend(); ++i_hist) {
        cumulative += i_hist->second;
        inner.emplace_back(i_hist->first, cumulative);
    }

    std::reverse(inner.begin(), inner.end());
}

size_t CDF::query(float complexity) const {
    auto compare_to_first = [](auto&& p, float v) { return p.first < v; };
    auto it = std::lower_bound(inner.begin(), inner.end(), complexity, compare_to_first);
    if (it == inner.end())
        return 0;
    return it->second;
}

float CDF::max_threshold_to_store(size_t chunk_count) const {
    size_t granularity = 512;
    for (size_t i = 0; i <= granularity; i++) {
        float threshold = (float)(granularity - i) / (float)granularity;
        if (this->query(threshold) >= chunk_count)
            return threshold;
    }
    return -1.0f;
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
    chunks.chunks.resize(17);
    size_t i = 0;
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

    CDF cdf(chunks);

    for (size_t i = 1; i <= 17; i++) {
        float expected_threshold = 1.0f - 0.0625f * (i - 1);
        float actual_threshold = cdf.max_threshold_to_store(i);
        ASSERT_EQ(expected_threshold, actual_threshold);
    }

    ASSERT_EQ(cdf.max_threshold_to_store(0), 1.0f);
    ASSERT_LT(cdf.max_threshold_to_store(18), 0.0f);
}

#endif // STEG_TEST
