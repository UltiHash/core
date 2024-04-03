#define BOOST_TEST_MODULE "data_store tests"

#include "common/utils/common.h"
#include "common/utils/free_spot_manager.h"
#include "common/utils/temp_directory.h"
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

struct fixture {

    void setup() {

        for (auto& offset : m_offsets) {
            const auto n0 = rand() % std::numeric_limits<uint64_t>::max();
            const auto n1 = rand() % std::numeric_limits<uint64_t>::max();
            offset = {n0, n1};
        }

        for (auto& size : m_sizes) {
            size = {rand() % std::numeric_limits<uint64_t>::max()};
        }
    }

    [[nodiscard]] const auto& get_log_file() const { return m_log_file; }

    std::array<uint128_t, 100> m_offsets;
    std::array<std::size_t, 100> m_sizes{};

private:
    temp_directory m_dir;
    std::filesystem::path m_log_file = m_dir.path() / "free_spot_log";
};

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(freespot_fixture_test, fixture)

BOOST_AUTO_TEST_CASE(push_free_spot_test) {

    free_spot_manager fsm(get_log_file(), 0);
    std::size_t expected_total_free_size = 0;

    for (size_t i = 0; i < m_offsets.size(); ++i) {
        fsm.push_free_spot(m_offsets[i], m_sizes[i]);
        expected_total_free_size += m_sizes[i];
    }
    BOOST_TEST(
        (fsm.total_free_spots() == big_int{0, expected_total_free_size}));
}

BOOST_AUTO_TEST_CASE(note_free_spot_test) {

    free_spot_manager fsm(get_log_file(), 0);
    std::size_t expected_total_free_size = 0;

    for (size_t i = 0; i < m_offsets.size(); ++i) {
        fsm.note_free_spot(m_offsets[i], m_sizes[i]);
        expected_total_free_size += m_sizes[i];
    }

    BOOST_TEST((fsm.total_free_spots() == big_int{}));

    fsm.push_noted_free_spots();

    BOOST_TEST(
        (fsm.total_free_spots() == big_int{0, expected_total_free_size}));
}

BOOST_AUTO_TEST_CASE(pop_free_spot_test) {

    free_spot_manager fsm(get_log_file(), 0);
    std::size_t expected_total_free_size = 0;

    int failure = 0;
    for (size_t i = 0; i < m_offsets.size(); ++i) {
        fsm.push_free_spot(m_offsets[i], m_sizes[i]);
        expected_total_free_size += m_sizes[i];

        if (fsm.total_free_spots() != big_int{0, expected_total_free_size}) {
            failure++;
        }
    }
    BOOST_TEST(failure == 0);

    for (int i = m_sizes.size() - 1; i >= 0; i--) {
        fsm.pop_free_spot();
    }
    fsm.apply_popped_items();
    BOOST_TEST((fsm.total_free_spots() == big_int{0, 0}));
}

BOOST_FIXTURE_TEST_CASE(persistent_free_spot_test, fixture) {
    std::size_t expected_total_free_size = 0;

    {
        free_spot_manager fsm(get_log_file(), 0);
        for (size_t i = 0; i < m_offsets.size(); ++i) {
            fsm.push_free_spot(m_offsets[i], m_sizes[i]);
            expected_total_free_size += m_sizes[i];
        }
    }

    {
        free_spot_manager fsm(get_log_file(), 0);
        BOOST_TEST(
            (fsm.total_free_spots() == big_int{0, expected_total_free_size}));
    }
}

BOOST_AUTO_TEST_SUITE_END()

// ---------------------------------------------------------------------

} // end namespace uh::cluster
