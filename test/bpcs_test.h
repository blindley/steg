#ifndef BCPS_TEST_202306250729
#define BCPS_TEST_202306250729

#include <vector>
#include "../src/image.h"
#include "../src/utility.h"
#include "../src/bpcs.h"

std::vector<u8> chunkify(Image const& img);
void de_chunkify(Image& img, std::vector<u8> const& chunked_data);
u8 binary_to_gray_code(u8 binary);
u8 gray_code_to_binary(u8 gray_code);
void binary_to_gray_code_inplace(std::vector<u8>& vec);
void gray_code_to_binary_inplace(std::vector<u8>& vec);
u8 extract_bitplane_byte(u8 const* byte_ptr, size_t bitplane_index);
void insert_bitplane_byte(u8* byte_ptr, size_t bitplane_index, u8 inserted_byte);
void extract_bitplane(u8 const* data_ptr, size_t bitplane_index, u8* planed_data_ptr,
    size_t planed_data_byte_count);
void insert_bitplane(u8* data_ptr, size_t bitplane_index, u8 const* planed_data_ptr,
    size_t planed_data_byte_count);
DataChunkArray planify(std::vector<u8> const& data);
std::vector<u8> de_planify(DataChunkArray const& planed_data);
DataChunkArray chunk_and_planify(Image const& img);
void de_chunk_and_planify(Image& img, DataChunkArray const& planed_data);
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
std::vector<u8> bpcs_unhide_message(float threshold, Image const& img);

DataChunkArray chunkify_all_at_once(Image& img);
void re_chunk_that_b(Image& img, DataChunkArray const& planed_data);

#endif // BCPS_TEST_202306250729
