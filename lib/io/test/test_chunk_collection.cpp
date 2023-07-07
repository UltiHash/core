//
// Created by benjamin-elias on 27.05.23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibUtil ChunkCollection Tests"
#endif

#include <test/ipsum.h>
#include <util/exception.h>
#include <io/temp_file.h>
#include <io/chunk_collection.h>
#include <serialization/fragment_size_struct.h>

#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <limits>

using namespace uh::util;
using namespace uh::io;
using namespace uh::test;

namespace
{

// ---------------------------------------------------------------------

const std::filesystem::path TEMP_DIR = "/tmp";

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(temporary_chunk_collecion)
{
    std::filesystem::path work_path;

    {
        chunk_collection temporary_chunk_collection(TEMP_DIR, true);
        work_path = temporary_chunk_collection.getPath();
        BOOST_REQUIRE(std::filesystem::exists(work_path));
    }
    BOOST_REQUIRE(!std::filesystem::exists(work_path));

    {
        chunk_collection temporary_chunk_collection2(TEMP_DIR, true);
        temporary_chunk_collection2.release_to(temporary_chunk_collection2.getPath());
        work_path = temporary_chunk_collection2.getPath();
    }
    BOOST_REQUIRE(std::filesystem::exists(work_path));

    chunk_collection file_chunk_collection(work_path);

    std::filesystem::remove(work_path);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(write_read_chunk_collection)
{
    BOOST_CHECK_THROW(chunk_collection(std::filesystem::path(TEMP_DIR)), std::exception);
    chunk_collection cc(TEMP_DIR, true);

    for (uint16_t i = 0; i <= std::numeric_limits<uint8_t>::max() + 1; i++)
    {
        std::string input = uh::test::LOREM_IPSUM + std::to_string(i);

        if (i == std::numeric_limits<uint8_t>::max() + 1)
        {
            BOOST_REQUIRE_THROW(cc.write_indexed(input), std::exception);
            break;
        }
        else
        {
            auto check_header = cc.write_indexed(input);
            BOOST_CHECK_EQUAL(check_header.header_size, 4);
        }

        auto read_back = cc.read_indexed(static_cast<uint8_t>(i));
        BOOST_REQUIRE_EQUAL_COLLECTIONS(input.begin(), input.end(),
                                        read_back.first.begin(), read_back.first.end());
        BOOST_CHECK_EQUAL(read_back.second.header_size, 4);
        BOOST_CHECK_EQUAL(cc.size(i), input.size() + read_back.second.header_size);
    }

    std::string input3 = uh::test::LOREM_IPSUM + std::to_string(3);
    auto read_back3 = cc.read_indexed(3);

    BOOST_CHECK_EQUAL_COLLECTIONS(input3.begin(), input3.end(),
                                  read_back3.first.begin(), read_back3.first.end());
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(write_read_multi_chunk_collection)
{
    chunk_collection cc(TEMP_DIR, true);

    std::vector<std::string> to_write;

    for (uint16_t i = 0; i < std::numeric_limits<uint8_t>::max() + 1; i++)
    {
        std::string input = uh::test::LOREM_IPSUM + std::to_string(i);
        to_write.push_back(input);
    }

    std::vector<std::span<const char>> to_write_span;

    to_write_span.reserve(to_write.size());

    for (const auto& item : to_write)
    {
        to_write_span.emplace_back(item.data(), item.size());
    }

    cc.write_indexed_multi(to_write_span);

    auto valid_indexes = cc.get_index_num_content_list();
    std::vector<uint8_t> valid_indexes_simulation(valid_indexes.size());
    for (uint16_t i2 = 0; valid_indexes.size() > static_cast<std::size_t>(i2); i2++)
    {
        valid_indexes_simulation[i2] = i2;
    }

    BOOST_REQUIRE_EQUAL_COLLECTIONS(valid_indexes.cbegin(), valid_indexes.cend(),
                                    valid_indexes_simulation.cbegin(), valid_indexes_simulation.cend());

    std::ranges::reverse(valid_indexes.begin(), valid_indexes.end());

    BOOST_REQUIRE_THROW(cc.read_indexed_multi(std::vector<uint8_t>{0, 0}), std::exception);

    auto read_all_back = cc.read_indexed_multi(valid_indexes);

    auto to_write_reverse_beg = to_write.crbegin();
    for (const auto& item : read_all_back)
    {
        BOOST_CHECK_EQUAL_COLLECTIONS(item.first.cbegin(), item.first.cend(),
                                      to_write_reverse_beg->cbegin(), to_write_reverse_beg->cend());
        to_write_reverse_beg++;
    }
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(remove_fragment_multi_chunk_collection)
{
    chunk_collection cc(TEMP_DIR, true);

    std::vector<std::string> to_write;

    for (uint16_t i = 0; i < std::numeric_limits<uint8_t>::max() + 1; i++)
    {
        std::string input = uh::test::LOREM_IPSUM + std::to_string(i);
        to_write.push_back(input);
    }

    std::vector<std::span<const char>> to_write_span;

    to_write_span.reserve(to_write.size());

    for (const auto& item : to_write)
    {
        to_write_span.emplace_back(item.data(), item.size());
    }

    cc.write_indexed_multi(to_write_span);

    cc.remove(3);

    auto valid_indexes = cc.get_index_num_content_list();
    std::vector<uint8_t> valid_indexes_simulation;

    for (uint16_t i2 = 0; valid_indexes.size() + 1 > static_cast<std::size_t>(i2); i2++)
    {
        if (i2 == 3)
        {
            continue;
        }
        valid_indexes_simulation.push_back(i2);
    }

    BOOST_REQUIRE_EQUAL_COLLECTIONS(valid_indexes.cbegin(), valid_indexes.cend(),
                                    valid_indexes_simulation.cbegin(), valid_indexes_simulation.cend());

    auto read_all_back = cc.read_indexed_multi(valid_indexes);

    auto to_write_beg = to_write.cbegin();
    for (const auto& item : read_all_back)
    {
        auto copy_beg_it = to_write_beg;
        to_write_beg++;

        if (std::distance(to_write.cbegin(), to_write_beg) == 3)
        {
            to_write_beg++;
        }

        BOOST_CHECK_EQUAL_COLLECTIONS(item.first.cbegin(), item.first.cend(),
                                      copy_beg_it->cbegin(), copy_beg_it->cend());

    }
}

// ---------------------------------------------------------------------

} // namespace

