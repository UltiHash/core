#define BOOST_TEST_MODULE "disk-cache object_handle tests"

#include <boost/test/unit_test.hpp>
#include <chrono>
#include <common/types/address.h>
#include <proxy/cache/disk/object.h>
#include <thread>

namespace uh::cluster::proxy::cache::disk {

BOOST_AUTO_TEST_SUITE(a_disk_cache_object_handle)

BOOST_AUTO_TEST_CASE(expire_and_refresh) {
    address addr;
    addr.push({456, 20});
    object_handle objh(address(addr), "etag-exp", std::chrono::seconds(1));

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    BOOST_TEST(objh.is_expired());

    objh.touch(std::chrono::seconds(1));
    BOOST_TEST(!objh.is_expired());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::proxy::cache::disk
