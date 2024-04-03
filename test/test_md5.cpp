#define BOOST_TEST_MODULE "md5 hash tests"

#include "common/utils/md5.h"
#include <boost/test/unit_test.hpp>

using namespace uh::cluster;

namespace {

// FROM RFC 1321
constexpr const char* TEST_STRING_1 = "";
constexpr const char* HASH_STRING_1 = "d41d8cd98f00b204e9800998ecf8427e";

constexpr const char* TEST_STRING_2 = "a";
constexpr const char* HASH_STRING_2 = "0cc175b9c0f1b6a831c399e269772661";

constexpr const char* TEST_STRING_3 = "abc";
constexpr const char* HASH_STRING_3 = "900150983cd24fb0d6963f7d28e17f72";

constexpr const char* TEST_STRING_4 = "message digest";
constexpr const char* HASH_STRING_4 = "f96b697d7cb7938d525a2f31aaf161d0";

constexpr const char* TEST_STRING_5 = "abcdefghijklmnopqrstuvwxyz";
constexpr const char* HASH_STRING_5 = "c3fcd3d76192e4007dfb496cca67e13b";

constexpr const char* TEST_STRING_6 =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
constexpr const char* HASH_STRING_6 = "d174ab98d277d9f5a5611c2c9f419d9f";

constexpr const char* TEST_STRING_7 =
    "12345678901234567890123456789012345678901234567890123456789012345678901234"
    "567890";
constexpr const char* HASH_STRING_7 = "57edf4a22be3c955ac49da2e2107b67a";

BOOST_AUTO_TEST_CASE(test_md5) {

    BOOST_CHECK(calculate_md5(TEST_STRING_1) == HASH_STRING_1);
    BOOST_CHECK(calculate_md5(TEST_STRING_2) == HASH_STRING_2);
    BOOST_CHECK(calculate_md5(TEST_STRING_3) == HASH_STRING_3);
    BOOST_CHECK(calculate_md5(TEST_STRING_4) == HASH_STRING_4);
    BOOST_CHECK(calculate_md5(TEST_STRING_5) == HASH_STRING_5);
    BOOST_CHECK(calculate_md5(TEST_STRING_6) == HASH_STRING_6);
    BOOST_CHECK(calculate_md5(TEST_STRING_7) == HASH_STRING_7);
}

} // namespace
