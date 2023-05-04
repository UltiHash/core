//
// Created by masi on 4/18/23.
//

#ifndef CORE_HIERARCHICAL_STORAGE_H
#define CORE_HIERARCHICAL_STORAGE_H

#include <storage/backend.h>
#include <storage/compressed_file_store.h>
#include "io/sha512.h"

#include <metrics/storage_metrics.h>


namespace uh::dbn::storage {

class hierarchical_storage : public backend {

public:
    hierarchical_storage(std::filesystem::path db_root, size_t size_bytes, uh::dbn::metrics::storage_metrics& storage_metrics);

    void start() override;

    std::unique_ptr<io::device> read_block(const uh::protocol::blob& hash) override;

    size_t free_space() override;

    size_t used_space() override;

    size_t allocated_space() override;

    std::string backend_type() override;

    std::unique_ptr<uh::protocol::allocation> allocate(std::size_t size) override;

    std::unique_ptr<uh::protocol::allocation> allocate_multi (std::size_t size) override;

    class hierarchical_allocation;
    class hierarchical_multi_block_allocation: public uh::protocol::allocation {
    public:
        explicit hierarchical_multi_block_allocation (hierarchical_storage &storage_backend,
                                                      compressed_file_store& store,
                                                      std::size_t size);

        void open_new_block (std::size_t block_size);
        bool block_is_open ();
        void close_block ();
        io::device& device() override;
        uh::protocol::block_meta_data persist() override;
        ~hierarchical_multi_block_allocation() override;
        hierarchical_multi_block_allocation(const hierarchical_storage&) = delete;
        hierarchical_multi_block_allocation &operator=(const hierarchical_storage &) = delete;
    private:

        hierarchical_storage &m_storage_backend;
        std::unique_ptr <io::temp_file> m_tmp {nullptr};
        std::unique_ptr <io::sha512> m_sha {nullptr};
        std::size_t m_size;
        std::size_t m_block_size {};
        std::size_t m_persisted_size {};
        std::size_t m_effective_size {};
        compressed_file_store& m_store;
    };
private:


    void update_space_consumption();

    void acquire_storage_size (std::size_t size);

    [[nodiscard]] std::filesystem::path get_hash_path (const std::string &hash) const;

    constexpr static std::string_view m_type = "HierarchicalStorage";
    constexpr static unsigned int m_levels = 4;

    const std::filesystem::path m_root;

    const std::size_t m_alloc;
    std::atomic<std::size_t> m_used;
    uh::dbn::metrics::storage_metrics& m_storage_metrics;
    compressed_file_store m_store;
};

}
#endif //CORE_HIERARCHICAL_STORAGE_H
