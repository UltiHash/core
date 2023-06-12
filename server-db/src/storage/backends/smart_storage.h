//
// Created by masi on 5/30/23.
//

#ifndef CORE_SMART_STORAGE_H
#define CORE_SMART_STORAGE_H

#include <vector>

#include "storage/backends/smart_backend/smart_config.h"
#include "storage/backends/smart_backend/smart_core.h"
#include "storage/backend.h"
#include "io/span_generator.h"
#include <openssl/evp.h>
#include <openssl/sha.h>


namespace uh::dbn::storage::smart {
    smart_config make_smart_config (const std::filesystem::path& root, size_t size, size_t max_file_size);
} // end namespace uh::dbn::storage::smart

namespace uh::dbn::storage {

class smart_storage : public uh::dbn::storage::backend {

public:

    explicit smart_storage (const smart::smart_config& smart_conf, uh::dbn::metrics::storage_metrics& storage_metrics);

    void start() override;

    void stop() override;

    std::unique_ptr<io::data_generator> read_block(const std::span <char>& hash) override;

    std::pair <std::size_t, std::vector <char>> write_block (const std::span <char>& data) override;

    size_t free_space() override;

    size_t used_space() override;

    size_t allocated_space() override;

    std::string backend_type() override;

private:

    void update_space_consumption();

    const smart::smart_config m_smart_conf;
    smart::smart_core m_smart_core;
    const std::size_t m_size;
    std::atomic <std::size_t> m_used;
    constexpr static std::string_view m_type = "SmartStorage";
    EVP_MD_CTX* m_sha_ctx;
    std::shared_mutex m_mutex;
    uh::dbn::metrics::storage_metrics& m_storage_metrics;

};
} // end namespace uh::dbn::storage
#endif //CORE_SMART_STORAGE_H
