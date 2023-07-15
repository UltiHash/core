#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibSerialization Serialization Tests"
#endif

#include <boost/test/unit_test.hpp>
#include "serialization/index_fragment_serializer.h"
#include "serialization/index_fragment_deserializer.h"
#include "serialization/index_fragment_serialization.h"

#include "io/buffer.h"

using namespace uh::serialization;

// ------------- Tests Follow --------------

BOOST_AUTO_TEST_CASE(arithmetic_types) {
    auto max_uint64 = std::stoull("0123456789");

    uh::io::buffer buf;
    index_fragment_serialization ser(buf);

    auto bytes_non_zero = ser.bytes_non_zero(max_uint64);
    ser.write(max_uint64, bytes_non_zero);
    auto read_back = ser.read<unsigned long>(bytes_non_zero);

    BOOST_CHECK(max_uint64 == read_back);
}

BOOST_AUTO_TEST_CASE(simple_arithmetic_serialization_type_tests) {
    typedef index_fragment_serialization <index_fragment_serializer, index_fragment_deserializer> sertype;

    BOOST_CHECK (is_index_fragment_serializer <index_fragment_serializer>::value);
    BOOST_CHECK (is_index_fragment_deserializer <index_fragment_deserializer>::value);
    BOOST_CHECK (is_index_fragment_serialization_type <sertype>::value);
}

