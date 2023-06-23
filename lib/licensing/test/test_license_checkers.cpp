#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibLicensing License Check Tests"
#endif

#include <boost/test/unit_test.hpp>

#include <util/exception.h>
#include <util/temp_dir.h>
#include <licensing/license_spring.h>
#include <licensing/license_package.h>


using namespace uh;
using namespace uh::licensing;

namespace
{

// ---------------------------------------------------------------------

const std::string product_Id_test = "uhtest";
const std::string appName_test = "uh-test";
const std::string appVersion_test = "0.2.0";
const std::string licenseKey_test = "GZM9-S88G-RNEK-2EUH";

// ---------------------------------------------------------------------

auto mk_ls_config(const std::filesystem::path& path)
{
    // TODO why do we need to give the filename twice
    return license_spring_config {
        .productId = product_Id_test,
        .appName = appName_test,
        .appVersion = appVersion_test,
        .path = path / "test.lic" / "test.lic"
    };
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(valid_default_license)
{
    util::temp_directory temp;
    license_package pkg(licensing::config{
        .ls_config = mk_ls_config(temp.path()),
        .activation_key = licenseKey_test });

    BOOST_CHECK(pkg.valid());

    BOOST_CHECK(pkg.check(feature::STORAGE));
    BOOST_CHECK_NO_THROW(pkg.require(feature::STORAGE, 200000));
    BOOST_CHECK_THROW(pkg.require(feature::STORAGE, 1000000), util::exception);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(ls_activate)
{
    util::temp_directory temp;

    {
        license_spring lic(mk_ls_config(temp.path()), licenseKey_test);

        BOOST_REQUIRE(lic.valid());
    }

    {
        license_spring lic(mk_ls_config(temp.path()));

        BOOST_REQUIRE(lic.valid());
    }
}

// ---------------------------------------------------------------------

} // namespace
