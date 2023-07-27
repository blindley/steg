#ifndef MESSAGE_202307050547
#define MESSAGE_202307050547

#include "utility.h"
#include "bpcs.h"

#include <vector>

extern u8 const SIGNATURE[3];
extern u8 const MAGIC_14[14];

DataChunkArray format_message(std::vector<u8> const& message);
std::vector<u8> unformat_message(DataChunkArray formatted_data);

size_t calculate_formatted_message_size(size_t message_size);
size_t calculate_message_capacity_from_chunk_count(size_t chunk_count);

#endif // MESSAGE_202307050547
