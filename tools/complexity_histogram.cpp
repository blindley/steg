#include <map>
#include <iostream>
#include <random>
#include <cstdlib>
#include <cmath>

#include "../src/bpcs.h"

float measure_plane_chunk_complexity(DataChunk const& chunk);

using Generator = std::mt19937_64;
void randomize(Generator& gen, DataChunk& chunk);

int main() {
    std::random_device rd;
    auto seed = ((u64)rd()) ^ ((u64)std::time(nullptr));
    Generator gen(seed);

    DataChunk chunk = {};
    std::map<float, int> histogram;
    for (int i = 0; i < 1130000; i++) {
        randomize(gen, chunk);
        auto complexity = measure_plane_chunk_complexity(chunk);
        histogram[complexity]++;
    }

    for (auto& kv : histogram) {
        auto k = std::roundf(kv.first * 112.0f * 100.0f) / 100.0f;
        std::cout << k << " : " << kv.second << '\n';
    }
}

void randomize(Generator& gen, DataChunk& chunk) {
    auto v = gen();
    std::memcpy(chunk.bytes, &v, 8);
}
