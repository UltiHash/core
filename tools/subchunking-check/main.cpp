

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


std::string old;
void integrate (const std::filesystem::path &path, uh::client::chunking::mod &chunking_module, size_t min_block) {
    uh::io::file f (path, std::ios::in);

    auto chunker = chunking_module.create_chunker(f);

    for (auto chunk = chunker->next_chunk(); !chunk.empty(); chunk = chunker->next_chunk()) {
        std::string chunk_str {chunk.data (), chunk.size()};

        auto eq_range = blocks.equal_range(chunk_str);
        if (eq_range.first != blocks.begin()) {
            eq_range.first --;
        }
        std::pair <size_t, decltype(eq_range.first)> max_fit {0, blocks.end()};


        for (auto itr = eq_range.first; itr != eq_range.second ; itr++) {
            if (const auto lcp = largest_common_prefix(chunk_str, itr->first); lcp > max_fit.first) {
                max_fit = {lcp, itr};
            }
        }

        total_size += chunk.size();

        if (max_fit.second == blocks.end()) {
            blocks.emplace(chunk_str, 1);
            continue;
        }
        const auto &key = max_fit.second->first;
        size_t i = 0;
        const auto min_size = std::min (key.size(), chunk.size());
        while (i < min_size and key[i] == chunk[i]) {
            i++;
        }

        if (i < min_block or chunk.size() - i < min_block or key.size() - i < min_block) {
            blocks.emplace(chunk_str, 1);
            continue;
        }


        if (i > 0) {
            blocks.emplace(key.substr(0, i), max_fit.second->second + 1);
        }
        if (i < chunk.size()) {
            auto news = chunk_str.substr(i);
            blocks.emplace(news, 1);
        }
        if (i < key.size()) {
            auto news = key.substr(i);

            blocks.emplace(news, max_fit.second->second);
        }
        if (i > 0 or i < key.size()) {
            blocks.erase(max_fit.second->first);
        }
    }
}

int main(int argc, const char *argv[]) {

    application_config config;
    if (config.evaluate(argc, argv) == uh::options::action::exit)
    {
        return 0;
    }

    auto chunking_cfg = config.chunking();
    const auto min_block_size = 0;
    chunking_cfg.fast_cdc.min_size = 8;
    chunking_cfg.fast_cdc.normal_size = 16;
    chunking_cfg.fast_cdc.max_size = 32;
    uh::client::chunking::mod chunking_module(chunking_cfg);

    const auto root = config.subchunking().path;
    unsigned long count = 0;


/*
    uh::io::file f (root, std::ios::in);

    auto chunker = chunking_module.create_chunker(f);
    for (auto chunk = chunker->next_chunk(); !chunk.empty(); chunk = chunker->next_chunk()) {
        blocks.emplace(std::string {chunk.data (), chunk.size ()}, 1);
    }

    for (const auto b: blocks) {
        std::cout << b.first << std::endl;
    }

    return 0;
*/

        if (std::filesystem::is_directory(root)) {
        for (const auto &file: std::filesystem::recursive_directory_iterator(root)) {
            if (file.is_directory()) {
                continue;
            }
            integrate(file, chunking_module, min_block_size);
            std::cout << "integrated " << file << std::endl;
            count++;
        }
    }
    else {
        integrate(root, chunking_module, min_block_size);
        std::cout << "integrated " << root << std::endl;
    }

    size_t effective_size = 0;
    for (const auto &item: blocks) {
        std::cout << item.first.size() << std::endl;
        effective_size += item.first.size();
    }

    double ratio = static_cast <double> (effective_size) / static_cast <double> (total_size);
    std::cout << std::endl;
    std::cout << "number of files " << count << std::endl;
    std::cout << "total size " << static_cast <double> (total_size) / static_cast <double> (1024 * 1024 * 1024) << " GB" << std::endl;
    std::cout << "effective size " << static_cast <double> (effective_size) / static_cast <double> (1024 * 1024 * 1024) << " GB" << std::endl;
    std::cout << "chunking algorithm " << config.chunking().chunking_strategy << std::endl;
    std::cout << "deduplication ratio is " << ratio << std::endl;
}
