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
        std::string prefix_wrap(std::size_t input_size){
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
                    prefix.insert(prefix.cbegin(),mem_size_convert[byte_count-i-1]);
                }
                else{
                    prefix.insert(prefix.cbegin(),mem_size_convert[i]);
                }
            }
            prefix.insert(prefix.cbegin(),byte_count);
            return std::string{prefix.cbegin(),prefix.cend()};
        }

        std::size_t prefix_unwrap(std::string::iterator in){
            return (std::size_t)(reinterpret_cast<unsigned char*>(*in)[0])+1;
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

        //write a string and get a reference string back
        std::string write(const std::string& input){

        }
        
    };
}

#endif //UHLIBCOMMON_TREE_STORAGE_H
