#define BOOST_TEST_MODULE "strings tests"

#include "common/utils/strings.h"

#include <boost/test/unit_test.hpp>
#include <cstring>

namespace uh::cluster {

BOOST_AUTO_TEST_CASE(string_split) {
    BOOST_CHECK(split("").empty());
    BOOST_CHECK(split("abc").size() == 1);

    {
        auto v = split("abc def gih");
        BOOST_CHECK(v.size() == 3);
        BOOST_CHECK(v[0] == "abc");
        BOOST_CHECK(v[1] == "def");
        BOOST_CHECK(v[2] == "gih");
    }

    {
        auto v = split("abc-def-gih", '-');
        BOOST_CHECK(v.size() == 3);
        BOOST_CHECK(v[0] == "abc");
        BOOST_CHECK(v[1] == "def");
        BOOST_CHECK(v[2] == "gih");
    }
}

BOOST_AUTO_TEST_CASE(string_base64_decode) {
    constexpr const char* test_base64 =
        "Q29ycG9yaXMgZWEgc2FlcGUgdG90YW0gcmVwcmVoZW5kZXJpdCBuaWhpbCBmdWdpdCBhbG"
        "lxdWFtLiBFeHBsaWNhYm8gZmFjZXJlIGNvbW1vZGkgdWxsYW0gYXV0IGVhIGVhLiBWb2x1"
        "cHRhdGVtIHF1aWRlbSBjb25zZXF1YXR1ciBkaWduaXNzaW1vcyBpZC4gU3VzY2lwaXQgY2"
        "9uc2VxdWF0dXIgcXVpYSByZWljaWVuZGlzIGF1dCBkb2xvcmVzIGRpY3RhIGVsaWdlbmRp"
        "IGl0YXF1ZS4gTWF4aW1lIHNpbWlsaXF1ZSByZXJ1bSB0ZW5ldHVyIGV4ZXJjaXRhdGlvbm"
        "VtIGlwc3VtIGVhcXVlIHRlbXBvcmEgaGljLiBFc3Qgbm9uIGVhcXVlIG9jY2FlY2F0aSBk"
        "b2xvciBlc3Qgdm9sdXB0YXMgYXQu";

    constexpr const char* test_plain =
        "Corporis ea saepe totam reprehenderit nihil fugit aliquam. Explicabo "
        "facere commodi ullam aut ea ea. Voluptatem quidem consequatur digniss"
        "imos id. Suscipit consequatur quia reiciendis aut dolores dicta elige"
        "ndi itaque. Maxime similique rerum tenetur exercitationem ipsum eaque"
        " tempora hic. Est non eaque occaecati dolor est voluptas at.";

    auto decoded = base64_decode(test_base64);
    auto plain =
        std::vector<char>(test_plain, test_plain + ::strlen(test_plain));

    BOOST_CHECK(decoded == plain);
}

} // namespace uh::cluster
