//
// Created by masi on 4/18/23.
//
#include "hierarchical_storage.h"
#include "io/file.h"
#include "io/sha512.h"

namespace uh::dbn::storage {

class hierarchical_storage::hierarchical_allocation: public uh::protocol::allocation {

public:
    explicit hierarchical_allocation(hierarchical_storage &storage_backend, std::size_t size):
        m_storage_backend (storage_backend),
        m_tmp(io::temp_file (storage_backend.m_root)),
        m_sha(m_tmp),
        m_size (size)
        {}

    io::device& device() override {
        return m_sha;
    }

    uh::protocol::block_meta_data persist() override {
        const auto hash = m_sha.finalize();
        const std::string string_hash = to_hex_string(hash.begin(), hash.end());
        const auto file_path = m_storage_backend.get_hash_path(string_hash);

        std::filesystem::create_directories(file_path.parent_path());
        try {
            m_tmp.release_to(file_path);
            m_effective_size = m_size;
        }
        catch (const util::file_exists&) {
            m_effective_size = 0u;
        }
        return {hash, m_effective_size};
    }

    ~hierarchical_allocation() override {
        if (m_effective_size == 0) {
            std::size_t used;
            do {
                used = m_storage_backend.m_used;
            } while (!m_storage_backend.m_used.compare_exchange_weak(used, used - m_size));
        }
    }

    hierarchical_allocation(const hierarchical_storage&) = delete;
    hierarchical_allocation &operator=(const hierarchical_storage &) = delete;

private:
    hierarchical_storage &m_storage_backend;
    io::temp_file m_tmp;
    io::sha512 m_sha;
    std::size_t m_size;
    std::size_t m_effective_size {};

};

hierarchical_storage::hierarchical_storage(std::filesystem::path db_root, size_t size_bytes,
                                                    uh::dbn::metrics::storage_metrics& storage_metrics):
    m_root(std::move (db_root)),
    m_alloc(size_bytes),
    m_used(0),
    m_storage_metrics(storage_metrics)
    {
        if( !std::filesystem::is_directory(m_root) ) {
            throw std::runtime_error("path does not exist: " + m_root.string());
        }
        else {
            m_used = get_dir_size(m_root);
            if (m_used >= m_alloc)
            {
                THROW(util::exception, "database used over limit");
            }
            update_space_consumption();
        }
    }


void hierarchical_storage::start() {
    INFO << "--- Storage backend initialized --- " << std::filesystem::absolute(this->m_root);
    INFO << "        backend type   : " << backend_type();
    INFO << "        root directory : " << std::filesystem::absolute(this->m_root);
    INFO << "        space allocated: " << allocated_space();
    INFO << "        space available: " << free_space();
    INFO << "        space consumed : " << used_space();
}

size_t hierarchical_storage::free_space() {
    return m_alloc - m_used;
}

size_t hierarchical_storage::used_space() {
    return m_used;
}

size_t hierarchical_storage::allocated_space() {
    return m_alloc;
}

std::string hierarchical_storage::backend_type() {
    return std::string(m_type);
}

void hierarchical_storage::update_space_consumption() {
    m_storage_metrics.alloc_space().Set(m_alloc);
    m_storage_metrics.free_space().Set(m_alloc - m_used);
    m_storage_metrics.used_space().Set(m_used);
}

std::unique_ptr<io::device> hierarchical_storage::read_block(const protocol::blob &hash) {
    std::string hex = to_hex_string(hash.begin(), hash.end());

    const auto file_path = get_hash_path(hex);

    auto file = std::make_unique<io::file> (file_path);
    if (!file->valid()) {
        THROW(util::exception, "unknown hash: " + hex);
    }

    return file;
}

std::unique_ptr<uh::protocol::allocation> hierarchical_storage::allocate(std::size_t size) {
    while (true)
    {
        std::size_t used = m_used;
        if (m_alloc - used <= size)
        {
            THROW(util::no_space_error, "out of space");
        }

        auto new_val = used + size;
        if (m_used.compare_exchange_weak(used, new_val))
        {
            break;
        }
    }

    return std::make_unique<hierarchical_allocation>(*this, size);
}

std::filesystem::path hierarchical_storage::get_hash_path (const std::string &hash) const {
    auto file_path = m_root;
    for (unsigned int i = 0; i < m_levels; i++) {
        const auto directory_name = hash.substr(2 * i, 2);
        file_path = file_path / directory_name;
    }
    const auto file_name = hash.substr(2 * m_levels);
    file_path = file_path / file_name;

    return file_path;
}

}