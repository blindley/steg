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
std::vector<u8> planify(std::vector<u8> const& vec);

#endif // BCPS_TEST_202306250729
