#define BOOST_TEST_MODULE "serialization tests"

#include "common/types/common_types.h"
#include "deduplicator/dedupe_set/fragment_set_log.h"
#include <boost/test/unit_test.hpp>
#include <zpp_bits.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(serialization_frag_set_log_entry) {
    fragment_set_log::log_entry entry_orig(
        INSERT, {0x6465647570, 0x6c69636174696f6e}, 13);

    std::array<char, fragment_set_log::m_entry_size> serData{};
    zpp::bits::out{serData, zpp::bits::size4b{}}(entry_orig).or_throw();
    fragment_set_log::log_entry entry_deserialized;
    zpp::bits::in{serData, zpp::bits::size4b{}}(entry_deserialized).or_throw();

    BOOST_CHECK(entry_orig == entry_deserialized);
}
// ---------------------------------------------------------------------

} // end namespace uh::cluster
