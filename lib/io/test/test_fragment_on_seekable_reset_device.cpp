//
// Created by benjamin-elias on 27.05.23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibUtil TempFile Tests"
#endif

#include <test/ipsum.h>
#include <util/exception.h>
#include <io/temp_file.h>
#include <io/fragment_on_seekable_reset_device.h>
#include <io/buffer.h>

#include <boost/test/unit_test.hpp>


using namespace uh::util;
using namespace uh::io;
using namespace uh::test;

namespace
{

// ---------------------------------------------------------------------

const static std::filesystem::path TEMP_DIR = "/tmp";

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_reset_device)
{
    static std::unique_ptr<temp_file> temp_buf = std::make_unique<temp_file>
        (TEMP_DIR, std::ios_base::in | std::ios_base::out);
    std::unique_ptr<fragment_on_seekable_reset_device> fragmented =
        std::make_unique<fragment_on_seekable_reset_device>(*temp_buf);

    const std::string test_string1(uh::test::LOREM_IPSUM),
        test_string2(uh::test::LOREM_IPSUM + "another ipsum");

    auto written_serialized_size1 =
        fragmented->write(test_string1);
    fragmented->write(test_string2);

    std::unique_ptr<buffer> tb = std::make_unique<buffer>();
    tb->write({test_string1.data(), test_string1.size()});

    fragmented->reset();
    auto size1 = fragmented->skip();

    BOOST_REQUIRE(size1.content_size == test_string1.size());

    BOOST_CHECK(fragmented->valid());
    fragmented->reset();
    BOOST_CHECK(fragmented->valid());

    std::vector<char> read_back_second;
    read_back_second.resize(test_string1.size(), 0);

    auto size2 = fragmented->read({read_back_second.data(), read_back_second.size()});

    BOOST_REQUIRE(size2.content_size == test_string1.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(test_string1.begin(), test_string1.end(),
                                  read_back_second.begin(), read_back_second.end());

    BOOST_CHECK(!fragmented->valid());//implement eof detection to unvalidate
    fragmented->reset();
    BOOST_CHECK(fragmented->valid());

    std::unique_ptr<fragment_on_seekable_reset_device> fragmented_back =
        std::make_unique<fragment_on_seekable_reset_device>
            (*temp_buf, 0,
             written_serialized_size1.header_size +
                 written_serialized_size1.content_size);

    fragmented_back->reset();

    read_back_second.resize(test_string2.size(), 0);
    auto size_back = fragmented->read({read_back_second.data(), read_back_second.size()});

    BOOST_REQUIRE(size_back.content_size == test_string2.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(test_string2.begin(), test_string2.end(),
                                  read_back_second.begin(), read_back_second.end());
}

// ---------------------------------------------------------------------

} // namespace

