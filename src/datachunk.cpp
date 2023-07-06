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

#endif // STEG_TEST
