#ifndef BCPS_TEST_202306250729
#define BCPS_TEST_202306250729

#include <vector>
#include "../src/image.h"
#include "../src/utility.h"

std::vector<u8> chunkify(Image const& img);
void de_chunkify(Image& img, std::vector<u8> const& chunked_data);
u8 binary_to_gray_code(u8 binary);
u8 gray_code_to_binary(u8 gray_code);
void binary_to_gray_code_inplace(std::vector<u8>& vec);
void gray_code_to_binary_inplace(std::vector<u8>& vec);
u8 extract_bitplane_byte(u8 const* byte_ptr, size_t bitplane_index);
void insert_bitplane_byte(u8* byte_ptr, size_t bitplane_index, u8 inserted_byte);
std::vector<u8> planify(std::vector<u8> const& data);
std::vector<u8> de_planify(std::vector<u8> const& planed_data);
std::vector<u8> format_message_for_hiding(float threshold, std::vector<u8> const& message);
std::vector<u8> unformat_message(std::vector<u8> formatted_data);
void bcps_hide_message(float threshold, Image& img, std::vector<u8> const& message);
std::vector<u8> bcps_unhide_message(float threshold, Image const& img);

#endif // BCPS_TEST_202306250729
