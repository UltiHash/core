#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibLicensing License Check Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <licensing/check_airgap_license.h>
#include <licensing/check_online_license.h>
#include <io/temp_file.h>


using namespace uh;
using namespace uh::licensing;

namespace
{

// ---------------------------------------------------------------------

const static std::filesystem::path TEMP_DIR = "/tmp";

// ---------------------------------------------------------------------

typedef boost::mpl::vector<
    check_online_license,
    check_airgap_license
> license_types;

// ---------------------------------------------------------------------

struct Fixture {};

// ---------------------------------------------------------------------

/**
 * To be implemented for each type in `license_types`: constructs a device
 * that will read the text given in LOREM_IPSUM.
 */
template <typename T>
std::unique_ptr<T> make_test_device();

// ---------------------------------------------------------------------

std::string license_path;

template <>
std::unique_ptr<check_airgap_license> make_test_device<check_airgap_license>()
{
    io::temp_file license_file(TEMP_DIR);
    license_file.write("GZ8F-LNZ5-FGAK-2T02");
    license_file.release_to(license_file.path());
    license_path = license_file.path();

    return std::make_unique<check_airgap_license>(TEMP_DIR,"UltiHash agency_node","0.1.0");
}

// ---------------------------------------------------------------------

template <>
std::unique_ptr<check_online_license> make_test_device<check_online_license>()
{
    io::temp_file license_file(TEMP_DIR);
    license_file.write("License");
    license_file.release_to(license_file.path());

    return std::make_unique<check_online_license>(TEMP_DIR,"UltiHash agency_node","0.1.0",
                                                  "benjamin@ultihash.io",
                                                  "#u5huzU!ita*o&I4@ona2+OVlGlhehe0!dLDeslticO#r?!3@*$a@$x*hl1lxisW");
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE(valid_default, T, license_types, Fixture )
{
    auto dev = make_test_device<T>();

    BOOST_CHECK(dev->valid());
    std::filesystem::remove(license_path);
}

// ---------------------------------------------------------------------

} // namespace
