#define BOOST_TEST_MODULE "etcd manager tests"

#include "common/etcd/utils.h"
#include "fakeit/fakeit.hpp"
#include "utils/system.h"

#include <boost/test/unit_test.hpp>
#include <cstdlib>
#include <functional>
#include <thread>

using namespace fakeit;
using namespace std::chrono_literals;

namespace uh::cluster {

BOOST_AUTO_TEST_SUITE(MockCallbackTestSuite)

BOOST_AUTO_TEST_CASE(watcher_handles_etcd_restart) {
    auto manager = etcd_manager({});
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
