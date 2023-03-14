#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibHash SHA512 test file"
#endif

#include <boost/test/unit_test.hpp>
#include <util/exception.h>
#include <io/buffer.h>
#include <hash/sha512.h>


using namespace uh;
using namespace uh::util;
using namespace uh::hash;

namespace
{

// ---------------------------------------------------------------------

const static std::string TEST_TEXT = "Lorem ipsum dolor sit amet";
const static std::vector<unsigned char> TEST_HASH {
    0xb1, 0xf4, 0xaa, 0xa6, 0xb5, 0x1c, 0x19, 0xff, 0xbe, 0x4b, 0x1b, 0x6f,
    0xa1, 0x07, 0xbe, 0x09, 0xc8, 0xac, 0xaf, 0xd7, 0xc7, 0x68, 0x10, 0x6a,
    0x3f, 0xaf, 0x47, 0x5b, 0x1e, 0x27, 0xa9, 0x40, 0xd3, 0xc0, 0x75, 0xfd,
    0xa6, 0x71, 0xea, 0xdf, 0x46, 0xc6, 0x8f, 0x93, 0xd7, 0xea, 0xbc, 0xf6,
    0x04, 0xbc, 0xbf, 0x70, 0x55, 0xda, 0x0d, 0xc4, 0xea, 0xe6, 0x74, 0x36,
    0x07, 0xa2, 0xfc, 0x3f };

// ---------------------------------------------------------------------

std::vector<char> to_char_vector(const std::vector<unsigned char>& uc)
{
    std::vector<char> rv;
    rv.reserve(uc.size());

    for (auto ch : uc)
    {
        rv.push_back(static_cast<char>(ch));
    }

    return rv;
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( digest_function )
{
    auto hash = sha512_digest({ TEST_TEXT.begin(), TEST_TEXT.size() });
    BOOST_CHECK(hash == to_char_vector(TEST_HASH));
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( device )
{
    io::buffer buffer(4096);
    sha512 dev(buffer);

    dev.write({ TEST_TEXT.data(), TEST_TEXT.size() });

    auto hash = dev.finalize();
    BOOST_CHECK(hash == to_char_vector(TEST_HASH));
}

// ---------------------------------------------------------------------

} // namespace

