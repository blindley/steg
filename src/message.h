#ifndef MESSAGE_202307050547
#define MESSAGE_202307050547

#include "utility.h"
#include "bpcs.h"

#include <vector>

extern u8 const SIGNATURE[3];

DataChunkArray format_message_v2(float threshold, std::vector<u8> const& message);
void conjugate(DataChunk& chunk);
void conjugate_data(float threshold, DataChunkArray& formatted_data);
void de_conjugate_data(DataChunkArray& formatted_data);
DataChunkArray format_message_for_hiding(float threshold, std::vector<u8> const& message);
std::vector<u8> unformat_message(DataChunkArray formatted_data);

DataChunkArray format_message_v2(float threshold, std::vector<u8> const& message);
std::vector<u8> unformat_message_v2(DataChunkArray formatted_data);

#endif // MESSAGE_202307050547
