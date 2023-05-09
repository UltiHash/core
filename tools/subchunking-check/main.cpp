

#include <options/app_config.h>

#include <config.hpp>

#include <unordered_map>
#include <string_view>
#include <iostream>
#include <cstring>
#include <filesystem>
#include <list>
#include "io/file.h"
#include "../../client-shell/src/chunking/options.h"
#include "subchunking_options.h"
#include <algorithm>
#include <span>
#include <cstring>

APPLICATION_CONFIG
(
(subchunking, uh::tools::subchunking_options),
(chunking, uh::client::chunking::options)
);

struct bytes {


    bytes (std::span <char> data): m_size (data.size()), m_data (new unsigned char[m_size]) {
        assert(m_size % sizeof(uint64_t) == 0);
        std::memcpy(m_data, data.data(), m_size);
    }

    bytes (bytes &&b) noexcept {
        m_data = b.m_data;
        m_size = b.m_size;
        b.m_data = nullptr;
        b.m_size = 0;
    }

    std::size_t m_size;
    unsigned char *m_data;
    bool operator < (const bytes &b) {
        for (std::size_t i = 0; i < m_size; i+=sizeof (uint64_t)) {
            const auto v1 = *reinterpret_cast <uint64_t *> (m_data + i);
            const auto v2 = *reinterpret_cast <uint64_t *> (b.m_data + i);
            if (v1 == v2) {
                continue;
            }
            return v1 < v2;
        }
        return false;
    }

    bool operator <= (const bytes &b) {
        for (std::size_t i = 0; i < m_size; i+=sizeof (uint64_t)) {
            const auto v1 = *reinterpret_cast <uint64_t *> (m_data + i);
            const auto v2 = *reinterpret_cast <uint64_t *> (b.m_data + i);
            if (v1 == v2) {
                continue;
            }
            return v1 < v2;
        }
        return true;
    }

    bool operator == (const bytes &b) {
        for (std::size_t i = 0; i < m_size; i+=sizeof (uint64_t)) {
            const auto v1 = *reinterpret_cast <uint64_t *> (m_data + i);
            const auto v2 = *reinterpret_cast <uint64_t *> (b.m_data + i);
            if (v1 == v2) {
                continue;
            }
            return false;
        }
        return true;
    }

    bool operator != (const bytes &b) {
        for (std::size_t i = 0; i < m_size; i+=sizeof (uint64_t)) {
            const auto v1 = *reinterpret_cast <uint64_t *> (m_data + i);
            const auto v2 = *reinterpret_cast <uint64_t *> (b.m_data + i);
            if (v1 == v2) {
                continue;
            }
            return true;
        }
        return false;
    }

    bool operator > (const bytes &b) {
        for (std::size_t i = 0; i < m_size; i+=sizeof (uint64_t)) {
            const auto v1 = *reinterpret_cast <uint64_t *> (m_data + i);
            const auto v2 = *reinterpret_cast <uint64_t *> (b.m_data + i);
            if (v1 == v2) {
                continue;
            }
            return v1 > v2;
        }
        return false;
    }

    bool operator >= (const bytes &b) {
        for (std::size_t i = 0; i < m_size; i+=sizeof (uint64_t)) {
            const auto v1 = *reinterpret_cast <uint64_t *> (m_data + i);
            const auto v2 = *reinterpret_cast <uint64_t *> (b.m_data + i);
            if (v1 == v2) {
                continue;
            }
            return v1 > v2;
        }
        return true;
    }


    ~bytes() {
        m_size = 0;
        if (m_data != nullptr) {
            delete[] m_data;
            m_data = nullptr;
        }
    }
};

size_t largest_common_prefix (const std::string &str1, const std::string& str2) {
    size_t i = 0;
    const auto min_size = std::min (str1.size(), str2.size());
    while (i < min_size and str1[i] == str2[i]) {
        i++;
    }
    return i;
}

struct lexical_comp {
    bool operator()(const std::string& a, const std::string& b) const {
        const auto lcp = largest_common_prefix(a, b);
        if (lcp == a.size()) {
            return true;
        }
        else if (lcp == b.size()) {
            return false;
        }
        else {
            return a.at(lcp) < b.at(lcp);

        }
        //return std::lexicographical_compare (a.cbegin(), a.cend(), b.cbegin(), b.cend());
    }
};

std::map <std::string, unsigned long> blocks;
size_t total_size = 0;

void insert_block (const std::string &chunk_str, size_t min_block) {

    auto eq_range = blocks.equal_range(chunk_str);
    if (eq_range.first != blocks.begin()) {
        eq_range.first --;
    }
    if (eq_range.second != blocks.end()) {
        eq_range.second ++;
    }

    auto res = std::max_element (eq_range.first, eq_range.second, [&chunk_str] (const auto &block1, const auto &block2) {
        return largest_common_prefix(chunk_str, block1.first) < largest_common_prefix(chunk_str, block2.first);
    });

    if (res == blocks.end()) {
        blocks.emplace_hint (res, chunk_str, 1);
        return;
    }

    const auto key = res->first;
    const auto i = largest_common_prefix(chunk_str, key);

    if (i < min_block or (chunk_str.size() - i < min_block and chunk_str.size() - i > 0) or (key.size() - i < min_block and key.size() - i  > 0)) {
        blocks.emplace_hint(res, chunk_str, 1);
        return;
    }

    if (i > 0 and i < key.size()) {
        blocks.emplace_hint (res, key.substr(0, i), res->second + 1);
    }
    if (i > 0 and i < key.size()) {
        blocks.erase(res);
    }
    if (i < chunk_str.size()) {
        insert_block(chunk_str.substr(i), min_block);
    }
    if (i > 0 and i < key.size()) {
        insert_block(key.substr(i), min_block);
    }

}

void integrate (const std::filesystem::path &path, uh::client::chunking::mod &chunking_module, size_t min_block) {
    uh::io::file f (path, std::ios::in);

    auto chunker = chunking_module.create_chunker(f);

    for (auto chunk = chunker->next_chunk(); !chunk.empty(); chunk = chunker->next_chunk()) {
        std::string chunk_str {chunk.data (), chunk.size()};


        total_size += chunk.size();
        insert_block(chunk_str, min_block);

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
    uh::client::chunking::mod chunking_module(chunking_cfg);

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
    for (const auto &item: blocks) {
        //std::cout << item.first << std::endl;
        effective_size += item.first.size();
    }

    double ratio = 1 - static_cast <double> (effective_size) / static_cast <double> (total_size);
    std::cout << std::endl;
    std::cout << "number of files " << count << std::endl;
    std::cout << "total size " << static_cast <double> (total_size) / static_cast <double> (1024 * 1024 * 1024) << " GB" << std::endl;
    std::cout << "effective size " << static_cast <double> (effective_size) / static_cast <double> (1024 * 1024 * 1024) << " GB" << std::endl;
    std::cout << "chunking algorithm " << config.chunking().chunking_strategy << std::endl;
    std::cout << "deduplication ratio is " << ratio << std::endl;
}
