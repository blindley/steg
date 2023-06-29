#ifndef BCPS_202306171341
#define BCPS_202306171341

#include "image.h"
#include "utility.h"
#include <vector>

struct DataChunk {
    u8 bytes[8];

    bool operator==(DataChunk const& other) const {
        return std::memcmp(bytes, other.bytes, 8) == 0;
    }

    bool operator!=(DataChunk const& other) const {
        return !(*this == other);
    }
};

static_assert(sizeof(DataChunk) == 8);

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

void bcps_hide_message(float threshold, Image& img, std::vector<u8> const& message);
std::vector<u8> bcps_unhide_message(float threshold, Image const& img);

#endif // BCPS_202306171341
