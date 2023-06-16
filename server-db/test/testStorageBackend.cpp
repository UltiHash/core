//
// Created by juan on 01.12.22.
//
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uh-data-node Backend Tests"
#endif

#include <util/temp_dir.h>
#include <io/buffer.h>
#include <metrics/mod.h>
#include <state/scheduled_compressions_state.h>
#include <storage/backends/hierarchical_storage.h>

#include <boost/test/unit_test.hpp>

namespace {

class storage_fixture {
public:
    static constexpr std::size_t ALLOCATED_BYTES = 1e6;

    storage_fixture()
            : m_metrics_service({}),
              m_metrics(m_metrics_service),
              m_hierarchical({ m_tmp.path(), ALLOCATED_BYTES }, m_metrics, m_scheduled_compressions) {
    }

    uh::dbn::storage::backend &backend() {
        return m_hierarchical;
    }

private:
    uh::util::temp_directory m_tmp;
    uh::metrics::service m_metrics_service;
    uh::dbn::metrics::storage_metrics m_metrics;
    uh::dbn::state::scheduled_compressions_state m_scheduled_compressions;
    uh::dbn::storage::hierarchical_storage m_hierarchical;

};

static const std::string CONTENTS_STR = "These are the contents of test_input_file.txt and test_input_file_2.txt";

std::vector<char> to_vector(const std::string& s)
{
    return {s.begin(), s.end()};
}

uh::protocol::block_meta_data integrate_data (const std::vector <char> &data, uh::dbn::storage::backend &storage_backend) {
    auto d1 = to_vector(CONTENTS_STR);
    auto res = storage_backend.write_block(d1);
    return {res.second, res.first};
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE( dump_storage_io, storage_fixture )
{
    auto block_md = integrate_data(to_vector(CONTENTS_STR), backend());

    auto data = backend().read_block(block_md.hash);
    uh::io::buffer buffer(data->size());

    buffer.write_range(*data);

    auto fetched_hex = uh::dbn::storage::to_hex_string(buffer.data().begin(), buffer.data().end());
    auto original_hex = uh::dbn::storage::to_hex_string(CONTENTS_STR.begin(), CONTENTS_STR.end());

    BOOST_CHECK(fetched_hex == original_hex);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE( dump_storage_no_duplicates, storage_fixture )
{
    auto block_md1 = integrate_data (to_vector(CONTENTS_STR), backend());
    auto block_md2 = integrate_data (to_vector(CONTENTS_STR), backend());

    auto hash1 =  uh::dbn::storage::to_hex_string (block_md1.hash.begin(), block_md1.hash.end ());
    auto hash2 =  uh::dbn::storage::to_hex_string (block_md2.hash.begin(), block_md2.hash.end ());

    BOOST_CHECK(hash1 == hash2);
}


} // namespace
