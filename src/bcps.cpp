#include <gtest/gtest.h>

#include "utility.h"
#include "image.h"

#include <vector>
#include <cstdlib>

#define CHUNK_SIZE 8
#define NUM_CHUNK_BORDERS (2 * BITPLANE_CHUNK_SIZE * (BITPLANE_CHUNK_SIZE - 1))

namespace bcps {

    // Functions for converting between pbc (pure binary coding) and cgc (canonical gray coding)
    u8 pbc_to_cgc(u8 byte) {
        return byte ^ (byte >> 1);
    }

    u8 cgc_to_pbc(u8 byte) {
        u8 mask = byte;
        while (mask) {
            mask >>= 1;
            byte ^= mask;
        }
        return byte;
    }

    void pbc_to_cgc(u8* data, size_t len) {
        for (size_t i = 0; i < len; i++)
            data[i] = pbc_to_cgc(data[i]);
    }

    void cgc_to_pbc(u8* data, size_t len) {
        for (size_t i = 0; i < len; i++)
            data[i] = cgc_to_pbc(data[i]);
    }

    u8 extract_bitplane_byte(u8 const* pixel_ptr, size_t bitplane_index) {
        pixel_ptr += bitplane_index / 8;
        size_t bitshift = 7 - (bitplane_index % 8);
        u8 byte = 0;
        for (int i = 0; i < 8; i++) {
            byte <<= 1;
            byte |= ((*pixel_ptr >> bitshift) & 1);
            pixel_ptr += 4;
        }
        return byte;
    }

    void insert_bitplane_byte(u8 byte, u8* pixel_ptr, size_t bitplane_index) {
        pixel_ptr += bitplane_index / 8;
        size_t bitshift = 7 - (bitplane_index % 8);
        for (int i = 0; i < 8; i++) {
            u8 mask_byte = ((byte >> (7 - i)) & 1) << bitshift;
            *pixel_ptr &= ~(1 << bitshift);
            *pixel_ptr |= mask_byte;
            pixel_ptr += 4;
        }
    }

    void extract_bitplane_chunk(u8 const* pixel_ptr, size_t bitplane_index, size_t stride, u8* out_ptr) {
        for (size_t i = 0; i < 8; i++) {
            *out_ptr++ = extract_bitplane_byte(pixel_ptr, bitplane_index);
            pixel_ptr += stride;
        }
    }

    std::vector<u8> extract_bitplane_chunks(Image const& img) {
        std::vector<u8> chunks;
        size_t chunks_per_width = img.width / CHUNK_SIZE;
        size_t chunks_per_height = img.height / CHUNK_SIZE;
        size_t chunk_count = chunks_per_width * chunks_per_height * 24;

        chunks.resize(chunk_count * 8);
        u8* out_ptr = chunks.data();

        // start with the least significant bitplanes, { 23, 15, 7, 22, 14, 6 ... }
        size_t bitplane_priority[24];
        for (size_t i = 0, bp=23; i < 24; i++) {
            bitplane_priority[i] = bp;
            if (bp < 8)
                bp += 15;
            else
                bp -= 8;
        }

        size_t const pixel_width_bytes = 4;
        size_t const chunk_width_bytes = CHUNK_SIZE * pixel_width_bytes;
        size_t image_width_bytes = img.width * pixel_width_bytes;

        for (size_t i = 0; i < 24; i++) {
            size_t bitplane_index = bitplane_priority[i];
            for (size_t chunk_y = 0; chunk_y < chunks_per_height; chunk_y++) {
                u8 const* pixel_ptr = img.pixel_data.data() + chunk_y * CHUNK_SIZE * image_width_bytes;
                for (size_t chunk_x = 0; chunk_x < chunks_per_width; chunk_x++) {
                    extract_bitplane_chunk(pixel_ptr, bitplane_index, image_width_bytes, out_ptr);
                    out_ptr += CHUNK_SIZE;
                    pixel_ptr += chunk_width_bytes;
                }
            }
        }

        return chunks;
    }

    namespace test {
        int count_bits_set(u8 byte) {
            int count = 0;
            for (int i = 0; i < 8; i++) {
                u8 mask = 1 << i;
                if (byte & mask) {
                    count++;
                }
            }
            return count;
        }

        TEST(bcps_tests, count_bits_set) {
            EXPECT_EQ(count_bits_set(0), 0);
            EXPECT_EQ(count_bits_set(255), 8);
            for (int i = 0; i < 8; i++) {
                u8 byte = 1 << i;
                EXPECT_EQ(count_bits_set(byte), 1);
                EXPECT_EQ(count_bits_set((u8)(~byte)), 7);
            }
        }

        TEST(bcps_tests, GrayCodes) {
            using namespace ::bcps;
            for (int i = 0; i < 256; i++) {
                // subsequent gray codes should have exactly 1 bit difference
                // so xoring them together should result in a byte with exactly
                // 1 bit set
                auto diff = pbc_to_cgc(i) ^ pbc_to_cgc(i+1);
                EXPECT_EQ(count_bits_set(diff), 1);
            }
        }

        u8 get_bit_by_index(u8 value, size_t bit_index) {
            size_t shift = 7 - bit_index;
            return (value >> shift) & 1;
        }

        TEST(bcps_tests, get_bit_by_index) {
            u8 value = 0x55;
            u8 vcomp = ~value;
            for (size_t i = 0; i < 8; i++) {
                EXPECT_EQ(get_bit_by_index(value, i), i % 2);
                EXPECT_EQ(get_bit_by_index(vcomp, i), (i + 1) % 2);
            }
        }

        TEST(bcps_tests, extract_bitplane_byte) {
            u8 test_buffer[32];
            for (int i = 0; i < 32; i++) {
                test_buffer[i] = i;
            }

            std::vector<int> bits;
            for (size_t i = 0; i < 32; i++) {
                for (size_t j = 0; j < 8; j++) {
                    int bit_value = get_bit_by_index(test_buffer[i], j);
                    bits.push_back(bit_value);
                }
            }

            using ::bcps::extract_bitplane_byte;
            for (size_t i = 0; i < 32; i++) {
                u8 expected_value = 0;
                for (size_t j = 0; j < 8; j++) {
                    expected_value <<= 1;
                    expected_value |= bits[j * 32 + i];
                }
                EXPECT_EQ(extract_bitplane_byte(test_buffer, i), expected_value);
            }
        }

        TEST(bcps_tests, insert_bitplane_byte) {
            u8 random_buffer[32];
            for (size_t i = 0; i < 32; i++) {
                random_buffer[i] = rand();
            }

            u8 expected_values[32] = {
                0b00000000, 0b00000000, 0b00000000, 0b00000000,
                0b00000000, 0b00000000, 0b00000000, 0b00000000,
                0b00000000, 0b00000000, 0b00000000, 0b00000000,
                0b00000000, 0b00000000, 0b11111111, 0b11111111,
                0b00000000, 0b11111111, 0b00000000, 0b11111111,
                0b00001111, 0b00001111, 0b00001111, 0b00001111,
                0b00110011, 0b00110011, 0b00110011, 0b00110011,
                0b01010101, 0b01010101, 0b01010101, 0b01010101,
            };

            using ::bcps::insert_bitplane_byte;
            for (size_t i = 0; i < 32; i++) {
                insert_bitplane_byte(i, random_buffer, i);
            }

            for (size_t i = 0; i < 32; i++) {
                EXPECT_EQ(random_buffer[i], expected_values[i]);
            }
        }
    }
}
