


#include <config.hpp>

#include <unordered_map>
#include <string_view>
#include <iostream>
#include <cstring>
#include <filesystem>
#include <list>
#include "io/file.h"
#include "../../client-shell/src/chunking/options.h"
#include "timer_pack.hpp"
#include <algorithm>
#include <span>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <tuple>

struct substr_match {
    long m_chunk_offset;
    int m_size;
    long m_data_offset;

    bool operator < (const substr_match &sm) const {
        return m_chunk_offset < sm.m_chunk_offset;
    }
};

struct substr_non_match {
    long m_chunk_offset;
    int m_size;

    bool operator < (const substr_non_match &sm) const {
        return m_chunk_offset < sm.m_chunk_offset;
    }
};

/*
struct window {
    size_t offset {};
    size_t data_size {};
    long window_size {};
    std::vector <char> data;
};

window file_window;
*/
std::unordered_map <std::string, std::set <substr_match>> blocks;
size_t non_deduplicated_size = 0;
long backend_size = 0;
std::vector <int> size_table;



std::set <substr_match> chunks_longest_common_substrings (uh::client::chunking::mod &chunking_module, std::string path, char* data_device, const long window_size, size_t min_subchunk_size) {
    uh::io::file f (path, std::ios::in);

    auto chunker = chunking_module.create_chunker(f);
    //std::vector <char> window (window_size);
    //data_device.read(window.data(), window_size);
    //auto data_size = data_device.gcount();
    //data_device.clear();
    std::set <substr_match> substrings;

    //data_device.seekg(0);

    for (auto chunk = chunker->next_chunk(); !chunk.empty(); chunk = chunker->next_chunk()) {
    }

        long backend_index = 0;
    while (backend_index < backend_size) {

            for (long i = 0; i < chunk.size(); i++) {
            const auto row = i * window_size;
            const auto prev_row = row - window_size;
            const auto max_backend_index = std::min (backend_size, backend_index + window_size);
            for (long idx = backend_index; idx < max_backend_index; idx++) {
                if (chunk[i] != data_device[idx]) [[likely]] {
                    continue;
                }
                const auto j = idx - backend_index;
                auto &size_item = size_table [row + j];
                size_item = (i && j) ? 1 + size_table [prev_row + j - 1] : 1;
                if (size_item >= min_subchunk_size) {
                    uint16_t size = size_item + 1;
                    substrings.insert({i - size_item + 1, size, j - size_item + 1});
                }
            }
        }
        backend_index += window_size;

    }

    return substrings;
}

std::set <substr_match> longest_common_substrings (std::span <char> &chunk, char* data_device, const long window_size, size_t min_subchunk_size) {

    //std::vector <char> window (window_size);
    //data_device.read(window.data(), window_size);
    //auto data_size = data_device.gcount();
    //data_device.clear();
    std::set <substr_match> substrings;

    //data_device.seekg(0);


    long backend_index = 0;
    while (backend_index < backend_size) {
        for (long i = 0; i < chunk.size(); i++) {
            const auto row = i * window_size;
            const auto prev_row = row - window_size;
            const auto max_backend_index = std::min (backend_size, backend_index + window_size);
            for (long idx = backend_index; idx < max_backend_index; idx++) {
                if (chunk[i] != data_device[idx]) [[likely]] {
                    continue;
                }
                const auto j = idx - backend_index;
                auto &size_item = size_table [row + j];
                size_item = (i && j) ? 1 + size_table [prev_row + j - 1] : 1;
                if (size_item >= min_subchunk_size) {
                    uint16_t size = size_item + 1;
                    substrings.insert({i - size_item + 1, size, j - size_item + 1});
                }
            }
        }
        backend_index += window_size;

    }

    return substrings;
}

std::pair <std::set <substr_match>, std::set <substr_non_match>> get_substrings_coverage (const std::set <substr_match>& longest_substrings, std::size_t chunk_size) {
    std::set <substr_match> min_cover_substrings;
    std::set <substr_non_match> non_covered_substrings;

    long offset = 0;
    auto itr = longest_substrings.cbegin();
    while (offset < chunk_size and itr != longest_substrings.cend()) {
        const auto &substr = *itr;
        if (substr.m_chunk_offset + substr.m_size <= offset) {
            continue;
        }
        const long overlap = static_cast <long> (offset) - substr.m_chunk_offset;
        if (overlap <= 0) {

            min_cover_substrings.emplace_hint(min_cover_substrings.end(), substr);

            if (overlap < 0) {
                non_covered_substrings.emplace_hint (non_covered_substrings.end(), substr_non_match {offset, static_cast <int> (-overlap)});
            }
        }
        else {
            min_cover_substrings.emplace_hint(min_cover_substrings.end(), substr_match {offset, static_cast <int> (substr.m_size - overlap), substr.m_data_offset+overlap});
        }
        offset = substr.m_chunk_offset + substr.m_size;
        itr ++;
    }

    if (offset < chunk_size) {
        non_covered_substrings.emplace_hint (non_covered_substrings.end(), substr_non_match {offset, static_cast <int> (chunk_size - offset)});

    }
    return {min_cover_substrings, non_covered_substrings};
}


void integrate (char *backend, const std::filesystem::path &path, uh::client::chunking::mod &chunking_module, const long window_size, size_t min_subchunk_size) {
    uh::io::file f (path, std::ios::in);

    auto chunker = chunking_module.create_chunker(f);
    timer_pack <5> timer;
    long counter = 0;
    timer.start(3);
    timer.start(4);
    for (auto chunk = chunker->next_chunk(); !chunk.empty(); chunk = chunker->next_chunk()) {

        const auto longest_substrings = longest_common_substrings(chunk, backend, window_size, min_subchunk_size);
    /*
        std::cout << "searching chunk: " << std::string {chunk.data(), chunk.size()} << std::endl;
        std::cout << "window of " << window_size << " in data: ";
        std::cout.write(backend, backend_size);
        std::cout << std::endl;
        for (const auto &s: longest_substrings) {
            std::cout << s.m_chunk_offset << ":" << s.m_chunk_offset + s.m_size << " in " << s.m_data_offset << " i.e. " << std::string {chunk.data() + s.m_chunk_offset, static_cast<size_t>(s.m_size)} << std::endl;
        }
        std::cout << "--------------" << std::endl;
*/

        auto [covered, uncovered] = get_substrings_coverage (longest_substrings, chunk.size());


        auto it = covered.begin();
        for (const auto &substr: uncovered) {
            substr_match synced_substr {substr.m_chunk_offset, substr.m_size,  backend_size};
            std::memcpy(backend + backend_size, chunk.data() + substr.m_chunk_offset, substr.m_size);
            backend_size += substr.m_size;
            it = covered.emplace_hint(it, synced_substr);
        }

        blocks.emplace(std::string{chunk.data(), chunk.size()}, std::move (covered));
        non_deduplicated_size += chunk.size();

    }
}


int main(int argc, const char *argv[]) {

    uh::client::chunking::chunking_config chunking_cfg;
    const auto max_backend_size = 100 * 1024 * 1024;
    const auto min_subchunk_size = 512;
    const auto window_size = 16*1024;

    //chunking_cfg.chunking_strategy = "FastCDC";
    //chunking_cfg.fast_cdc.min_size = 4*1024;
    //chunking_cfg.fast_cdc.normal_size = 8*1024;
    //chunking_cfg.fast_cdc.max_size = 16*1024;
    chunking_cfg.chunking_strategy = "FixedSize";
    chunking_cfg.chunk_size_in_bytes = 8*1024;
    uh::client::chunking::mod chunking_module(chunking_cfg);

    const auto root = "/home/masi/Workspace/core/cmake-build-debug/client-shell/data.mp4";
    unsigned long count = 0;
    size_table.resize(window_size * chunking_cfg.chunk_size_in_bytes);
    int fd = open ("backend", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU); //backend {"backend", std::ios::out | std::ios::trunc | std::ios::binary | std::ios::in};
    ftruncate(fd, max_backend_size);
    char *ptr = static_cast<char *>(mmap(nullptr, max_backend_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

    if (std::filesystem::is_directory(root)) {
        for (const auto &file: std::filesystem::recursive_directory_iterator(root)) {
            if (file.is_directory()) {
                continue;
            }
            integrate(ptr, file, chunking_module, window_size, min_subchunk_size);
            std::cout << "integrated " << file << std::endl;
            count++;
        }
    }
    else {
        integrate(ptr, root, chunking_module, window_size, min_subchunk_size);
        std::cout << "integrated " << root << std::endl;
    }

    for (const auto &item: blocks) {
        std::cout << item.first << std::endl;

        //for (const auto &subchunk: item.second) {
        //    std::cout << subchunk <<std::endl;
       // }
    }

    double ratio = 1 - static_cast <double> (backend_size) / static_cast <double> (non_deduplicated_size);
    std::cout << std::endl;
    std::cout << "number of files " << count << std::endl;
    std::cout << "total size " << static_cast <double> (non_deduplicated_size) / static_cast <double> (1024 * 1024 * 1024) << " GB" << std::endl;
    std::cout << "effective size " << static_cast <double> (backend_size) / static_cast <double> (1024 * 1024 * 1024) << " GB" << std::endl;
    std::cout << "chunking algorithm " << chunking_cfg.chunking_strategy << std::endl;
    std::cout << "deduplication ratio is " << ratio << std::endl;
}
