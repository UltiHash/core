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
#include <shared_mutex>
#include <memory>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif // _WIN32
#include "sys/types.h"
#include "sys/sysinfo.h"

namespace uh::trees {
#define N 256
#define STORE_MAX (unsigned int) std::numeric_limits<unsigned int>::max()
#define STORE_HARD_LIMIT (unsigned long) (std::numeric_limits<unsigned int>::max() * 2)
    typedef struct tree_storage tree_storage;

    struct tree_storage {
    private:
        void mem_wait(std::size_t mem){
            std::lock_guard lock(global_var_mutex);
            struct sysinfo memInfo{};
            unsigned long totalFreeVirtualMem;
            do{
                sysinfo (&memInfo);
                totalFreeVirtualMem = memInfo.freeram;
                totalFreeVirtualMem += memInfo.freeswap;
                totalFreeVirtualMem *= memInfo.mem_unit;
                if(totalFreeVirtualMem<mem){
#ifdef _WIN32
                    Sleep(10);
#else
                    usleep(10 * 1000);
#endif // _WIN32
                }
            }
            while(totalFreeVirtualMem<mem);
        }
    protected:
        //every file storage level contains a maximum of 256 storage chunks and 256 folders to deeper levels
        std::shared_ptr<std::vector<std::tuple<std::size_t,std::shared_ptr<std::shared_mutex>,unsigned char>>> size{};//different storage chunks with write protection
        std::shared_ptr<std::vector<std::tuple<std::size_t, tree_storage *,unsigned char>>> children{};//deeper tree storage blocks and folders
        //radix_tree* block_indexes[N]{}; // index local block finds
        std::shared_ptr<std::filesystem::path> combined_path{};
        std::shared_ptr<short> i_constructor = std::make_unique<short>(-1);
        std::shared_mutex global_var_mutex;//protect everything out of size array
        bool path_is_set_up = false;

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
            for (unsigned char i = 0; i < byte_count+1; i++) {
                prefix.push_back((unsigned char) (input_size >> (i * 8)));
            }
            if (prefix.empty())prefix.push_back(0);
            prefix.insert(prefix.cbegin(), byte_count);
            return prefix;
        }

    public:
        explicit tree_storage(const std::filesystem::path &root, unsigned short num_threads = 1) {
            //expected are 4 bytes that mimic hexadecimal string representation

            std::unique_lock lock(global_var_mutex);
            if(!path_is_set_up){
                std::string parent_name = root.filename().string();
                bool valid_root = parent_name.size() == 4 and root.extension().string().empty();
                if (valid_root)
                    for (const char &i: parent_name) {
                        if (!(('0' <= i and i <= '9') || ('A' <= i and i <= 'F'))) {
                            valid_root = false;
                            break;
                        }
                    }
                *combined_path = root;
                if (!valid_root) {
                    *combined_path /= "0000";
                }
                std::filesystem::create_directories(*combined_path);
                path_is_set_up = true;
            }
            lock.unlock();

            auto multithread_constructor = [&](){
                if(*i_constructor+1 == (short)N)return;
                for (short i=(*i_constructor+=1); i < (short) N;) {
                    std::string s_tmp = boost::algorithm::hex(std::string{(char)i});
                    std::filesystem::path chunk = *combined_path / s_tmp;
                    if (std::filesystem::exists(chunk)) {
                        size->emplace_back(std::filesystem::file_size(chunk),std::make_unique<std::shared_mutex>(),i);
                    }

                    std::string fname = combined_path->filename().string();
                    s_tmp.insert(s_tmp.cbegin(),fname.cbegin() + 2, fname.cbegin() + 4);
                    std::filesystem::path deeper_tree = *combined_path / s_tmp;
                    //check if sub folder in tree exists
                    if (std::filesystem::exists(deeper_tree)) {
                        auto* tmp_tree = new tree_storage(deeper_tree);
                        children->emplace_back(tmp_tree->get_size(),tmp_tree,i);
                    }
                    std::lock_guard lock2(global_var_mutex);
                    *i_constructor=i=(short)std::max(*i_constructor+1,i+1);
                }
            };

            if(num_threads == 1){
                multithread_constructor();
            }
            else{
                std::vector<std::thread> workers;
                for(unsigned short i=0;i<num_threads;i++){
                    std::thread w(multithread_constructor);
                    workers.push_back(std::move(w));
                }
                for (auto& th : workers)
                    th.join();
            }

            if(!std::is_sorted(size->begin(),size->end(),[](const auto &a, const auto &b){
                return std::get<2>(a)<std::get<2>(b);
            }))std::sort(size->begin(),size->end(),[](const auto &a, const auto &b){
                return std::get<2>(a)<std::get<2>(b);
            });
            if(!std::is_sorted(children->begin(),children->end(),[](const auto &a, const auto &b){
                return std::get<2>(a)<std::get<2>(b);
            }))std::sort(children->begin(),children->end(),[](const auto &a, const auto &b){
                return std::get<2>(a)<std::get<2>(b);
            });
        }

        std::size_t get_size() {
            std::size_t s{};
            std::lock_guard lock(global_var_mutex);
            for (unsigned short i = 0; i < (unsigned short) N; i++) {
                s += size->size()>i?std::get<0>(size->at(i)):0;
                if (children->size()>i) {
                    s += std::get<0>(children->at(i));
                }
                if(i>=size->size()&&i>=children->size())break;
            }
            return s;
        }

        //write a string and get size of written block representation and a reference string back
        std::vector<unsigned char> write(const std::vector<unsigned char> &input) {
            if(input.empty())return std::vector<unsigned char>{};
            if (input.size() > STORE_MAX) {
                FATAL << "A block could not be written because it exceeded maximum size of blocks \"" +
                         std::to_string(STORE_MAX) +
                         "\" with a size of \"" + std::to_string(input.size()) + "\".";
                std::exit(EXIT_FAILURE);
            }
            std::vector<unsigned char> prefix = prefix_wrap(input.size());
            std::size_t total_size = input.size() + prefix.size();
            //check block fill of this node, look for free space
            unsigned char min_pos{};
            std::unique_lock lock(global_var_mutex);
            std::size_t min_val = !size->empty()?std::get<0>(size->at(0)):0;

            bool no_deeper = min_val < STORE_MAX && min_val + total_size < STORE_HARD_LIMIT;
            std::size_t size_tmp{};
            if(no_deeper){
                size_tmp = std::get<0>(size->at(min_pos));
                std::get<0>(size->at(min_pos)) += total_size;
            }
            else{
                min_val = std::get<0>(children->at(0));
                min_pos = 0;
                //find or create balanced deeper tree node to store
                if((unsigned short) children->size() < (unsigned short)N){
                    min_pos = (unsigned short) children->size();
                    std::string ref_name{boost::algorithm::hex(std::string{(char) min_pos})};
                    std::string fname = combined_path->filename().string();
                    ref_name.insert(ref_name.cbegin(),fname.cbegin() + 2, fname.cbegin() + 4);
                    std::filesystem::path deeper_tree = *combined_path / ref_name;
                    auto* tmp_tree = new tree_storage(deeper_tree);
                    children->emplace_back(tmp_tree->get_size(),tmp_tree,min_pos);
                }
                else{
                    for (unsigned short i3 = 0; i3 < (unsigned short) children->size(); i3++) {
                        size_t size_old = min_val;
                        min_val = std::min(min_val,std::get<0>(children->at(i3)));
                        if (size_old-min_val) min_pos = (unsigned char) i3;
                    }
                }
                std::get<0>(children->at(min_pos)) += total_size;
            }
            lock.unlock();//go into deeper structure and release lock, else only

            if (no_deeper) {
                //store block to this position
                std::string ref_name{boost::algorithm::hex(std::string{(char) min_pos})};
                std::filesystem::path read_chunk = *combined_path / ref_name;

                std::unique_lock no_write(*std::get<1>(size->at(min_pos)));

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

                no_write.unlock();

                //start position of the block for seeking it later on is the old size
                std::vector<unsigned char> out_vec;
                out_vec.reserve(sizeof(unsigned int));
                for (unsigned char i = 0; i < (unsigned char) sizeof(unsigned int); i++) {//STORE_MAX will fit in 4 bytes
                    out_vec.push_back((unsigned char) (size_tmp >> (i * 8)));
                }
                out_vec.insert(out_vec.cbegin(), min_pos);

                return out_vec;//structure: BIN,OFFSET * 4 BYTES
            } else {
                std::vector<unsigned char> out_vec = std::get<1>(children->at(min_pos))->write(input);
                out_vec.insert(out_vec.cbegin(), min_pos);
                return out_vec;
            }
        }

        std::vector<unsigned char> read(const std::vector<unsigned char> &block_code) {
            if(block_code.empty()){
                return {};
            }
            if (block_code.size() > 5) {
                //size encoding is not reached yet, read along tree path
                if ((short)children->size() - 1 < (short)block_code[0] || !std::get<0>(children->at(block_code[0]))) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    DEBUG << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " could not be found in storage tree \"" + combined_path->string() + "\".";
                    return std::vector<unsigned char>{};
                } else {
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1, block_code.cend()};
                    std::vector<unsigned char> out_vec = std::get<1>(children->at(block_code[0]))->read(sub_block_code);
                    if (out_vec.empty()) {
                        std::string not_found((const char *) block_code.data(), block_code.size());
                        DEBUG << "<Block error trace on return>: Block code " + boost::algorithm::hex(not_found) +
                                 " could not be found in storage tree \"" + combined_path->string() + "\".";
                    }
                    return out_vec;
                }
            } else {
                //the block code should have a size of 5; one chunk index and 4 bytes of encoding for the offset
                if ((short)size->size() - 1 < (short)block_code[0] || !std::get<0>(size->at(block_code[0]))) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    DEBUG
                        << "<Block error trace on return, final tree>: Block code " + boost::algorithm::hex(not_found) +
                           " could not be found in storage tree \"" + combined_path->string() +
                           "\" and size of storage chunk was 0.";
                    return std::vector<unsigned char>{};
                } else {
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1,
                                                              block_code.cend()}; //copy offset code and rebuild
                    std::size_t offset{};
                    for (unsigned char i = 0; i < (unsigned char) sizeof(unsigned int); i++) {
                        offset += (((std::size_t) sub_block_code[i]) << (i * 8));
                    }
                    std::string ref_name{boost::algorithm::hex(std::string{(char) block_code[0]})};
                    std::filesystem::path read_path = *combined_path / ref_name;

                    std::unique_lock no_read(*std::get<1>(size->at(block_code[0])));

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
                    mem_wait(output_size);
                    auto *tmp_buf = new unsigned char[output_size];
                    count = std::fread(tmp_buf, sizeof(char), output_size, reader);

                    if (count != output_size) {
                        FATAL << "I/O was not completed on path \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                    if (std::ferror(reader)) {
                        FATAL << "I/O error when reading prefix at path \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }

                    no_read.unlock();

                    std::fclose(reader);

                    std::vector<unsigned char> out_vec{};
                    out_vec.assign(tmp_buf, tmp_buf + output_size);
                    delete[] tmp_buf;

                    return out_vec;
                }
            }
        }

        //The index gives tuple<hash,local_block_reference>, always run index single without reading or writing interference
        std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>>> index(unsigned short num_threads = 1) {
            std::shared_ptr<std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>>>> search_index;
            for (unsigned short i = 0; i < (unsigned short) N; i++) {
                if (i < size->size() && std::get<0>(size->at(i)) > 0) {
                    //read entire block generating hashes and block references
                    std::string ref_name{boost::algorithm::hex(std::string{(char) i})};
                    std::filesystem::path read_path = *combined_path / ref_name;

                    std::unique_lock no_index(*std::get<1>(size->at(i)));

                    FILE *reader = std::fopen(read_path.c_str(), "rb");
                    if (!reader) {
                        ERROR << "File read opening failed at \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                    //File should have been opened or created here
                    if (std::ferror(reader)) {
                        FATAL << "I/O error when seeking \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                    std::size_t cur_pos = 0;

                    std::thread rt,ht;

                    std::vector<unsigned char> local_block_ref;
                    unsigned char* tmp_buf[2];
                    std::size_t output_size{};
                    bool parallel_switch{};
                    std::mutex m1{};
                    while (!std::feof(reader)) {
                        auto read_func = [&](){
                            std::unique_lock lock(m1);
                            local_block_ref.reserve(sizeof(unsigned int));

                            for (unsigned char i1 = 0; i1 < (unsigned char)sizeof(unsigned int); i1++) {//STORE_MAX will fit in 4 bytes
                                local_block_ref.push_back((unsigned char) (cur_pos >> (i1 * 8)));
                            }
                            local_block_ref.insert(local_block_ref.cbegin(), i);
                            lock.unlock();

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
                            output_size=0;
                            for (unsigned char buf_count = 0; buf_count <= buf_size; buf_count++) {
                                output_size += (((std::size_t) buffer_for_size[buf_count]) << (buf_count * 8));
                            }

                            mem_wait(output_size);

                            tmp_buf[parallel_switch] = new unsigned char[output_size];
                            count = std::fread(tmp_buf[parallel_switch], sizeof(char), output_size, reader);
                            cur_pos += count;

                            if (count != output_size) {
                                FATAL << "I/O was not completed on path \"" + read_path.string() + "\"";
                                std::exit(EXIT_FAILURE);
                            }
                            if (std::ferror(reader)) {
                                FATAL << "I/O error when reading prefix at path \"" + read_path.string() + "\"";
                                std::exit(EXIT_FAILURE);
                            }
                        };
                        if(rt.joinable())rt.join();
                        rt=std::thread(read_func);

                        auto hash_func = [&](){
                            std::unique_lock lock(m1);
                            unsigned char hash_buf[SHA512_DIGEST_LENGTH];//HASH GENERATION
                            bool parallel_switch_copy = parallel_switch;
                            parallel_switch=!parallel_switch;
                            auto output_tmp = output_size;
                            auto tmp_local_block_ref = local_block_ref;
                            local_block_ref.clear();
                            lock.unlock();
                            SHA512(tmp_buf[parallel_switch_copy], output_tmp, hash_buf);
                            delete[] tmp_buf[parallel_switch_copy];

                            std::vector<unsigned char> hash{hash_buf, hash_buf + SHA512_DIGEST_LENGTH};
                            search_index->emplace_back(hash, tmp_local_block_ref);
                            local_block_ref.clear();
                        };

                        if(ht.joinable())ht.join();
                        ht=std::thread(hash_func);

                    }
                    std::fclose(reader);
                    no_index.unlock();
                }
                //splice indexes of children plus the min_pos to decide local_block_ref of children array
                if (i < children->size() && std::get<0>(children->at(i)) > 0) {
                    auto append_list = std::get<1>(children->at(i))->index();
                    for(auto &el:append_list){
                        std::get<1>(el).insert(std::get<1>(el).cbegin(),i);
                    }
                    search_index->splice(search_index->cend(),append_list);
                }
                if(i>=size->size()&&i>=children->size())break;
            }
            return *search_index;
        }

        void delete_recursive(){
            for (unsigned short i = 0; i < (unsigned short) N; i++) {
                if (i<size->size() && std::get<0>(size->at(i)) > 0) {
                    std::string ref_name{boost::algorithm::hex(std::string{(char) i})};
                    std::filesystem::path read_path = *combined_path / ref_name;
                    if(std::remove(read_path.c_str())!=0){
                        FATAL << "Removing was not completed on path \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                }
                //splice indexes of children plus the min_pos to decide local_block_ref of children array
                if (i<children->size() && std::get<0>(children->at(i)) > 0) {
                    std::get<1>(children->at(i))->delete_recursive();
                }
                if(i>=size->size()&&i>=children->size())break;
            }
        }

        //TODO: integrate block time stamp, integrate delete block, make class thread safe

        ~tree_storage(){
            for (auto & i : *children) {
                if (std::get<1>(i) != nullptr) {
                    delete std::get<1>(i);
                }
            }
        }
    };
}

#endif //UHLIBCOMMON_TREE_STORAGE_H
