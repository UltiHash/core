#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibLicensing License Check Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <LicenseSpring/EncryptString.h>

#include <licensing/check_airgap_license.h>
#include <licensing/check_online_license.h>
#include <licensing/license_package.h>
#include <licensing/soft_metred_storage_resource.h>
#include <io/temp_file.h>


using namespace uh;
using namespace uh::licensing;

namespace {

// ---------------------------------------------------------------------

    const std::filesystem::path TEMP_DIR = "/tmp";
    const std::string apiKey_test = EncryptStr("f43e779a-71bc-460f-af89-69a1c47cbe8b");
    const std::string sharedKey_test = EncryptStr("UtVbATx32BYf9QAQtLmcEJ4U5-58SezMIkeyb2Cy8l0");
    const std::string product_Id_test = EncryptStr("01");

    const std::string appName_test = "uh-data-node";
    const std::string appVersion_test = "0.2.0";

    const std::string licenseKey_100 = "GZFP-Y2EK-A8EK-2L01";
    const std::string user_name_test = "benjamin@ultihash.io";
    const std::string password_test = "#u5huzU!ita*o&I4@ona2+OVlGlhehe0!dLDeslticO#r?!3@*$a@$x*hl1lxisW";

// ---------------------------------------------------------------------

    typedef boost::mpl::vector<
            //check_online_license,
            check_airgap_license
    > license_types;

// ---------------------------------------------------------------------

    struct Fixture {
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
    std::unique_ptr<check_airgap_license> make_test_license<check_airgap_license>() {
        std::filesystem::path license_path1;
        {
            check_airgap_license tmp_write_airgap(TEMP_DIR, apiKey_test,
                                                  sharedKey_test, product_Id_test);
            tmp_write_airgap.write_license(check_license::role::DATA_NODE, appName_test,
                                           appVersion_test, licenseKey_100);
            license_path1 = tmp_write_airgap.getLicensePath();
        }

        return std::make_unique<check_airgap_license>(license_path1,
                                                      apiKey_test,
                                                      sharedKey_test,
                                                      product_Id_test);
    }

// ---------------------------------------------------------------------

    template<>
    std::unique_ptr<check_online_license> make_test_license<check_online_license>() {
        std::filesystem::path license_path1;
        {
            check_online_license tmp_write_online(TEMP_DIR, apiKey_test,
                                                  sharedKey_test, product_Id_test);
            tmp_write_online.write_license(check_license::role::DATA_NODE, appName_test,
                                           appVersion_test, user_name_test,
                                           password_test);
            license_path1 = tmp_write_online.getLicensePath();
        }

        return std::make_unique<check_online_license>(license_path1,
                                                      apiKey_test,
                                                      sharedKey_test,
                                                      product_Id_test);
    }

// ---------------------------------------------------------------------

    BOOST_FIXTURE_TEST_CASE_TEMPLATE(valid_default_license, T, license_types, Fixture)
    {
        auto lic = make_test_license<T>();

        BOOST_CHECK(lic->valid());
        std::filesystem::remove("/tmp/"+appName_test+".lic");
        std::filesystem::remove("/tmp/"+appName_test+".lic_spring");
    }

// ---------------------------------------------------------------------

    BOOST_AUTO_TEST_CASE(license_package_test)
    {
        std::filesystem::path license_path1;
        {
            check_airgap_license tmp_write_airgap(TEMP_DIR, apiKey_test,
                                                  sharedKey_test, product_Id_test);
            tmp_write_airgap.write_license(check_license::role::DATA_NODE, appName_test,
                                           appVersion_test, licenseKey_100);
            license_path1 = tmp_write_airgap.getLicensePath();
        }

        {
            license_package lp(check_license::role::DATA_NODE,
                               license_path1,
                               apiKey_test,
                               sharedKey_test,
                               product_Id_test);

            license_package lp2(check_license::role::DATA_NODE,
                                license_path1.parent_path(),
                                apiKey_test,
                                sharedKey_test,
                                product_Id_test);

            BOOST_CHECK(lp.check_feature_enabled(license_package::feature::DEDUPLICATION));
            BOOST_CHECK(lp.check_feature_enabled(license_package::feature::METRICS));

            auto *soft_right = new soft_metred_storage_resource(100, 50);

            BOOST_REQUIRE_THROW(
                    lp.add_soft_metred_feature(license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY,
                                               new soft_metred_storage_resource(50, 100)), util::exception);

            lp.add_soft_metred_feature(license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY,
                                       soft_right);

            BOOST_REQUIRE_THROW(lp.check_role_enabled(check_license::role::AGENCY_NODE), util::exception);
            BOOST_REQUIRE_THROW(lp.check_role_enabled(check_license::role::THROW_ROLE), util::exception);

            lp.check_role_enabled(check_license::role::DATA_NODE);

            BOOST_REQUIRE_THROW(lp.check_license_enabled(check_license::license_type::
                                                         FLOATING_ONLINE_USER_LICENSE), util::exception);
            BOOST_REQUIRE_THROW(lp.check_license_enabled(check_license::license_type::
                                                         THROW_LICENSE_TYPE), util::exception);

            lp.check_license_enabled(check_license::license_type::AIRGAP_LICENSE_WITH_ONLINE_ACTIVATION);

            BOOST_CHECK(lp.valid());

            auto initial_usable_storage = lp.free_count(static_cast<license_package::hard_metered_feature>
                                                        (license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY));

            BOOST_CHECK(lp.soft_limit_allocate(license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY, 25));
            BOOST_CHECK(lp.hard_limit_allocate(static_cast<license_package::hard_metered_feature>
                                               (license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY), 75));

            BOOST_CHECK_EQUAL(initial_usable_storage, lp.free_count(static_cast<license_package::hard_metered_feature>
            (license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY)) + 100);

            BOOST_REQUIRE_THROW(
                    lp.soft_limit_allocate(license_package::soft_metered_feature::LIMIT_NETWORK_CONNECTIONS, 1),
                    util::exception);

            BOOST_REQUIRE_EQUAL(
                    lp.soft_limit_allocate(license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY,
                                           1000000000000000),
                    false);

            BOOST_REQUIRE_THROW(lp.deallocate(static_cast<license_package::hard_metered_feature>
                                              (license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY), 101),
                                util::exception);

            BOOST_CHECK_NO_THROW(lp.deallocate(static_cast<license_package::hard_metered_feature>
                                               (license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY), 100));

            auto feature_fields = lp.getCustomAndFeatureFields();

            BOOST_CHECK(feature_fields.find(lp.WARN_STORAGE_STRING) != feature_fields.end());
            BOOST_CHECK(feature_fields.find(lp.LIMIT_STORAGE_STRING) != feature_fields.end());
            BOOST_CHECK(feature_fields.find(lp.METRICS_STRING) != feature_fields.end());
            BOOST_CHECK(feature_fields.find(lp.DEDUPLICATION_STRING) != feature_fields.end());

            delete soft_right;
        }

        {
            check_airgap_license tmp_write_airgap(license_path1, apiKey_test,
                                                  sharedKey_test, product_Id_test);
            BOOST_CHECK_THROW(tmp_write_airgap.write_license(check_license::role::DATA_NODE, appName_test,
                                                             appVersion_test, licenseKey_100), util::exception);
        }

        {
            check_airgap_license tmp_write_airgap2(license_path1, apiKey_test,
                                                   sharedKey_test, product_Id_test,
                                                   appVersion_test, appVersion_test,
                                                   true);
            BOOST_CHECK_NO_THROW(tmp_write_airgap2.write_license(check_license::role::DATA_NODE, appName_test,
                                                                 appVersion_test, licenseKey_100));
        }


        std::filesystem::remove("/tmp/"+appName_test+".lic");
        std::filesystem::remove("/tmp/"+appName_test+".lic_spring");
    }

// ---------------------------------------------------------------------

} // namespace
