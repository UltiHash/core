//
// Created by tariq on 5/10/22.
//

#include "Data.h"

std::size_t Data::getNumBlocks() const {
    return numBlocks;
}

bool Data::emptyObject() const {
    return Prefix::emptyObject();
}

unsigned short Data::folderEnd() const {
    return Prefix::folderEnd();
}

bool Data::is_regular_file() const {
    return Prefix::is_regular_file();
}

std::string Data::getObjectName() {
    return this->object_name;
}

Data::Data(const std::string &db_config) : Prefix(){
    db_config_file=db_config;
}

Data::Data(const std::string &in, unsigned short folderE, const std::string &db_config) : Prefix(in, folderE) {
    std::cout << "Write to recompilation file: " << in <<  std::endl;
    db_config_file=db_config;
    if(this->is_regular_file() and not this->emptyObject())[[likely]]{
        std::ifstream myFile(in,std::ios::in | std::ios::binary);
        if(myFile.is_open()){
            const unsigned int buf_size = 1 << 22;
            std::ostringstream ss;
            ss << myFile.rdbuf();
            ss.flush();
            myFile.close();
            //split remember blocks
            std::size_t count=0;
            std::string block_string{};
            block_string.reserve(buf_size);
            for(const auto &i:std::string{ss.str()}){
                block_string.push_back(i);
                count++;
                if(count>buf_size)[[unlikely]]{
                    std::string individual_block_string{block_string.begin(),block_string.end()};
                    hash_blocks.emplace_back(individual_block_string);
                    block_string.clear();
                    block_string.reserve(buf_size);
                    count=0;
                }
            }
            hash_blocks.emplace_back(block_string);
            numBlocks=hash_blocks.size();
        }
        else{
            ERROR << "File " << in << " was not found or could not be opened!" << std::endl;
        }
    }
}
