#define BOOST_TEST_MODULE "global_data_view_fixture tests"

#include <boost/test/unit_test.hpp>

#include "lib/util/gdv_fixture.h"

namespace uh::cluster {

BOOST_FIXTURE_TEST_CASE(invalid_read_fragment, global_data_view_fixture) {
    // Do nothing. I'd like to test it's constructor and destuctor only.
}

} // namespace uh::cluster
