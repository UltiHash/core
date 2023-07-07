//
// Created by benjamin-elias on 22.05.23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibSerialization FragmentSerialization Tests"
#endif

#include <boost/test/unit_test.hpp>

#include "serialization/fragment_serializer.h"
#include "serialization/fragment_deserializer.h"
#include "serialization/buffered_fragment_serializer.h"
#include "serialization/fragment_serialization.h"
#include "io/sstream_device.h"

using namespace uh::serialization;

// ------------- Tests Follow --------------

BOOST_AUTO_TEST_CASE(serialize_fragment_size_len)
{
    struct test_serialize_size_len: sl_fragment_serializer, sl_fragment_deserializer
    {
        static void test()
        {
            for (int size_len = 0; size_len < 8; ++size_len)
            {
                char c = 0;
                set_control_byte_size_length(c, size_len);
                BOOST_TEST (get_control_byte_size_length(c) == size_len);
            }
        }
    };

    test_serialize_size_len::test();
}

BOOST_AUTO_TEST_CASE(serialize_fragment_size)
{
    struct test_serialize_size: sl_fragment_serializer, sl_fragment_deserializer
    {
        static void test()
        {
            for (unsigned long size = 0; size < 128l; ++size)
            {
                std::vector<char> buffer(1);
                set_data_size(buffer.data(), size, 1);
                BOOST_TEST (get_control_byte_size(buffer, 1) == size);
            }
        }
    };

    test_serialize_size::test();
}

template<typename T, typename Serializer = buffered_fragment_serializer<sl_fragment_serializer>, typename Deserializer = sl_fragment_deserializer>
void test_fragment_range_serialization(const T& data, const std::string test_name = "")
{
    uh::io::sstream_device dev;
    {
        Serializer serialize(dev);
        serialize.write(data, 0);
    }
    {
        Deserializer deserialize(dev);
        auto decoded = deserialize.template read<T>();

        if constexpr (std::ranges::contiguous_range<T>)
        {
            BOOST_TEST (decoded.first.size() == data.size(), test_name);
            if (data.size() > 0)
            {
                BOOST_TEST(std::strncmp(reinterpret_cast<const char*> (data.data()),
                                        reinterpret_cast<const char*> (decoded.first.data()), data.size()) == 0,
                           test_name);
            }
        }
        else
        {
            BOOST_TEST (data == decoded);
        }
    }
}

BOOST_AUTO_TEST_CASE(buffered_fragment_serializer_test)
{

    std::string str1 = "data1 data2 data3";
    std::string str2 = "fsdfsdg data2 data3 data 5 da t asdasf gfdg ytg";
    std::vector<long> lvec1{1, 5, 3, 5, 3, 5, 3, 6, 2, 23, 24};
    std::vector<double> dvec1{1.1, 3.2, 4.45, 3.76};
    std::vector<std::uint8_t> emptyvec{};
    std::vector<std::uint64_t> largevec(1024ul * 1024ul * 16ul);
    for (auto i = 0u; i < largevec.size(); i += 1024)
    {
        largevec[i] = i;
    }

    unsigned long ov1 = 2;
    double ov2 = 4.12;

    test_fragment_range_serialization<std::string, buffered_fragment_serializer<sl_fragment_serializer>>
        (str1, "string test");
    test_fragment_range_serialization<std::vector<long>, buffered_fragment_serializer<sl_fragment_serializer>>
        (lvec1, "long vector test");
    test_fragment_range_serialization<std::vector<double>, buffered_fragment_serializer<sl_fragment_serializer>>
        (dvec1, "double vector test");
    test_fragment_range_serialization<std::vector<std::uint8_t>,
                                      buffered_fragment_serializer<sl_fragment_serializer>>(emptyvec,
                                                                                            "empty vector test");
    test_fragment_range_serialization<std::vector<std::uint64_t>,
                                      buffered_fragment_serializer<sl_fragment_serializer>>(largevec,
                                                                                            "large vector test");
    test_fragment_range_serialization<std::string, buffered_fragment_serializer<sl_fragment_serializer>>
        (str2, "string 2 test");

}

BOOST_AUTO_TEST_CASE(buffered_fragment_serialization_test)
{

    std::string str1 = "data1 data2 data3";
    std::string str2 = "fsdfsdg data2 data3 data 5 da t asdasf gfdg ytg";
    std::vector<long> lvec1{1, 5, 3, 5, 3, 5, 3, 6, 2, 23, 24};
    std::vector<double> dvec1{1.1, 3.2, 4.45, 3.76};
    std::vector<std::uint8_t> emptyvec{};
    std::vector<std::uint64_t> largevec(1024ul * 1024ul * 256ul);
    for (auto i = 0u; i < largevec.size(); i += 1024)
    {
        largevec[i] = i;
    }

    constexpr std::size_t buffer_size = 1024;
    char buffer[buffer_size];
    for (auto i = 0u; i < buffer_size; i += 64)
    {
        buffer[i] = i;
    }
    std::span ds{buffer, buffer_size};

    uh::io::sstream_device dev;

    buffered_fragment_serialization ser(dev);
    ser.write(str1, 0);
    ser.write(str2, 1);
    ser.write(lvec1, 2);
    ser.write(dvec1, 3);
    ser.write(emptyvec, 4);
    ser.write(largevec, 5);
    ser.write(ds, 6);

    ser.sync();

    sl_fragment_deserializer des(dev);

    auto str1_read = des.read<std::string>();
    BOOST_TEST (str1_read.first == str1);
    BOOST_TEST (str1_read.second.index_num == 0);

    auto str2_read = ser.read<std::string>();
    BOOST_TEST (str2_read.first == str2);
    BOOST_TEST (str2_read.second.index_num == 1);

    auto lvec2_read = ser.read<std::vector<long>>();
    BOOST_TEST (lvec2_read.first.size() == lvec1.size());
    BOOST_TEST(strncmp(reinterpret_cast<const char*> (lvec1.data()),
                       reinterpret_cast<const char*> (lvec2_read.first.data()), lvec2_read.first.size()) == 0);
    BOOST_TEST (lvec2_read.second.index_num == 2);

    auto dvec2_read = ser.read<std::vector<double>>();
    BOOST_TEST (dvec2_read.first.size() == dvec1.size());
    BOOST_TEST(strncmp(reinterpret_cast<const char*> (dvec1.data()),
                       reinterpret_cast<const char*> (dvec2_read.first.data()), dvec2_read.first.size()) == 0);
    BOOST_TEST (dvec2_read.second.index_num == 3);

    auto empty2_read = ser.read<std::vector<std::uint8_t>>();
    BOOST_TEST (empty2_read.first.size() == emptyvec.size());
    BOOST_TEST (empty2_read.second.index_num == 4);

    auto largevec2_read = ser.read<std::vector<uint64_t>>();
    BOOST_TEST (largevec.size() == largevec2_read.first.size());
    BOOST_TEST(strncmp(reinterpret_cast<const char*> (largevec.data()),
                       reinterpret_cast<const char*> (largevec2_read.first.data()), largevec2_read.first.size()) == 0);
    BOOST_TEST (largevec2_read.second.index_num == 5);

    auto ds2_read = ser.read<std::vector<char>>();
    BOOST_TEST (ds2_read.first.size() == ds.size());
    BOOST_TEST(strncmp(reinterpret_cast<const char*> (ds2_read.first.data()),
                       reinterpret_cast<const char*> (ds.data()), ds2_read.first.size()) == 0);
    BOOST_TEST (ds2_read.second.index_num == 6);
}

BOOST_AUTO_TEST_CASE(fragment_serialization_type_tests)
{
    typedef fragment_serialization<buffered_fragment_serializer<sl_fragment_serializer>, sl_fragment_deserializer>
        sertype;

    BOOST_ASSERT (is_fragment_serializer<sl_fragment_serializer>::value);
    BOOST_ASSERT (is_fragment_deserializer<sl_fragment_deserializer>::value);
    BOOST_ASSERT (is_fragment_serialization_type<sertype>::value);

}

