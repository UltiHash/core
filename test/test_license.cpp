#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "license tests"
#endif

#include "common/license/license.h"
#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <fstream>
#include <test_config.h>

using namespace uh::cluster;

namespace {

std::filesystem::path LIC_1GB = TEST_DATA_DIR "/licenses/UltiHash-Test-1GB.lic";
std::filesystem::path LIC_4TB = TEST_DATA_DIR "/licenses/UltiHash-Test-4TB.lic";
std::filesystem::path LIC_128TB =
    TEST_DATA_DIR "/licenses/UltiHash-Test-128TB.lic";
std::filesystem::path LIC_1PB = TEST_DATA_DIR "/licenses/UltiHash-Test-1PB.lic";

std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path);

    if (!in) {
        throw std::runtime_error("cannot find " + path.string());
    }

    return {std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()};
}

license read_license(const std::filesystem::path& path) {

    return check_license(read_file(path));
}

BOOST_AUTO_TEST_CASE(sample_licenses) {
    {
        auto lic = read_license(LIC_1GB);
        BOOST_CHECK(lic.customer == "UltiHash-Test");
        BOOST_CHECK(lic.max_data_store_size == 1 * GIGA_BYTE);
    }
    {
        auto lic = read_license(LIC_4TB);
        BOOST_CHECK(lic.customer == "UltiHash-Test");
        BOOST_CHECK(lic.max_data_store_size == 4 * TERA_BYTE);
    }
    {
        auto lic = read_license(LIC_128TB);
        BOOST_CHECK(lic.customer == "UltiHash-Test");
        BOOST_CHECK(lic.max_data_store_size == 128 * TERA_BYTE);
    }
    {
        auto lic = read_license(LIC_1PB);
        BOOST_CHECK(lic.customer == "UltiHash-Test");
        BOOST_CHECK(lic.max_data_store_size == 1 * PETA_BYTE);
    }
}

BOOST_AUTO_TEST_CASE(exception) {
    { BOOST_CHECK_THROW(check_license(""), std::exception); }
    {
        auto code = read_file(LIC_1GB);
        code[0] ^= 1;
        BOOST_CHECK_THROW(check_license(code), std::exception);
    }
    {
        auto code = read_file(LIC_1GB);
        code[code.size() / 2] ^= 1;
        BOOST_CHECK_THROW(check_license(code), std::exception);
    }
}

} // namespace
