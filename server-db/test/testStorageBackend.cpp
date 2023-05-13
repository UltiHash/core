//
// Created by juan on 01.12.22.
//
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhServerDb Backend Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <metrics/mod.h>
#include <storage/backends/dump_storage.h>
#include <util/temp_dir.h>
#include <storage/backends/hierarchical_storage.h>

namespace {

class storage_fixture {
public:
    static constexpr std::size_t ALLOCATED_BYTES = 1e6;

    storage_fixture()
            : m_metrics_service({}),
              m_metrics(m_metrics_service),
              m_dump(m_tmp.path(), ALLOCATED_BYTES, m_metrics),
              m_hierarchical({ m_tmp.path(), ALLOCATED_BYTES }, m_metrics) {
    }

    uh::dbn::storage::backend &backend() {
        return m_hierarchical;
    }

private:
    uh::util::temp_directory m_tmp;
    uh::metrics::service m_metrics_service;
    uh::dbn::metrics::storage_metrics m_metrics;
    uh::dbn::storage::dump_storage m_dump;
    uh::dbn::storage::hierarchical_storage m_hierarchical;

};

static const std::string CONTENTS_STR = "These are the contents of test_input_file.txt and test_input_file_2.txt";

std::vector<char> to_vector(const std::string& s)
{
    return std::vector<char>(s.begin(), s.end());
}

uh::protocol::block_meta_data integrate_data (const std::vector <char> &data, uh::dbn::storage::backend &storage_backend) {
    auto d1 = to_vector(CONTENTS_STR);
    auto allocation1 = storage_backend.allocate(d1.size());
    allocation1->device().write(d1);
    return allocation1->persist();
}

// ---------------------------------------------------------------------


BOOST_FIXTURE_TEST_CASE( dump_storage_io, storage_fixture )
{
    auto block_md = integrate_data (to_vector(CONTENTS_STR), backend());

    auto read_device = backend().read_block(block_md.hash);
    std::vector <char> fetched_data;
    fetched_data.resize(CONTENTS_STR.size());
    read_device->read(fetched_data);
    auto fetched_hex =  uh::dbn::storage::to_hex_string (fetched_data.begin(), fetched_data.end ());
    auto original_hex =  uh::dbn::storage::to_hex_string (CONTENTS_STR.begin(), CONTENTS_STR.end ());

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

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE( dump_storage_allocation, storage_fixture )
{
    BOOST_CHECK_THROW(backend().allocate(ALLOCATED_BYTES + 1), uh::util::exception);

    {
        auto allocation = backend().allocate(ALLOCATED_BYTES - 1);
        BOOST_CHECK_THROW(backend().allocate(2), std::exception);

        allocation.reset();
        backend().allocate(2);
    }

    {
        auto allocation = backend().allocate(ALLOCATED_BYTES - 1);
        BOOST_CHECK_THROW(backend().allocate(2), std::exception);

        allocation->persist();
        allocation.reset();
        BOOST_CHECK_THROW(backend().allocate(2), std::exception);
    }
}


} // namespace
