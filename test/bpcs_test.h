#ifndef BCPS_TEST_202306250729
#define BCPS_TEST_202306250729

#include <vector>
#include "../src/image.h"
#include "../src/utility.h"
#include "../src/bpcs.h"

u8 binary_to_gray_code(u8 binary);
u8 gray_code_to_binary(u8 gray_code);
void binary_to_gray_code_inplace(std::vector<u8>& vec);
void gray_code_to_binary_inplace(std::vector<u8>& vec);
size_t count_bit_transitions(u8 byte);
size_t count_bit_differences(u8 a, u8 b);
float measure_plane_chunk_complexity(DataChunk const& chunk);
void hide_raw_bytes(float threshold, DataChunkArray& cover, DataChunkArray const& formatted_message);
DataChunkArray unhide_raw_bytes(float threshold, DataChunkArray const& cover);
u8 get_bit(u8 const* data, size_t bit_index);
void set_bit(u8* data, size_t bit_index, u8 bit_value);
void conjugate(DataChunk& chunk);
void conjugate_data(float threshold, DataChunkArray& formatted_data);
void de_conjugate_data(DataChunkArray& formatted_data);
DataChunkArray format_message_for_hiding(float threshold, std::vector<u8> const& message);
std::vector<u8> unformat_message(DataChunkArray formatted_data);
void bpcs_hide_message(float threshold, Image& img, std::vector<u8> const& message);
std::vector<u8> bpcs_unhide_message(float threshold, Image& img);

DataChunkArray chunkify(Image& img);
void de_chunkify(Image& img, DataChunkArray const& planed_data);

#endif // BCPS_TEST_202306250729
