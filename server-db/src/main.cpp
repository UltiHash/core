#include <iostream>
#include <string>
#include "storage_backend.h"

using namespace uh::dbn;

//TODO - Consider using mmap
int main(){
    // initialize the UltiHash Storage Backend
    // config contains the location of the db in the filesystem
    uh::dbn::db_config config;

    if(!(boost::filesystem::exists(config.db_dir))) {
        std::string msg("Path does not exist: " + config.db_dir.string());
        throw std::runtime_error(msg);
    }
    storage_backend uhsb(config);

    std::string s = "TheseAreTheContentsOfTheFile";
    std::vector<char> x(s.begin(), s.end());

    // Write block.
    std::vector<char> hash_key = uhsb.write_block(x);

    // Read block.
    std::vector<char> y = uhsb.read_block(hash_key);

    // Check that what was read is the same as what was written.
    if(y == x){
       std::cout << "Yes!" << std::endl;
    }
}
