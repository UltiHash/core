#define BOOST_TEST_MODULE "hash unit tests"

#include "common/crypto/hash.h"
#include "common/utils/strings.h"
#include <boost/test/unit_test.hpp>

using namespace uh::cluster;

namespace {

BOOST_AUTO_TEST_CASE(test_md5) {

    // RFC 1321 test strings
    BOOST_CHECK_EQUAL(to_hex(md5::from_string("")),
                      "d41d8cd98f00b204e9800998ecf8427e");
    BOOST_CHECK_EQUAL(to_hex(md5::from_string("a")),
                      "0cc175b9c0f1b6a831c399e269772661");
    BOOST_CHECK_EQUAL(to_hex(md5::from_string("abc")),
                      "900150983cd24fb0d6963f7d28e17f72");
    BOOST_CHECK_EQUAL(to_hex(md5::from_string("message digest")),
                      "f96b697d7cb7938d525a2f31aaf161d0");
    BOOST_CHECK_EQUAL(to_hex(md5::from_string("abcdefghijklmnopqrstuvwxyz")),
                      "c3fcd3d76192e4007dfb496cca67e13b");
    BOOST_CHECK_EQUAL(
        to_hex(md5::from_string(
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789")),
        "d174ab98d277d9f5a5611c2c9f419d9f");
    BOOST_CHECK_EQUAL(
        to_hex(md5::from_string(
            "12345678901234567890123456789012345678901234567890123"
            "456789012345678901234567890")),
        "57edf4a22be3c955ac49da2e2107b67a");
}

BOOST_AUTO_TEST_CASE(test_sha256) {

    BOOST_CHECK_EQUAL(
        to_hex(sha256::from_string("")),
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}
} // namespace
