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
#include "io/file.h"

using namespace uh::serialization;

// ------------- Tests Follow --------------


BOOST_AUTO_TEST_CASE(serialize_size_len)
{
    struct test_serialize_size_len: serialization {
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

BOOST_AUTO_TEST_CASE(serialize_size)
{
    struct test_serialize_size_len: serialization {
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

BOOST_AUTO_TEST_CASE(integral_types)
{


    int ov1 = 2, dv1;
    int ov2 = 4.12, dv2;

    {
        uh::io::file dev("data", std::ios::trunc | std::ios::out | std::ios::binary);
        serializer serialize(dev);
        serialize.write(ov1);
        serialize.write(ov2);
    }
    {
        uh::io::file dev ("data");
        deserializer deserialize (dev);
        dv1 = deserialize.read <int> ();
        dv2 = deserialize.read <int> ();
    }
    BOOST_TEST (dv1 == ov1);
    BOOST_TEST (dv2 == ov2);

}

BOOST_AUTO_TEST_CASE(range_types)
{

    std::string str1 = "data1 data2 data3";
    std::string str2 = "fsdfsdg data2 data3 data 5 da t asdasf gfdg ytg";

    std::vector <long> lvec1 {1, 5, 3, 5,3 ,5, 3, 6, 2, 23, 24};
    //std::vector <double> dvec1 {1.1, 3.2, 4.45, 3.76};
    {
        uh::io::file dev("data");
        serializer serialize(dev);
        serialize.write(str1);
        serialize.write(str2);

        //serialize.write(lvec1);
        //serialize.write(dvec1);
    }

    std::string str3, str4;
    std::vector <long> lvec2;
    //std::vector <double> dvec2;

    {
        uh::io::file dev("data");
        deserializer deserialize (dev);

        str3 = deserialize.read <std::string> ();
        str4 = deserialize.read <std::string> ();

        //lvec2 = deserialize.read <std::vector <long>> ();
        //dvec2 = deserialize.read <std::vector <double>> ();
    }

    BOOST_TEST (str1 == str3);
    BOOST_TEST (str2 == str4);

    //BOOST_TEST (lvec1.size() == lvec2.size());
    //BOOST_TEST (dvec1.size() == dvec2.size());
    //BOOST_TEST (strcmp(reinterpret_cast<char *> (lvec1.data()), reinterpret_cast<char *> (lvec2.data())) == 0);
    //for (int i = 0; i < lvec1.size(); ++i) {
    //    std::cout << lvec1[i] << " " << lvec2[i] << std::endl;
    //}
    //BOOST_TEST (strcmp(reinterpret_cast<char *> (dvec1.data()), reinterpret_cast<char *> (dvec2.data())) == 0);


}
