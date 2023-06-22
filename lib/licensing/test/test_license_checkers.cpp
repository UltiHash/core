#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibLicensing License Check Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <licensing/check_airgap_license.h>
#include <licensing/license_package.h>


using namespace uh;
using namespace uh::licensing;

namespace
{

// ---------------------------------------------------------------------

const std::filesystem::path TEMP_DIR = "/tmp";

const std::string product_Id_test = EncryptStr("01");

const std::string appName_test = "uh-data-node";

const std::string appVersion_test = "0.2.0";

const std::string licenseKey_100 = "GZLF-TD88-AZAK-2F01";

const std::string user_name_test = "benjamin@ultihash.io";

const std::string password_test = "#u5huzU!ita*o&I4@ona2+OVlGlhehe0!dLDeslticO#r?!3@*$a@$x*hl1lxisW";

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
std::unique_ptr<T> make_test_license();

// ---------------------------------------------------------------------

template<>
std::unique_ptr<check_airgap_license> make_test_license<check_airgap_license>()
{
    auto lic_config = license_config();
    lic_config.license_path = TEMP_DIR / (appName_test + ".lic");

    auto api = api_config{ product_Id_test };
    auto credential = credential_config{ appName_test, appVersion_test };
    auto activate = license_activate_config{ .key = licenseKey_100 };

    return std::make_unique<check_airgap_license>(lic_config,
                                                  api,
                                                  credential,
                                                  activate);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE(valid_default_license, T, license_types, Fixture)
{
    std::filesystem::remove("/tmp/" + appName_test + ".lic");

    auto lic = make_test_license<T>();

    BOOST_CHECK(lic->valid());
    std::filesystem::remove("/tmp/" + appName_test + ".lic");
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE(license_package_test, T, license_types, Fixture)
{
    std::filesystem::remove("/tmp/" + appName_test + ".lic");

    std::shared_ptr<T> tmp_write_airgap = make_test_license<T>();

    BOOST_REQUIRE(tmp_write_airgap->valid());

    auto lic_config = license_config();
    lic_config.license_path = TEMP_DIR;

    auto api = api_config{ product_Id_test };
    auto credential = credential_config{ appName_test, appVersion_test };
    auto activate = license_activate_config{ .key = licenseKey_100 };

    T tmp_write_airgap2(lic_config,
                        api,
                        credential,
                        activate);

    BOOST_REQUIRE(tmp_write_airgap2.valid());

    if constexpr (std::is_same_v<T,check_airgap_license>){
        license_package lp(tmp_write_airgap);

        BOOST_CHECK(lp.feature_enabled(feature::DEDUPLICATION));
        BOOST_CHECK(lp.feature_enabled(feature::METRICS));

        BOOST_REQUIRE_THROW(
            lp.add_metred_feature(metered_feature::LIMIT_STORAGE_CAPACITY,
                                       std::make_shared<metered_resource>(50, 100)), util::exception);

        lp.add_metred_feature(metered_feature::LIMIT_STORAGE_CAPACITY,
                                   std::make_unique<metered_resource>(100, 50));

        BOOST_REQUIRE_THROW(
            lp.check(metered_feature::LIMIT_NETWORK_CONNECTIONS, 1),
            util::exception);

        BOOST_REQUIRE_THROW(
            lp.check(metered_feature::LIMIT_STORAGE_CAPACITY, 1000000000000000),
            std::exception);

        auto feature_fields = tmp_write_airgap->getCustomAndFeatureFields();

        BOOST_CHECK(feature_fields.find(WARN_STORAGE_STRING) != feature_fields.end());
        BOOST_CHECK(feature_fields.find(LIMIT_STORAGE_STRING) != feature_fields.end());
        BOOST_CHECK(lp.has_metred_feature(uh::licensing::metered_feature::LIMIT_STORAGE_CAPACITY));

        BOOST_CHECK(feature_fields.find(lp.METRICS_STRING) != feature_fields.end());
        BOOST_CHECK(feature_fields.find(lp.DEDUPLICATION_STRING) != feature_fields.end());
    }

    std::filesystem::remove("/tmp/" + appName_test + ".lic");
}

// ---------------------------------------------------------------------

} // namespace
