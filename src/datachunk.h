#ifndef DATACHUNK_202307051716
#define DATACHUNK_202307051716

#include "utility.h"

#include <cstdlib>
#include <vector>

/// @brief 64 bits, the fundamental unit of data hiding in BPCS
/// 
/// The cover image is broken up into an array data chunks, each representing
/// an 8x8 section of a bitplane in the image.
/// The message also is formatted as an array of data chunks, which are used
/// to replace some subset of the data chunks from the cover image.
struct DataChunk {
    u8 bytes[8];

    bool operator==(DataChunk const& other) const {
        return std::memcmp(bytes, other.bytes, 8) == 0;
    }

    bool operator!=(DataChunk const& other) const {
        return !(*this == other);
    }

    float measure_complexity() const;
    void conjugate();
};

static_assert(sizeof(DataChunk) == 8);

/// @brief An array of data chunks
///
/// Just some conveniences added on top of vector<DataChunk>
struct DataChunkArray {
    std::vector<DataChunk> chunks;

    DataChunk* begin() { return chunks.data(); }
    DataChunk* end() { return chunks.data() + chunks.size(); }

    DataChunk const* begin() const { return chunks.data(); }
    DataChunk const* end() const { return chunks.data() + chunks.size(); }

    u8* bytes_begin() { return chunks.data()->bytes; }
    u8* bytes_end() { return chunks.data()->bytes + chunks.size() * sizeof(DataChunk); }

    u8 const* bytes_begin() const { return chunks.data()->bytes; }
    u8 const* bytes_end() const { return chunks.data()->bytes + chunks.size() * sizeof(DataChunk); }

    bool operator==(DataChunkArray const& other) const {
        return chunks == other.chunks;
    }

    bool operator!=(DataChunkArray const& other) const {
        return !(chunks == other.chunks);
    }
};

float calculate_max_threshold(size_t message_chunk_count, DataChunkArray const& cover_chunks,
    std::vector<size_t> const& bitplane_priority);

#endif // DATACHUNK_202307051716
