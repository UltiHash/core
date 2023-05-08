

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

APPLICATION_CONFIG
(
(subchunking, uh::tools::subchunking_options),
(chunking, uh::client::chunking::options)
);

std::unordered_map <std::string, std::list <std::filesystem::path>> blocks;
size_t total_size = 0;

void integrate (const std::filesystem::path &path, uh::client::chunking::mod &chunking_module) {
    uh::io::file f (path, std::ios::in);

    auto chunker = chunking_module.create_chunker(f);

    for (auto chunk = chunker->next_chunk(); !chunk.empty(); chunk = chunker->next_chunk()) {
        blocks[{chunk.data(), chunk.size()}].push_back(path);
        total_size += chunk.size();
    }
}

int main(int argc, const char *argv[]) {

    application_config config;
    if (config.evaluate(argc, argv) == uh::options::action::exit)
    {
        return 0;
    }

    auto chunking_cfg = config.chunking();
    uh::client::chunking::mod chunking_module(chunking_cfg);

    const auto root = config.subchunking().path;
    unsigned long count = 0;

    for (const auto &file : std::filesystem::recursive_directory_iterator(root)) {
        if (file.is_directory()) {
            continue;
        }
        integrate (file, chunking_module);
        std::cout << "integrated " << file << std::endl;
        count ++;
    }

    size_t effective_size = 0;
    for (const auto &item: blocks) {
        effective_size += item.first.size();
        if (item.second.size() > 1) {
            std::cout << "-------------------" << std::endl;
            for (const auto &path: item.second) {
                std::cout << "deduplicated file: " << path << std::endl;
            }
        }
    }

    double ratio = static_cast <double> (effective_size) / static_cast <double> (total_size);
    std::cout << std::endl;
    std::cout << "number of files " << count << std::endl;
    std::cout << "total size " << static_cast <double> (total_size) / static_cast <double> (1024 * 1024 * 1024) << " GB" << std::endl;
    std::cout << "effective size " << static_cast <double> (effective_size) / static_cast <double> (1024 * 1024 * 1024) << " GB" << std::endl;
    std::cout << "chunking algorithm " << config.chunking().chunking_strategy << std::endl;
    std::cout << "deduplication ratio is " << ratio << std::endl;
}
