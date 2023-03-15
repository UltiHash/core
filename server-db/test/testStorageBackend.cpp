//
// Created by juan on 01.12.22.
//
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhServerDb Dummy Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>
#include <metrics/mod.h>
#include <storage/backends/dump_storage.h>
#include <storage/backends/tuesday/tuesday_dedup.h>
#include <hash/sha512.h>
#include <util/temp_dir.h>

namespace
{

// ---------------------------------------------------------------------

using namespace uh::dbn::storage;

// ---------------------------------------------------------------------

template <typename storage>
storage create_storage(const std::filesystem::path& path,
                       std::size_t alloc_size,
                       uh::dbn::metrics::storage_metrics& metrics);

// ---------------------------------------------------------------------

template <>
dump_storage create_storage<dump_storage>(
    const std::filesystem::path& path,
    std::size_t alloc_size,
    uh::dbn::metrics::storage_metrics& metrics)
{
    return dump_storage(path, alloc_size, metrics);
}

// ---------------------------------------------------------------------

template <>
tuesday_dedup create_storage<tuesday_dedup>(
    const std::filesystem::path&,
    std::size_t alloc_size,
    uh::dbn::metrics::storage_metrics& metrics)
{
    return tuesday_dedup(metrics, alloc_size);
}

// ---------------------------------------------------------------------

static constexpr std::size_t ALLOCATED_BYTES = 1e6;

// ---------------------------------------------------------------------

template <typename storage>
class storage_fixture
{
public:

    storage_fixture()
        : m_metrics_service({}),
          m_metrics(m_metrics_service),
          m_storage(create_storage<storage>(m_tmp.path(), ALLOCATED_BYTES, m_metrics))
    {
    }

    uh::dbn::storage::backend& backend()
    {
        return m_storage;
    }

    uh::protocol::blob read_block(const uh::protocol::blob& hash)
    {
        auto device = backend().read_block(hash);
        return uh::io::read_to_buffer(*device);
    }

    uh::protocol::blob write_block(std::vector<char> block)
    {
        auto alloc = backend().allocate(block.size());
        uh::io::write_from_buffer(alloc->device(), { block.begin(), block.end() });
        return alloc->persist().hash;
    }

private:
    uh::util::temp_directory m_tmp;
    uh::metrics::service m_metrics_service;
    uh::dbn::metrics::storage_metrics m_metrics;
    storage m_storage;
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

typedef boost::mpl::vector<
    dump_storage,
    tuesday_dedup
    > storage_types;

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( hashing_function_expected_hash )
{
    uh::protocol::blob vec_hash = uh::hash::sha512_digest(CONTENTS_STR);
    std::string hash_string = uh::dbn::storage::to_hex_string(vec_hash.begin(), vec_hash.end());

    BOOST_CHECK(hash_string == EXPECTED_SHA512_HASH);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( storage_io, T, storage_types, storage_fixture<T>)
{
    uh::protocol::blob x = to_vector(CONTENTS_STR);

    // Write block.
    uh::protocol::blob hash_key = this->write_block(x);

    // Read block.
    uh::protocol::blob y = this->read_block(hash_key);

    BOOST_CHECK(x == y);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( storage_no_duplicates, T, storage_types, storage_fixture<T>)
{
    uh::protocol::blob x = this->write_block(to_vector(CONTENTS_STR));
    uh::protocol::blob y = this->write_block(to_vector(CONTENTS_STR));

    BOOST_CHECK(x == y);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( storage_expected_hash, T, storage_types, storage_fixture<T>)
{
    uh::protocol::blob x = this->write_block(to_vector(CONTENTS_STR));
    std::string x_str = uh::dbn::storage::to_hex_string(x.begin(), x.end());

    BOOST_CHECK(x_str == EXPECTED_SHA512_HASH);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( storage_allocation, T, storage_types, storage_fixture<T>)
{
    auto& backend = this->backend();

    BOOST_CHECK_THROW(backend.allocate(ALLOCATED_BYTES + 1), uh::util::exception);

    {
        auto allocation = backend.allocate(ALLOCATED_BYTES - 1);
        BOOST_CHECK_THROW(backend.allocate(2), std::exception);

        allocation.reset();
        backend.allocate(2);
    }

    {
        auto allocation = backend.allocate(ALLOCATED_BYTES - 1);
        BOOST_CHECK_THROW(backend.allocate(2), std::exception);

        allocation->persist();
        allocation.reset();
        BOOST_CHECK_THROW(backend.allocate(2), std::exception);
    }
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( storage_dedup, T, storage_types, storage_fixture<T>)
{
    auto& backend = this->backend();
    auto block = to_vector(CONTENTS_STR);

    {
        auto alloc = backend.allocate(block.size());
        uh::io::write_from_buffer(alloc->device(), { block.begin(), block.end() });
        auto meta_data = alloc->persist();
        BOOST_CHECK(meta_data.effective_size == block.size());
    }

    {
        auto alloc = backend.allocate(block.size());
        uh::io::write_from_buffer(alloc->device(), { block.begin(), block.end() });
        auto meta_data = alloc->persist();
        BOOST_CHECK(meta_data.effective_size == 0u);
    }
}

// ---------------------------------------------------------------------

} // namespace
