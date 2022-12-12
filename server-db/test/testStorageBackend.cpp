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
#include <bits/stdc++.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include "../src/storage_backend.h"


struct db_test_config{
    fs::path test_db_dir = "./TEST_DB_DIR";
    fs::path test_input_dir = "./TEST_INPUT_DIR";
    fs::path test_incoming_file_name = "test_input_file.txt";
    fs::path test_incoming_file_name_2 = "test_input_file_2.txt";
    fs::path test_input_filepath = test_input_dir / test_incoming_file_name;
    fs::path test_input_filepath_2 = test_input_dir / test_incoming_file_name_2;
    std::string contents_str = "These are the contents of test_input_file.txt and test_input_file_2.txt";
    //std::string expected_sha512_hash = "57579fadafb55d4fe92f84c699d8cce55d238a1b1747914f56c9f1c35fbb35f6ba41b986f071ef1da60f0f4482339860d4bbcccf7b24336c5ae44f47c73acdbe";
    //std::string expected_sha512_hash = "8a5cc075bbca891c91a1000b5fc5858d96c79bae07eb3498569df2ed9fa5def8d205d717e117bcbeeab18cc061072687c93e6203f267e691dfb8053d62653878";
    std::string expected_sha512_hash = "2610fa1ed2dc40f92a3e44cb894b757e4e4469a053b5b2ccf69179b577cfac29403aed645ecab45e10c5db2d9c6bbb0916b0b7c9caa635d271f5274b3e868011";
};

void create_dir(fs::path dirpath){
    if(!(boost::filesystem::exists(dirpath))) {
        if (mkdir(dirpath.c_str(), 0777) == -1)
            std::cerr << "Error :  " << strerror(errno) << std::endl;
        else
            std::cerr << "Directory created: " << dirpath.string();
    }
}

void write_input_test_file(fs::path test_input_filepath){
    db_test_config c;
    fs::path filepath = test_input_filepath;
    if(!(boost::filesystem::exists(filepath))) {

        std::string msg("Path does not exist: " + filepath.string());

        std::ofstream outfile(filepath, std::ios::out | std::ios::binary);
        if(!outfile.is_open()) {
            std::string msg("Error opening file: " + filepath.string());
            throw std::ofstream::failure(msg);
        }

        std::string contents_str = c.contents_str;
        std::vector<char> some_data(contents_str.begin(), contents_str.end());
        outfile.write(some_data.data(), some_data.size());
        std::cerr << "Test file written: " << filepath.string();
    }
}

uh::dbn::db_config create_test_db_and_file(){
    db_test_config c;
    fs::path test_db_dir = c.test_db_dir;
    fs::path test_input_dir = c.test_input_dir;
    create_dir(test_db_dir);
    create_dir(test_input_dir);
    write_input_test_file(c.test_input_filepath);
    write_input_test_file(c.test_input_filepath_2);
    uh::dbn::db_config config;
    config.db_dir = test_db_dir;
    return config;
}

bool test_storage_backend_io(uh::dbn::storage_backend &uhsb){
    bool tf = false;
    db_test_config c;
    fs::path input_filepath = c.test_input_filepath;
    std::cout << "original filepath: " << input_filepath << std::endl;
    std::ifstream infile(input_filepath, std::ios::binary);
    std::vector<char> x(std::istreambuf_iterator<char>(infile), {});

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

std::vector<char> write_block_from_file(fs::path input_filepath, uh::dbn::storage_backend &uhsb){

    std::cout << "original filepath: " << input_filepath << std::endl;
    std::ifstream infile(input_filepath, std::ios::binary);
    std::vector<char> x(std::istreambuf_iterator<char>(infile), {});

    // Write block.
    std::vector<char> hash_key = uhsb.write_block(x);
    return hash_key;
}

// ------------- Tests Follow --------------

BOOST_AUTO_TEST_CASE( hashing_function_expected_hash )
{
    db_test_config c;
    std::vector<char> vec_input(c.contents_str.begin(), c.contents_str.end());
    std::vector<char> vec_hash = uh::dbn::sha512(vec_input);
    std::string hash_string = uh::dbn::to_hex_string(vec_hash.begin(), vec_hash.end());

    BOOST_CHECK(hash_string == c.expected_sha512_hash);
}

BOOST_AUTO_TEST_CASE( dump_storage_io )
{
    bool success = false;
    db_test_config c;
    uh::dbn::db_config config;
    config = create_test_db_and_file();
    fs::path input_filepath = c.test_input_filepath;

    uh::dbn::dump_storage uhsb(config);
    success = test_storage_backend_io(uhsb);

    BOOST_CHECK(success);
}

BOOST_AUTO_TEST_CASE( dump_storage_no_duplicates )
{
    db_test_config c;
    uh::dbn::db_config config;
    config = create_test_db_and_file();
    // Create backend.
    uh::dbn::dump_storage uhsb(config);

    // File 1 and file 2 should produce the same hash key. Since teh hash key is in 1-1 correspondence with the contents
    // of the written file, no duplicates will be written.
    std::vector<char> x = write_block_from_file(c.test_input_filepath, uhsb);
    std::vector<char> y = write_block_from_file(c.test_input_filepath_2, uhsb);

    BOOST_CHECK(x == y);
}

BOOST_AUTO_TEST_CASE( dump_storage_expected_hash )
{
    db_test_config c;
    uh::dbn::db_config config;
    config = create_test_db_and_file();

    // Create backend.
    uh::dbn::dump_storage uhsb(config);

    std::vector<char> x = write_block_from_file(c.test_input_filepath, uhsb);
    std::string x_str = uh::dbn::to_hex_string(x.begin(), x.end());

    BOOST_CHECK(x_str == c.expected_sha512_hash);
}
