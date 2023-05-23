//
// Created by masi on 4/18/23.
//
#include "hierarchical_storage.h"

#include <io/buffer_generator.h>
#include <io/file.h>

#include <memory>


namespace uh::dbn::storage {


hierarchical_storage::hierarchical_multi_block_allocation::hierarchical_multi_block_allocation(
    hierarchical_storage &storage_backend,
    compressed_file_store& store,
    std::size_t size)
    : m_storage_backend(storage_backend),
      m_size(size),
      m_store(store)
{
}

void hierarchical_storage::hierarchical_multi_block_allocation::open_new_block(std::size_t block_size) {
    if (block_is_open ()) {
        THROW(util::exception, "a block is already open. close the block or persist it before opening a new block.");
    }
    if (m_persisted_size + block_size > m_size) {
        THROW(util::exception, "multi allocation overflow.");
    }

    m_tmp = m_store.temp_file(m_storage_backend.m_root);
    m_sha = std::make_unique<io::sha512>(*m_tmp);
    m_block_size = block_size;
}

bool hierarchical_storage::hierarchical_multi_block_allocation::block_is_open () {
    return m_sha != nullptr and m_tmp != nullptr;
}

void hierarchical_storage::hierarchical_multi_block_allocation::close_block () {
    m_sha.reset(nullptr);
    m_tmp.reset(nullptr);
}

io::device& hierarchical_storage::hierarchical_multi_block_allocation::device() {
    if (!block_is_open ()) {
        THROW(util::exception, "get device: no open block.");
    }
    return *m_sha;
}

uh::protocol::block_meta_data hierarchical_storage::hierarchical_multi_block_allocation::persist() {
    if (!block_is_open ()) {
        THROW(util::exception, "persist: no open block.");
    }

    const auto hash = m_sha->finalize();
    const std::string string_hash = to_hex_string(hash.begin(), hash.end());
    const auto file_path = m_storage_backend.get_hash_path(string_hash);

    std::filesystem::create_directories(file_path.parent_path());

    m_persisted_size += m_block_size;
    try {
        m_tmp->release_to(file_path);
        m_effective_size += m_block_size;
        m_store.compress(file_path);
    }
    catch (const util::file_exists&) {
        m_block_size = 0ul;
    }

    close_block();

    return {hash, m_block_size};
}


hierarchical_storage::hierarchical_multi_block_allocation::~hierarchical_multi_block_allocation()
{
    if (m_effective_size < m_size)
    {
        m_storage_backend.return_space(m_size - m_effective_size);
    }
}

// -----------------------------------------------------

class hierarchical_storage::hierarchical_allocation: public uh::protocol::allocation {

public:
    explicit hierarchical_allocation(hierarchical_storage &storage_backend,
                                     compressed_file_store& store,
                                     std::size_t size)
        : m_storage_backend (storage_backend),
          m_store(store),
          m_tmp(m_store.temp_file(storage_backend.m_root)),
          m_sha(*m_tmp),
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
            m_tmp->release_to(file_path);
            m_effective_size = m_size;
            m_store.compress(file_path);
        }
        catch (const util::file_exists&) {
            m_effective_size = 0u;
        }

        return {hash, m_effective_size};
    }

    ~hierarchical_allocation() override
    {
        if (m_effective_size == 0)
        {
            m_storage_backend.return_space(m_size);
        }
    }

    hierarchical_allocation(const hierarchical_storage&) = delete;
    hierarchical_allocation& operator=(const hierarchical_storage&) = delete;

private:
    hierarchical_storage& m_storage_backend;
    compressed_file_store& m_store;
    std::unique_ptr<io::temp_file> m_tmp;
    io::sha512 m_sha;
    std::size_t m_size;
    std::size_t m_effective_size {};
};


hierarchical_storage::hierarchical_storage(
    const hierarchical_storage_config& config,
    uh::dbn::metrics::storage_metrics& storage_metrics,
    persistence::scheduled_compressions_persistence& scheduled_compressions)
    : m_root(config.db_root),
      m_alloc(config.size_bytes),
      m_used(0),
      m_storage_metrics(storage_metrics),
      m_store(config.compressed,
              storage_metrics,
              scheduled_compressions,
              [this](std::streamsize s){ this->return_space(s); })
{
    if (!std::filesystem::is_directory(m_root))
    {
        throw std::runtime_error("path does not exist: " + m_root.string());
    }

    m_used = get_dir_size(m_root);
    if (m_used >= m_alloc)
    {
        THROW(util::exception, "database used over limit");
    }

    update_space_consumption();
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

std::unique_ptr<io::data_generator> hierarchical_storage::read_block(const std::span <char>& hash) {
    std::string hex = to_hex_string(hash.begin(), hash.end());

    const auto file_path = get_hash_path(hex);

    auto file = m_store.open(file_path);
    if (!file->valid()) {
        THROW(util::exception, "unknown hash: " + hex);
    }

    std::vector<char> buffer(BUFFER_SIZE);
    std::size_t ofs = 0;
    std::size_t size = 0;

    do
    {
        if (buffer.size() - ofs < BUFFER_SIZE)
        {
            buffer.resize(buffer.size() + BUFFER_SIZE);
        }

        size = file->read({ buffer.begin() + ofs, buffer.size() - ofs });
        ofs += size;
    }
    while (size != 0);

    buffer.resize(ofs);
    return std::make_unique<io::buffer_generator>(std::move(buffer));
}

std::unique_ptr<uh::protocol::allocation> hierarchical_storage::allocate(std::size_t size) {
    acquire_storage_size(size);

    return std::make_unique<hierarchical_allocation>(*this, m_store, size);
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

std::unique_ptr<uh::protocol::allocation> hierarchical_storage::allocate_multi(std::size_t size) {

    acquire_storage_size(size);
    return std::make_unique<hierarchical_multi_block_allocation>(*this, m_store, size);
}

void hierarchical_storage::return_space(std::size_t size)
{
    m_used -= size;
    update_space_consumption();
}

void hierarchical_storage::acquire_storage_size(std::size_t size) {
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

    update_space_consumption();
}

}
