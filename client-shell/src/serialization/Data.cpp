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

Data::Data(uh::protocol::client& client) : m_client(client) {
}

Data::Data(const std::string &in, unsigned short folderE, uh::protocol::client& client) : Prefix(in, folderE), m_client(client) {
    std::cout << "Write to recompilation file: " << in <<  "\n\n";
    if(this->is_regular_file() and not this->emptyObject())[[likely]]{
        std::ifstream myFile(in,std::ios::in | std::ios::binary);
        if(myFile.is_open()){
            const unsigned int buf_size = 1 << 22;
            std::ostringstream ss;

            // benchmarking read operation
            auto start = std::chrono::steady_clock::now();
            ss << myFile.rdbuf();
            ss.flush();
            auto end = std::chrono::steady_clock::now();
            INFO << "Read to string stream: Size " << myFile.tellg()  <<  "Bytes - Time " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns, "
                << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " µs, "
                << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms, "
                << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " sec";

            myFile.close();
            //split remember blocks
            std::size_t count=0;
            std::string block_string{};
            block_string.reserve(buf_size);

            start = std::chrono::steady_clock::now();
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
            end = std::chrono::steady_clock::now();
            INFO << "Splitting to Blocks: Time " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns, "
                 << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " µs, "
                 << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms, "
                 << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " sec";

            hash_blocks.emplace_back(block_string);
            numBlocks=hash_blocks.size();
        }
        else{
            ERROR << "File " << in << " was not found or could not be opened!" << std::endl;
        }
    }
}

