//
// Created by benjamin-elias on 17.05.23.
//


#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibUtil fragmentedDevice Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <util/exception.h>

#include <io/fragment_on_device.h>
#include <io/fragment_on_seekable_device.h>
#include <io/buffer.h>
#include <io/temp_file.h>

#include <serialization/serialization.h>

using namespace uh::util;
using namespace uh::io;

namespace
{
// ---------------------------------------------------------------------

const static std::filesystem::path TEMP_DIR = "/tmp";

// ---------------------------------------------------------------------

const std::string LOREM_IPSUM =
        "Lorem ipsum dolor sit amet, consectetur adipiscing "
        "elit, sed do eiusmod tempor incididunt ut labore et "
        "dolore magna aliqua. Ut enim ad minim veniam, quis "
        "nostrud exercitation ullamco laboris nisi ut "
        "aliquip ex ea commodo consequat. Duis aute irure "
        "dolor in reprehenderit in voluptate velit esse "
        "cillum dolore eu fugiat nulla pariatur. Excepteur "
        "sint occaecat cupidatat non proident, sunt in culpa "
        "qui officia deserunt mollit anim id est laborum.";

// ---------------------------------------------------------------------

struct Fixture {};

// ---------------------------------------------------------------------

typedef boost::mpl::vector<
        fragment_on_device
> device_types_no_seek;

// ---------------------------------------------------------------------

typedef boost::mpl::vector<
        fragment_on_seekable_device
> device_types_seek;

// ---------------------------------------------------------------------

/**
 * To be implemented for each type in `device_types_no_seek` and `device_types_seek`: constructs a device
 * that will read the text given in TEST_TEXT.
 */
template <typename T>
std::unique_ptr<T> make_test_device();

// ---------------------------------------------------------------------

template <>
std::unique_ptr<fragment_on_device> make_test_device<fragment_on_device>()
{
    static std::unique_ptr<buffer> buf;
    buf = std::make_unique<buffer>();

    auto rv = std::make_unique<fragment_on_device>(*buf);

    return rv;
}

// ---------------------------------------------------------------------


BOOST_FIXTURE_TEST_CASE_TEMPLATE( multi_fragment_on_device_test, T, device_types_no_seek, Fixture )
{
    std::unique_ptr<T> fragmented = make_test_device<T>();

    const std::string test_string1(LOREM_IPSUM),
    test_string2(LOREM_IPSUM+"another ipsum");

    fragmented->write(test_string1);
    fragmented->write(test_string2);

    std::unique_ptr<buffer> tb = std::make_unique<buffer>();
    tb->write({test_string1.data(),test_string1.size()});

    auto header_size1 = uh::serialization::serialization<uh::serialization::sl_serializer,
    uh::serialization::sl_deserializer>::sl_serializer::get_header(test_string1.size()).size();
    auto size1 = fragmented->skip();

    BOOST_REQUIRE(size1 == test_string1.size()+header_size1);

    std::vector<char> read_back_second;
    read_back_second.resize(test_string2.size(),0);

    auto size2 = fragmented->read({read_back_second.data(),read_back_second.size()});

    BOOST_REQUIRE(size2 == test_string2.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(test_string2.begin(),test_string2.end(),
                                  read_back_second.begin(),read_back_second.end());

    BOOST_CHECK(!fragmented->valid());
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( fragment_partial_read_exceptions, T, device_types_no_seek, Fixture )
{
    std::unique_ptr<T> fragmented = make_test_device<T>();

    const std::string test_string1(LOREM_IPSUM);

    fragmented->write(test_string1);

    std::string partial_buffer1;
    partial_buffer1.resize(8,0);
    std::string partial_buffer2;
    partial_buffer2.resize(test_string1.size()-8,0);

    auto partial_size1 = fragmented->read({partial_buffer1.data(),partial_buffer1.size()});
    BOOST_REQUIRE_EQUAL(partial_size1,partial_buffer1.size());

    BOOST_REQUIRE_THROW(fragmented->write({partial_buffer1.data(),partial_buffer1.size()}),std::exception);

    auto partial_size2 = fragmented->read({partial_buffer2.data(),partial_buffer2.size()});
    BOOST_REQUIRE_EQUAL(partial_size2,partial_buffer2.size());

    BOOST_CHECK_EQUAL(partial_size1+partial_size2,test_string1.size());
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( multi_fragment_seek_on_device_test, T, device_types_seek , Fixture )
{
    static std::unique_ptr<temp_file> tempFile;
    tempFile = std::make_unique<temp_file>(TEMP_DIR,std::ios_base::in | std::ios_base::out);

    auto fragmented = std::make_unique<T>(*tempFile);

    const std::string test_string1(LOREM_IPSUM),
            test_string2(LOREM_IPSUM+"another ipsum");

    fragmented->write(test_string1);
    fragmented->write(test_string2);

    std::unique_ptr<buffer> tb = std::make_unique<buffer>();
    tb->write({test_string1.data(),test_string1.size()});

    auto header_size1 = uh::serialization::serialization<uh::serialization::sl_serializer,
            uh::serialization::sl_deserializer>::sl_serializer::get_header(test_string1.size()).size();
    auto size1 = fragmented->skip();

    BOOST_REQUIRE(size1 == test_string1.size()+header_size1);

    std::vector<char> read_back_second;
    read_back_second.resize(test_string2.size(),0);

    tempFile->seek(0,std::ios_base::beg);
    auto size2 = fragmented->read({read_back_second.data(),read_back_second.size()});

    BOOST_REQUIRE(size2 == test_string2.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(test_string2.begin(),test_string2.end(),
                                  read_back_second.begin(),read_back_second.end());

    BOOST_CHECK(!fragmented->valid());
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( fragment_partial_read_seek_exceptions, T, device_types_seek , Fixture )
{
    static std::unique_ptr<temp_file> tempFile;
    tempFile = std::make_unique<temp_file>(TEMP_DIR,std::ios_base::in | std::ios_base::out);

    auto fragmented = std::make_unique<T>(*tempFile);

    const std::string test_string1(LOREM_IPSUM);

    fragmented->write(test_string1);

    std::string partial_buffer1;
    partial_buffer1.resize(8,0);
    std::string partial_buffer2;
    partial_buffer2.resize(test_string1.size()-8,0);

    tempFile->seek(0,std::ios_base::beg);
    auto partial_size1 = fragmented->read({partial_buffer1.data(),partial_buffer1.size()});
    BOOST_REQUIRE_EQUAL(partial_size1,partial_buffer1.size());

    BOOST_REQUIRE_THROW(fragmented->write({partial_buffer1.data(),partial_buffer1.size()}),std::exception);

    auto partial_size2 = fragmented->read({partial_buffer2.data(),partial_buffer2.size()});
    BOOST_REQUIRE_EQUAL(partial_size2,partial_buffer2.size());

    BOOST_CHECK_EQUAL(partial_size1+partial_size2,test_string1.size());
}

} // namespace