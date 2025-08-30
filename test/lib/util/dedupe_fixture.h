#pragma once

#include <deduplicator/interfaces/local_deduplicator.h>
#include <mock/storage/mock_data_view.h>
#include <util/coroutine.h>
#include <util/temp_directory.h>

#define MAX_DATA_STORE_SIZE_BYTES (4 * MEBI_BYTE)
#define MAX_FILE_SIZE_BYTES (128 * KIBI_BYTE)
#define DATA_STORE_ID 1

namespace uh::cluster {

struct dedupe_fixture : public coro_fixture {
    dedupe_fixture()
        : coro_fixture(2),
          dir{},
          config{.max_file_size = MAX_FILE_SIZE_BYTES,
                 .max_data_store_size = MAX_DATA_STORE_SIZE_BYTES,
                 .page_size = DEFAULT_PAGE_SIZE},
          data_store{config, dir.path().string(), DATA_STORE_ID, 0},
          data_view{data_store},
          cache{m_ioc, data_view, 4000ul},
          dedup{{}, data_view, cache} {

        auto log_config = log::config{
            .sinks = {log::sink_config{.type = log::sink_type::cout,
                                       .level = boost::log::trivial::fatal,
                                       .service_role = DEDUPLICATOR_SERVICE}}};
        log::init(log_config);
    }

    temp_directory dir;
    data_store_config config;
    mock_data_store data_store;
    mock_data_view data_view;
    storage::global::cache cache;
    local_deduplicator dedup;
};

} // namespace uh::cluster
