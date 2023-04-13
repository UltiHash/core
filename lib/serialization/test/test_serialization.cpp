//
// Created by masi on 02.03.23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibSerialization Serialization Tests"
#endif

#include <boost/test/unit_test.hpp>
#include "serialization/serializer.h"
#include "serialization/deserializer.h"
#include "serialization/buffered_serializer_2.h"
#include "serialization/serialization.h"


#include "io/file.h"
#include "io/sstream_device.h"
#include "serialization/buffered_serializer.h"

using namespace uh::serialization;

// ------------- Tests Follow --------------

BOOST_AUTO_TEST_CASE(serialize_size_len)  {
    struct test_serialize_size_len: sl_serializer, sl_deserializer {
        static void test () {
            for (int size_len = 0; size_len < 8; ++size_len) {
                char c = 0;
                set_control_byte_size_length (c, size_len);
                BOOST_TEST (get_control_byte_size_length(c) == size_len);
            }
        }
    };

    test_serialize_size_len::test();
}

BOOST_AUTO_TEST_CASE(serialize_size) {
    struct test_serialize_size: sl_serializer, sl_deserializer {
        static void test () {
            for (unsigned long size = 0; size < 128l; ++size) {
                std::vector <char> buffer (1);
                set_data_size(buffer.data(), size, 1);
                BOOST_TEST (get_control_byte_size(buffer, 1) == size);
            }
        }
    };

    test_serialize_size::test();
}

BOOST_AUTO_TEST_CASE(integral_types) {


    unsigned long ov1 = 2, dv1;
    double ov2 = 4.12, dv2;

    uh::io::sstream_device dev;
    {
        sl_serializer serialize(dev);
        serialize.write(ov1);
        serialize.write(ov2);
    }
    {
        sl_deserializer deserialize (dev);
        dv1 = deserialize.read <unsigned long> ();
        dv2 = deserialize.read <double> ();
    }
    BOOST_TEST (dv1 == ov1);
    BOOST_TEST (dv2 == ov2);

}

template <typename T, typename Serializer = buffered_serializer <sl_serializer>, typename Deserializer = sl_deserializer>
void test_range_serialization (const T& data, const std::string test_name = "") {
    uh::io::sstream_device dev;
    {
        Serializer serialize(dev);
        serialize.write(data);
    }
    {
        Deserializer deserialize (dev);
        auto decoded = deserialize.template read <T> ();

        if constexpr (std::ranges::contiguous_range <T>) {
            BOOST_TEST (decoded.size() == data.size(), test_name);
            if (data.size() > 0) {
                BOOST_TEST(std::strncmp(reinterpret_cast<const char *> (data.data()),
                                   reinterpret_cast<const char *> (decoded.data()), data.size()) == 0, test_name);
            }
        } else {
            BOOST_TEST (data == decoded);
        }
    }
}


BOOST_AUTO_TEST_CASE(range_types) {

    std::string str1 = "data1 data2 data3";
    std::string str2 = "fsdfsdg data2 data3 data 5 da t asdasf gfdg ytg";
    std::vector <long> lvec1 {1, 5, 3, 5,3 ,5, 3, 6, 2, 23, 24};
    std::vector <double> dvec1 {1.1, 3.2, 4.45, 3.76};
    std::vector <std::uint8_t> emptyvec {};
    std::vector <std::uint64_t> largevec (1024ul*1024ul*16ul);
    for (auto i = 0u; i < largevec.size(); i+=1024) {
        largevec[i] = i;
    }

    test_range_serialization (str1);
    test_range_serialization (str2);
    test_range_serialization (lvec1);
    test_range_serialization (dvec1);
    test_range_serialization (emptyvec);
    test_range_serialization (largevec);

}


BOOST_AUTO_TEST_CASE(buffered_serializer_test) {

    std::string str1 = "data1 data2 data3";
    std::string str2 = "fsdfsdg data2 data3 data 5 da t asdasf gfdg ytg";
    std::vector <long> lvec1 {1, 5, 3, 5,3 ,5, 3, 6, 2, 23, 24};
    std::vector <double> dvec1 {1.1, 3.2, 4.45, 3.76};
    std::vector <std::uint8_t> emptyvec {};
    std::vector <std::uint64_t> largevec (1024ul*1024ul*16ul);
    for (auto i = 0u; i < largevec.size(); i+=1024) {
        largevec[i] = i;
    }

    unsigned long ov1 = 2;
    double ov2 = 4.12;

    test_range_serialization <std::string, buffered_serializer <sl_serializer>> (str1, "string test");
    test_range_serialization < std::vector <long>, buffered_serializer<sl_serializer>> (lvec1, "long vector test");
    test_range_serialization < std::vector <double>, buffered_serializer<sl_serializer>> (dvec1, "double vector test");
    test_range_serialization <std::vector <std::uint8_t>, buffered_serializer<sl_serializer>> (emptyvec, "empty vector test");
    test_range_serialization <std::vector <std::uint64_t>, buffered_serializer<sl_serializer>> (largevec, "large vector test");
    test_range_serialization <std::string, buffered_serializer<sl_serializer>> (str2, "string 2 test");

    test_range_serialization <unsigned long, buffered_serializer<sl_serializer>> (ov1, "unsigned long test");
    test_range_serialization <double, buffered_serializer<sl_serializer>> (ov2, "double test");


}

BOOST_AUTO_TEST_CASE(buffered_serialization_test) {

    std::string str1 = "data1 data2 data3";
    std::string str2 = "fsdfsdg data2 data3 data 5 da t asdasf gfdg ytg";
    std::vector<long> lvec1{1, 5, 3, 5, 3, 5, 3, 6, 2, 23, 24};
    std::vector<double> dvec1{1.1, 3.2, 4.45, 3.76};
    std::vector<std::uint8_t> emptyvec{};
    std::vector<std::uint64_t> largevec(1024ul * 1024ul * 256ul);
    for (auto i = 0u; i < largevec.size(); i += 1024) {
        largevec[i] = i;
    }

    constexpr std::size_t buffer_size = 1024;
    char buffer[buffer_size];
    for (auto i = 0u; i < buffer_size; i += 64) {
        buffer[i] = i;
    }
    std::span ds{buffer, buffer_size};

    unsigned long ov1 = 2;
    double ov2 = 4.12;

    uh::io::sstream_device dev;

    buffered_serialization ser(dev);
    ser.write(str1);
    ser.write(str2);
    ser.write(lvec1);
    ser.write(dvec1);
    ser.write(emptyvec);
    ser.write(largevec);
    ser.write(ov1);
    ser.write(ov2);
    ser.write(ds);

    ser.sync();

    sl_deserializer des (dev);
    BOOST_TEST (des.read<std::string>() == str1);
    BOOST_TEST (ser.read<std::string>() == str2);
    auto lvec2 = ser.read<std::vector <long>>();
    BOOST_TEST (lvec2.size() == lvec1.size());
    BOOST_TEST(strncmp(reinterpret_cast<const char *> (lvec1.data()),
                       reinterpret_cast<const char *> (lvec2.data()), lvec2.size()) == 0);
    auto dvec2 = ser.read<std::vector <double>>();
    BOOST_TEST (dvec2.size() == dvec1.size());
    BOOST_TEST(strncmp(reinterpret_cast<const char *> (dvec1.data()),
                       reinterpret_cast<const char *> (dvec2.data()), dvec2.size()) == 0);
    auto empty2 = ser.read<std::vector <std::uint8_t>>();
    BOOST_TEST (empty2.size() == emptyvec.size());
    auto largevec2 = ser.read<std::vector <uint64_t>>();
    BOOST_TEST (largevec.size() == largevec2.size());
    BOOST_TEST(strncmp(reinterpret_cast<const char *> (largevec.data()),
                       reinterpret_cast<const char *> (largevec2.data()), largevec2.size()) == 0);
    BOOST_TEST (ser.read<unsigned long>() == ov1);
    BOOST_TEST (ser.read<double>() == ov2);

    auto ds2 = ser.read<std::vector <char>>();
    BOOST_TEST (ds2.size() == ds.size());
    BOOST_TEST(strncmp(reinterpret_cast<const char *> (ds2.data()),
                       reinterpret_cast<const char *> (ds.data()), ds2.size()) == 0);
}



BOOST_AUTO_TEST_CASE(serialization_type_tests) {
    typedef serialization <buffered_serializer<sl_serializer>, sl_deserializer> sertype;

    BOOST_ASSERT (is_serializer <sl_serializer>::value);
    BOOST_ASSERT (is_deserializer <sl_deserializer>::value);
    BOOST_ASSERT (is_serialization_type <sertype>::value);


}

