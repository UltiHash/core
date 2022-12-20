//
// Created by benjamin-elias on 19.12.22.
//

#ifndef UHLIBCOMMON_TREE_STORAGE_H
#define UHLIBCOMMON_TREE_STORAGE_H

#include "conceptTypes/conceptTypes.h"
#include "logging/logging_boost.h"
#include <filesystem>
#include "boost/algorithm/hex.hpp"

namespace uh::trees{
#define N 256
#define STORE_MAX (unsigned int) std::numeric_limits<unsigned int>::max()
#define STORE_HARD_LIMIT (unsigned long) (std::numeric_limits<unsigned int>::max() * 2)
    typedef struct tree_storage tree_storage;

    struct tree_storage {
    protected:
        //every file storage level contains a maximum of 256 storage chunks and 256 folders to deeper levels
        std::size_t size[N]{};
        std::tuple<std::size_t,tree_storage*> children[N]{};//deeper tree storage blocks and folders
        //radix_tree* block_indexes[N]{}; // index local block finds
        std::filesystem::path combined_path{};

        //returns wrapped string
        static std::vector<unsigned char> prefix_wrap(std::size_t input_size){
            std::bitset<64> h_bit{input_size};
            unsigned char total_bit_count{};
            //counting max bits
            while(h_bit!=0)[[likely]]{
                total_bit_count++;
                h_bit>>=1;
            }
            unsigned char byte_count=total_bit_count/8;
            if(total_bit_count%8 or byte_count==0)[[likely]]{
                byte_count++;
            }
            std::vector<unsigned char> prefix{};

            auto mem_size_convert=std::array<unsigned char,sizeof(input_size)>();
            std::memcpy(mem_size_convert.data(),reinterpret_cast<unsigned char*>(&input_size),sizeof(input_size));
            for(unsigned char i=0; i<byte_count;i++){
                if constexpr (std::endian::native == std::endian::big){
                    prefix.push_back(mem_size_convert[byte_count-i-1]);
                }
                else{
                    prefix.push_back(mem_size_convert[i]);
                }
            }
            prefix.insert(prefix.cbegin(),byte_count);
            return prefix;
        }

        std::size_t prefix_unwrap_size(std::vector<unsigned char>::iterator in){
            return (std::size_t)(*in+1);
        }

        std::size_t prefix_unwrap_content_size(std::vector<unsigned char>::iterator &in){
            std::size_t byte_count = prefix_unwrap_size(in)-1;
            in++;
            std::vector<unsigned char> in_buf{in,in+static_cast<long>(byte_count)};
            in+=static_cast<long>(byte_count);
            std::size_t output_size{};
            while(in_buf.size()<sizeof(output_size))in_buf.push_back(0);
            if constexpr (std::endian::native == std::endian::big){
                auto mem_size_convert=std::array<unsigned char,sizeof(output_size)>();
                for(unsigned char i=0; i<byte_count;i++){
                    mem_size_convert[byte_count-i-1] = in_buf[i];
                }
                std::memcpy(reinterpret_cast<void *>(&output_size), mem_size_convert.data(), in_buf.size());
            }
            else{
                std::memcpy(reinterpret_cast<void *>(&output_size), in_buf.data(), in_buf.size());
            }
            return output_size;
        }

    public:
        explicit tree_storage(const std::filesystem::path& root){
            //expected are 4 bytes that mimic hexadecimal string representation
            std::string parent_name = root.filename().string();
            bool valid_root=parent_name.size()==4 and root.extension().string().empty();
            if(valid_root)for(const char &i:parent_name){
                    if(!(('0'<= i and i <= '9')||('A'<= i and i <= 'F'))){
                        valid_root=false;
                        break;
                    }
                }
            combined_path = root;
            if(!valid_root){
                combined_path /= "/0000";
            }
            std::filesystem::create_directories(combined_path);
            for(unsigned char i=0; ;i++){
                std::string s_tmp(reinterpret_cast<const char *>(i),1);
                std::filesystem::path chunk=combined_path/boost::algorithm::hex(s_tmp);
                if(std::filesystem::exists(chunk)){
                    size[i] = std::filesystem::file_size(chunk);
                }
                else size[i] = 0;

                s_tmp=std::string(combined_path.c_str()+2,combined_path.c_str()+4);
                s_tmp+=boost::algorithm::hex(std::string(reinterpret_cast<const char *>(i),1));
                std::filesystem::path deeper_tree=combined_path/s_tmp;
                //check if sub folder in tree exists
                if(std::filesystem::exists(deeper_tree)){
                    std::get<1>(children[i]) = new tree_storage(deeper_tree);
                    std::get<0>(children[i]) = std::get<1>(children[i])->get_size();
                }
                else{
                    std::get<0>(children[i]) = 0;
                    std::get<1>(children[i]) = nullptr;
                }
                if(i==(unsigned char)N)break;
            }
        }

        std::size_t get_size(){
            std::size_t s{};
            for(unsigned char i=0; ;i++){
                s+=size[i];
                if(std::get<1>(children[i]) != nullptr){
                    s+=std::get<0>(children[i]);
                }
                if(i==(unsigned char)N)break;
            }
            return s;
        }

        //write a string and get size of written block representation and a reference string back
        std::tuple<std::size_t,std::string> write(const std::vector<unsigned char>& input){
            if(input.size()>STORE_MAX){
                FATAL << "A block could not be written because it exceeded maximum size of blocks \""+std::to_string(STORE_MAX)+
                "\" with a size of \""+std::to_string(input.size())+"\".";
                std::exit(EXIT_FAILURE);
            }
            std::vector<unsigned char> prefix = prefix_wrap(input.size());
            std::size_t total_size = input.size() + prefix.size();
            //check block fill of this node, look for free space
            std::size_t min_val=size[0];
            unsigned char min_pos{};
            for(unsigned char i=0; ;i++){
                if(size[i]<min_val){
                    min_val = size[i];
                    min_pos = i;
                }
                if(i==(unsigned char)N)break;
            }
            if(min_val < STORE_MAX && min_val+total_size < STORE_HARD_LIMIT){
                //store block to this position
                std::filesystem::path read_chunk = combined_path/boost::algorithm::hex(std::string(reinterpret_cast<const char *>(min_pos),1));
                FILE* writer = std::fopen(read_chunk.c_str(), "ab+");
                if(!writer) {
                    ERROR << "File opening failed at \""+read_chunk.string()+"\"";
                    std::exit(EXIT_FAILURE);
                }
                //File should have been opened or created here
                std::fwrite(prefix.data(),prefix.size(), sizeof(unsigned char),writer);
                std::fwrite(input.data(),input.size(), sizeof(unsigned char),writer);

                if (std::ferror(writer)) {
                    FATAL << "I/O error when reading \""+read_chunk.string()+"\"";
                    std::exit(EXIT_FAILURE);
                }
                std::fclose(writer);
            }
            else{
                //find or create balanced deeper tree node to store
            }
        }
        
    };
}

#endif //UHLIBCOMMON_TREE_STORAGE_H
