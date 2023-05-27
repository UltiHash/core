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
#include <io/device.h>
#include <io/temp_file.h>
#include <io/file.h>

#include <test/ipsum.h>

using namespace uh::util;
using namespace uh::io;

namespace
{

// ---------------------------------------------------------------------

const static std::filesystem::path TEMP_DIR = "/tmp";

// ---------------------------------------------------------------------

struct Fixture {};

// ---------------------------------------------------------------------

typedef boost::mpl::vector<
        fragment_on_device,
        fragment_on_seekable_device
> device_types;

// ---------------------------------------------------------------------



// ---------------------------------------------------------------------

/**
 * To be implemented for each type in `device_types`: constructs a device
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

template <>
std::unique_ptr<fragment_on_seekable_device> make_test_device<fragment_on_seekable_device>()
{
    static std::unique_ptr<temp_file> temp_buf = std::make_unique<temp_file>
            (TEMP_DIR,std::ios_base::in | std::ios_base::out);
    auto rv = std::make_unique<fragment_on_seekable_device>(*temp_buf);

    return rv;
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( multi_fragment_on_device_skip_test, T, device_types, Fixture )
{
    std::unique_ptr<T> fragmented = make_test_device<T>();
    static std::unique_ptr<temp_file> temp_buf;
    if constexpr (std::is_same_v<T,fragment_on_seekable_device>){
        temp_buf = std::make_unique<temp_file>
                (TEMP_DIR,std::ios_base::in | std::ios_base::out);
        fragmented = std::make_unique<T>(*temp_buf);
    }

    const std::string test_string1(uh::test::LOREM_IPSUM),
    test_string2(uh::test::LOREM_IPSUM+"another ipsum");

    fragmented->write(test_string1);
    fragmented->write(test_string2);

    std::unique_ptr<buffer> tb = std::make_unique<buffer>();
    tb->write({test_string1.data(),test_string1.size()});

    if constexpr (std::is_same_v<T,fragment_on_seekable_device>){
        temp_buf->seek(0,std::ios_base::beg);
    }

    auto size1 = fragmented->skip();

    BOOST_REQUIRE(size1.content_size == test_string1.size());

    BOOST_CHECK(!fragmented->valid());
    fragmented->reset();
    BOOST_CHECK(fragmented->valid());

    std::vector<char> read_back_second;
    read_back_second.resize(test_string2.size(),0);

    auto size2 = fragmented->read({read_back_second.data(),read_back_second.size()});

    BOOST_REQUIRE(size2.content_size == test_string2.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(test_string2.begin(),test_string2.end(),
                                  read_back_second.begin(),read_back_second.end());

    BOOST_CHECK(!fragmented->valid());//implement eof detection to unvalidate
    fragmented->reset();

    if constexpr (std::is_same_v<T,fragment_on_seekable_device>){
        BOOST_CHECK(fragmented->valid());
    }
    else{
        BOOST_CHECK(!fragmented->valid());
    }

    auto size3 = fragmented->read({read_back_second.data(),read_back_second.size()});
    BOOST_REQUIRE_EQUAL(size3.content_size,0);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( fragment_partial_read_write_exceptions, T, device_types, Fixture )
{
    if constexpr (std::is_same_v<T,fragment_on_seekable_device>)
    {
        std::unique_ptr<T> fragmented, fragmented_read;

        static std::unique_ptr<temp_file> temp_buf = std::make_unique<temp_file>
                (TEMP_DIR,std::ios_base::in | std::ios_base::out);
        fragmented = std::make_unique<T>(*temp_buf);
        fragmented_read = std::make_unique<T>(*temp_buf);

        std::streamsize written{},read{};

        do{
            buffer small_buf(uh::test::LOREM_IPSUM.size()/3);

            auto maximum_writeable = std::min(small_buf.data().size(),
                                              static_cast<std::size_t>(std::distance(uh::test::LOREM_IPSUM.cbegin()+written,
                                                                                     uh::test::LOREM_IPSUM.cend())));

            uh::io::write(small_buf,{uh::test::LOREM_IPSUM.data()+written,maximum_writeable});

            bool first_write = !written;
            written += fragmented->write(small_buf.data(),uh::test::LOREM_IPSUM.size()).content_size;

            if(first_write){
                std::vector<char> throw_buffer(uh::test::LOREM_IPSUM.size());
                try{
                    fragmented->read({throw_buffer.data(),throw_buffer.size()});
                    BOOST_REQUIRE(false);
                }
                catch (std::exception& e){
                    BOOST_REQUIRE(true);
                }
            }
        }
        while(written != uh::test::LOREM_IPSUM.size());

        BOOST_CHECK_EQUAL(written,uh::test::LOREM_IPSUM.size());

        std::string test_read{};

        temp_buf->seek(0,std::ios_base::beg);

        do{
            std::vector<char> small_buf(uh::test::LOREM_IPSUM.size()/4);

            auto maximum_readable = std::min(small_buf.size(),
                                             static_cast<std::size_t>(std::distance(uh::test::LOREM_IPSUM.cbegin()+read,
                                                                                    uh::test::LOREM_IPSUM.cend())));

            bool first_read = !read;

            read += fragmented_read->read(small_buf).content_size;
            if(first_read){
                try{
                    fragmented_read->write(small_buf);
                    BOOST_REQUIRE(false);
                }
                catch (std::exception& e){
                    BOOST_REQUIRE(true);
                }
            }

            test_read.append(small_buf.data(),maximum_readable);
        }
        while(fragmented_read->valid());

        BOOST_CHECK_EQUAL(read,uh::test::LOREM_IPSUM.size());
        BOOST_CHECK_EQUAL(test_read, uh::test::LOREM_IPSUM);
    }
    else
    {
        std::unique_ptr<T> fragmented, fragmented_read;

        static std::unique_ptr<buffer> temp_buf = std::make_unique<buffer>();
        fragmented = std::make_unique<T>(*temp_buf);
        fragmented_read = std::make_unique<T>(*temp_buf);

        std::streamsize written{},read{};

        do{
            buffer small_buf(uh::test::LOREM_IPSUM.size()/3);

            auto maximum_writeable = std::min(small_buf.data().size(),
                                              static_cast<std::size_t>(
                                                      std::distance(uh::test::LOREM_IPSUM.cbegin()+written,
                                                                    uh::test::LOREM_IPSUM.cend())));

            uh::io::write(small_buf,{uh::test::LOREM_IPSUM.data()+written,maximum_writeable});

            bool first_write = !written;
            written += fragmented->write(small_buf.data(),uh::test::LOREM_IPSUM.size()).content_size;

            if(first_write){
                std::vector<char> throw_buffer(uh::test::LOREM_IPSUM.size());
                try{
                    fragmented->read({throw_buffer.data(),throw_buffer.size()});
                    BOOST_REQUIRE(false);
                }
                catch (std::exception& e){
                    BOOST_REQUIRE(true);
                }
            }
        }
        while(written != uh::test::LOREM_IPSUM.size());

        BOOST_CHECK_EQUAL(written,uh::test::LOREM_IPSUM.size());

        std::string test_read{};

        do{
            std::vector<char> small_buf(uh::test::LOREM_IPSUM.size()/4);

            auto maximum_readable = std::min(small_buf.size(),
                                             static_cast<std::size_t>(
                                                     std::distance(uh::test::LOREM_IPSUM.cbegin()+read,
                                                                   uh::test::LOREM_IPSUM.cend())));

            bool first_read = !read;

            read += fragmented_read->read(small_buf).content_size;
            if(first_read){
                try{
                    fragmented_read->write(small_buf);
                    BOOST_REQUIRE(false);
                }
                catch (std::exception& e){
                    BOOST_REQUIRE(true);
                }
            }

            test_read.append(small_buf.data(),maximum_readable);
        }
        while(fragmented_read->valid());

        BOOST_CHECK_EQUAL(read,uh::test::LOREM_IPSUM.size());
        BOOST_CHECK_EQUAL(test_read, uh::test::LOREM_IPSUM);
    }

}

// ---------------------------------------------------------------------

} // namespace