#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibSerialization Serialization Tests"
#endif

#include <boost/test/unit_test.hpp>
#include "serialization/shrink_arithmetic_serializer.h"
#include "serialization/shrink_arithmetic_deserializer.h"
#include "serialization/shrink_arithmetic_serialization.h"

#include "io/buffer.h"

using namespace uh::serialization;

// ------------- Tests Follow --------------

BOOST_AUTO_TEST_CASE(arithmetic_types) {
    auto max_uint64 = std::stoull("0123456789");

    uh::io::buffer buf;
    shrink_arithmetic_serialization ser(buf);

    auto bytes_non_zero = ser.bytes_non_zero(max_uint64);
    ser.write(max_uint64, bytes_non_zero);
    auto read_back = ser.read<unsigned long>(bytes_non_zero);

    BOOST_CHECK(max_uint64 == read_back);
}

BOOST_AUTO_TEST_CASE(simple_arithmetic_serialization_type_tests) {
    typedef shrink_arithmetic_serialization <shrink_arithmetic_serializer, shrink_arithmetic_deserializer> sertype;

    BOOST_CHECK (is_shrink_arithmetic_serializer <shrink_arithmetic_serializer>::value);
    BOOST_CHECK (is_shrink_arithmetic_deserializer <shrink_arithmetic_deserializer>::value);
    BOOST_CHECK (is_shrink_arithmetic_serialization_type <sertype>::value);
}

