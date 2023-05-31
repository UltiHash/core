

#include <options/app_config.h>

#include <config.hpp>

#include <unordered_map>
#include <string_view>
#include <iostream>
#include <cstring>
#include <filesystem>
#include <list>
#include "io/file.h"
#include "options/chunking_options.h"
#include "subchunking_options.h"
#include <algorithm>
#include <span>
#include <cstring>

APPLICATION_CONFIG
(
(subchunking, uh::tools::subchunking_options),
(chunking, uh::options::chunking_options)
);


std::map <std::string, unsigned long> fragments;
std::unordered_map <std::string, unsigned long> blocks;


size_t non_deduplicated_size = 0;
long chunk_count = 0;

size_t largest_common_prefix (const std::string &str1, const std::string& str2) {
    size_t i = 0;
    const auto min_size = std::min (str1.size(), str2.size());
    while (i < min_size and str1[i] == str2[i]) {
        i++;
    }
    return i;
}

int insert_block (const std::string &chunk_str, size_t min_block) {

    auto eq_range = fragments.equal_range(chunk_str);
    if (eq_range.first != fragments.begin()) {
        eq_range.first --;
    }
    if (eq_range.second != fragments.end()) {
        eq_range.second ++;
    }

    auto res = std::max_element (eq_range.first, eq_range.second, [&chunk_str] (const auto &block1, const auto &block2) {
        return largest_common_prefix(chunk_str, block1.first) < largest_common_prefix(chunk_str, block2.first);
    });

    if (res == fragments.end()) {
        fragments.emplace_hint (res, chunk_str, 1);
        return 1;
    }

    const auto key = res->first;
    const auto i = largest_common_prefix(chunk_str, key);

    if (i < min_block or (chunk_str.size() - i < min_block and chunk_str.size() - i > 0) or (key.size() - i < min_block and key.size() - i  > 0)) {
        fragments.emplace_hint(res, chunk_str, 1);
        return 1;
    }

    int count = 0;
    if (i > 0 and i < key.size()) {
        fragments.emplace_hint (res, key.substr(0, i), res->second + 1);
        count ++;
    }
    if (i > 0 and i < key.size()) {
        fragments.erase(res);
        count --;
    }
    if (i < chunk_str.size()) {
        count += insert_block(chunk_str.substr(i), min_block);
    }
    if (i > 0 and i < key.size()) {
        count += insert_block(key.substr(i), min_block);
    }

    return count;

}

void integrate (const std::filesystem::path &path, uh::chunking::mod &chunking_module, size_t min_block) {
    uh::io::file f (path, std::ios::in);

    auto chunker = chunking_module.create_chunker(f);


    for (auto chunk = chunker->next_chunk(); !chunk.empty(); chunk = chunker->next_chunk()) {
        std::string chunk_str {chunk.data (), chunk.size()};



        non_deduplicated_size += chunk.size();
        blocks.emplace(chunk_str, insert_block(chunk_str, min_block));
        chunk_count ++;
    }
}

int main(int argc, const char *argv[]) {

    application_config config;
    if (config.evaluate(argc, argv) == uh::options::action::exit)
    {
        return 0;
    }

    auto chunking_cfg = config.chunking();
    const auto min_subchunk_size = 2*1024;
    chunking_cfg.fast_cdc.min_size = 4*1024;
    chunking_cfg.fast_cdc.normal_size = 8*1024;
    chunking_cfg.fast_cdc.max_size = 1024*512;
    uh::chunking::mod chunking_module(chunking_cfg);

    const auto root = config.subchunking().path;
    unsigned long count = 0;


    if (std::filesystem::is_directory(root)) {
        for (const auto &file: std::filesystem::recursive_directory_iterator(root)) {
            if (file.is_directory()) {
                continue;
            }
            integrate(file, chunking_module, min_subchunk_size);
            //std::cout << "integrated " << file << std::endl;
            count++;
        }
    }
    else {
        integrate(root, chunking_module, min_subchunk_size);
        //std::cout << "integrated " << root << std::endl;
    }

    size_t effective_size = 0;
    for (const auto &item: fragments) {
        //std::cout << item.first << std::endl;
        effective_size += item.first.size();
    }

    long total = 0;
    for (const auto &block: blocks) {
        std::cout << block.second << std::endl;
        if (block.second == 0) {
            throw 1;
        }
        total += block.second;
    }

    double ratio = 1 - static_cast <double> (effective_size) / static_cast <double> (non_deduplicated_size);
    std::cout << std::endl;
    std::cout << "number of files " << count << std::endl;
    std::cout << "total size " << static_cast <double> (non_deduplicated_size) / static_cast <double> (1024 * 1024 * 1024) << " GB" << std::endl;
    std::cout << "effective size " << static_cast <double> (effective_size) / static_cast <double> (1024 * 1024 * 1024) << " GB" << std::endl;
    std::cout << "chunking algorithm " << config.chunking().chunking_strategy << std::endl;
    std::cout << "fragments " << fragments.size() << std::endl;
    std::cout << "chunks " << chunk_count << std::endl;
    std::cout << "average fragment/block count " << static_cast <double> (total) / static_cast <double> (chunk_count)  << ", total number of fragment references " << total << std::endl;
    std::cout << "deduplication ratio is " << ratio << std::endl;
}
