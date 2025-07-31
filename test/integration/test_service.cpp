#define BOOST_TEST_MODULE "service tests"

#include <algorithm>
#include <common/ec/reedsolomon_c.h>
#include <common/telemetry/log.h>
#include <common/types/common_types.h>
#include <common/utils/time_utils.h>
#include <deduplicator/service.h>
#include <entrypoint/service.h>
#include <util/gdv_fixture.h>
#include <util/random.h>

#include <boost/test/unit_test.hpp>

#include "test_config.h"

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_FIXTURE_TEST_SUITE(rr_storage, global_data_view_fixture)

BOOST_AUTO_TEST_CASE(supports_repeated_killing_and_reviving_one_storage) {
    auto config = get_group_config();

    for (auto k = 0ul; k < 100; ++k) {
        LOG_WARN() << "## iteration " << k;
        deactivate_storage(config.storages - 1);
        activate_storage(config.storages - 1);
    }
}

BOOST_AUTO_TEST_CASE(supports_repeated_killing_and_reviving_all_storages) {
    auto config = get_group_config();

    for (auto k = 0ul; k < 20; ++k) {
        LOG_WARN() << "## iteration " << k;
        for (auto i = 0ul; i < config.storages; ++i) {
            LOG_DEBUG() << "kill storage " << i;
            deactivate_storage(i);
        }
        for (auto i = 0ul; i < config.storages; ++i) {
            LOG_DEBUG() << "revive storage " << i;
            activate_storage(i);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

struct ec_fixture : public global_data_view_fixture {

    ec_fixture()
        : global_data_view_fixture({
              .id = 0,
              .type = storage::group_config::type_t::ERASURE_CODING,
              .storages = 6,
              .data_shards = 4,
              .parity_shards = 2,
              .stripe_size_kib = 4 * 2,
          }) {}
};

BOOST_FIXTURE_TEST_SUITE(ec_storage, ec_fixture)

BOOST_AUTO_TEST_CASE(supports_repeated_killing_and_reviving) {
    auto config = get_group_config();

    for (auto k = 0ul; k < 100; ++k) {
        LOG_WARN() << "## iteration " << k;
        LOG_WARN() << "### Destroy storage...";
        deactivate_storage(config.storages - 1);
        LOG_WARN() << "### Create storage...";
        activate_storage(config.storages - 1);
    }
}

BOOST_AUTO_TEST_CASE(supports_repeated_killing_and_reviving_all_storages) {
    auto config = get_group_config();

    for (auto k = 0ul; k < 20; ++k) {
        LOG_WARN() << "## iteration " << k;
        for (auto i = 0ul; i < config.storages; ++i) {
            LOG_DEBUG() << "kill storage " << i;
            deactivate_storage(i);
        }
        for (auto i = 0ul; i < config.storages; ++i) {
            LOG_DEBUG() << "revive storage " << i;
            activate_storage(i);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
