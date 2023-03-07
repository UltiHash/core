//
// Created by masi on 02.03.23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibSerialization2 Serialization Tests"
#endif

#include <boost/test/unit_test.hpp>
#include "serialization/serializer.h"
#include "serialization/deserializer.h"

#include "io/file.h"

using namespace uh::serialization;

// ------------- Tests Follow --------------


BOOST_AUTO_TEST_CASE(serialize_size_len)  {
    struct test_serialize_size_len: serializer, deserializer {
        static void test () {
            for (int size_len = 0; size_len < 8; ++size_len) {
                char c = 0;
                set_size_length(c, size_len);
                BOOST_TEST (get_size_length(c) == size_len);
            }
        }
    };

    test_serialize_size_len::test();
}

BOOST_AUTO_TEST_CASE(serialize_size) {
    struct test_serialize_size_len: serializer, deserializer {
        static void test () {
            for (int size = 0; size < 128; ++size) {
                std::vector <char> buffer (1);
                set_nl_size(buffer.data (), size, 1);
                BOOST_TEST (get_nl_size (buffer, 1) == size);
            }
        }
    };

    test_serialize_size_len::test();
}

BOOST_AUTO_TEST_CASE(integral_types) {


    unsigned long ov1 = 2, dv1;
    double ov2 = 4.12, dv2;

    {
        uh::io::file dev("data", std::ios::trunc | std::ios::out | std::ios::binary);
        serializer serialize(dev);
        serialize.write(ov1);
        serialize.write(ov2);
    }
    {
        uh::io::file dev ("data");
        deserializer deserialize (dev);
        dv1 = deserialize.read <unsigned long> ();
        dv2 = deserialize.read <double> ();
    }
    BOOST_TEST (dv1 == ov1);
    BOOST_TEST (dv2 == ov2);

}

template <typename T>
requires std::ranges::contiguous_range <T>
void test_range_serialization (const T& data) {
    {
        uh::io::file dev("data");
        serializer serialize(dev);
        serialize.write(data);
    }
    {
        uh::io::file dev("data");
        deserializer deserialize (dev);
        auto decoded = deserialize.read <T> ();

        BOOST_TEST (decoded.size () == data.size ());
        if (data.size () > 0) {
            BOOST_TEST(strncmp(reinterpret_cast<const char *> (data.data()),
                              reinterpret_cast<const char *> (decoded.data()), data.size ()) == 0);
        }
    }
}

BOOST_AUTO_TEST_CASE(range_types) {

    std::string str1 = "data1 data2 data3";
    std::string str2 = "fsdfsdg data2 data3 data 5 da t asdasf gfdg ytg";
    std::vector <long> lvec1 {1, 5, 3, 5,3 ,5, 3, 6, 2, 23, 24};
    std::vector <double> dvec1 {1.1, 3.2, 4.45, 3.76};
    std::vector <std::uint8_t> emptyvec {};
    std::vector <std::uint64_t> largevec (1024ul*1024ul*256ul);


    test_range_serialization (str1);
    test_range_serialization (str2);
    test_range_serialization (lvec1);
    test_range_serialization (dvec1);
    test_range_serialization (emptyvec);
    test_range_serialization (largevec);

}
