#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "xml parser tests"
#endif

#include "entrypoint/utils/xml_parser.h"
#include <boost/test/unit_test.hpp>

using namespace uh::cluster;

namespace {

const std::string TEST_STRING_1 = "test_string_1";
const std::string TEST_VERSION_1 = "3";

const std::string TEST_STRING_2 = "test_string_2";
const std::string TEST_VERSION_2_ERROR = "3.5";

const std::string PARSABLE_XML_STRING = "<Delete>\n"
                                        "   <Object>\n"
                                        "       <Key>" +
                                        TEST_STRING_1 +
                                        "</Key>\n"
                                        "       <VersionId>" +
                                        TEST_VERSION_1 +
                                        "</VersionId>\n"
                                        "   </Object>\n"
                                        "   <Object>\n"
                                        "       <Key>" +
                                        TEST_STRING_2 +
                                        "</Key>\n"
                                        "       <VersionId>" +
                                        TEST_VERSION_2_ERROR +
                                        "</VersionId>\n"
                                        "   </Object>\n"
                                        "   <Quiet>boolean</Quiet>\n"
                                        "</Delete>";

const std::string UNPARSABLE_XML_STRING = R"(<Delete>
                                                   <Object>
                                                      <Key>string</Key>
                                                      <VersionId>3.4</VersionId>
                                                   </Object>
                                                      <Key>string</Key>
                                                      <VersionId>string</VersionId>
                                                   </Object>
                                                   <Quiet>boolean</Quiet>
                                                </Delete>)";

BOOST_AUTO_TEST_CASE(test_parsing) {

    {
        xml_parser xml_parser;

        bool parsed = xml_parser.parse(UNPARSABLE_XML_STRING);
        BOOST_CHECK(parsed == false);

        auto nodes = xml_parser.get_nodes("Delete.Object");
        BOOST_CHECK(nodes.empty() == true);
    }

    xml_parser xml_parser;

    {
        bool parsed = xml_parser.parse(PARSABLE_XML_STRING);
        BOOST_CHECK(parsed == true);
    }

    {
        auto object_nodes = xml_parser.get_nodes("Delete.Object");
        BOOST_CHECK(object_nodes.size() == 2);

        auto key_1 = object_nodes[0].get().get<std::string>("Key");
        BOOST_CHECK(key_1 == TEST_STRING_1);

        auto key_2 = object_nodes[1].get().get<std::string>("Key");
        BOOST_CHECK(key_2 == TEST_STRING_2);

        auto version_1 = object_nodes[0].get().get<std::size_t>("VersionId");
        BOOST_CHECK(version_1 == std::stoul(TEST_VERSION_1));

        BOOST_CHECK_THROW(object_nodes[1].get().get<std::size_t>("VersionId"),
                          std::exception);
    }
}

} // namespace
