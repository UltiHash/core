#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "xml parser tests"
#endif

#include "common/utils/xml_parser.h"
#include <boost/test/unit_test.hpp>

using namespace uh::cluster;

namespace {

const std::string KEY_1 = "test_string_1";
const std::string VERSION_1 = "3";

const std::string KEY_2 = "test_string_2";
const std::string ERRONEOUS_VERSION_2 = "3.5";

const std::string PARSABLE_XML_STRING_1 = "<Delete>\n"
                                          "   <Object>\n"
                                          "       <Key>" +
                                          KEY_1 +
                                          "</Key>\n"
                                          "       <VersionId>" +
                                          VERSION_1 +
                                          "</VersionId>\n"
                                          "   </Object>\n"
                                          "   <Object>\n"
                                          "       <Key>" +
                                          KEY_2 +
                                          "</Key>\n"
                                          "       <VersionId>" +
                                          ERRONEOUS_VERSION_2 +
                                          "</VersionId>\n"
                                          "   </Object>\n"
                                          "   <Quiet>boolean</Quiet>\n"
                                          "</Delete>";

const std::string PARSABLE_STRING_2 = R"(<company>
                                            <employees>
                                                <employee id="1">
                                                    <name>John Doe</name>
                                                    <department>Engineering</department>
                                                    <position>Software Engineer</position>
                                                    <salary currency="USD">100000</salary>
                                                    <projects>
                                                        <project id="101">
                                                            <name>Project X</name>
                                                            <description>A cutting-edge research project</description>
                                                        </project>
                                                        <project id="102">
                                                            <name>Project Y</name>
                                                            <description>An innovative product development initiative</description>
                                                        </project>
                                                    </projects>
                                                </employee>
                                                <employee id="2">
                                                    <name>Jane Smith</name>
                                                    <department>Marketing</department>
                                                    <position>Marketing Manager</position>
                                                    <salary currency="EUR">85000</salary>
                                                    <projects>
                                                        <project id="103">
                                                            <name>Marketing Campaign A</name>
                                                            <description>Targeted campaign for new product launch</description>
                                                        </project>
                                                    </projects>
                                                </employee>
                                            </employees>
                                            <departments>
                                                <department id="1">
                                                    <name>Engineering</name>
                                                    <location>Building A, Floor 3</location>
                                                    <head>John Doe</head>
                                                </department>
                                                <department id="2">
                                                    <name>Marketing</name>
                                                    <location>Building B, Floor 1</location>
                                                    <head>Jane Smith</head>
                                                </department>
                                            </departments>
                                            </company>)";

const std::string UNPARSABLE_XML_STRING_1 = R"(<Delete>
                                                   <Object>
                                                      <Key>string</Key>
                                                      <VersionId>3.4</VersionId>
                                                   </Object>
                                                      <Key>string</Key>
                                                      <VersionId>string</VersionId>
                                                   </Object>
                                                   <Quiet>boolean</Quiet>
                                                </Delete>)";

const std::string UNPARSABLE_XML_STRING_2 = R"(<Object>
                                                      <Key>string</Key>
                                                      <VersionId>3.4</VersionId>
                                                   </Object>
                                                    <Object>
                                                      <Key>string</Key>
                                                      <VersionId>string</VersionId>
                                                   </Object>
                                                   <Quiet>boolean</Quiet>)";

BOOST_AUTO_TEST_CASE(test_parsing) {

    {
        xml_parser xml_parser;

        bool parsed = xml_parser.parse(UNPARSABLE_XML_STRING_1);
        BOOST_CHECK(parsed == false);

        auto nodes = xml_parser.get_nodes("Delete.Object");
        BOOST_CHECK(nodes.empty() == true);
    }

    {
        xml_parser xml_parser;

        bool parsed = xml_parser.parse(PARSABLE_STRING_2);
        BOOST_CHECK(parsed);

        auto nodes =
            xml_parser.get_nodes("company.employees.employee.projects.project");
        BOOST_TEST(nodes.size() == 3);

        BOOST_CHECK_THROW(xml_parser.get_nodes(""), std::exception);
    }

    xml_parser xml_parser;

    {
        bool parsed = xml_parser.parse(PARSABLE_XML_STRING_1);
        BOOST_CHECK(parsed == true);
    }

    {
        auto object_nodes = xml_parser.get_nodes("Delete.Object");
        BOOST_TEST(object_nodes.size() == 2);

        auto key_1 = object_nodes[0].get().get<std::string>("Key");
        BOOST_CHECK(key_1 == KEY_1);

        auto key_2 = object_nodes[1].get().get<std::string>("Key");
        BOOST_CHECK(key_2 == KEY_2);

        auto version_1 = object_nodes[0].get().get<std::size_t>("VersionId");
        BOOST_CHECK(version_1 == std::stoul(VERSION_1));

        BOOST_CHECK_THROW(object_nodes[1].get().get<std::size_t>("VersionId"),
                          std::exception);
    }
}

} // namespace
