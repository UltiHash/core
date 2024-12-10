#define BOOST_TEST_MODULE "deduplicator tests"

#include "deduplicator/interfaces/local_deduplicator.h"

#include <boost/test/unit_test.hpp>

namespace uh::cluster {

BOOST_AUTO_TEST_CASE(test_used_and_available_space) {
    // auto data_view_config = global_data_view_config();
    auto m_ioc = boost::asio::io_context(boost::asio::io_context(1));
    // auto storage_maintainer = service_maintainer<data_store_mock>();
    // auto data_view = global_data_view(data_view_config, m_ioc, );
    // auto dedup =
    //     local_deduplicator({}, global_data_view)
}

} // namespace uh::cluster
