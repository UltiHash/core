#define BOOST_TEST_MODULE "data_file tests"

#include <boost/test/unit_test.hpp>

#include <common/utils/random.h>
#include <storage/data_file.h>

#include <util/temp_directory.h>

using namespace uh::cluster;

static std::vector<char> WRITE_BUFFER = std::vector<char>(4 * KIBI_BYTE, 0);

BOOST_FIXTURE_TEST_CASE(create, temp_directory) {

    auto file = data_file::create(path() / "data", 4 * KIBI_BYTE);

    BOOST_CHECK_EQUAL(file.filesize(), 4 * KIBI_BYTE);
    BOOST_CHECK_EQUAL(file.used_space(), 0ull);
}

BOOST_FIXTURE_TEST_CASE(data_write, temp_directory) {

    auto file = data_file::create(path() / "data", 4 * KIBI_BYTE);
    BOOST_CHECK_EQUAL(file.filesize(), 4 * KIBI_BYTE);
    BOOST_CHECK_EQUAL(file.used_space(), 0ull);

    auto offs_1 = 0ull;
    auto bytes_1 = file.write(offs_1, {WRITE_BUFFER.data(), 1 * KIBI_BYTE});
    BOOST_CHECK_EQUAL(file.used_space(), 1 * KIBI_BYTE);
    BOOST_CHECK_EQUAL(bytes_1, 1 * KIBI_BYTE);

    auto offs_2 = 1 * KIBI_BYTE;
    auto bytes_2 = file.write(offs_2, {WRITE_BUFFER.data(), 4 * KIBI_BYTE});
    BOOST_CHECK_EQUAL(file.used_space(), 4 * KIBI_BYTE);
    BOOST_CHECK_EQUAL(bytes_2, 3 * KIBI_BYTE);
}

BOOST_FIXTURE_TEST_CASE(data_read, temp_directory) {

    auto file = data_file::create(path() / "data", 4 * KIBI_BYTE);
    BOOST_CHECK_EQUAL(file.filesize(), 4 * KIBI_BYTE);
    BOOST_CHECK_EQUAL(file.used_space(), 0ull);

    auto offs_1 = 0ull;
    {
        std::vector<char> buffer(1 * KIBI_BYTE, 0);
        auto bytes = file.read(offs_1, buffer);
        BOOST_CHECK_EQUAL(bytes, 1 * KIBI_BYTE);
    }

    auto offs_2 = 1 * KIBI_BYTE;
    {
        BOOST_CHECK_EQUAL(offs_1 + 1 * KIBI_BYTE, offs_2);
        std::vector<char> buffer(8 * KIBI_BYTE, 0);
        auto bytes = file.read(offs_1, buffer);
        BOOST_CHECK_EQUAL(bytes, 4 * KIBI_BYTE);
    }
}

BOOST_FIXTURE_TEST_CASE(read_write, temp_directory) {

    auto file = data_file::create(path() / "data", 4 * KIBI_BYTE);
    BOOST_CHECK_EQUAL(file.filesize(), 4 * KIBI_BYTE);
    BOOST_CHECK_EQUAL(file.used_space(), 0ull);

    auto data = random_string(1 * KIBI_BYTE);
    auto offs = 0ull;
    auto wrote = file.write(offs, data);
    BOOST_CHECK_EQUAL(file.used_space(), 1 * KIBI_BYTE);
    BOOST_CHECK_EQUAL(wrote, 1 * KIBI_BYTE);

    std::string input(1 * KIBI_BYTE, 0);
    auto read = file.read(offs, input);
    BOOST_CHECK_EQUAL(read, 1 * KIBI_BYTE);

    BOOST_CHECK_EQUAL(input, data);
}

BOOST_FIXTURE_TEST_CASE(release, temp_directory) {

    auto file = data_file::create(path() / "data", 4 * KIBI_BYTE);
    BOOST_CHECK_EQUAL(file.filesize(), 4 * KIBI_BYTE);
    BOOST_CHECK_EQUAL(file.used_space(), 0ull);

    auto data = random_string(4 * KIBI_BYTE);
    auto offs = 0ull;
    file.write(offs, data);
    BOOST_CHECK_EQUAL(file.used_space(), 4 * KIBI_BYTE);

    file.release(offs, 2 * KIBI_BYTE);

    BOOST_CHECK_EQUAL(file.used_space(), 2 * KIBI_BYTE);
}

BOOST_FIXTURE_TEST_CASE(reload, temp_directory) {
    auto data = random_string(1 * KIBI_BYTE);
    std::size_t offs = 0ull;

    {
        auto file = data_file::create(path() / "data", 4 * KIBI_BYTE);
        BOOST_CHECK_EQUAL(file.filesize(), 4 * KIBI_BYTE);
        BOOST_CHECK_EQUAL(file.used_space(), 0ull);

        offs = 0ull;
        auto wrote = file.write(offs, data);
        BOOST_CHECK_EQUAL(wrote, 1 * KIBI_BYTE);
        BOOST_CHECK_EQUAL(file.used_space(), 1 * KIBI_BYTE);
    }

    {
        data_file file(path() / "data");

        std::string input(1 * KIBI_BYTE, 0);
        auto read = file.read(offs, input);

        BOOST_CHECK_EQUAL(read, 1 * KIBI_BYTE);
        BOOST_CHECK_EQUAL(input, data);

        BOOST_CHECK_EQUAL(file.filesize(), 4 * KIBI_BYTE);
        BOOST_CHECK_EQUAL(file.used_space(), 1 * KIBI_BYTE);
    }
}
