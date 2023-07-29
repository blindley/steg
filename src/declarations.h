#ifndef DECLARATIONS_202307272153
#define DECLARATIONS_202307272153

#include <cstdint>
#include <string>
#include <vector>
#include <array>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

////////////////////////////////////////////////////////////////////////////////
// args.cpp
////////////////////////////////////////////////////////////////////////////////

// Command line arguments parsed
struct Args {
    bool help;
    bool extract;
    bool hide;
    bool measure;
    int random_count;
    std::string message_file;
    std::string cover_file;
    std::string stego_file;
    std::string output_file;
    float threshold;
    u8 rmax;
    u8 gmax;
    u8 bmax;
    u8 amax;
};

Args parse_args(int argc, char** argv);
void print_usage(char const* exe_name);
void print_help(char const* exe_name);

////////////////////////////////////////////////////////////////////////////////
// utility.cpp
////////////////////////////////////////////////////////////////////////////////
u8 get_bit(u8 const* data, size_t bit_index);
void set_bit(u8* data, size_t bit_index, u8 bit_value);
std::string get_file_extension(std::string const& filename);
void save_file(std::string const& filename, u8 const* data, size_t len);
void save_file(std::string const& filename, std::vector<u8> const& data);
std::vector<u8> load_file(std::string const& filename);
std::vector<u8> random_bytes(size_t size);


////////////////////////////////////////////////////////////////////////////////
// datachunk.cpp
////////////////////////////////////////////////////////////////////////////////

// 64 bits, the fundamental unit of data hiding in BPCS
// 
// The cover image is broken up into an array data chunks, each representing
// an 8x8 section of a bitplane in the image.
// The message also is formatted as an array of data chunks, which are used
// to replace some subset of the data chunks from the cover image.
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

// An array of data chunks
//
// Just some conveniences added on top of vector<DataChunk>
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


////////////////////////////////////////////////////////////////////////////////
// datachunk.cpp
////////////////////////////////////////////////////////////////////////////////
struct Image {
    size_t width;
    size_t height;
    std::vector<u8> pixel_data;

    void save(std::string const& filename);
    static Image load(std::string const& filename);
};


////////////////////////////////////////////////////////////////////////////////
// message.cpp
////////////////////////////////////////////////////////////////////////////////
extern u8 const SIGNATURE[3];
extern u8 const MAGIC_14[14];

DataChunkArray format_message(std::vector<u8> const& message, u8 rmax, u8 gmax, u8 bmax, u8 amax);
std::vector<u8> unformat_message(DataChunkArray formatted_data);

size_t calculate_formatted_message_size(size_t message_size);
size_t calculate_message_capacity_from_chunk_count(size_t chunk_count);
std::array<DataChunk, 2> generate_magic_chunks(u8 rmax, u8 gmax, u8 bmax, u8 amax);

////////////////////////////////////////////////////////////////////////////////
// bpcs.cpp
////////////////////////////////////////////////////////////////////////////////
struct HideStats {
    float threshold;
    size_t chunks_used;
    size_t chunks_per_bitplane;
    size_t chunks_used_per_bitplane[32];
    size_t message_size;
    size_t message_bytes_hidden;
};

HideStats bpcs_hide_message(float threshold, Image& img, std::vector<u8> const& message,
    u8 rmax, u8 gmax, u8 bmax, u8 amax);
std::vector<u8> bpcs_unhide_message(Image& img);
HideStats measure_capacity(float threshold, Image& img, u8 rmax, u8 gmax, u8 bmax, u8 amax);


#endif // DECLARATIONS_202307272153
