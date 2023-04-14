#include "dump_storage.h"

#include <io/file.h>
#include <io/sha512.h>


namespace uh::dbn::storage {

// ---------------------------------------------------------------------

class dump_storage_allocation : public protocol::allocation
{
public:
    dump_storage_allocation(std::unique_ptr<io::temp_file> device,
                            const std::filesystem::path& target_dir,
                            std::atomic<size_t>& used,
                            std::size_t size)
        : m_tmp(std::move(device)),
          m_target_dir(target_dir),
          m_used(used),
          m_size(size),
          m_sha(*m_tmp),
          m_dangling(true)
    {
    }

    ~dump_storage_allocation()
    {
        if (m_dangling)
        {
            m_used -= m_size;
        }
    }

    virtual io::device& device() override
    {
        return m_sha;
    }

    virtual uh::protocol::block_meta_data persist() override
    {
        auto hash = m_sha.finalize();

        std::string string_hash;
        boost::algorithm::hex(hash.begin(), hash.end(), std::back_inserter(string_hash));
        boost::algorithm::to_lower(string_hash);

        std::uint64_t effective_size = m_size;
        try
        {
            m_tmp->release_to(m_target_dir / string_hash);
        }
        catch (const util::file_exists&)
        {
            m_used -= m_size;
            effective_size = 0u;
        }

        m_dangling = false;
        return { hash, effective_size };
    }

private:
    std::unique_ptr<io::temp_file> m_tmp;
    std::filesystem::path m_target_dir;
    std::atomic<size_t>& m_used;
    std::size_t m_size;
    io::sha512 m_sha;
    bool m_dangling;
};

// ---------------------------------------------------------------------

uh::protocol::blob dump_storage::hashing_function(const uh::protocol::blob &data) {
    return uh::dbn::storage::sha512(data);
}

// ---------------------------------------------------------------------

void dump_storage::start(){

    INFO << "--- Storage backend initialized --- " << std::filesystem::absolute(this->m_root);
    INFO << "        backend type   : " << backend_type();
    INFO << "        root diretcory : " << std::filesystem::absolute(this->m_root);
    INFO << "        space allocated: " << allocated_space();
    INFO << "        space available: " << free_space();
    INFO << "        space consumed : " << used_space();
}

// ---------------------------------------------------------------------

std::unique_ptr<io::device> dump_storage::read_block(const uh::protocol::blob& hash) {

    std::string hash_string(hash.begin(), hash.end());

    std::string hex = to_hex_string(hash.begin(), hash.end());
    std::filesystem::path filepath = this->m_root / hex;

    auto file = std::make_unique<io::file>(filepath);
    if (!file->valid())
    {
        THROW(util::exception, "unknown hash: " + hex);
    }

    return file;
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::allocation> dump_storage::allocate(std::size_t size)
{
    while (true)
    {
        std::size_t used = m_used;
        if (m_alloc - used <= size)
        {
            THROW(util::exception, "out of space");
        }

        auto new_val = used + size;
        if (m_used.compare_exchange_weak(used, new_val))
        {
            break;
        }
    }

    try
    {
        return std::make_unique<dump_storage_allocation>(
            std::make_unique<io::temp_file>(m_root),
            m_root,
            m_used,
            size);
    }
    catch (...)
    {
        m_used -= size;
        throw;
    }
}

// ---------------------------------------------------------------------

void dump_storage::update_space_consumption()
{
    m_storage_metrics.alloc_space().Set(m_alloc);
    m_storage_metrics.free_space().Set(m_alloc - m_used);
    m_storage_metrics.used_space().Set(m_used);
}

// ---------------------------------------------------------------------

} //namespace uh::dbn::storage
