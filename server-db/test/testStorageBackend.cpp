//
// Created by juan on 01.12.22.
//
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhServerDb Dummy Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <metrics/mod.h>
#include <storage/mod.h>


struct db_test_config{
    std::filesystem::path test_db_dir = "./TEST_DB_DIR";
    std::filesystem::path test_input_dir = "./TEST_INPUT_DIR";
    std::filesystem::path test_incoming_file_name = "test_input_file.txt";
    std::filesystem::path test_incoming_file_name_2 = "test_input_file_2.txt";
    std::filesystem::path test_input_filepath = test_input_dir / test_incoming_file_name;
    std::filesystem::path test_input_filepath_2 = test_input_dir / test_incoming_file_name_2;
    std::size_t test_allocated_bytes = 1e6;
    std::string contents_str = "These are the contents of test_input_file.txt and test_input_file_2.txt";
    //std::string expected_sha512_hash = "57579fadafb55d4fe92f84c699d8cce55d238a1b1747914f56c9f1c35fbb35f6ba41b986f071ef1da60f0f4482339860d4bbcccf7b24336c5ae44f47c73acdbe";
    //std::string expected_sha512_hash = "8a5cc075bbca891c91a1000b5fc5858d96c79bae07eb3498569df2ed9fa5def8d205d717e117bcbeeab18cc061072687c93e6203f267e691dfb8053d62653878";
    std::string expected_sha512_hash = "2610fa1ed2dc40f92a3e44cb894b757e4e4469a053b5b2ccf69179b577cfac29403aed645ecab45e10c5db2d9c6bbb0916b0b7c9caa635d271f5274b3e868011";
};

void create_dir(std::filesystem::path dirpath){
    if(!(std::filesystem::exists(dirpath))) {
        if (mkdir(dirpath.c_str(), 0777) == -1)
            std::cerr << "Error :  " << strerror(errno) << std::endl;
        else
            std::cerr << "Directory created: " << dirpath.string();
    }
}

void write_input_test_file(std::filesystem::path test_input_filepath){
    db_test_config c;
    std::filesystem::path filepath = test_input_filepath;
    if(!(std::filesystem::exists(filepath))) {

        std::string msg("Path does not exist: " + filepath.string());

        std::ofstream outfile(filepath, std::ios::out | std::ios::binary);
        if(!outfile.is_open()) {
            std::string msg("Error opening file: " + filepath.string());
            throw std::ofstream::failure(msg);
        }

        std::string contents_str = c.contents_str;
        uh::protocol::blob some_data(contents_str.begin(), contents_str.end());
        outfile.write(some_data.data(), some_data.size());
        std::cerr << "Test file written: " << filepath.string();
    }
}

void create_test_db_and_file(){
    db_test_config c;
    create_dir(c.test_db_dir);
    create_dir(c.test_input_dir);
    write_input_test_file(c.test_input_filepath);
    write_input_test_file(c.test_input_filepath_2);
}

bool test_storage_backend_io(uh::dbn::storage::mod &mod){
    db_test_config c;
    bool tf = false;
    std::filesystem::path input_filepath = c.test_input_filepath;
    std::cout << "original filepath: " << input_filepath << std::endl;
    std::ifstream infile(input_filepath, std::ios::binary);
    uh::protocol::blob x(std::istreambuf_iterator<char>(infile), {});

    // Write chunk.
    uh::protocol::blob hash_key = mod.write_chunk(x);

    // Read chunk.
    uh::protocol::blob y = mod.read_chunk(hash_key);

    // Check that what was read is the same as what was written.
    if(y == x){
        tf = true;
    }

    return tf;
}

uh::protocol::blob write_chunk_from_file(std::filesystem::path input_filepath, uh::dbn::storage::mod &mod){

    std::cout << "original filepath: " << input_filepath << std::endl;
    std::ifstream infile(input_filepath, std::ios::binary);
    uh::protocol::blob x(std::istreambuf_iterator<char>(infile), {});

    // Write chunk.
    uh::protocol::blob hash_key = mod.write_chunk(x);
    return hash_key;
}

// ------------- Tests Follow --------------

BOOST_AUTO_TEST_CASE( hashing_function_expected_hash )
{
    db_test_config c;
    uh::protocol::blob vec_input(c.contents_str.begin(), c.contents_str.end());
    uh::protocol::blob vec_hash = uh::dbn::storage::sha512(vec_input);
    std::string hash_string = uh::dbn::storage::to_hex_string(vec_hash.begin(), vec_hash.end());

    BOOST_CHECK(hash_string == c.expected_sha512_hash);
}

BOOST_AUTO_TEST_CASE( dump_storage_io )
{
    bool success = false;

    db_test_config c;
    uh::dbn::storage::storage_config cfg;
    cfg.db_root = c.test_db_dir;
    cfg.allocate_bytes = c.test_allocated_bytes;
    create_test_db_and_file();

    // Create backend.
    uh::dbn::metrics::mod metrics_module({});
    uh::dbn::storage::mod storage_module(cfg, metrics_module.storage());
    storage_module.start();
    success = test_storage_backend_io(storage_module);

    BOOST_CHECK(success);
}

BOOST_AUTO_TEST_CASE( dump_storage_no_duplicates )
{
    db_test_config c;
    uh::dbn::storage::storage_config cfg;
    cfg.db_root = c.test_db_dir;
    cfg.allocate_bytes = c.test_allocated_bytes;
    create_test_db_and_file();

    // Create backend.
    uh::dbn::metrics::mod metrics_module({});
    uh::dbn::storage::mod storage_module(cfg, metrics_module.storage());
    storage_module.start();

    // File 1 and file 2 should produce the same hash key. Since teh hash key is in 1-1 correspondence with the contents
    // of the written file, no duplicates will be written.
    uh::protocol::blob x = write_chunk_from_file(c.test_input_filepath, storage_module);
    uh::protocol::blob y = write_chunk_from_file(c.test_input_filepath_2, storage_module);

    BOOST_CHECK(x == y);
}

BOOST_AUTO_TEST_CASE( dump_storage_expected_hash )
{
    db_test_config c;
    uh::dbn::storage::storage_config cfg;
    cfg.db_root = c.test_db_dir;
    cfg.allocate_bytes = c.test_allocated_bytes;
    create_test_db_and_file();

    // Create backend.
    uh::dbn::metrics::mod metrics_module({});
    uh::dbn::storage::mod storage_module(cfg, metrics_module.storage());
    storage_module.start();

    uh::protocol::blob x = write_chunk_from_file(c.test_input_filepath, storage_module);
    std::string x_str = uh::dbn::storage::to_hex_string(x.begin(), x.end());

    BOOST_CHECK(x_str == c.expected_sha512_hash);
}
