//
// Created by juan on 01.12.22.
//
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhServerDb Dummy Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <metrics/mod.h>
#include <storage/backends/dump_storage.h>
#include <util/temp_dir.h>

namespace
{

// ---------------------------------------------------------------------

class storage_fixture
{
public:
    static constexpr std::size_t ALLOCATED_BYTES = 1e6;

    storage_fixture()
        : m_metrics_service({}),
          m_metrics(m_metrics_service),
          m_dump(m_tmp.path(), ALLOCATED_BYTES, m_metrics)
    {
    }

    uh::dbn::storage::backend& backend()
    {
        return m_dump;
    }

private:
    uh::util::temp_directory m_tmp;
    uh::metrics::service m_metrics_service;
    uh::dbn::metrics::storage_metrics m_metrics;
    uh::dbn::storage::dump_storage m_dump;
};

// ---------------------------------------------------------------------

static const std::string CONTENTS_STR = "These are the contents of test_input_file.txt and test_input_file_2.txt";
static const std::string EXPECTED_SHA512_HASH = "2610fa1ed2dc40f92a3e44cb894b757e4e4469a053b5b2ccf69179b577cfac29403aed645ecab45e10c5db2d9c6bbb0916b0b7c9caa635d271f5274b3e868011";

// ---------------------------------------------------------------------

std::vector<char> to_vector(const std::string& s)
{
    return std::vector<char>(s.begin(), s.end());
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( hashing_function_expected_hash )
{
    uh::protocol::blob vec_hash = uh::dbn::storage::sha512(to_vector(CONTENTS_STR));
    std::string hash_string = uh::dbn::storage::to_hex_string(vec_hash.begin(), vec_hash.end());

    BOOST_CHECK(hash_string == EXPECTED_SHA512_HASH);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE( dump_storage_io, storage_fixture )
{
    uh::protocol::blob x = to_vector(CONTENTS_STR);

    // Write block.
    uh::protocol::blob hash_key = backend().write_block(x);

    // Read block.
    uh::protocol::blob y = uh::io::read_to_buffer(*backend().read_block(hash_key));

    BOOST_CHECK(x == y);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE( dump_storage_no_duplicates, storage_fixture )
{
    uh::protocol::blob x = backend().write_block(to_vector(CONTENTS_STR));
    uh::protocol::blob y = backend().write_block(to_vector(CONTENTS_STR));

    BOOST_CHECK(x == y);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE( dump_storage_expected_hash, storage_fixture )
{
    uh::protocol::blob x = backend().write_block(to_vector(CONTENTS_STR));
    std::string x_str = uh::dbn::storage::to_hex_string(x.begin(), x.end());

    BOOST_CHECK(x_str == EXPECTED_SHA512_HASH);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE( dump_storage_allocation, storage_fixture )
{
    BOOST_CHECK_THROW(backend().allocate(ALLOCATED_BYTES + 1), uh::util::exception);

    auto allocation = backend().allocate(ALLOCATED_BYTES - 1);
    BOOST_CHECK_THROW(backend().allocate(2), std::exception);

    allocation.reset();
    backend().allocate(2);
}

// ---------------------------------------------------------------------

} // namespace
