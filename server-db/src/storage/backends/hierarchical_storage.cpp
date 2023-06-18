//
// Created by masi on 4/18/23.
//
#include "hierarchical_storage.h"

#include <io/buffer_generator.h>
#include <io/file.h>
#include "licensing/global_licensing.h"

#include <memory>


namespace uh::dbn::storage
{

hierarchical_storage::hierarchical_storage(
    const hierarchical_storage_config &config,
    uh::dbn::metrics::storage_metrics &storage_metrics,
    state::scheduled_compressions_state &scheduled_compressions)
    : m_root(config.db_root),
      m_alloc(config.size_bytes),
      m_used(0),
      m_storage_metrics(storage_metrics),
      m_store(config.compressed,
              storage_metrics,
              scheduled_compressions,
              [this](std::streamsize s)
              { this->return_space(s); })
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

    try
    {
        metered_alloc(m_used);
    }
    catch (std::exception &e)
    {
        THROW(util::no_space_error,
              "Hierarchical storage is over license limit for this reason: " + std::string(e.what()));
    }

    update_space_consumption();
}

// ---------------------------------------------------------------------

void hierarchical_storage::start()
{
    INFO << "--- Storage backend initialized --- " << std::filesystem::absolute(this->m_root);
    INFO << "        backend type   : " << backend_type();
    INFO << "        root directory : " << std::filesystem::absolute(this->m_root);
    INFO << "        space allocated: " << allocated_space();
    INFO << "        space available: " << free_space();
    INFO << "        space consumed : " << used_space();
}

// ---------------------------------------------------------------------

void hierarchical_storage::stop()
{
    INFO << "stopping hierarchical_storage";
    m_store.stop();
}

// ---------------------------------------------------------------------

size_t hierarchical_storage::free_space()
{
    return std::min(m_alloc - m_used, metered_free_count());
}

// ---------------------------------------------------------------------

size_t hierarchical_storage::used_space()
{
    return m_used;
}

// ---------------------------------------------------------------------

size_t hierarchical_storage::allocated_space()
{
    return m_alloc;
}

// ---------------------------------------------------------------------

std::string hierarchical_storage::backend_type()
{
    return std::string(m_type);
}

// ---------------------------------------------------------------------

void hierarchical_storage::update_space_consumption()
{
    m_storage_metrics.alloc_space().Set(m_alloc);
    m_storage_metrics.free_space().Set(m_alloc - m_used);
    m_storage_metrics.used_space().Set(m_used);
}

// ---------------------------------------------------------------------

std::unique_ptr<io::data_generator> hierarchical_storage::read_block(const std::span<char> &hash)
{
    std::string hex = to_hex_string(hash.begin(), hash.end());

    const auto file_path = get_hash_path(hex);

    auto file = m_store.open(file_path);
    if (!file->valid())
    {
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

        size = file->read({buffer.begin() + ofs, buffer.size() - ofs});
        ofs += size;
    }
    while (size != 0);

    buffer.resize(ofs);
    return std::make_unique<io::buffer_generator>(std::move(buffer));
}

// ---------------------------------------------------------------------

std::filesystem::path hierarchical_storage::get_hash_path(const std::string_view &hash) const
{
    auto file_path = m_root;
    for (unsigned int i = 0; i < m_levels; i++)
    {
        const auto directory_name = hash.substr(2 * i, 2);
        file_path = file_path / directory_name;
    }
    const auto file_name = hash.substr(2 * m_levels);
    file_path = file_path / file_name;

    return file_path;
}

// ---------------------------------------------------------------------

void hierarchical_storage::return_space(std::size_t size)
{
    m_used -= size;
    metered_dealloc(size);
    update_space_consumption();
}

// ---------------------------------------------------------------------

void hierarchical_storage::acquire_storage_size(std::size_t size)
{
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

    try
    {
        metered_alloc(size);
    }
    catch (std::exception &e)
    {
        THROW(util::no_space_error, "Out of space for this reason: " + std::string(e.what()));
    }

    update_space_consumption();
}

// ---------------------------------------------------------------------

std::pair<std::size_t, std::vector<char>> hierarchical_storage::write_block(const std::span<char> &data)
{
    acquire_storage_size(data.size());
    auto m_tmp = m_store.temp_file(m_root);
    auto m_sha = std::make_unique<io::sha512>(*m_tmp);
    m_sha->write(data);
    auto hash = m_sha->finalize();
    const std::string string_hash = to_hex_string(hash.begin(), hash.end());
    const auto file_path = get_hash_path(string_hash);
    std::filesystem::create_directories(file_path.parent_path());

    try
    {
        m_tmp->release_to(file_path);
        m_store.compress(file_path);
    }
    catch (const util::file_exists &)
    {
        return_space(data.size());
        return {0, std::move(hash)};
    }

    return {data.size(), std::move(hash)};

}

// ---------------------------------------------------------------------

void hierarchical_storage::metered_alloc(std::size_t alloc)
{
    try
    {
        if (uh::dbn::licensing::global_license_pointer->license_package().has_soft_metred_feature(
            uh::licensing::license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY))
        {
            if (!uh::dbn::licensing::global_license_pointer->license_package()
                .soft_limit_allocate(uh::licensing::license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY,
                                     alloc))
            {
                WARNING << "Storage backend \"" + backend_type() + "\" has only " +
                        std::to_string(uh::dbn::licensing::global_license_pointer->license_package()
                                           .free_count(uh::licensing::license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY))
                        + " bytes free on its license!";

                if (!uh::dbn::licensing::global_license_pointer->license_package()
                    .hard_limit_allocate(uh::licensing::license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY,
                                         alloc))
                {
                    std::string full_string = "Storage backend \"" + backend_type() + "\" is full. It has only " +
                        std::to_string(uh::dbn::licensing::global_license_pointer->license_package()
                                           .free_count(uh::licensing::license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY))
                        + " bytes free on its license and by that cannot store another chunk with "
                        + std::to_string(alloc) + " bytes!";

                    ERROR << full_string;
                    THROW(util::exception, full_string);
                }
            }
        }
    }
    catch (std::exception &e)
    {
        FATAL << "Could not access global licensing module for this reason: " + std::string(e.what());
    }
}

// ---------------------------------------------------------------------

void hierarchical_storage::metered_dealloc(std::size_t dealloc)
{
    try
    {
        uh::dbn::licensing::global_license_pointer->license_package()
            .deallocate(uh::licensing::license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY, dealloc);
    }
    catch (std::exception &e)
    {
        FATAL << "Could not access global licensing module for this reason: " + std::string(e.what());
    }
}

// ---------------------------------------------------------------------

std::size_t hierarchical_storage::metered_free_count()
{
    try
    {
        return uh::dbn::licensing::global_license_pointer->license_package()
            .free_count(uh::licensing::license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY);
    }
    catch (std::exception &e)
    {
        FATAL << "Could not access global licensing module for this reason: " + std::string(e.what());
        return {};
    }
}

// ---------------------------------------------------------------------

}
