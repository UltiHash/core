//
// Created by masi on 7/24/23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "transaction_log tests"
#endif

#include <boost/test/unit_test.hpp>
#include <directory_node/transaction_log.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

struct config_fixture
{
    static std::filesystem::path get_log_path () {
        return "_tmp_transaction_log_test_path";
    }

    static void cleanup () {
        std::filesystem::remove ("_tmp_transaction_log_test_path");
    }
};


// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE (transaction_log_test, config_fixture)
{

    cleanup();

    {
        uh::cluster::transaction_log kl(get_log_path());
        const auto m1 = kl.replay();
        BOOST_CHECK(m1.empty());

        kl.append("key1", 100, transaction_log::INSERT_START);
        kl.append("key2", 200, transaction_log::INSERT_START);
        kl.append("key1", 100, transaction_log::INSERT_END);
        kl.append("key2", 200, transaction_log::INSERT_END);
        kl.append("key3", 300, transaction_log::INSERT_START);
        kl.append("key3", 300, transaction_log::INSERT_END);
        kl.append("key4", 400, transaction_log::INSERT_START);
        kl.append("key5", 500, transaction_log::INSERT_START);
        kl.append("key5", 500, transaction_log::INSERT_END);
        kl.append("key4", 400, transaction_log::INSERT_END);
        kl.append("key1", 100, transaction_log::INSERT_START);
        kl.append("key1", 100, transaction_log::INSERT_END);

        const auto m2 = kl.replay();
        BOOST_TEST (m2.size() == 5);
        BOOST_CHECK(m2.at("key1") == 100);
        BOOST_CHECK(m2.at("key2") == 200);
        BOOST_CHECK(m2.at("key3") == 300);
        BOOST_CHECK(m2.at("key4") == 400);
        BOOST_CHECK(m2.at("key5") == 500);

        kl.append("key1", 0, transaction_log::REMOVE_START);
        kl.append("key2", 0, transaction_log::REMOVE_START);
        kl.append("key1", 0, transaction_log::REMOVE_END);
        kl.append("key2", 0, transaction_log::REMOVE_END);
    }

    {
        uh::cluster::transaction_log kl(get_log_path());
        const auto m1 = kl.replay();
        BOOST_TEST (m1.size() == 3);
        BOOST_CHECK(m1.at("key3") == 300);
        BOOST_CHECK(m1.at("key4") == 400);
        BOOST_CHECK(m1.at("key5") == 500);


        kl.append("key3", 600, transaction_log::UPDATE_START);
        kl.append("key5", 1000, transaction_log::UPDATE_START);
        kl.append("key3", 600, transaction_log::UPDATE_END);
        kl.append("key5", 1000, transaction_log::UPDATE_END);

        const auto m2 = kl.replay();
        BOOST_TEST (m2.size() == 3);
        BOOST_CHECK(m2.at("key3") == 600);
        BOOST_CHECK(m2.at("key4") == 400);
        BOOST_CHECK(m2.at("key5") == 1000);

        kl.append("key6", 600, transaction_log::INSERT_START);
    }

    {
        uh::cluster::transaction_log kl(get_log_path());
        BOOST_CHECK_THROW (kl.replay(), std::runtime_error);
    }

    cleanup();
}

// ---------------------------------------------------------------------

} // end namespace uh::cluster
