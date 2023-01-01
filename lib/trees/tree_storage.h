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
#include <atomic>
#include <execution>

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
        static void mem_wait(std::size_t mem) {
            struct sysinfo memInfo{};
            unsigned long totalFreeVirtualMem;
            do {
                sysinfo (&memInfo);
                totalFreeVirtualMem = memInfo.freeram;
                totalFreeVirtualMem += memInfo.freeswap;
                totalFreeVirtualMem *= memInfo.mem_unit;
                if (totalFreeVirtualMem < mem) {
#ifdef _WIN32
                    Sleep(10);
#else
                    usleep(10 * 1000);
#endif // _WIN32
                }
            } while (totalFreeVirtualMem < mem);
        }

    protected:
        //every file storage level contains a maximum of 256 storage chunks and 256 folders to deeper levels
        std::atomic<std::shared_ptr<std::vector<std::tuple<std::size_t, unsigned char, std::shared_ptr<std::atomic_flag>, std::shared_ptr<std::atomic<std::size_t>>, std::shared_ptr<std::atomic_flag>>>>> size{};//different storage chunks with write, read and maintain protection flags
        std::atomic<std::shared_ptr<std::vector<std::tuple<std::size_t, tree_storage *, unsigned char>>>> children{};//deeper tree storage blocks and folders
        std::atomic<std::shared_ptr<std::filesystem::path>> combined_path{};
        std::shared_mutex global_var_mutex{};//protect everything out of size array

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
            for (unsigned char i = 0; i < byte_count + 1; i++) {
                prefix.push_back((unsigned char) (input_size >> (i * 8)));
            }
            if (prefix.empty())prefix.push_back(0);
            prefix.insert(prefix.cbegin(), byte_count);
            return prefix;
        }

    public:
        explicit tree_storage(const std::filesystem::path &root,
                              unsigned short num_threads = std::thread::hardware_concurrency()) {
            if (!num_threads)return;
            std::atomic<std::size_t> i_constructor{};

            std::unique_lock lock(global_var_mutex);
            if (combined_path.load()->empty()) {
                std::string parent_name = root.filename().string();
                bool valid_root = parent_name.size() == 4 and root.extension().string().empty();
                if (valid_root)
                    for (const char &i: parent_name) {
                        if (!(('0' <= i and i <= '9') || ('A' <= i and i <= 'F'))) {
                            valid_root = false;
                            break;
                        }
                    }
                combined_path.load()->operator=(root);
                if (!valid_root) {
                    combined_path.load()->operator/=("0000");
                }
                std::filesystem::create_directories(combined_path.load()->string());
            }
            lock.unlock();

            auto multithread_constructor = [&]() {
                std::size_t i = i_constructor.load();
                if (i == N)return;
                for (; i < N;) {
                    std::string s_tmp = boost::algorithm::hex(std::string{(char) i});
                    std::filesystem::path chunk = *combined_path.load(std::memory_order_relaxed) / s_tmp;
                    if (std::filesystem::exists(chunk)) {
                        std::shared_ptr<std::atomic_flag> f1{ATOMIC_FLAG_INIT}, f3{ATOMIC_FLAG_INIT};
                        std::shared_ptr<std::atomic<std::size_t>> f2{};
                        size.load()->emplace_back(std::filesystem::file_size(chunk), i, f1, f2, f3);
                    }

                    std::string fname = combined_path.load(std::memory_order_relaxed)->filename().string();
                    s_tmp.insert(s_tmp.cbegin(), fname.cbegin() + 2, fname.cbegin() + 4);
                    std::filesystem::path deeper_tree = *combined_path.load(std::memory_order_relaxed) / s_tmp;
                    //check if sub folder in tree exists
                    if (std::filesystem::exists(deeper_tree)) {
                        auto *tmp_tree = new tree_storage(deeper_tree, 1);
                        children.load()->emplace_back(tmp_tree->get_size(), tmp_tree, i);
                    }
                    i = (i_constructor += 1);
                }
            };

            if (num_threads == 1) {
                multithread_constructor();
            } else {
                std::vector<std::thread> workers;
                for (unsigned short i = 0; i < num_threads; i++) {
                    std::thread w(multithread_constructor);
                    workers.push_back(std::move(w));
                }
                for (auto &th: workers)
                    th.join();
            }

            if (!std::is_sorted(std::execution::par_unseq, size.load()->begin(), size.load()->end(),
                                [](const auto &a, const auto &b) {
                                    return std::get<1>(a) < std::get<1>(b);
                                }))
                std::sort(std::execution::par_unseq, size.load()->begin(), size.load()->end(),
                          [](const auto &a, const auto &b) {
                              return std::get<1>(a) < std::get<1>(b);
                          });
            if (!std::is_sorted(std::execution::par_unseq, children.load()->begin(), children.load()->end(),
                                [](const auto &a, const auto &b) {
                                    return std::get<1>(a) < std::get<1>(b);
                                }))
                std::sort(std::execution::par_unseq, children.load()->begin(), children.load()->end(),
                          [](const auto &a, const auto &b) {
                              return std::get<1>(a) < std::get<1>(b);
                          });
        }

        std::size_t get_size() {
            std::size_t s{};
            std::lock_guard lock(global_var_mutex);
            for (unsigned short i = 0; i < (unsigned short) N; i++) {
                s += size.load()->size() > i ? std::get<0>(size.load()->at(i)) : 0;
                if (children.load()->size() > i) {
                    s += std::get<0>(children.load()->at(i));
                }
                if (i >= size.load()->size() && i >= children.load()->size())break;
            }
            return s;
        }

        //write a string and get size of written block representation and a reference string back
        std::vector<unsigned char> write(const std::vector<unsigned char> &input,
                                         unsigned long current_time = (unsigned long) std::chrono::nanoseconds(
                                                 std::chrono::high_resolution_clock::now().time_since_epoch()).count()) {
            if (input.empty())return std::vector<unsigned char>{};
            if (input.size() > STORE_MAX) {
                FATAL << "A block could not be written because it exceeded maximum size of blocks \"" +
                         std::to_string(STORE_MAX) +
                         "\" with a size of \"" + std::to_string(input.size()) + "\".";
                std::exit(EXIT_FAILURE);
            }
            std::vector<unsigned char> prefix = prefix_wrap(input.size());
            std::size_t total_size = input.size() + prefix.size() + sizeof(unsigned long);
            //check block fill of this node, look for free space

            std::unique_lock lock(global_var_mutex);
            unsigned char min_pos;
            std::size_t min_val;
            if(size.load()->size() < N){
                min_pos = size.load()->size();
                min_val = 0;
                std::shared_ptr<std::atomic_flag> f1{ATOMIC_FLAG_INIT}, f3{ATOMIC_FLAG_INIT};
                std::shared_ptr<std::atomic<std::size_t>> f2{};
                size.load()->emplace_back(min_val, min_pos, f1, f2, f3);
            }
            else{
                auto min_el = *std::min_element(size.load()->begin(),size.load()->end(),[](auto &a, auto &b){
                    return std::get<0>(a) < std::get<0>(b) && !std::get<4>(a)->test() && !std::get<4>(b)->test();//skip maintain chunks for smaller check
                });
                min_pos = std::get<1>(min_el);
                min_val = std::get<0>(size.load()->at(min_pos));
            }

            bool no_deeper = min_val < STORE_MAX && min_val + total_size < STORE_HARD_LIMIT && !(std::get<4>(size.load()->at(min_pos))->test());
            std::size_t size_tmp{};
            if (no_deeper) {
                size_tmp = std::get<0>(size.load()->at(min_pos));
                std::get<0>(size.load()->at(min_pos)) += total_size;
            } else {
                //find or create balanced deeper tree node to store
                if ((unsigned short) children.load()->size() < (unsigned short) N) {
                    min_pos = (unsigned short) children.load()->size();
                    std::string ref_name{boost::algorithm::hex(std::string{(char) min_pos})};
                    std::string fname = combined_path.load(std::memory_order_relaxed)->filename().string();
                    ref_name.insert(ref_name.cbegin(), fname.cbegin() + 2, fname.cbegin() + 4);
                    std::filesystem::path deeper_tree = *combined_path.load(std::memory_order_relaxed) / ref_name;
                    auto *tmp_tree = new tree_storage(deeper_tree);
                    min_val = tmp_tree->get_size();
                    children.load()->emplace_back(min_val, tmp_tree, min_pos);
                } else {
                    auto min_el = *std::min_element(children.load()->begin(),children.load()->end(),[](auto &a,auto &b){
                        return std::get<0>(a) < std::get<0>(b);
                    });
                    min_val = std::get<0>(min_el);
                    min_pos = std::get<2>(min_el);
                }
                std::get<0>(children.load()->at(min_pos)) += total_size;
            }
            lock.unlock();//go into deeper structure and release lock, else only

            if (no_deeper) {
                //store block to this position
                std::string ref_name{boost::algorithm::hex(std::string{(char) min_pos})};
                std::filesystem::path read_chunk = *combined_path.load(std::memory_order_relaxed) / ref_name;

                //calculate binary of timestamp
                std::vector<unsigned char> bin_time;
                for (unsigned char i = 0; i < (unsigned char) sizeof(current_time); i++) {
                    bin_time.push_back((unsigned char) (current_time >> (i * 8)));
                }

                auto write_ptr = std::get<2>(size.load()->at(min_pos));
                auto read_ptr = std::get<3>(size.load()->at(min_pos));
                auto maintain_ptr = std::get<4>(size.load()->at(min_pos));

                while (std::atomic_flag_test_and_set_explicit(&(*maintain_ptr), std::memory_order_acquire)) {
                    maintain_ptr->wait(true);
                }
                while (std::atomic_flag_test_and_set_explicit(&(*write_ptr), std::memory_order_acquire)) {
                    write_ptr->wait(true);
                }

                while (read_ptr->load() > 0) {
#ifdef _WIN32
                    Sleep(10);
#else
                    usleep(10 * 1000);
#endif // _WIN32
                }

                FILE *writer = std::fopen(read_chunk.c_str(), "ab");
                if (!writer) {
                    ERROR << "File write opening failed at \"" + read_chunk.string() + "\"";
                    std::exit(EXIT_FAILURE);
                }
                //File should have been opened or created here
                std::fwrite(bin_time.data(), bin_time.size(), sizeof(unsigned char), writer);
                std::fwrite(prefix.data(), prefix.size(), sizeof(unsigned char), writer);
                std::fwrite(input.data(), input.size(), sizeof(unsigned char), writer);

                if (std::ferror(writer)) {
                    FATAL << "I/O error when writing \"" + read_chunk.string() + "\"";
                    std::exit(EXIT_FAILURE);
                }
                std::fclose(writer);

                std::atomic_flag_clear_explicit(&(*write_ptr), std::memory_order_release);
                if (!write_ptr->test())write_ptr->notify_all();
                std::atomic_flag_clear_explicit(&(*maintain_ptr), std::memory_order_release);
                if (!maintain_ptr->test())maintain_ptr->notify_one();

                //start position of the block for seeking it later on is the old size
                std::vector<unsigned char> out_vec{};
                out_vec.reserve(sizeof(unsigned int));
                for (unsigned char i = 0;
                     i < (unsigned char) sizeof(unsigned int); i++) {//STORE_MAX will fit in 4 bytes
                    out_vec.push_back((unsigned char) (size_tmp >> (i * 8)));
                }
                out_vec.insert(out_vec.cbegin(), min_pos);

                return out_vec;//structure: BIN,OFFSET * 4 BYTES
            } else {
                auto tmp_ptr = std::get<1>(children.load()->at(min_pos));
                std::vector<unsigned char> out_vec = tmp_ptr->write(input);
                out_vec.insert(out_vec.cbegin(), min_pos);
                return out_vec;
            }
        }

        //returns a tuple with block time and binary vector of block
        std::tuple<unsigned long, std::vector<unsigned char>> read(const std::vector<unsigned char> &block_code) {
            if (block_code.empty()) {
                return {};
            }
            if (block_code.size() > 5) {
                //size encoding is not reached yet, read along tree path
                if ((short) children.load()->size() - 1 < (short) block_code[0] ||
                    !std::get<0>(children.load()->at(block_code[0]))) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    DEBUG << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " could not be found in storage tree \"" +
                             combined_path.load(std::memory_order_relaxed)->string() + "\".";
                    return {0, std::vector<unsigned char>{}};
                } else {
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1, block_code.cend()};
                    auto out_vec = std::get<1>(children.load()->at(block_code[0]))->read(sub_block_code);
                    if (std::get<1>(out_vec).empty()) {
                        std::string not_found((const char *) block_code.data(), block_code.size());
                        DEBUG << "<Block error trace on return>: Block code " + boost::algorithm::hex(not_found) +
                                 " could not be found in storage tree \"" +
                                 combined_path.load(std::memory_order_relaxed)->string() + "\".";
                    }
                    return out_vec;
                }
            } else {
                if (block_code.size() < 5) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    FATAL << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " was too short for storage tree \"" +
                             combined_path.load(std::memory_order_relaxed)->string() + "\".";
                    std::exit(EXIT_FAILURE);
                }
                //the block code should have a size of 5; one chunk index and 4 bytes of encoding for the offset
                if ((short) size.load()->size() - 1 < (short) block_code[0] ||
                    !std::get<0>(size.load()->at(block_code[0]))) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    DEBUG
                        << "<Block error trace on return, final tree>: Block code " + boost::algorithm::hex(not_found) +
                           " could not be found in storage tree \"" + combined_path.load()->string() +
                           "\" and size of storage chunk was 0.";
                    return {0, std::vector<unsigned char>{}};
                } else {
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1,
                                                              block_code.cend()}; //copy offset code and rebuild
                    std::size_t offset{};
                    for (unsigned char i = 0; i < (unsigned char) sizeof(unsigned int); i++) {
                        offset += (((std::size_t) sub_block_code[i]) << (i * 8));
                    }
                    std::string ref_name{boost::algorithm::hex(std::string{(char) block_code[0]})};
                    std::filesystem::path read_path = *combined_path.load(std::memory_order_relaxed) / ref_name;

                    auto write_ptr = std::get<2>(size.load()->at(block_code[0]));
                    auto read_ptr = std::get<3>(size.load()->at(block_code[0]));

                    *read_ptr += 1;

                    while (write_ptr->test()) {
                        write_ptr->wait(true);
                    }

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

                    unsigned char time_buf[sizeof(unsigned long)];
                    std::size_t count = std::fread(&time_buf, sizeof(char), sizeof(unsigned long), reader);
                    if (count != sizeof(unsigned long)) {
                        FATAL
                            << "I/O time first 8 bytes reading was not completed on path \"" + read_path.string() +
                               "\"";
                        std::exit(EXIT_FAILURE);
                    }

                    if (std::ferror(reader)) {
                        FATAL << "I/O error when reading time of block at path \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                    unsigned long block_time{};
                    for (unsigned char i = 0; i < (unsigned char) sizeof(unsigned long); i++) {
                        block_time += (((std::size_t) time_buf[i]) << (i * 8));
                    }

                    unsigned char buf_size = 0;
                    count = std::fread(&buf_size, sizeof(char), 1, reader);
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

                    std::fclose(reader);
                    *read_ptr -= 1;
                    if (!*read_ptr)write_ptr->notify_one();

                    std::vector<unsigned char> out_vec{};
                    out_vec.assign(tmp_buf, tmp_buf + output_size);
                    delete[] tmp_buf;

                    return {block_time, out_vec};
                }
            }
        }

        //The index gives tuple<hash,local_block_reference>, always run index single without reading or writing interference
        std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, unsigned long>>
        index(unsigned short num_threads = std::thread::hardware_concurrency()) {
            if (!num_threads)return {};
            std::atomic<std::shared_ptr<std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, unsigned long>>>> search_index;
            std::atomic<std::size_t> i_constructor{};

            auto multithread_index = [&]() {
                std::size_t i = i_constructor.load();
                if (i == N)return;
                for (; i < N;) {

                    if (i < size.load()->size() && std::get<0>(size.load()->at(i)) > 0) {
                        //read entire block generating hashes and block references
                        std::string ref_name{boost::algorithm::hex(std::string{(char) i})};
                        std::filesystem::path read_path = *combined_path.load(std::memory_order_relaxed) / ref_name;

                        auto write_ptr = std::get<2>(size.load()->at(i));
                        auto read_ptr = std::get<3>(size.load()->at(i));

                        *read_ptr += 1;

                        while (write_ptr->test()) {
                            write_ptr->wait(true);
                        }

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
                        std::shared_ptr<std::size_t> cur_pos{};

                        std::thread rt, ht;

                        std::shared_ptr<std::vector<unsigned char>> local_block_ref{};
                        std::shared_ptr<unsigned long> block_time_current = std::make_shared<unsigned long>(0);
                        std::shared_ptr<unsigned char *> tmp_buf[2];
                        std::shared_ptr<std::size_t> output_size{};
                        std::shared_ptr<bool> parallel_switch{};
                        std::shared_mutex m1{};
                        while (!std::feof(reader)) {
                            auto read_func = [&]() {
                                std::unique_lock lock(m1);
                                local_block_ref->reserve(sizeof(unsigned int));

                                for (unsigned char i1 = 0;
                                     i1 < (unsigned char) sizeof(unsigned int); i1++) {//STORE_MAX will fit in 4 bytes
                                    local_block_ref->push_back((unsigned char) (*cur_pos >> (i1 * 8)));
                                }
                                local_block_ref->insert(local_block_ref->cbegin(), i);
                                bool parallel_switch_tmp = *parallel_switch;

                                unsigned char time_buf[sizeof(unsigned long)];
                                std::size_t count = std::fread(&time_buf, sizeof(char), sizeof(unsigned long), reader);
                                *cur_pos += count;
                                if (count != sizeof(unsigned long)) {
                                    FATAL
                                        << "I/O time first 8 bytes reading was not completed on path \"" +
                                           read_path.string() + "\"";
                                    std::exit(EXIT_FAILURE);
                                }

                                if (std::ferror(reader)) {
                                    FATAL << "I/O error when reading time of block at path \"" + read_path.string() +
                                             "\"";
                                    std::exit(EXIT_FAILURE);
                                }
                                unsigned long block_time{};
                                for (unsigned char i4 = 0; i4 < (unsigned char) sizeof(unsigned long); i4++) {
                                    block_time += (((std::size_t) time_buf[i4]) << (i4 * 8));
                                }
                                *block_time_current = block_time;

                                unsigned char buf_size = 0;
                                count = std::fread(&buf_size, sizeof(char), 1, reader);
                                *cur_pos += count;
                                if (count != 1) {
                                    FATAL << "I/O prefix first byte reading was not completed on path \"" +
                                             read_path.string() +
                                             "\"";
                                    std::exit(EXIT_FAILURE);
                                }

                                if (std::ferror(reader)) {
                                    FATAL << "I/O error when reading prefix at path \"" + read_path.string() + "\"";
                                    std::exit(EXIT_FAILURE);
                                }
                                unsigned char buffer_for_size[buf_size + 1];
                                count = std::fread(&buffer_for_size, sizeof(char), buf_size + 1, reader);
                                *cur_pos += count;
                                if (count != buf_size + 1) {
                                    FATAL << "I/O prefix first byte reading was not completed on path \"" +
                                             read_path.string() +
                                             "\"";
                                    std::exit(EXIT_FAILURE);
                                }
                                *output_size = 0;
                                for (unsigned char buf_count = 0; buf_count <= buf_size; buf_count++) {
                                    *output_size += (((std::size_t) buffer_for_size[buf_count]) << (buf_count * 8));
                                }
                                lock.unlock();
                                mem_wait(*output_size);

                                *tmp_buf[parallel_switch_tmp] = new unsigned char[*output_size];
                                count = std::fread(*tmp_buf[parallel_switch_tmp], sizeof(char), *output_size, reader);
                                *cur_pos += count;

                                if (count != *output_size) {
                                    FATAL << "I/O was not completed on path \"" + read_path.string() + "\"";
                                    std::exit(EXIT_FAILURE);
                                }
                                if (std::ferror(reader)) {
                                    FATAL << "I/O error when reading prefix at path \"" + read_path.string() + "\"";
                                    std::exit(EXIT_FAILURE);
                                }
                            };
                            if (rt.joinable())rt.join();
                            rt = std::thread(read_func);

                            auto hash_func = [&]() {
                                std::unique_lock lock(m1);
                                unsigned char hash_buf[SHA512_DIGEST_LENGTH];//HASH GENERATION
                                bool parallel_switch_copy = *parallel_switch;
                                *parallel_switch = !*parallel_switch;
                                auto tmp_local_block_ref = *local_block_ref;
                                local_block_ref->clear();
                                auto block_time_cpy = *block_time_current;
                                auto output_tmp = *output_size;
                                lock.unlock();
                                SHA512(*tmp_buf[parallel_switch_copy], output_tmp, hash_buf);
                                delete[] *tmp_buf[parallel_switch_copy];

                                std::vector<unsigned char> hash{hash_buf, hash_buf + SHA512_DIGEST_LENGTH};
                                search_index.load()->emplace_back(hash, tmp_local_block_ref, block_time_cpy);
                            };

                            if (rt.joinable())rt.join();
                            if (ht.joinable())ht.join();
                            ht = std::thread(hash_func);

                            if (std::feof(reader)) {
                                if (rt.joinable())rt.join();
                                if (ht.joinable())ht.join();
                                std::free(*tmp_buf[0]);
                                std::free(*tmp_buf[1]);
                            }
                        }
                        std::fclose(reader);
                        *read_ptr -= 1;
                        if (!*read_ptr)write_ptr->notify_one();
                    }
                    //splice indexes of children plus the min_pos to decide local_block_ref of children array
                    if (i < children.load()->size() && std::get<0>(children.load()->at(i)) > 0) {
                        auto append_list = std::get<1>(children.load()->at(i))->index(1);
                        for (auto &el: append_list) {
                            std::get<1>(el).insert(std::get<1>(el).cbegin(), i);
                        }
                        auto cur_end = search_index.load(std::memory_order_relaxed)->cend();
                        search_index.load()->splice(cur_end, append_list);
                    }
                    if (i >= size.load()->size() && i >= children.load()->size())break;
                    i = (i_constructor += 1);
                }
            };

            if (num_threads == 1) {
                multithread_index();
            } else {
                std::vector<std::thread> workers;
                for (unsigned short i = 0; i < num_threads; i++) {
                    std::thread w(multithread_index);
                    workers.push_back(std::move(w));
                }
                for (auto &th: workers)
                    th.join();
            }

            return *search_index.load();
        }

        std::size_t delete_recursive(unsigned short num_threads = std::thread::hardware_concurrency()) {
            if (!num_threads)return {};
            std::atomic<std::size_t> i_constructor{};
            std::atomic<std::size_t> out_size{};

            auto multithread_index = [&]() {
                std::size_t i = i_constructor.load();
                if (i == N)return;
                for (; i < N;) {

                    if (i < size.load()->size() && std::get<0>(size.load()->at(i)) > 0) {
                        std::string ref_name{boost::algorithm::hex(std::string{(char) i})};
                        std::filesystem::path read_path = *combined_path.load(std::memory_order_relaxed) / ref_name;
                        std::size_t vanish_size = std::filesystem::file_size(read_path);
                        out_size += vanish_size;
                        std::get<0>(size.load()->at(i)) -= vanish_size;
                        if (std::remove(read_path.c_str()) != 0) {
                            FATAL << "Removing was not completed on path \"" + read_path.string() + "\"";
                            std::exit(EXIT_FAILURE);
                        }
                    }
                    //splice indexes of children plus the min_pos to decide local_block_ref of children array
                    if (i < children.load()->size() && std::get<0>(children.load()->at(i)) > 0) {
                        std::size_t vanish_size = std::get<1>(children.load()->at(i))->delete_recursive(1);
                        out_size += vanish_size;
                        std::get<0>(size.load()->at(i)) -= vanish_size;
                    }
                    if (i >= size.load()->size() && i >= children.load()->size())break;

                    i = (i_constructor += 1);
                }
            };

            if (num_threads == 1) {
                multithread_index();
            } else {
                std::vector<std::thread> workers;
                for (unsigned short i = 0; i < num_threads; i++) {
                    std::thread w(multithread_index);
                    workers.push_back(std::move(w));
                }
                for (auto &th: workers)
                    th.join();
            }
            return out_size.load();
        }

        //returns a tuple with block time and block size from disk
        std::tuple<unsigned long, std::size_t> get_info(const std::vector<unsigned char> &block_code) {
            if (block_code.empty()) {
                return {};
            }
            if (block_code.size() > 5) {
                //size encoding is not reached yet, get_info along tree path
                if ((short) children.load()->size() - 1 < (short) block_code[0] ||
                    !std::get<0>(children.load()->at(block_code[0]))) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    DEBUG << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " could not be found in storage tree \"" +
                             combined_path.load(std::memory_order_relaxed)->string() + "\".";
                    return {0, 0};
                } else {
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1, block_code.cend()};
                    auto out_vec = std::get<1>(children.load()->at(block_code[0]))->get_info(sub_block_code);
                    if (std::get<1>(out_vec) == 0) {
                        std::string not_found((const char *) block_code.data(), block_code.size());
                        DEBUG << "<Block error trace on return>: Block code " + boost::algorithm::hex(not_found) +
                                 " could not be found in storage tree \"" +
                                 combined_path.load(std::memory_order_relaxed)->string() + "\".";
                    }
                    return out_vec;
                }
            } else {
                if (block_code.size() < 5) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    FATAL << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " was too short for storage tree \"" +
                             combined_path.load(std::memory_order_relaxed)->string() + "\".";
                    std::exit(EXIT_FAILURE);
                }
                //the block code should have a size of 5; one chunk index and 4 bytes of encoding for the offset
                if ((short) size.load()->size() - 1 < (short) block_code[0] ||
                    !std::get<0>(size.load()->at(block_code[0]))) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    DEBUG
                        << "<Block error trace on return, final tree>: Block code " + boost::algorithm::hex(not_found) +
                           " could not be found in storage tree \"" +
                           combined_path.load(std::memory_order_relaxed)->string() +
                           "\" and size of storage chunk was 0.";
                    return {0, 0};
                } else {
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1,
                                                              block_code.cend()}; //copy offset code and rebuild
                    std::size_t offset{};
                    for (unsigned char i = 0; i < (unsigned char) sizeof(unsigned int); i++) {
                        offset += (((std::size_t) sub_block_code[i]) << (i * 8));
                    }
                    std::string ref_name{boost::algorithm::hex(std::string{(char) block_code[0]})};
                    std::filesystem::path read_path = *combined_path.load(std::memory_order_relaxed) / ref_name;

                    auto write_ptr = std::get<2>(size.load()->at(block_code[0]));
                    auto read_ptr = std::get<3>(size.load()->at(block_code[0]));

                    *read_ptr += 1;

                    while (write_ptr->test()) {
                        write_ptr->wait(true);
                    }

                    FILE *reader = std::fopen(read_path.c_str(), "rb");
                    if (!reader) {
                        ERROR << "File get_info opening failed at \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                    //File should have been opened or created here
                    std::fseek(reader, static_cast<long>(offset), SEEK_SET);

                    if (std::ferror(reader)) {
                        FATAL << "I/O error when seeking \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }

                    unsigned char time_buf[sizeof(unsigned long)];
                    std::size_t count = std::fread(&time_buf, sizeof(char), sizeof(unsigned long), reader);
                    if (count != sizeof(unsigned long)) {
                        FATAL
                            << "I/O time first 8 bytes reading was not completed on path \"" + read_path.string() +
                               "\"";
                        std::exit(EXIT_FAILURE);
                    }

                    if (std::ferror(reader)) {
                        FATAL << "I/O error when reading time of block at path \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                    unsigned long block_time{};
                    for (unsigned char i = 0; i < (unsigned char) sizeof(unsigned long); i++) {
                        block_time += (((std::size_t) time_buf[i]) << (i * 8));
                    }

                    unsigned char buf_size = 0;
                    count = std::fread(&buf_size, sizeof(char), 1, reader);
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

                    std::fclose(reader);
                    *read_ptr -= 1;
                    if (!*read_ptr)write_ptr->notify_one();

                    return {block_time, output_size};
                }
            }
        }

        //returns a tuple with block time and binary vector of block
        bool set_time(const std::vector<unsigned char> &block_code,
                      unsigned long current_time = (unsigned long) std::chrono::nanoseconds(
                              std::chrono::high_resolution_clock::now().time_since_epoch()).count()) {
            if (block_code.empty()) {
                return false;
            }
            if (block_code.size() > 5) {
                //size encoding is not reached yet, set_time along tree path
                if ((short) children.load()->size() - 1 < (short) block_code[0] ||
                    !std::get<0>(children.load()->at(block_code[0]))) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    DEBUG << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " could not be found in storage tree \"" +
                             combined_path.load(std::memory_order_relaxed)->string() + "\".";
                    return false;
                } else {
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1, block_code.cend()};
                    auto out_vec = std::get<1>(children.load()->at(block_code[0]))->set_time(sub_block_code,
                                                                                             current_time);
                    if (!out_vec) {
                        std::string not_found((const char *) block_code.data(), block_code.size());
                        DEBUG << "<Block error trace on return>: Block code " + boost::algorithm::hex(not_found) +
                                 " could not be found in storage tree \"" + combined_path.load()->string() + "\".";
                    }
                    return out_vec;
                }
            } else {
                if (block_code.size() < 5) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    FATAL << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " was too short for storage tree \"" +
                             combined_path.load(std::memory_order_relaxed)->string() + "\".";
                    std::exit(EXIT_FAILURE);
                }
                //the block code should have a size of 5; one chunk index and 4 bytes of encoding for the offset
                if ((short) size.load()->size() - 1 < (short) block_code[0] ||
                    !std::get<0>(size.load()->at(block_code[0]))) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    DEBUG
                        << "<Block error trace on return, final tree>: Block code " + boost::algorithm::hex(not_found) +
                           " could not be found in storage tree \"" + combined_path.load()->string() +
                           "\" and size of storage chunk was 0.";
                    return false;
                } else {
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1,
                                                              block_code.cend()}; //copy offset code and rebuild
                    std::size_t offset{};
                    for (unsigned char i = 0; i < (unsigned char) sizeof(unsigned int); i++) {
                        offset += (((std::size_t) sub_block_code[i]) << (i * 8));
                    }
                    std::string ref_name{boost::algorithm::hex(std::string{(char) block_code[0]})};
                    std::filesystem::path read_path = *combined_path.load(std::memory_order_relaxed) / ref_name;

                    //calculate binary of timestamp
                    std::vector<unsigned char> bin_time;
                    for (unsigned char i = 0; i < (unsigned char) sizeof(current_time); i++) {
                        bin_time.push_back((unsigned char) (current_time >> (i * 8)));
                    }

                    auto write_ptr = std::get<2>(size.load()->at(block_code[0]));
                    auto read_ptr = std::get<3>(size.load()->at(block_code[0]));

                    while (std::atomic_flag_test_and_set_explicit(&(*write_ptr), std::memory_order_acquire)) {
                        write_ptr->wait(true);
                    }

                    while (read_ptr->load() > 0) {
#ifdef _WIN32
                        Sleep(10);
#else
                        usleep(10 * 1000);
#endif // _WIN32
                    }

                    FILE *writer = std::fopen(read_path.c_str(), "wb");
                    if (!writer) {
                        ERROR << "File write opening failed at \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                    //File should have been opened or created here
                    std::fseek(writer, static_cast<long>(offset), SEEK_SET);

                    if (std::ferror(writer)) {
                        FATAL << "I/O error when seeking \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                    //File should have been opened or created here
                    std::fwrite(bin_time.data(), bin_time.size(), sizeof(unsigned char), writer);

                    if (std::ferror(writer)) {
                        FATAL << "I/O error when writing \"" + read_path.string() + "\"";
                        std::exit(EXIT_FAILURE);
                    }
                    std::fclose(writer);
                    std::atomic_flag_clear_explicit(&(*write_ptr), std::memory_order_release);
                    if (!write_ptr->test())write_ptr->notify_all();

                    return true;
                }
            }
        }

        //TODO: integrate delete blocks, maintain valid time
        //after deletion some blocks are de-fragmented in descending order. Behind the deleted block(s) all blocks need to be re-mapped
        //returns the deleted total size and a list of tuple<old_block_reference with first element tree reference, new_block_reference with first element tree reference,reference to tree_storage of change>
        //maintaining the system can be done in 2 steps: delete_blocks_copy and after that we need to re-map all local block references
        //after function delete_blocks_copy was called the atomic_flag securing write on the chunk will be active and we need to re-map all blocks of a chunk before setting the write flag back to false
        //it is wise to call a delete on blocks on the same chunk, one at a time
        std::tuple<std::size_t, std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, tree_storage *>>>
        delete_blocks_copy(
                std::vector<std::vector<unsigned char>> &block_codes,
                unsigned short num_threads = std::thread::hardware_concurrency()) {
            if (block_codes.empty() || !num_threads)return {};
            std::atomic<std::shared_ptr<std::size_t>> active_threads{};
            std::atomic<std::size_t> out_size{};
            std::atomic<std::shared_ptr<std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, tree_storage *>>>> out_change_list{};
            //sort for lexicographic to find blocks within the same chunks that all need to be deleted
            std::sort(std::execution::par_unseq, block_codes.begin(), block_codes.end(), [](auto &a, auto &b) {
                return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
            });
            //scan and filter for size == 5 and delete blocks from chunks, deliver deleted size and changed local block codes via chunk level indexing after change spot
            //use multithreading with a thread management system so that threads from deleting go on to deeper delete

            unsigned char current;
            bool first = true;
            auto beg = block_codes.begin();
            auto end = beg;
            auto cur = end;
            for (; end < block_codes.end(); end++) {
                if (first) {
                    first = false;
                    current = (*end)[0];
                } else {
                    if (current != (*end)[0] || end == block_codes.end() - 1) {
                        //first filter all blocks with size 5 from the incoming sequence and delete them within this tree level
                        std::shared_mutex m1{};
                        std::unique_lock lock(m1);
                        auto first_index_exe_function = [&]() {
                            //parallel start
                            std::vector<std::vector<unsigned char>> deeper_codes{}, delete_here_codes{};
                            auto beg_tmp = beg;
                            auto cur_tmp = cur;
                            //take a branch to delete multiple blocks within
                            std::for_each(beg_tmp, cur_tmp, [&](auto &a) {
                                if (a.size() > 5)deeper_codes.emplace_back(a.begin() + 1, a.end());
                                else {
                                    if (a.size() < 5) {
                                        std::string not_found((const char *) a.data(), a.size());
                                        FATAL << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                                                 " was too short for storage tree \"" +
                                                 combined_path.load(std::memory_order_relaxed)->string() + "\".";
                                        std::exit(EXIT_FAILURE);
                                    } else delete_here_codes.emplace_back(a.begin(), a.end());
                                }
                            });

                            if(delete_here_codes.empty())lock.unlock();

                            if (!delete_here_codes.empty() && (*cur_tmp)[0] < size.load()->size() &&
                                std::get<0>(size.load()->at((*cur_tmp)[0])) > 0) {
                                auto maintain_ptr = std::get<4>(size.load()->at((*cur_tmp)[0]));
                                auto read_ptr = std::get<3>(size.load()->at((*cur_tmp)[0]));
                                auto write_ptr = std::get<2>(size.load()->at((*cur_tmp)[0]));

                                while (std::atomic_flag_test_and_set_explicit(&(*maintain_ptr),
                                                                              std::memory_order_acquire)) {
                                    maintain_ptr->wait(true);
                                }

                                lock.unlock();

                                *read_ptr += 1;

                                while (write_ptr->test()) {
                                    write_ptr->wait(true);
                                }

                                //sort blocks to be deleted here after their offset on the chunk
                                std::string ref_name{boost::algorithm::hex(std::string{(char) (*cur_tmp)[0]})};
                                std::filesystem::path chunk = *combined_path.load(std::memory_order_relaxed) / ref_name;
                                std::filesystem::path chunk_maintain = chunk.parent_path() / (chunk.filename().string()+"_maintain");

                                std::sort(delete_here_codes.begin(),delete_here_codes.end(),[](auto &a, auto &b){
                                    std::vector<unsigned char> sub_block_code1{a.cbegin() + 1, a.cend()}, sub_block_code2{b.cbegin() + 1, b.cend()};
                                    std::size_t offset1{}, offset2{};
                                    for (unsigned char i = 0; i < (unsigned char) sizeof(unsigned int); i++) {
                                        offset1 += (((std::size_t) sub_block_code1[i]) << (i * 8));
                                        offset2 += (((std::size_t) sub_block_code2[i]) << (i * 8));
                                    }
                                    return offset1 < offset2;
                                });


                                auto block_step_beg = delete_here_codes.cbegin();

                                auto offset_calc = [](const auto &a_ref, const auto &b_ref){
                                    std::vector<unsigned char> sub_block_code{a_ref + 1,
                                                                              b_ref}; //copy offset code and rebuild
                                    std::size_t offset{};
                                    for (unsigned char i = 0; i < (unsigned char) sizeof(unsigned int); i++) {
                                        offset += (((std::size_t) sub_block_code[i]) << (i * 8));
                                    }
                                    return offset;
                                };

                                //read chunk at index (*cur_tmp)[0]
                                FILE *reader = std::fopen(chunk.c_str(), "rb");
                                if (!reader) {
                                    ERROR << "File read opening failed at \"" + chunk.string() + "\"";
                                    std::exit(EXIT_FAILURE);
                                }
                                std::size_t cur_pos = offset_calc(block_step_beg->cbegin(),block_step_beg->cend());

                                std::fseek(reader, static_cast<long>(cur_pos), SEEK_SET);
                                if (std::ferror(reader)) {
                                    FATAL << "I/O error when seeking \"" + chunk.string() + "\"";
                                    std::exit(EXIT_FAILURE);
                                }

                                std::size_t trunc_at = cur_pos;
                                std::size_t delete_size{};
                                //producer consumer queue for reading and writing; contains RAM pointer; size of RAM pointer; local tree storage reference of;
                                // *this tree where the referring chunk is stored; truncate size for the chunk; new storage reference after append
                                std::atomic<std::shared_ptr<std::list<std::tuple<unsigned char*,std::size_t,std::vector<unsigned char>, tree_storage *, std::size_t>>>> multithreading_factory{};
                                std::shared_ptr<std::atomic_flag> write_control{};

                                while (std::atomic_flag_test_and_set_explicit(&(*write_control), std::memory_order_acquire)) {
                                    write_control->wait(true);
                                }

                                FILE *writer = std::fopen(chunk_maintain.c_str(), "ab");
                                if (!writer) {
                                    ERROR << "File write opening failed at \"" + chunk_maintain.string() + "\"";
                                    std::exit(EXIT_FAILURE);
                                }

                                auto consumer_function = [&](){
                                    while(write_control->test()){
                                        while(!multithreading_factory.load()->empty()){
                                            std::vector<unsigned char> out_vec{};
                                            out_vec.reserve(sizeof(unsigned int));
                                            for (unsigned char i = 0;
                                                 i < (unsigned char) sizeof(unsigned int); i++) {//STORE_MAX will fit in 4 bytes
                                                out_vec.push_back((unsigned char) (std::get<4>(*multithreading_factory.load()->cbegin()) >> (i * 8)));
                                            }
                                            out_vec.insert(out_vec.cbegin(), std::get<2>(*multithreading_factory.load()->cbegin())[0]);

                                            std::fwrite(std::get<0>(*multithreading_factory.load()->cbegin()), std::get<1>(*multithreading_factory.load()->cbegin()), sizeof(unsigned char), writer);
                                            if (std::ferror(writer)) {
                                                FATAL << "I/O error when writing \"" + chunk_maintain.string() + "\"";
                                                std::exit(EXIT_FAILURE);
                                            }
                                            std::free(std::get<0>(*multithreading_factory.load()->begin()));
                                            //tmp_buf,write_back_size,out_vec,this,trunc_at
                                            //std::vector<unsigned char>, std::vector<unsigned char>, tree_storage *, std::size_t>>>
                                            out_change_list.load()->emplace_back(std::get<2>(*multithreading_factory.load()->cbegin()),out_vec,std::get<3>(*multithreading_factory.load()->cbegin()));
                                            multithreading_factory.load()->pop_front();
                                        }
#ifdef _WIN32
                                        Sleep(10);
#else
                                        usleep(10 * 1000);
#endif // _WIN32
                                    }
                                    std::fclose(writer);
                                };
                                std::thread w1(consumer_function);

                                while (!std::feof(reader)) {
                                    //File should have been opened or created here, seek for first block
                                    //start position of the block for seeking it later on is the old size
                                    std::vector<unsigned char> out_vec{};
                                    out_vec.reserve(sizeof(unsigned int));
                                    for (unsigned char i = 0;
                                         i < (unsigned char) sizeof(unsigned int); i++) {//STORE_MAX will fit in 4 bytes
                                        out_vec.push_back((unsigned char) (cur_pos >> (i * 8)));
                                    }
                                    out_vec.insert(out_vec.cbegin(), (*cur_tmp)[0]);

                                    unsigned char time_buf[sizeof(unsigned long)];
                                    std::size_t count = std::fread(&time_buf, sizeof(char), sizeof(unsigned long), reader);
                                    if (count != sizeof(unsigned long)) {
                                        FATAL
                                            << "I/O time first 8 bytes reading was not completed on path \"" + chunk.string() +
                                               "\"";
                                        std::exit(EXIT_FAILURE);
                                    }
                                    cur_pos += count;

                                    if (std::ferror(reader)) {
                                        FATAL << "I/O error when reading time of block at path \"" + chunk.string() + "\"";
                                        std::exit(EXIT_FAILURE);
                                    }

                                    unsigned char buf_size = 0;
                                    count = std::fread(&buf_size, sizeof(char), 1, reader);
                                    if (count != 1) {
                                        FATAL
                                            << "I/O prefix first byte reading was not completed on path \"" + chunk.string() + "\"";
                                        std::exit(EXIT_FAILURE);
                                    }
                                    cur_pos += count;

                                    if (std::ferror(reader)) {
                                        FATAL << "I/O error when reading prefix at path \"" + chunk.string() + "\"";
                                        std::exit(EXIT_FAILURE);
                                    }
                                    unsigned char buffer_in[buf_size + 1];
                                    count = std::fread(&buffer_in, sizeof(char), buf_size + 1, reader);
                                    if (count != buf_size + 1) {
                                        FATAL
                                            << "I/O prefix first byte reading was not completed on path \"" + chunk.string() + "\"";
                                        std::exit(EXIT_FAILURE);
                                    }
                                    cur_pos += count;
                                    std::size_t output_size{};
                                    for (unsigned char buf_count = 0; buf_count <= buf_size; buf_count++) {
                                        output_size += (((std::size_t) buffer_in[buf_count]) << (buf_count * 8));
                                    }

                                    std::size_t write_back_size = sizeof(time_buf) + 1 + sizeof(buffer_in) + output_size;

                                    if(block_step_beg<delete_here_codes.cend() && std::equal(out_vec.cbegin(),out_vec.cend(),block_step_beg->cbegin(),block_step_beg->cend())){
                                        //skip block copy and seek over it
                                        std::fseek(reader, static_cast<long>(output_size), SEEK_CUR);
                                        if (std::ferror(reader)) {
                                            FATAL << "I/O error when seeking \"" + chunk.string() + "\"";
                                            std::exit(EXIT_FAILURE);
                                        }
                                        cur_pos += output_size;
                                        delete_size += write_back_size;
                                        out_size += write_back_size;
                                        block_step_beg++;
                                    }
                                    else{
                                        //copy block to maintain file
                                        mem_wait(write_back_size);
                                        auto *tmp_buf = new unsigned char[write_back_size];
                                        unsigned char i = 0;
                                        for(; i < (unsigned char) sizeof(time_buf);i++){
                                            tmp_buf[i] = time_buf[i];
                                        }
                                        tmp_buf[i] = buf_size;
                                        i++;
                                        unsigned char meta_offset = i;
                                        i=0;
                                        for(; i < (unsigned char) sizeof(buffer_in);i++){
                                            tmp_buf[i + meta_offset] = buffer_in[i];
                                        }

                                        count = std::fread(tmp_buf+(write_back_size-output_size), sizeof(char), output_size, reader);

                                        if (count != output_size) {
                                            FATAL << "I/O was not completed on path \"" + chunk.string() + "\"";
                                            std::exit(EXIT_FAILURE);
                                        }
                                        if (std::ferror(reader)) {
                                            FATAL << "I/O error when reading prefix at path \"" + chunk.string() + "\"";
                                            std::exit(EXIT_FAILURE);
                                        }
                                        cur_pos += count;
                                        multithreading_factory.load()->emplace_back(tmp_buf,write_back_size,out_vec,this,cur_pos-write_back_size-delete_size);
                                    }
                                }
                                std::fclose(reader);
                                *read_ptr -= 1;
                                std::atomic_flag_clear_explicit(&(*write_control), std::memory_order_release);
                                if(w1.joinable())w1.join();

                                for(auto it = block_step_beg; it < delete_here_codes.cend(); it++){
                                    DEBUG << "Block code \""+boost::algorithm::hex(std::string{it->cbegin(),it->cend()})+"\" could not be deleted. Please check for bugs.";
                                }

                                //after the truncate position are known and the following blocks are filtered, truncation and append need to take place under atomic flag protection
                                //write and truncate
                                truncate64(chunk.c_str(),trunc_at);

                                //TODO: move to init_maintained_chunk_reset
                                std::atomic_flag_clear_explicit(&(*maintain_ptr), std::memory_order_release);
                                if(!maintain_ptr->test())maintain_ptr->notify_one();
                            }

                            //delete deeper codes
                            if (!deeper_codes.empty() && (*cur_tmp)[0] < children.load()->size() &&
                                std::get<0>(children.load()->at((*cur_tmp)[0])) > 0) {
                                auto tmp_deeper_tree_ptr = std::get<1>(children.load()->at((*cur_tmp)[0]));
                                //parallel start
                                auto deeper_delete = tmp_deeper_tree_ptr->delete_blocks_copy(deeper_codes, 1);
                                std::get<0>(children.load()->at((*cur_tmp)[0])) -= std::get<0>(deeper_delete);//subtract deleted size from deeper node
                                out_size += std::get<0>(deeper_delete);//add total deleted size
                                for(auto &it:std::get<1>(deeper_delete)){
                                    //deeper elements have their first position filtered so that we need to rebuild it
                                    std::get<0>(it).insert(std::get<0>(it).cbegin(),(*cur_tmp)[0]);
                                    std::get<1>(it).insert(std::get<1>(it).cbegin(),(*cur_tmp)[0]);
                                }

                                auto last_end = out_change_list.load()->cend();
                                out_change_list.load()->splice(last_end, std::get<1>(deeper_delete));
                            }
                            if ((*cur_tmp)[0] >= size.load()->size() || (*cur_tmp)[0] >= children.load()->size()) {
                                std::string not_found((const char *) cur_tmp->data(), cur_tmp->size());
                                DEBUG << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                                         " was exceeding limits of storage tree \"" +
                                         combined_path.load(std::memory_order_relaxed)->string() +
                                         "\" and was skipped!.";
                                lock.unlock();
                            }
                            //parallel end
                            *active_threads.load() -= 1;
                        };
                        if (num_threads == 1)first_index_exe_function();
                        else {
                            //threading manager
                            while (*active_threads.load() >= num_threads) {
#ifdef _WIN32
                                Sleep(10);
#else
                                usleep(10 * 1000);
#endif // _WIN32
                            }
                            *active_threads.load() += 1;
                            std::thread(first_index_exe_function).detach();
                        }
                        std::lock_guard lock2(m1);

                        beg = end;
                        current = (*end)[0];
                    }
                }
                cur = end;
            }
            return {out_size.load(), *out_change_list.load()};
        }

        ~tree_storage() {
            for (auto &i: *children.load()) {
                if (std::get<1>(i) != nullptr) {
                    delete std::get<1>(i);
                }
            }
        }
    };
}

#endif //UHLIBCOMMON_TREE_STORAGE_H
