//
// Created by masi on 6/5/23.
//

#ifndef CORE_COMBINED_BACKEND_H
#define CORE_COMBINED_BACKEND_H

#include <storage/backend.h>
#include "hierarchical_storage.h"
#include <storage/backends/smart_backend/smart_storage.h>

namespace uh::dbn::storage {

class combined_backend: public backend {

    struct smart_worker {
        void operator () (std::filesystem::path path, std::vector<char> sha);
    };

public:

    void start() override;

    std::unique_ptr<io::data_generator> read_block(const std::span <char>& hash) override;

    std::pair <std::size_t, std::vector <char>> write_block (const std::span <char>& data) override;

    size_t free_space() override;

    size_t used_space() override;

    size_t allocated_space() override;

    std::string backend_type() override;

    std::unique_ptr<uh::protocol::allocation> allocate (std::size_t size) override;

    std::unique_ptr<uh::protocol::allocation> allocate_multi (std::size_t size) override;

private:

    hierarchical_storage m_hierarchical_storage;
    smart::smart_storage m_smart_storage;
    util::job_queue<void, const std::filesystem::path&, const std::vector<char>&> m_worker;
    constexpr static std::string_view m_type = "CombinedStorage";

};
} // end namespace uh::dbn::storage


#endif //CORE_COMBINED_BACKEND_H
