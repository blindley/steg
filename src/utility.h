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

#endif // UTILITY_202306151744
