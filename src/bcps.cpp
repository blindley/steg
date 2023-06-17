#include "utility.h"
#include "image.h"

#include <vector>

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

}
