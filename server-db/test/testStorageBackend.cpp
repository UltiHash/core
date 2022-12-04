//
// Created by juan on 01.12.22.
//
#define BOOST_TEST_DYN_LINK
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhServerDb Dummy Tests"
#endif

#include <boost/test/unit_test.hpp>
#include "../src/storage_backend.h"

bool test_storage_backend_io(uh::dbn::storage_backend &uhsb){
    bool tf = false;

    std::string s = "TheseAreTheContentsOfTheFile";
    std::vector<char> x(s.begin(), s.end());

    // Write block.
    std::vector<char> hash_key = uhsb.write_block(x);

    // Read block.
    std::vector<char> y = uhsb.read_block(hash_key);

    // Check that what was read is the same as what was written.
    if(y == x){
        tf = true;
    }

    return tf;
}

// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( uhsb )
{
    uh::dbn::db_config config;

    //TODO
    // Create real tests

    //if(!(boost::filesystem::exists(config.db_dir))) {
    //    std::string msg("Path does not exist: " + config.db_dir.string());
    //    throw std::runtime_error(msg);
    //}
    uh::dbn::dump_storage uhsb(config);

    BOOST_CHECK(true);
}
