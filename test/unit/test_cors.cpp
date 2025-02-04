#define BOOST_TEST_MODULE "CORS parser tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <entrypoint/cors/parser.h>

using namespace uh::cluster::ep::cors;

const char* SAMPLE_CORS = R"(
<CORSConfiguration>
 <CORSRule>
   <AllowedOrigin>http://www.example.com</AllowedOrigin>

   <AllowedMethod>PUT</AllowedMethod>
   <AllowedMethod>POST</AllowedMethod>
   <AllowedMethod>DELETE</AllowedMethod>

   <AllowedHeader>*</AllowedHeader>
 </CORSRule>
 <CORSRule>
   <AllowedOrigin>*</AllowedOrigin>
   <AllowedMethod>GET</AllowedMethod>
 </CORSRule>
</CORSConfiguration>)";

BOOST_AUTO_TEST_CASE(reading_cors_example) {
    auto info = parser::parse(SAMPLE_CORS);

    BOOST_CHECK(info.contains("http://www.example.com"));

    auto example_com = info["http://www.example.com"];
    BOOST_CHECK_EQUAL(example_com.allowed_delete, true);
    BOOST_CHECK_EQUAL(example_com.allowed_get, false);
    BOOST_CHECK_EQUAL(example_com.allowed_head, false);
    BOOST_CHECK_EQUAL(example_com.allowed_post, true);
    BOOST_CHECK_EQUAL(example_com.allowed_put, true);

    BOOST_CHECK(info.contains("*"));

    auto wildcard = info["*"];
    BOOST_CHECK_EQUAL(wildcard.allowed_delete, false);
    BOOST_CHECK_EQUAL(wildcard.allowed_get, true);
    BOOST_CHECK_EQUAL(wildcard.allowed_head, false);
    BOOST_CHECK_EQUAL(wildcard.allowed_post, false);
    BOOST_CHECK_EQUAL(wildcard.allowed_put, false);
}
