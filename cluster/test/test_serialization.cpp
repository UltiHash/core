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

BOOST_AUTO_TEST_CASE (directory_request_serialization) {

    directory_request req_orig;
    req_orig.bucket_id = "very_important_data";
    req_orig.object_key = "unbelievable_object";
    fragment frag;
    frag.size = 42;
    frag.pointer = 0xAFFEAFFEAFFEAFFE;
    req_orig.addr = address(frag);

    std::vector<char> serData;
    zpp::bits::out{serData}(req_orig).or_throw();
    directory_request req_deserialized;
    zpp::bits::in{serData}(req_deserialized).or_throw();


    BOOST_CHECK(req_orig == req_deserialized);

}

// ---------------------------------------------------------------------

} // end namespace uh::cluster