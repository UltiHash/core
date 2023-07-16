//
// Created by benjamin-elias on 22.05.23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibSerialization FragmentSerialization Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <serialization/fragment_size_struct.h>
#include <io/device.h>
#include <io/buffer.h>

using namespace uh::serialization;
using namespace uh::io;

// ------------- Tests Follow --------------

BOOST_AUTO_TEST_CASE(test_fragment_format_serialization)
{
    uh::serialization::fragment_serialize_size_format some_metadata(3,400), read_meta;
    uh::io::buffer buf;

    uh::io::write(buf, some_metadata.serialize());

    read_meta.deserialize(buf);

    BOOST_CHECK(some_metadata.index_num == read_meta.index_num);
    BOOST_CHECK(some_metadata.content_buf_size == read_meta.content_buf_size);
    BOOST_CHECK(some_metadata.content_size == read_meta.content_size);
}

