#ifndef UTILITY_202306151744
#define UTILITY_202306151744

#include <cstdint>
#include <variant>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

inline u8 set_lsb(u8 byte_in, bool bit_value) {
    u8 bit_mask = bit_value ? 1 : 0;
    return (byte_in & 0xfe) | bit_mask;
}

inline u32 u32_from_bytes_be(u8 const* bytes) {
    u32 result = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    return result;
}

inline void u32_to_bytes_be(u32 value, u8* bytes_out) {
    bytes_out[0] = (value >> 24);
    bytes_out[1] = (value >> 16);
    bytes_out[2] = (value >> 8);
    bytes_out[3] = value;
}

inline u8 get_bit(u8 const* data, size_t bit_index) {
    size_t byte_index = bit_index / 8;
    size_t shift = 7 - (bit_index % 8);
    return (data[byte_index] >> shift) & 1;
}

inline void set_bit(u8* data, size_t bit_index, u8 bit_value) {
    size_t byte_index = bit_index / 8;
    size_t shift = bit_index % 8;
    if (bit_value) {
        data[byte_index] |= (0x80 >> shift);
    } else {
        data[byte_index] &= ~(0x80 >> shift);
    }
}

template<typename C, typename T>
bool contains(C const& container, T const& item) {
    auto b = std::begin(container);
    auto e = std::end(container);
    auto found = std::find(b, e, item) != e;
    return found;
}

#endif // UTILITY_202306151744
