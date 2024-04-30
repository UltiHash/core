#define BOOST_TEST_MODULE "serialization tests"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"

#include "common/types/common_types.h"
#include "deduplicator/dedupe_set/fragment_set_log.h"
#include <boost/test/unit_test.hpp>
#include <third-party/zpp_bits/zpp_bits.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(serialization_directory_request_test) {

    directory_message msg_orig;
    msg_orig.bucket_id = "very_important_data";
    msg_orig.object_key = std::make_unique<std::string>("unbelievable_object");
    fragment frag = {
        .pointer = 0x00,
        .size = 42,
    };
    msg_orig.addr = std::make_unique<address>(frag);

    std::vector<char> serData;
    zpp::bits::out{serData, zpp::bits::size4b{}}(msg_orig).or_throw();
    directory_message msg_deserialized;
    zpp::bits::in{serData, zpp::bits::size4b{}}(msg_deserialized).or_throw();

    BOOST_CHECK(msg_orig == msg_deserialized);
}

BOOST_AUTO_TEST_CASE(serialization_frag_set_log_entry) {
    fragment_set_log::log_entry entry_orig{
        .op = INSERT,
        .pointer = {0x6465647570, 0x6c69636174696f6e},
        .size = 13};

    std::array<char, fragment_set_log::m_entry_size> serData{};
    zpp::bits::out{serData, zpp::bits::size4b{}}(entry_orig).or_throw();
    fragment_set_log::log_entry entry_deserialized;
    zpp::bits::in{serData, zpp::bits::size4b{}}(entry_deserialized).or_throw();

    BOOST_CHECK(entry_orig == entry_deserialized);
}
// ---------------------------------------------------------------------

} // end namespace uh::cluster
#pragma GCC diagnostic pop
