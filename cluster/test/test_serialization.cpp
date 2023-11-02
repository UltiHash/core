//
// Created by max on 9/8/23.
//
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "serialization tests"
#endif

#include <boost/test/unit_test.hpp>
#include <third-party/zpp_bits/zpp_bits.h>
#include "common/common_types.h"

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE (serialization_directory_request_test) {

    directory_message msg_orig;
    msg_orig.bucket_id = "very_important_data";
    msg_orig.object_key = std::make_unique<std::string>("unbelievable_object");
    fragment frag = {
            .pointer = 0x00,
            .size = 42,
        };
    msg_orig.addr = std::make_unique<address>(frag);

    std::vector<char> serData;
    zpp::bits::out{serData}(msg_orig).or_throw();
        directory_message msg_deserialized;
    zpp::bits::in{serData}(msg_deserialized).or_throw();


    BOOST_CHECK(msg_orig == msg_deserialized);

}

// ---------------------------------------------------------------------

} // end namespace uh::cluster