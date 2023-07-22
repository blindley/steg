#ifndef UTILITY_202306151744
#define UTILITY_202306151744

#include <cstdint>
#include <variant>
#include <string>
#include <vector>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

u8 get_bit(u8 const* data, size_t bit_index);
void set_bit(u8* data, size_t bit_index, u8 bit_value);
std::string get_file_extension(std::string const& filename);
void save_file(std::string const& filename, u8 const* data, size_t len);
void save_file(std::string const& filename, std::vector<u8> const& data);
std::vector<u8> load_file(std::string const& filename);
std::vector<u8> random_bytes(size_t size);

#endif // UTILITY_202306151744
