//
// Created by masi on 6/5/23.
//

#ifndef CORE_COMBINED_BACKEND_H
#define CORE_COMBINED_BACKEND_H

#include <storage/backend.h>
#include "hierarchical_storage.h"
#include "smart_storage.h"

namespace uh::dbn::storage {

class combined_backend: public backend {

    struct smart_worker {
        smart_storage& m_smart_storage;
        storage_metrics& m_metrics;

        smart_worker(smart_storage& smart, storage_metrics& metrics);
        void operator () (std::filesystem::path path, std::vector<char> sha);
    };

public:

    combined_backend (const hierarchical_storage_config &hierarchical_config,
                      state::scheduled_compressions& scheduled_compressions,
                      storage_metrics &storage_metrics);

    void start() override;

    std::unique_ptr<io::data_generator> read_block(const std::span <char>& hash) override;

    std::pair <std::size_t, std::vector <char>> write_block (const std::span <char>& data) override;

    size_t free_space() override;

    size_t used_space() override;

    size_t allocated_space() override;

    std::string backend_type() override;

private:

    hierarchical_storage_config m_hierarchical_config;
    uh::dbn::metrics::storage_metrics& m_storage_metrics;
    hierarchical_storage m_hierarchical_storage;
    smart_storage m_smart_storage;
    util::job_queue<void, const std::filesystem::path&, const std::vector<char>&> m_worker;
    constexpr static std::string_view m_type = "CombinedStorage";

};
} // end namespace uh::dbn::storage


#endif //CORE_COMBINED_BACKEND_H
