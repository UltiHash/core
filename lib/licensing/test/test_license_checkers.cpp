#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibLicensing License Check Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <util/temp_dir.h>
#include <licensing/check_airgap_license.h>
#include <licensing/license_package.h>


using namespace uh;
using namespace uh::licensing;

namespace
{

// ---------------------------------------------------------------------

const std::string product_Id_test = "uhtest";
const std::string appName_test = "uh-test";
const std::string appVersion_test = "0.2.0";
const std::string licenseKey_100 = "GZM9-S88G-RNEK-2EUH";

// ---------------------------------------------------------------------

typedef boost::mpl::vector<
    check_airgap_license
> license_types;

// ---------------------------------------------------------------------

struct Fixture
{
};

// ---------------------------------------------------------------------

/**
 * To be implemented for each type in `license_types`: constructs a license
 * that will read the text given in LOREM_IPSUM.
 */
template<typename T>
std::unique_ptr<T> make_test_license(const std::filesystem::path& path);

// ---------------------------------------------------------------------

template<>
std::unique_ptr<check_airgap_license> make_test_license<check_airgap_license>(
    const std::filesystem::path& path)
{
    // TODO why do we need to give the filename twice
    auto config = ls_airgap_config {
        .productId = product_Id_test,
        .appName = appName_test,
        .appVersion = appVersion_test,
        .path = path / "test.lic" / "test.lic"
    };

    return std::make_unique<check_airgap_license>(config, licenseKey_100);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE(valid_default_license, T, license_types, Fixture)
{
    util::temp_directory temp;
    auto lic = make_test_license<T>(temp.path());

    BOOST_CHECK(lic->valid());
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE(license_package_test, T, license_types, Fixture)
{
    util::temp_directory temp;

    std::shared_ptr<T> tmp_write_airgap = make_test_license<T>(temp.path());

    BOOST_REQUIRE(tmp_write_airgap->valid());

    auto config = ls_airgap_config {
        .productId = product_Id_test,
        .appName = appName_test,
        .appVersion = appVersion_test,
        .path = temp.path() / "test.lic" / "test.lic"
    };

    T tmp_write_airgap2(config, licenseKey_100);

    BOOST_REQUIRE(tmp_write_airgap2.valid());
}

// ---------------------------------------------------------------------

} // namespace
