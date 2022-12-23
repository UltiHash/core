//
// Created by benjamin-elias on 19.12.22.
//

#ifndef UHLIBCOMMON_TREE_STORAGE_H
#define UHLIBCOMMON_TREE_STORAGE_H

#include "conceptTypes/conceptTypes.h"
#include "logging/logging_boost.h"
#include <filesystem>
#include "boost/algorithm/hex.hpp"
#include <openssl/sha.h>

namespace uh::trees {
#define N 2
#define STORE_MAX (unsigned int) std::numeric_limits<unsigned int>::max()
#define STORE_HARD_LIMIT (unsigned long) (std::numeric_limits<unsigned int>::max() * 2)
    typedef struct tree_storage tree_storage;

    struct tree_storage {
    protected:
        //every file storage level contains a maximum of 256 storage chunks and 256 folders to deeper levels
        std::size_t size[N]{};
        std::tuple<std::size_t, tree_storage *> children[N]{};//deeper tree storage blocks and folders
        //radix_tree* block_indexes[N]{}; // index local block finds
        std::filesystem::path combined_path{};

        //returns wrapped string
        static std::vector<unsigned char> prefix_wrap(std::size_t input_size) {
            std::size_t h_bit{input_size};
            unsigned char total_bit_count{};
            //counting max bits
            while (h_bit != 0)[[likely]] {
                total_bit_count++;
                h_bit >>= 1;
            }
            unsigned char byte_count = total_bit_count / 8;
            std::vector<unsigned char> prefix{};
            for (unsigned char i = 0; i < byte_count; i++) {
                prefix.push_back((unsigned char) (input_size >> (i * 8)));
            }
            if (prefix.empty())prefix.push_back(0);
            prefix.insert(prefix.cbegin(), byte_count);
            return prefix;
        }

    public:
        explicit tree_storage(const std::filesystem::path &root) {
            //expected are 4 bytes that mimic hexadecimal string representation
            std::string parent_name = root.filename().string();
            bool valid_root = parent_name.size() == 4 and root.extension().string().empty();
            if (valid_root)
                for (const char &i: parent_name) {
                    if (!(('0' <= i and i <= '9') || ('A' <= i and i <= 'F'))) {
                        valid_root = false;
                        break;
                    }
                }
            combined_path = root;
            if (!valid_root) {
                combined_path /= "0000";
            }
            std::filesystem::create_directories(combined_path);
            for (unsigned short i = 0; i < (unsigned short) N; i++) {
                std::string s_tmp = boost::algorithm::hex(std::string{(char)i});
                std::filesystem::path chunk = combined_path / s_tmp;
                if (std::filesystem::exists(chunk)) {
                    size[i] = std::filesystem::file_size(chunk);
                } else size[i] = 0;

                std::string fname = combined_path.filename().string();
                s_tmp.insert(s_tmp.cbegin(),fname.cbegin() + 2, fname.cbegin() + 4);
                std::filesystem::path deeper_tree = combined_path / s_tmp;
                //check if sub folder in tree exists
                if (std::filesystem::exists(deeper_tree)) {
                    std::get<1>(children[i]) = new tree_storage(deeper_tree);
                    std::get<0>(children[i]) = std::get<1>(children[i])->get_size();
                } else {
                    std::get<0>(children[i]) = 0;
                    std::get<1>(children[i]) = nullptr;
                }
            }
        }

        std::size_t get_size() {
            std::size_t s{};
            for (unsigned short i = 0; i < (unsigned short) N; i++) {
                s += size[i];
                if (std::get<1>(children[i]) != nullptr) {
                    s += std::get<0>(children[i]);
                }
            }
            return s;
        }

        //write a string and get size of written block representation and a reference string back
        std::vector<unsigned char> write(const std::vector<unsigned char> &input) {
            if(input.empty())return std::vector<unsigned char>{};
            /*
            if (input.size() < 16) {
                std::string s_tmp = boost::algorithm::hex(std::string{input.begin(), input.end()});
                ERROR << "Tried to write a binary string smaller 16 unsigned char elements. This is not allowed!"
                         "The input was \"" + s_tmp + "\"";
                return std::vector<unsigned char>{};
            }*/
            if (input.size() > STORE_MAX) {
                FATAL << "A block could not be written because it exceeded maximum size of blocks \"" +
                         std::to_string(STORE_MAX) +
                         "\" with a size of \"" + std::to_string(input.size()) + "\".";
                std::exit(EXIT_FAILURE);
            }
            std::vector<unsigned char> prefix = prefix_wrap(input.size());
            std::size_t total_size = input.size() + prefix.size();
            //check block fill of this node, look for free space
            std::size_t min_val = size[0];
            unsigned char min_pos{};
            for (unsigned short i = 0; i < (unsigned short) N; i++) {
                if (size[i] < min_val) {
                    min_val = size[i];
                    min_pos = (unsigned char) i;
                }
            }
            if (min_val < STORE_MAX && min_val + total_size < STORE_HARD_LIMIT) {
                //store block to this position
                std::string ref_name{boost::algorithm::hex(std::string{(char) min_pos})};
                std::filesystem::path read_chunk = combined_path / ref_name;
                FILE *writer = std::fopen(read_chunk.c_str(), "ab+");
                if (!writer) {
                    ERROR << "File write opening failed at \"" + read_chunk.string() + "\"";
                    std::exit(EXIT_FAILURE);
                }
                //File should have been opened or created here
                std::fwrite(prefix.data(), prefix.size(), sizeof(unsigned char), writer);
                std::fwrite(input.data(), input.size(), sizeof(unsigned char), writer);

                if (std::ferror(writer)) {
                    FATAL << "I/O error when writing \"" + read_chunk.string() + "\"";
                    std::exit(EXIT_FAILURE);
                }
                std::fclose(writer);
                //start position of the block for seeking it later on is the old size
                std::vector<unsigned char> out_vec;
                out_vec.reserve(sizeof(unsigned int));
                for (unsigned char i = 0; i < sizeof(unsigned int); i++) {//STORE_MAX will fit in 4 bytes
                    out_vec.push_back((unsigned char) (size[min_pos] >> (i * 8)));
                }
                out_vec.insert(out_vec.cbegin(), min_pos);
                size[min_pos] += total_size;
                return out_vec;//structure: BIN,OFFSET * 4 BYTES
            } else {
                //find or create balanced deeper tree node to store
                min_val = std::get<0>(children[0]);
                min_pos = 0;
                for (unsigned short i = 0; i < (unsigned short) N; i++) {
                    if (std::get<0>(children[i]) < min_val) {
                        min_val = std::get<0>(children[i]);
                        min_pos = i;
                    }
                }
                if (std::get<1>(children[min_pos]) == nullptr) {
                    std::string ref_name{boost::algorithm::hex(std::string{(char) min_pos})};
                    std::string fname = combined_path.filename().string();
                    ref_name.insert(ref_name.cbegin(),fname.cbegin() + 2, fname.cbegin() + 4);
                    std::filesystem::path deeper_tree = combined_path / ref_name;
                    std::get<1>(children[min_pos]) = new tree_storage(deeper_tree);
                }
                std::vector<unsigned char> out_vec = std::get<1>(children[min_pos])->write(input);
                out_vec.insert(out_vec.cbegin(), min_pos);
                std::get<0>(children[min_pos]) += total_size;
                return out_vec;
            }
        }

        std::vector<unsigned char> read(const std::vector<unsigned char> &block_code) {
            if(block_code.empty()){
                return {};
            }
            if (block_code.size() > 5) {
                //size encoding is not reached yet, read along tree path
                if (std::get<1>(children[block_code[0]]) == nullptr) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    DEBUG << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " could not be found in storage tree \"" + combined_path.string() + "\".";
                    return std::vector<unsigned char>{};
                } else {
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1, block_code.cend()};
                    std::vector<unsigned char> out_vec = std::get<1>(children[block_code[0]])->read(sub_block_code);
                    if (out_vec.empty()) {
                        std::string not_found((const char *) block_code.data(), block_code.size());
                        DEBUG << "<Block error trace on return>: Block code " + boost::algorithm::hex(not_found) +
                                 " could not be found in storage tree \"" + combined_path.string() + "\".";
                    }
                    return out_vec;
                }
            } else {
                //the block code should have a size of 5; one chunk index and 4 bytes of encoding for the offset
                if (!size[block_code[0]]) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    DEBUG
                        << "<Block error trace on return, final tree>: Block code " + boost::algorithm::hex(not_found) +
                           " could not be found in storage tree \"" + combined_path.string() +
                           "\" and size of storage chunk was 0.";
                    return std::vector<unsigned char>{};
                } else {
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1,
                                                              block_code.cend()}; //copy offset code and rebuild
                    std::size_t offset{};
                    for (unsigned char i = 0; i < sizeof(unsigned int); i++) {
                        offset += (((std::size_t) sub_block_code[i]) << (i * 8));
                    }
                    std::string ref_name{boost::algorithm::hex(std::string{(char) block_code[0]})};
                    std::filesystem::path read_path = combined_path / ref_name;
                    FILE *reader = std::fopen(read_path.c_str(), "rb");
                    if (!reader) {
                        ERROR << "File read opening failed at \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                    //File should have been opened or created here
                    std::fseek(reader, static_cast<long>(offset), SEEK_SET);

                    if (std::ferror(reader)) {
                        FATAL << "I/O error when seeking \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }

                    unsigned char buf_size = 0;
                    std::size_t count = std::fread(&buf_size, sizeof(char), 1, reader);
                    if (count != 1) {
                        FATAL
                            << "I/O prefix first byte reading was not completed on path \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }

                    if (std::ferror(reader)) {
                        FATAL << "I/O error when reading prefix at path \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                    unsigned char buffer_in[buf_size + 1];
                    count = std::fread(&buffer_in, sizeof(char), buf_size + 1, reader);
                    if (count != buf_size + 1) {
                        FATAL
                            << "I/O prefix first byte reading was not completed on path \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                    std::size_t output_size{};
                    for (unsigned char buf_count = 0; buf_count <= buf_size; buf_count++) {
                        output_size += (((std::size_t) buffer_in[buf_count]) << (buf_count * 8));
                    }

                    std::vector<unsigned char> out_vec{};
                    auto *tmp_buf = new unsigned char[output_size];
                    count = std::fread(tmp_buf, sizeof(char), output_size, reader);
                    out_vec.assign(tmp_buf, tmp_buf + output_size);
                    delete[] tmp_buf;

                    if (count != output_size) {
                        FATAL << "I/O was not completed on path \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                    if (std::ferror(reader)) {
                        FATAL << "I/O error when reading prefix at path \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }

                    std::fclose(reader);

                    return out_vec;
                }
            }
        }

        //The index gives tuple<hash,local_block_reference>
        std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>>> index() {
            std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>>> search_index;
            for (unsigned short i = 0; i < (unsigned short) N; i++) {
                if (size[i] > 0) {
                    //read entire block generating hashes and block references
                    std::string ref_name{boost::algorithm::hex(std::string{(char) i})};
                    std::filesystem::path read_path = combined_path / ref_name;
                    FILE *reader = std::fopen(read_path.c_str(), "rb");
                    std::size_t cur_pos = 0;
                    while (!std::feof(reader)) {
                        if (!reader) {
                            ERROR << "File read opening failed at \"" + read_path.string() + "\"";
                            std::exit(EXIT_FAILURE);
                        }
                        //File should have been opened or created here
                        if (std::ferror(reader)) {
                            FATAL << "I/O error when seeking \"" + read_path.string() + "\"";
                            std::exit(EXIT_FAILURE);
                        }

                        std::vector<unsigned char> hash, local_block_ref;
                        local_block_ref.reserve(sizeof(unsigned int));
                        for (unsigned char i1 = 0; i1 < (unsigned char)sizeof(unsigned int); i1++) {//STORE_MAX will fit in 4 bytes
                            local_block_ref.push_back((unsigned char) (cur_pos >> (i1 * 8)));
                        }
                        local_block_ref.insert(local_block_ref.cbegin(), i);

                        unsigned char buf_size = 0;
                        std::size_t count = std::fread(&buf_size, sizeof(char), 1, reader);
                        cur_pos += count;
                        if (count != 1) {
                            FATAL << "I/O prefix first byte reading was not completed on path \"" + read_path.string() +
                                     "\"";
                            std::exit(EXIT_FAILURE);
                        }

                        if (std::ferror(reader)) {
                            FATAL << "I/O error when reading prefix at path \"" + read_path.string() + "\"";
                            std::exit(EXIT_FAILURE);
                        }
                        unsigned char buffer_for_size[buf_size + 1];
                        count = std::fread(&buffer_for_size, sizeof(char), buf_size + 1, reader);
                        cur_pos += count;
                        if (count != buf_size + 1) {
                            FATAL << "I/O prefix first byte reading was not completed on path \"" + read_path.string() +
                                     "\"";
                            std::exit(EXIT_FAILURE);
                        }
                        std::size_t output_size{};
                        for (unsigned char buf_count = 0; buf_count <= buf_size; buf_count++) {
                            output_size += (((std::size_t) buffer_for_size[buf_count]) << (buf_count * 8));
                        }

                        auto *tmp_buf = new unsigned char[output_size];
                        count = std::fread(tmp_buf, sizeof(char), output_size, reader);
                        cur_pos += count;

                        if (count != output_size) {
                            FATAL << "I/O was not completed on path \"" + read_path.string() + "\"";
                            std::exit(EXIT_FAILURE);
                        }
                        if (std::ferror(reader)) {
                            FATAL << "I/O error when reading prefix at path \"" + read_path.string() + "\"";
                            std::exit(EXIT_FAILURE);
                        }

                        unsigned char hash_buf[SHA512_DIGEST_LENGTH];//HASH GENERATION
                        SHA512(tmp_buf, output_size, hash_buf);
                        delete[] tmp_buf;

                        hash.assign(hash_buf, hash_buf + SHA512_DIGEST_LENGTH);
                        search_index.emplace_back(hash, local_block_ref);
                    }
                    std::fclose(reader);
                }
                //splice indexes of children plus the min_pos to decide local_block_ref of children array
                if (std::get<0>(children[i]) > 0 and std::get<1>(children[i]) != nullptr) {
                    auto append_list = std::get<1>(children[i])->index();
                    for(auto &el:append_list){
                        std::get<1>(el).insert(std::get<1>(el).cbegin(),i);
                    }
                    search_index.splice(search_index.cend(),append_list);
                }
            }
            return search_index;
        }

        void delete_recursive(){
            for (unsigned short i = 0; i < (unsigned short) N; i++) {
                if (size[i] > 0) {
                    std::string ref_name{boost::algorithm::hex(std::string{(char) i})};
                    std::filesystem::path read_path = combined_path / ref_name;
                    if(std::remove(read_path.c_str())!=0){
                        FATAL << "Removing was not completed on path \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                }
                //splice indexes of children plus the min_pos to decide local_block_ref of children array
                if (std::get<0>(children[i]) > 0 and std::get<1>(children[i]) != nullptr) {
                    std::get<1>(children[i])->delete_recursive();
                }
            }
        }

        //TODO: integrate block time stamp, integrate delete block, make class thread safe

        ~tree_storage(){
            for (auto & i : children) {
                if (std::get<1>(i) != nullptr) {
                    delete std::get<1>(i);
                }
            }
        }
    };
}

#endif //UHLIBCOMMON_TREE_STORAGE_H
