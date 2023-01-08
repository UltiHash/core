//
// Created by benjamin-elias on 19.12.22.
//

#ifndef UHLIBCOMMON_TREE_STORAGE_H
#define UHLIBCOMMON_TREE_STORAGE_H

#include "logging/logging_boost.h"
#include <filesystem>
#include "boost/algorithm/hex.hpp"
#include <openssl/sha.h>
#include <shared_mutex>
#include <memory>
#include <thread>
#include <atomic>
#include <execution>
#include <mutex>
#include "util/exception.h"
#include "trees/tree_storage_config.h"
#include "trees/tree_storage_config.h.in"

#ifdef _WIN32
#include <windows.h>
#else

#include <unistd.h>

#endif // _WIN32

#include "sys/types.h"
#include "sys/sysinfo.h"

namespace uh::trees {
    DEFINE_EXCEPTION(out_of_memory);

#define N (unsigned int) (std::numeric_limits<unsigned char>::max() + 1)

    typedef struct tree_storage tree_storage;

    /*
     * module description: Tree storage is fast tree based interface to write binary information to disk at a given root folder, the tree storage root
     * The module is a thread safe and multithreading optimized approach to manage data on the later system. The storage tree
     * automatically balances load among the used B+-Tree file tree to share load of all hard drives of the RAID controller in an optimal fashion
     * While maintaining the tree storage, deleting blocks with an outdated timestamp, we should prevent the to be deleted blocks from being read (only not thread safe aspect)
     */

    struct tree_storage {

    protected:
        //every file storage level contains a maximum of 256 storage chunks and 256 folders to deeper levels
        //different storage chunks with write, read and maintain protection flags
        std::shared_mutex size_protect = std::shared_mutex();
        std::shared_ptr<std::vector<std::tuple<std::size_t, unsigned char, std::shared_ptr<std::atomic_flag>, std::shared_ptr<std::atomic<std::size_t>>, std::shared_ptr<std::atomic_flag>>>> size = std::make_shared<std::vector<std::tuple<std::size_t, unsigned char, std::shared_ptr<std::atomic_flag>, std::shared_ptr<std::atomic<std::size_t>>, std::shared_ptr<std::atomic_flag>>>>();
        //deeper tree storage blocks and folders
        std::shared_mutex children_protect = std::shared_mutex(); // PROTOECT THE CHILDREN!!! :D
        std::shared_ptr<std::vector<std::tuple<std::size_t, tree_storage *, unsigned char>>> children = std::make_shared<std::vector<std::tuple<std::size_t, tree_storage *, unsigned char>>>();
        std::shared_mutex combined_path_protect = std::shared_mutex();
        std::shared_ptr<std::filesystem::path> combined_path = std::make_shared<std::filesystem::path>();
        std::shared_mutex work_steal_protect{};
        std::shared_mutex std_filesystem_protect{};
        std::shared_mutex memory_protect{};

        //returns wrapped string
        static std::vector<unsigned char> prefix_wrap(std::size_t input_size) {
            std::size_t h_bit{input_size};
            unsigned char total_bit_count{};
            //counting max bits
            while (h_bit != 0)[[likely]] {
                total_bit_count++;
                h_bit >>= 1;
            }
            unsigned char byte_count = total_bit_count / 8 + std::min(1, total_bit_count % 8);
            std::vector<unsigned char> prefix{};
            for (unsigned char i = 0; i < byte_count; i++) {
                prefix.push_back((unsigned char) (input_size >> (i * 8)));
            }
            if (prefix.empty())prefix.push_back(0);
            prefix.insert(prefix.cbegin(), byte_count - 1);
            return prefix;
        }

    private:
        template<typename ALLOC>
        std::vector<ALLOC> mem_wait(std::size_t mem) {
            //long pages = sysconf(_SC_PHYS_PAGES);
            //long page_size = sysconf(_SC_PAGE_SIZE);
            std::size_t count{};
            //auto totalMem = static_cast<unsigned long>(pages * page_size);
            std::size_t freeMem;

            do {
                std::unique_lock lock_mem(memory_protect,std::defer_lock);
                lock_mem.lock();
                struct sysinfo info{};
                sysinfo(&info);
                freeMem = info.freeram;

                if (freeMem >= mem * sizeof(ALLOC)) {
                    auto out_vec = std::vector<ALLOC>();
                    out_vec.resize(mem,0);
                    lock_mem.unlock();
                    return out_vec;
                }
                lock_mem.unlock();
#ifdef _WIN32
                Sleep(10);
#else
                usleep(10 * 1000);
#endif // _WIN32
                count++;
            } while (freeMem < mem * sizeof(ALLOC));
            std::scoped_lock lock_mem2(memory_protect);
            auto out_vec = std::vector<ALLOC>();
            out_vec.resize(mem,0);
            return out_vec;
        }

    public:
        /*
         * tell a root folder, if another root is detected it is loaded into a loose substructure that is not indexed yet
         * the module will not know the blocks within the storage chunks, reading will fail
         * call the index function after crating the tree_storage to proceed
         */
        explicit tree_storage(const std::filesystem::path &root,
                              unsigned short num_threads = std::thread::hardware_concurrency()) {
            if (!num_threads)return;
            std::atomic<long> i_constructor{-1};

            std::unique_lock lock5(combined_path_protect, std::defer_lock);
            lock5.lock();
            if (combined_path->empty()) {
                std::string parent_name = root.filename().string();
                bool valid_root = parent_name.size() == 4 and root.extension().string().empty();
                if (valid_root)
                    for (const char &i: parent_name) {
                        if (!(('0' <= i and i <= '9') || ('A' <= i and i <= 'F'))) {
                            valid_root = false;
                            break;
                        }
                    }
                combined_path->operator=(root);
                if (!valid_root) {
                    combined_path->operator/=("0000");
                }
                std::scoped_lock filesystem_lock(std_filesystem_protect);
                std::filesystem::create_directories(combined_path->string());
            }
            lock5.unlock();

            auto multithread_constructor = [&]() {
                std::unique_lock step_lock_init(work_steal_protect,std::defer_lock);
                step_lock_init.lock();
                std::size_t i = (i_constructor += 1);
                step_lock_init.unlock();

                if (i >= N)return;
                for (; i < N;) {
                    std::string s_tmp = boost::algorithm::hex(std::string{(char) i});
                    std::shared_lock lock4(combined_path_protect);
                    std::filesystem::path chunk = *combined_path / s_tmp;
                    lock4.unlock();

                    std::unique_lock filesystem_lock(std_filesystem_protect, std::defer_lock);
                    filesystem_lock.lock();
                    if (std::filesystem::exists(chunk)) {
                        filesystem_lock.unlock();
                        std::shared_ptr<std::atomic_flag> f1 = std::make_shared<std::atomic_flag>(), f3 = std::make_shared<std::atomic_flag>();
                        std::shared_ptr<std::atomic<std::size_t>> f2 = std::make_shared<std::atomic<std::size_t>>();
                        std::scoped_lock lock1(size_protect);
                        size->emplace_back(std::filesystem::file_size(chunk), i, f1, f2, f3);
                    } else filesystem_lock.unlock();

                    lock4.lock();
                    std::string fname = combined_path->filename().string();
                    lock4.unlock();
                    s_tmp.insert(s_tmp.cbegin(), fname.cbegin() + 2, fname.cbegin() + 4);
                    lock4.lock();
                    std::filesystem::path deeper_tree = *combined_path / s_tmp;
                    lock4.unlock();
                    //check if sub folder in tree exists

                    filesystem_lock.lock();
                    if (std::filesystem::exists(deeper_tree)) {
                        if (std::filesystem::is_empty(deeper_tree)) {
                            std::filesystem::remove_all(deeper_tree);
                            filesystem_lock.unlock();
                        } else {
                            filesystem_lock.unlock();
                            auto *tmp_tree = new tree_storage(deeper_tree, 1);
                            std::scoped_lock lock2(children_protect);
                            children->emplace_back(tmp_tree->get_size(), tmp_tree, i);
                        }
                    } else filesystem_lock.unlock();
                    std::scoped_lock step_lock(work_steal_protect);
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
            std::unique_lock lock_size(size_protect, std::defer_lock);
            lock_size.lock();
            if (!std::is_sorted(std::execution::par_unseq, size->begin(), size->end(),
                                [](const auto &a, const auto &b) {
                                    return std::get<1>(a) < std::get<1>(b);
                                }))
                std::sort(std::execution::par, size->begin(), size->end(),
                          [](const auto &a, const auto &b) {
                              return std::get<1>(a) < std::get<1>(b);
                          });
            lock_size.unlock();
            std::scoped_lock lock_children(children_protect);
            if (!std::is_sorted(std::execution::par_unseq, children->begin(), children->end(),
                                [](const auto &a, const auto &b) {
                                    return std::get<1>(a) < std::get<1>(b);
                                }))
                std::sort(std::execution::par, children->begin(), children->end(),
                          [](const auto &a, const auto &b) {
                              return std::get<1>(a) < std::get<1>(b);
                          });
        }

        /*
         * get the size of the entire tree, so the size of all contained information in total, the tree always maintains total size
         * call get_info() to get the real block size and the total size as well as the block time stamp in comparison
         */
        std::size_t get_size() {
            std::size_t s{};
            for (unsigned short i = 0; i < (unsigned short) N; i++) {
                std::unique_lock lock1(size_protect, std::defer_lock), lock2(children_protect, std::defer_lock);
                lock1.lock();
                s += size->size() > i ? std::get<0>(size->at(i)) : 0;
                lock1.unlock();
                lock2.lock();
                if (children->size() > i) {
                    s += std::get<0>(children->at(i));
                }
                lock2.unlock();
                std::lock(lock1, lock2);
                if (i >= size->size() && i >= children->size()) {
                    lock2.unlock();
                    lock1.unlock();
                    break;
                } else {
                    lock2.unlock();
                    lock1.unlock();
                }
            }
            return s;
        }

        //write a binary block to a certain creation date and get a local block reference in return
        std::vector<unsigned char> write(const std::vector<unsigned char> &input,
                                         unsigned long current_time = (unsigned long) std::chrono::nanoseconds(
                                                 std::chrono::high_resolution_clock::now().time_since_epoch()).count()) {
            if (input.empty())return std::vector<unsigned char>{};
            if (input.size() > STORE_MAX) {
                FATAL << "A block could not be written because it exceeded maximum size of blocks \"" +
                         std::to_string(STORE_MAX) +
                         "\" with a size of \"" + std::to_string(input.size()) + "\".";
                return std::vector<unsigned char>{};
            }
            std::vector<unsigned char> prefix = prefix_wrap(input.size());
            std::size_t total_size = input.size() + prefix.size() + sizeof(unsigned long);
            //check block fill of this node, look for free space

            std::unique_lock lock_size(size_protect, std::defer_lock);
            lock_size.lock();
            unsigned short min_pos;
            std::size_t min_val;
            if (size->size() < N) {
                min_pos = size->size();
                lock_size.unlock();
                min_val = 0;
                std::shared_ptr<std::atomic_flag> f1 = std::make_shared<std::atomic_flag>(), f3 = std::make_shared<std::atomic_flag>();
                std::shared_ptr<std::atomic<std::size_t>> f2 = std::make_shared<std::atomic<std::size_t>>();
                std::scoped_lock size_own1(size_protect);
                size->emplace_back(min_val, min_pos, f1, f2, f3);
            } else {
                auto min_el = *std::min_element(size->begin(), size->end(), [](auto &a, auto &b) {
                    return std::get<0>(a) < std::get<0>(b) && !std::get<4>(a)->test() &&
                           !std::get<4>(b)->test();//skip maintain chunks for smaller check
                });
                min_pos = std::get<1>(min_el);
                min_val = std::get<0>(size->at(min_pos));
                lock_size.unlock();
            }
            lock_size.lock();
            bool no_deeper = min_val < STORE_MAX && min_val + total_size < STORE_HARD_LIMIT &&
                             !(std::get<4>(size->at(min_pos))->test());
            lock_size.unlock();
            if (!no_deeper) {
                //find or create balanced deeper tree node to store
                std::unique_lock lock_children(children_protect, std::defer_lock);
                lock_children.lock();
                if ((unsigned short) children->size() < (unsigned short) N) {
                    min_pos = (unsigned short) children->size();
                    std::string ref_name{boost::algorithm::hex(std::string{(char) min_pos})};
                    std::shared_lock lock_path1(combined_path_protect);
                    std::string fname = combined_path->filename().string();
                    lock_path1.unlock();
                    ref_name.insert(ref_name.cbegin(), fname.cbegin() + 2, fname.cbegin() + 4);
                    lock_path1.lock();
                    std::filesystem::path deeper_tree = *combined_path / ref_name;
                    lock_path1.unlock();
                    auto *tmp_tree = new tree_storage(deeper_tree);
                    min_val = tmp_tree->get_size();
                    children->emplace_back(min_val, tmp_tree, min_pos);
                } else {
                    auto min_el = *std::min_element(children->begin(), children->end(), [](auto &a, auto &b) {
                        return std::get<0>(a) < std::get<0>(b);
                    });
                    min_val = std::get<0>(min_el);
                    min_pos = std::get<2>(min_el);
                }
                std::get<0>(children->at(min_pos)) += total_size;
                lock_children.unlock();

                std::unique_lock read_children(children_protect, std::defer_lock);
                read_children.lock();
                auto tree_ptr2 = std::get<1>(children->at(min_pos));
                read_children.unlock();
                std::vector<unsigned char> out_vec = tree_ptr2->write(input, current_time);
                out_vec.insert(out_vec.cbegin(), min_pos);
                return out_vec;
            } else {
                //store block to this position
                std::string ref_name{boost::algorithm::hex(std::string{(char) min_pos})};
                std::shared_lock path_lock2(combined_path_protect);
                std::filesystem::path read_chunk = *combined_path / ref_name;
                path_lock2.unlock();

                //calculate binary of timestamp
                std::vector<unsigned char> bin_time;
                for (unsigned short i = 0; i < (unsigned short) sizeof(current_time); i++) {
                    bin_time.push_back((unsigned char) (current_time >> (i * 8)));
                }
                lock_size.lock();
                auto write_ptr = &(*std::get<2>(size->at(min_pos)));
                auto read_ptr = &(*std::get<3>(size->at(min_pos)));
                auto maintain_ptr = &(*std::get<4>(size->at(min_pos)));
                lock_size.unlock();

                while (std::atomic_flag_test_and_set_explicit(maintain_ptr, std::memory_order_acquire)) {
                    maintain_ptr->wait(true);
                }
                while (std::atomic_flag_test_and_set_explicit(write_ptr, std::memory_order_acquire)) {
                    write_ptr->wait(true);
                }

                while (read_ptr->load() > 0) {
#ifdef _WIN32
                    Sleep(10);
#else
                    usleep(10 * 1000);
#endif // _WIN32
                }
                lock_size.lock();
                std::size_t size_tmp = std::get<0>(size->at(min_pos));
                lock_size.unlock();
                std::unique_lock write_size(size_protect, std::defer_lock);
                write_size.lock();
                std::get<0>(size->at(min_pos)) += total_size;
                write_size.unlock();

                FILE *writer = std::fopen(read_chunk.make_preferred().c_str(), "ab");
                auto end_write_sequence = [&]() {
                    std::fclose(writer);

                    std::atomic_flag_clear_explicit(write_ptr, std::memory_order_release);
                    if (!write_ptr->test())write_ptr->notify_all();
                    std::atomic_flag_clear_explicit(maintain_ptr, std::memory_order_release);
                    if (!maintain_ptr->test())maintain_ptr->notify_one();
                };

                if (!writer) {
                    ERROR << "File write opening failed at \"" + read_chunk.string() + "\"";
                    end_write_sequence();
                    return std::vector<unsigned char>{};
                }
                //File should have been opened or created here
                std::fwrite(bin_time.data(), bin_time.size(), sizeof(unsigned char), writer);
                std::fwrite(prefix.data(), prefix.size(), sizeof(unsigned char), writer);
                std::fwrite(input.data(), input.size(), sizeof(unsigned char), writer);

                if (std::ferror(writer)) {
                    FATAL << "I/O error when writing \"" + read_chunk.string() + "\"";
                    end_write_sequence();
                    return std::vector<unsigned char>{};
                }

                end_write_sequence();

                //start position of the block for seeking it later on is the old size
                std::vector<unsigned char> local_block_ref{};
                local_block_ref.reserve(sizeof(unsigned int));
                for (unsigned short i = 0;
                     i < (unsigned short) sizeof(unsigned int); i++) {//STORE_MAX will fit in 4 bytes
                    local_block_ref.push_back((unsigned char) (size_tmp >> (i * 8)));
                }
                local_block_ref.insert(local_block_ref.cbegin(), min_pos);

                return local_block_ref;//structure: BIN,OFFSET * 4 BYTES
            }
        }

        /*
         * returns a tuple with block time and binary vector of block when entering a block reference (as far as it exists)
         * WARNING: not existing blocks will still crash the procedure, a radix tree for block existence is needed
         */
        std::tuple<unsigned long, std::vector<unsigned char>> read(const std::vector<unsigned char> &block_code) {
            if (block_code.empty()) {
                return {};
            }
            if (block_code.size() > 5) {
                //size encoding is not reached yet, read along tree path
                std::unique_lock read_children(children_protect, std::defer_lock);
                read_children.lock();
                if ((short) children->size() - 1 < (short) block_code[0] ||
                    !std::get<0>(children->at(block_code[0]))) {
                    read_children.unlock();
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    std::scoped_lock read_path(combined_path_protect);
                    DEBUG << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " could not be found in storage tree \"" +
                             combined_path->string() + "\".";
                    return {0, std::vector<unsigned char>{}};
                } else {
                    auto tree_ptr3 = std::get<1>(children->at(block_code[0]));
                    read_children.unlock();
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1, block_code.cend()};
                    auto out_vec = tree_ptr3->read(sub_block_code);
                    if (std::get<1>(out_vec).empty()) {
                        std::string not_found((const char *) block_code.data(), block_code.size());
                        std::scoped_lock read_path(combined_path_protect);
                        DEBUG << "<Block error trace on return>: Block code " + boost::algorithm::hex(not_found) +
                                 " could not be found in storage tree \"" +
                                 combined_path->string() + "\".";
                    }
                    return out_vec;
                }
            } else {
                if (block_code.size() < 5) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    std::scoped_lock read_path(combined_path_protect);
                    FATAL << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " was too short for storage tree \"" +
                             combined_path->string() + "\".";
                    return {0, std::vector<unsigned char>{}};
                }
                //the block code should have a size of 5; one chunk index and 4 bytes of encoding for the offset
                std::unique_lock size_lock(size_protect, std::defer_lock);
                size_lock.lock();
                if ((short) size->size() - 1 < (short) block_code[0] ||
                    !std::get<0>(size->at(block_code[0]))) {
                    size_lock.unlock();
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    std::scoped_lock read_path(combined_path_protect);
                    DEBUG
                        << "<Block error trace on return, final tree>: Block code " + boost::algorithm::hex(not_found) +
                           " could not be found in storage tree \"" + combined_path->string() +
                           "\" and size of storage chunk was 0.";
                    return {0, std::vector<unsigned char>{}};
                } else {
                    size_lock.unlock();
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1,
                                                              block_code.cend()}; //copy offset code and rebuild
                    std::size_t offset{};
                    for (unsigned short i = 0; i < (unsigned short) sizeof(unsigned int); i++) {
                        offset += (((std::size_t) sub_block_code[i]) << (i * 8));
                    }
                    std::string ref_name{boost::algorithm::hex(std::string{(char) block_code[0]})};
                    std::shared_lock read_path_lock(combined_path_protect);
                    std::filesystem::path read_path = *combined_path / ref_name;
                    read_path_lock.unlock();

                    size_lock.lock();
                    auto write_ptr = &(*std::get<2>(size->at(block_code[0])));
                    auto read_ptr = &(*std::get<3>(size->at(block_code[0])));
                    size_lock.unlock();

                    *read_ptr += 1;

                    while (write_ptr->test()) {
                        write_ptr->wait(true);
                    }

                    FILE *reader = std::fopen(read_path.make_preferred().c_str(), "rb");
                    auto read_end_sequence = [&]() {
                        std::fclose(reader);
                        *read_ptr -= 1;
                        if (!read_ptr->load())write_ptr->notify_one();
                    };
                    if (!reader) {
                        ERROR << "File read opening failed at \"" + read_path.string() + "\"";
                        read_end_sequence();
                        return {0, std::vector<unsigned char>{}};
                    }
                    //File should have been opened or created here
                    std::fseek(reader, static_cast<long>(offset), SEEK_SET);

                    if (std::ferror(reader)) {
                        FATAL << "I/O error when seeking \"" + read_path.string() + "\"";
                        read_end_sequence();
                        return {0, std::vector<unsigned char>{}};
                    }

                    unsigned char time_buf[sizeof(unsigned long)];
                    std::size_t total_read{};
                    std::size_t count = std::fread(&time_buf, sizeof(unsigned char), sizeof(unsigned long), reader);
                    total_read += count;
                    if (count != sizeof(unsigned long)) {
                        FATAL
                            << "I/O time first 8 bytes reading was not completed on path \"" + read_path.string() +
                               "\"";
                        read_end_sequence();
                        return {0, std::vector<unsigned char>{}};
                    }

                    if (std::ferror(reader)) {
                        FATAL << "I/O error when reading time of block at path \"" + read_path.string() + "\"";
                        read_end_sequence();
                        return {0, std::vector<unsigned char>{}};
                    }
                    unsigned long block_time{};
                    for (unsigned short i = 0; i < (unsigned short) sizeof(unsigned long); i++) {
                        block_time += (((std::size_t) time_buf[i]) << (i * 8));
                    }

                    unsigned char buf_size = 0;
                    count = std::fread(&buf_size, sizeof(unsigned char), 1, reader);
                    total_read += count;
                    if (count != 1) {
                        FATAL
                            << "I/O prefix first byte reading was not completed on path \"" + read_path.string() + "\"";
                        read_end_sequence();
                        return {0, std::vector<unsigned char>{}};
                    }

                    if (std::ferror(reader)) {
                        FATAL << "I/O error when reading prefix at path \"" + read_path.string() + "\"";
                        read_end_sequence();
                        return {0, std::vector<unsigned char>{}};
                    }
                    unsigned char buffer_in[buf_size + 1];
                    count = std::fread(&buffer_in, sizeof(unsigned char), buf_size + 1, reader);
                    total_read += count;
                    if (count != buf_size + 1) {
                        FATAL
                            << "I/O prefix first byte reading was not completed on path \"" + read_path.string() + "\"";
                        read_end_sequence();
                        return {0, std::vector<unsigned char>{}};
                    }
                    std::size_t output_size{};
                    for (unsigned char buf_count = 0; buf_count <= buf_size; buf_count++) {
                        output_size += (((std::size_t) buffer_in[buf_count]) << (buf_count * 8));
                    }
                    auto tmp_buf = mem_wait<unsigned char>(output_size);
                    count = std::fread(tmp_buf.data(), sizeof(unsigned char), tmp_buf.size(), reader);
                    total_read += count;

                    if (count != output_size) {
                        FATAL << "I/O was not completed on path \"" + read_path.string() + "\"";
                        read_end_sequence();
                        return {0, std::vector<unsigned char>{}};
                    }
                    if (std::ferror(reader)) {
                        FATAL << "I/O error when reading prefix at path \"" + read_path.string() + "\"";
                        read_end_sequence();
                        return {0, std::vector<unsigned char>{}};
                    }

                    read_end_sequence();

                    return {block_time, tmp_buf};
                }
            }
        }

        /*
         * The index reads the entire information of the tree storage and calculates SHA512 hashes for every single block
         * matching a local storage reference
         * By running index on startup of a DB node it can scans information about the existence and validity of all blocks
         * returns hash, local block reference and the block last visited time
         */
        std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, unsigned long>>
        index(unsigned short num_threads = std::thread::hardware_concurrency()) {
            if (!num_threads)return {};
            std::shared_ptr<std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, unsigned long>>> search_index = std::make_shared<std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, unsigned long>>>();
            std::shared_mutex search_index_protect{};
            std::atomic<long> i_constructor{-1};
            std::atomic_flag error_flag{ATOMIC_FLAG_INIT};

            auto multithread_index = [&]() {
                std::unique_lock step_lock_init(work_steal_protect,std::defer_lock);
                step_lock_init.lock();
                std::size_t i = (i_constructor += 1);
                step_lock_init.unlock();

                if (i >= N)return;
                for (; i < N;) {
                    std::unique_lock size_lock(size_protect, std::defer_lock);
                    size_lock.lock();
                    if (i < size->size() && std::get<0>(size->at(i)) > 0) {
                        size_lock.unlock();
                        //read entire block generating hashes and block references
                        std::string ref_name{boost::algorithm::hex(std::string{(char) i})};
                        std::shared_lock read_path_lock(combined_path_protect);
                        std::filesystem::path read_path = *combined_path / ref_name;
                        read_path_lock.unlock();

                        size_lock.lock();
                        auto write_ptr = &(*std::get<2>(size->at(i)));
                        auto read_ptr = &(*std::get<3>(size->at(i)));
                        size_lock.unlock();

                        *read_ptr += 1;

                        while (write_ptr->test()) {
                            write_ptr->wait(true);
                        }

                        FILE *reader = std::fopen(read_path.make_preferred().c_str(), "rb");
                        std::unique_lock filesystem_lock(std_filesystem_protect, std::defer_lock);
                        filesystem_lock.lock();
                        std::size_t total_file_size = std::filesystem::file_size(read_path.make_preferred().c_str());
                        filesystem_lock.unlock();
                        auto read_end_sequence = [&]() {
                            std::fclose(reader);
                            *read_ptr -= 1;
                            if (!read_ptr->load())write_ptr->notify_one();
                        };
                        auto error_thread_sequence = [&]() {
                            while (std::atomic_flag_test_and_set_explicit(&error_flag, std::memory_order_acquire)) {
                                return;
                            }
                        };
                        if (!reader) {
                            ERROR << "File read opening failed at \"" + read_path.string() + "\"";
                            read_end_sequence();
                            error_thread_sequence();
                            if (error_flag.test())return;
                        }
                        //File should have been opened or created here
                        if (std::ferror(reader)) {
                            FATAL << "I/O error when seeking \"" + read_path.string() + "\"";
                            read_end_sequence();
                            error_thread_sequence();
                            if (error_flag.test())return;
                        }
                        std::atomic<std::size_t> cur_pos{};

                        std::thread rt, ht;

                        std::vector<unsigned char> local_block_ref[2];
                        std::atomic<unsigned long> block_time_current{};
                        std::vector<unsigned char>tmp_buf[2];

                        std::atomic<std::size_t> output_size{};
                        std::atomic<bool> parallel_switch{};
                        std::mutex m1{};
                        std::unique_lock global_thread_lock(m1, std::defer_lock);
                        while (!std::feof(reader) && cur_pos.load() < total_file_size) {
                            auto read_func = [&]() {
                                std::unique_lock local_thread_lock(m1);
                                auto parallel_switch_cpy = parallel_switch.load();
                                parallel_switch = !parallel_switch.load();
                                local_block_ref[parallel_switch_cpy].clear();

                                for (unsigned short i1 = 0;
                                     i1 < (unsigned short) sizeof(unsigned int); i1++) {//STORE_MAX will fit in 4 bytes
                                    local_block_ref[parallel_switch_cpy].push_back((unsigned char) (cur_pos.load() >> (i1 * 8)));
                                }
                                local_block_ref[parallel_switch_cpy].insert(local_block_ref[parallel_switch_cpy].cbegin(), i);

                                unsigned char time_buf[sizeof(unsigned long)];
                                std::size_t count = std::fread(&time_buf, sizeof(unsigned char), sizeof(unsigned long),
                                                               reader);
                                cur_pos += count;
                                if (count != sizeof(unsigned long)) {
                                    FATAL
                                        << "I/O time first 8 bytes reading was not completed on path \"" +
                                           read_path.string() + "\"";
                                    read_end_sequence();
                                    error_thread_sequence();
                                    if (error_flag.test())return;
                                }

                                if (std::ferror(reader)) {
                                    FATAL << "I/O error when reading time of block at path \"" + read_path.string() +
                                             "\"";
                                    read_end_sequence();
                                    error_thread_sequence();
                                    if (error_flag.test())return;
                                }
                                unsigned long block_time{};
                                for (unsigned short i4 = 0; i4 < (unsigned short) sizeof(unsigned long); i4++) {
                                    block_time += (((std::size_t) time_buf[i4]) << (i4 * 8));
                                }
                                block_time_current = block_time;

                                unsigned char buf_size = 0;
                                count = std::fread(&buf_size, sizeof(unsigned char), 1, reader);
                                cur_pos += count;
                                if (count != 1) {
                                    FATAL << "I/O prefix first byte reading was not completed on path \"" +
                                             read_path.string() +
                                             "\"";
                                    read_end_sequence();
                                    error_thread_sequence();
                                    if (error_flag.test())return;
                                }

                                if (std::ferror(reader)) {
                                    FATAL << "I/O error when reading prefix at path \"" + read_path.string() + "\"";
                                    read_end_sequence();
                                    error_thread_sequence();
                                    if (error_flag.test())return;
                                }
                                unsigned char buffer_for_size[buf_size + 1];
                                count = std::fread(&buffer_for_size, sizeof(unsigned char), buf_size + 1, reader);
                                cur_pos += count;
                                if (count != buf_size + 1) {
                                    FATAL << "I/O prefix first byte reading was not completed on path \"" +
                                             read_path.string() +
                                             "\"";
                                    read_end_sequence();
                                    error_thread_sequence();
                                    if (error_flag.test())return;
                                }
                                output_size = 0;
                                for (unsigned char buf_count = 0; buf_count <= buf_size; buf_count++) {
                                    output_size += (((std::size_t) buffer_for_size[buf_count]) << (buf_count * 8));
                                }
                                local_thread_lock.unlock();

                                tmp_buf[parallel_switch_cpy] = mem_wait<unsigned char>(output_size.load());
                                count = std::fread(tmp_buf[parallel_switch_cpy].data(), sizeof(unsigned char),
                                                   tmp_buf[parallel_switch_cpy].size(), reader);
                                cur_pos += count;

                                if (count != output_size.load()) {
                                    FATAL << "I/O was not completed on path \"" + read_path.string() + "\"";
                                    read_end_sequence();
                                    error_thread_sequence();
                                    if (error_flag.test())return;
                                }
                                if (std::ferror(reader)) {
                                    FATAL << "I/O error when reading prefix at path \"" + read_path.string() + "\"";
                                    read_end_sequence();
                                    error_thread_sequence();
                                    if (error_flag.test())return;
                                }
                            };

                            if (rt.joinable())rt.join();
                            if (num_threads > 1) rt = std::thread(read_func);
                            else read_func();

                            auto hash_func = [&]() {
                                unsigned char hash_buf[SHA512_DIGEST_LENGTH];//HASH GENERATION
                                auto block_time_cpy = block_time_current.load();
                                auto parallel_switch_cpy = parallel_switch.load();
                                global_thread_lock.unlock();
                                SHA512(tmp_buf[!parallel_switch_cpy].data(), tmp_buf[!parallel_switch_cpy].size(), hash_buf);

                                std::vector<unsigned char> hash{hash_buf, hash_buf + SHA512_DIGEST_LENGTH};
                                std::scoped_lock lock_emplace(search_index_protect);
                                search_index->emplace_back(hash, local_block_ref[!parallel_switch_cpy], block_time_cpy);
                            };

                            if (rt.joinable())rt.join();
                            if (error_flag.test()) {
                                FATAL << "Indexing reader thread quit unexpectedly!";
                                return;
                            }
                            if (ht.joinable())ht.join();
                            global_thread_lock.lock();
                            if (num_threads > 1){
                                ht = std::thread(hash_func);
                            }
                            else hash_func();

                            if (std::feof(reader) || cur_pos.load() >= total_file_size) {
                                if (num_threads > 1) {
                                    if (rt.joinable())rt.join();
                                    if (ht.joinable())ht.join();
                                }
                            }
                        }
                        read_end_sequence();
                    } else {
                        size_lock.unlock();
                    }
                    //splice indexes of children plus the min_pos to decide local_block_ref of children array
                    std::unique_lock children_lock(children_protect, std::defer_lock);
                    children_lock.lock();
                    if (i < children->size() && std::get<0>(children->at(i)) > 0) {
                        auto tree_ptr = std::get<1>(children->at(i));
                        children_lock.unlock();
                        auto append_list = tree_ptr->index(
                                std::min((unsigned short) 2, (unsigned short) (num_threads % 2)));
                        for (auto &el: append_list) {
                            std::get<1>(el).insert(std::get<1>(el).cbegin(), i);
                        }
                        std::scoped_lock lock_splice(search_index_protect);
                        search_index->splice(search_index->cend(), append_list);
                    } else {
                        children_lock.unlock();
                    }
                    std::lock(size_lock, children_lock);
                    if (i >= size->size() && i >= children->size()) {
                        size_lock.unlock();
                        children_lock.unlock();
                        break;
                    } else {
                        size_lock.unlock();
                        children_lock.unlock();
                    }
                    std::scoped_lock step_lock(work_steal_protect);
                    i = (i_constructor += 1);
                }
            };

            if (num_threads <= 2) {
                multithread_index();
            } else {
                std::vector<std::thread> workers;
                for (unsigned short i = 0; i < std::min((unsigned short) (num_threads / 2 + num_threads % 2),
                                                        (unsigned short) std::thread::hardware_concurrency()); i++) {
                    std::thread w(multithread_index);
                    workers.push_back(std::move(w));
                }
                for (auto &th: workers) {
                    th.join();
                }
            }
            if (error_flag.test()) {
                FATAL << "Indexing threading engine crashed unexpectedly!";
                return {};
            }

            std::scoped_lock lock_return(search_index_protect);
            return *search_index;
        }

        /*
         * deletes all storage recursively within its root file and unregisters it on RAM
         * In case of failure the node can clean itself off and reload blocks from different locations where the now
         * missing blocks still persist
         */
        std::size_t delete_recursive(unsigned short num_threads = std::thread::hardware_concurrency()) {
            if (!num_threads)return {};
            std::atomic<long> i_constructor{-1};
            std::atomic<std::size_t> out_size{};
            std::atomic_flag error_flag{ATOMIC_FLAG_INIT};

            auto multithread_index = [&]() {
                std::unique_lock step_lock_init(work_steal_protect,std::defer_lock);
                step_lock_init.lock();
                std::size_t i = (i_constructor += 1);
                step_lock_init.unlock();

                if (i >= N)return;
                for (; i < N;) {
                    std::unique_lock size_lock(size_protect, std::defer_lock);
                    size_lock.lock();
                    if (i < size->size() && std::get<0>(size->at(i)) > 0) {
                        std::string ref_name{boost::algorithm::hex(std::string{(char) i})};
                        std::shared_lock lock_path(combined_path_protect);
                        std::filesystem::path read_path = *combined_path / ref_name;
                        lock_path.unlock();
                        std::unique_lock filesystem_lock(std_filesystem_protect, std::defer_lock);
                        filesystem_lock.lock();
                        std::size_t vanish_size = std::filesystem::exists(read_path) ? std::filesystem::file_size(
                                read_path) : 0;
                        filesystem_lock.unlock();
                        out_size += vanish_size;
                        std::get<0>(size->at(i)) -= vanish_size;
                        size_lock.unlock();
                        filesystem_lock.lock();
                        if (std::remove(read_path.c_str()) != 0) {
                            filesystem_lock.unlock();
                            FATAL << "Removing was not completed on path \"" + read_path.string() + "\"";
                            while (std::atomic_flag_test_and_set_explicit(&error_flag, std::memory_order_acquire)) {
                                return;
                            }
                            if (error_flag.test())return;
                        } else filesystem_lock.unlock();
                    } else {
                        size_lock.unlock();
                    }
                    //splice indexes of children plus the min_pos to decide local_block_ref of children array
                    std::unique_lock children_lock(children_protect, std::defer_lock);
                    children_lock.lock();
                    if (i < children->size() && std::get<0>(children->at(i)) > 0) {
                        auto tree_ptr = std::get<1>(children->at(i));
                        children_lock.unlock();
                        std::size_t vanish_size = tree_ptr->delete_recursive(1);
                        std::unique_lock filesystem_lock(std_filesystem_protect, std::defer_lock);
                        filesystem_lock.lock();
                        std::shared_lock path_protect(combined_path_protect,std::defer_lock);
                        path_protect.lock();
                        if(std::filesystem::exists(*combined_path)){
                            std::filesystem::remove_all(*combined_path);
                        }
                        path_protect.unlock();
                        filesystem_lock.unlock();
                        out_size += vanish_size;
                        std::scoped_lock lg(size_protect);
                        std::get<0>(size->at(i)) -= vanish_size;
                    } else {
                        children_lock.unlock();
                    }

                    std::lock(size_lock, children_lock);
                    if (i >= size->size() && i >= children->size()) {
                        size_lock.unlock();
                        children_lock.unlock();
                        break;
                    } else {
                        size_lock.unlock();
                        children_lock.unlock();
                    }
                    std::scoped_lock step_lock(work_steal_protect);
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
            if (error_flag.test()) {
                FATAL << "Delete_recursive threading engine crashed unexpectedly!";
                return {};
            }
            return out_size.load();
        }

        //returns a tuple with block time and block size from disk and total size on disk on request of local block reference
        std::tuple<unsigned long, std::size_t, std::size_t> get_info(const std::vector<unsigned char> &block_code) {
            if (block_code.empty()) {
                return {};
            }
            if (block_code.size() > 5) {
                //size encoding is not reached yet, get_info along tree path
                std::unique_lock read_children(children_protect, std::defer_lock);
                read_children.lock();
                if ((short) children->size() - 1 < (short) block_code[0] ||
                    !std::get<0>(children->at(block_code[0]))) {
                    read_children.unlock();
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    std::scoped_lock read_path(combined_path_protect);
                    DEBUG << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " could not be found in storage tree \"" +
                             combined_path->string() + "\".";
                    return {};
                } else {
                    auto tree_ptr = std::get<1>(children->at(block_code[0]));
                    read_children.unlock();
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1, block_code.cend()};
                    auto out_vec = tree_ptr->get_info(sub_block_code);
                    if (std::get<1>(out_vec) == 0) {
                        std::string not_found((const char *) block_code.data(), block_code.size());
                        std::scoped_lock read_path(combined_path_protect);
                        DEBUG << "<Block error trace on return>: Block code " + boost::algorithm::hex(not_found) +
                                 " could not be found in storage tree \"" +
                                 combined_path->string() + "\".";
                    }
                    return out_vec;
                }
            } else {
                if (block_code.size() < 5) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    std::scoped_lock read_path(combined_path_protect);
                    FATAL << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " was too short for storage tree \"" +
                             combined_path->string() + "\".";
                    return {};
                }
                //the block code should have a size of 5; one chunk index and 4 bytes of encoding for the offset
                std::unique_lock read_size(size_protect, std::defer_lock);
                read_size.lock();
                if ((short) size->size() - 1 < (short) block_code[0] ||
                    !std::get<0>(size->at(block_code[0]))) {
                    read_size.unlock();
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    std::scoped_lock read_path(combined_path_protect);
                    DEBUG
                        << "<Block error trace on return, final tree>: Block code " + boost::algorithm::hex(not_found) +
                           " could not be found in storage tree \"" +
                           combined_path->string() +
                           "\" and size of storage chunk was 0.";
                    return {};
                } else {
                    read_size.unlock();
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1,
                                                              block_code.cend()}; //copy offset code and rebuild
                    std::size_t offset{};
                    for (unsigned short i = 0; i < (unsigned short) sizeof(unsigned int); i++) {
                        offset += (((std::size_t) sub_block_code[i]) << (i * 8));
                    }
                    std::string ref_name{boost::algorithm::hex(std::string{(char) block_code[0]})};
                    std::shared_lock read_path2(combined_path_protect);
                    std::filesystem::path read_path = *combined_path / ref_name;
                    read_path2.unlock();

                    read_size.lock();
                    auto write_ptr = &(*std::get<2>(size->at(block_code[0])));
                    auto read_ptr = &(*std::get<3>(size->at(block_code[0])));
                    read_size.unlock();

                    *read_ptr += 1;

                    while (write_ptr->test()) {
                        write_ptr->wait(true);
                    }

                    FILE *reader = std::fopen(read_path.make_preferred().c_str(), "rb");
                    auto read_end_sequence = [&]() {
                        std::fclose(reader);
                        *read_ptr -= 1;
                        if (!read_ptr->load())write_ptr->notify_one();
                    };
                    if (!reader) {
                        ERROR << "File get_info opening failed at \"" + read_path.string() + "\"";
                        read_end_sequence();
                        return {};
                    }
                    //File should have been opened or created here
                    std::fseek(reader, static_cast<long>(offset), SEEK_SET);

                    if (std::ferror(reader)) {
                        FATAL << "I/O error when seeking \"" + read_path.string() + "\"";
                        read_end_sequence();
                        return {};
                    }

                    unsigned char time_buf[sizeof(unsigned long)];
                    std::size_t count = std::fread(&time_buf, sizeof(unsigned char), sizeof(unsigned long), reader);
                    if (count != sizeof(unsigned long)) {
                        FATAL
                            << "I/O time first 8 bytes reading was not completed on path \"" + read_path.string() +
                               "\"";
                        read_end_sequence();
                        return {};
                    }

                    if (std::ferror(reader)) {
                        FATAL << "I/O error when reading time of block at path \"" + read_path.string() + "\"";
                        read_end_sequence();
                        return {};
                    }
                    unsigned long block_time{};
                    for (unsigned short i = 0; i < (unsigned short) sizeof(unsigned long); i++) {
                        block_time += (((std::size_t) time_buf[i]) << (i * 8));
                    }

                    unsigned char buf_size = 0;
                    count = std::fread(&buf_size, sizeof(unsigned char), 1, reader);
                    if (count != 1) {
                        FATAL
                            << "I/O prefix first byte reading was not completed on path \"" + read_path.string() + "\"";
                        read_end_sequence();
                        return {};
                    }

                    if (std::ferror(reader)) {
                        FATAL << "I/O error when reading prefix at path \"" + read_path.string() + "\"";
                        read_end_sequence();
                        return {};
                    }
                    unsigned char buffer_in[buf_size + 1];
                    count = std::fread(&buffer_in, sizeof(unsigned char), buf_size + 1, reader);
                    if (count != buf_size + 1) {
                        FATAL
                            << "I/O prefix first byte reading was not completed on path \"" + read_path.string() + "\"";
                        read_end_sequence();
                        return {};
                    }
                    std::size_t output_size{};
                    for (unsigned char buf_count = 0; buf_count <= buf_size; buf_count++) {
                        output_size += (((std::size_t) buffer_in[buf_count]) << (buf_count * 8));
                    }

                    read_end_sequence();

                    return {block_time, output_size, sizeof(time_buf) + 1 + sizeof(buffer_in) + output_size};
                }
            }
        }

        /*
         * returns success on changing touch time of a block, giving the function a valid block code
         * WARNING: Wrong block references will cause data loss!!!
         */
        bool set_block_time(const std::vector<unsigned char> &block_code,
                            unsigned long current_time = (unsigned long) std::chrono::nanoseconds(
                                    std::chrono::high_resolution_clock::now().time_since_epoch()).count()) {
            if (block_code.empty()) {
                return false;
            }
            if (block_code.size() > 5) {
                //size encoding is not reached yet, set_block_time along tree path
                std::unique_lock read_children(children_protect, std::defer_lock);
                read_children.lock();
                if ((short) children->size() - 1 < (short) block_code[0] ||
                    !std::get<0>(children->at(block_code[0]))) {
                    read_children.unlock();
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    std::scoped_lock read_path(combined_path_protect);
                    DEBUG << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " could not be found in storage tree \"" +
                             combined_path->string() + "\".";
                    return false;
                } else {
                    auto tree_ptr = std::get<1>(children->at(block_code[0]));
                    read_children.unlock();
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1, block_code.cend()};
                    auto out_vec = tree_ptr->set_block_time(sub_block_code, current_time);
                    if (!out_vec) {
                        std::string not_found((const char *) block_code.data(), block_code.size());
                        std::scoped_lock read_path(combined_path_protect);
                        DEBUG << "<Block error trace on return>: Block code " + boost::algorithm::hex(not_found) +
                                 " could not be found in storage tree \"" + combined_path->string() + "\".";
                    }
                    return out_vec;
                }
            } else {
                if (block_code.size() < 5) {
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    std::scoped_lock read_path(combined_path_protect);
                    FATAL << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " was too short for storage tree \"" +
                             combined_path->string() + "\".";
                    return false;
                }
                //the block code should have a size of 5; one chunk index and 4 bytes of encoding for the offset
                std::unique_lock size_read(size_protect, std::defer_lock);
                size_read.lock();
                if ((short) size->size() - 1 < (short) block_code[0] ||
                    !std::get<0>(size->at(block_code[0]))) {
                    size_read.unlock();
                    std::string not_found((const char *) block_code.data(), block_code.size());
                    std::scoped_lock read_path(combined_path_protect);
                    DEBUG
                        << "<Block error trace on return, final tree>: Block code " + boost::algorithm::hex(not_found) +
                           " could not be found in storage tree \"" + combined_path->string() +
                           "\" and size of storage chunk was 0.";
                    return false;
                } else {
                    size_read.unlock();
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1,
                                                              block_code.cend()}; //copy offset code and rebuild
                    std::size_t offset{};
                    for (unsigned short i = 0; i < (unsigned short) sizeof(unsigned int); i++) {
                        offset += (((std::size_t) sub_block_code[i]) << (i * 8));
                    }
                    std::string ref_name{boost::algorithm::hex(std::string{(char) block_code[0]})};
                    std::shared_lock read_path2(combined_path_protect);
                    std::filesystem::path read_path = *combined_path / ref_name;
                    read_path2.unlock();

                    //calculate binary of timestamp
                    std::vector<unsigned char> bin_time;
                    for (unsigned short i = 0; i < (unsigned short) sizeof(current_time); i++) {
                        bin_time.push_back((unsigned char) (current_time >> (i * 8)));
                    }

                    size_read.lock();
                    auto write_ptr = &(*std::get<2>(size->at(block_code[0])));
                    auto read_ptr = &(*std::get<3>(size->at(block_code[0])));
                    size_read.unlock();

                    while (std::atomic_flag_test_and_set_explicit(write_ptr, std::memory_order_acquire)) {
                        write_ptr->wait(true);
                    }

                    while (read_ptr->load() > 0) {
#ifdef _WIN32
                        Sleep(10);
#else
                        usleep(10 * 1000);
#endif // _WIN32
                    }

                    FILE *writer = std::fopen(read_path.make_preferred().c_str(), "rb+");
                    auto set_time_end_sequence = [&]() {
                        std::fclose(writer);
                        std::atomic_flag_clear_explicit(write_ptr, std::memory_order_release);
                        if (!write_ptr->test())write_ptr->notify_all();
                    };
                    if (!writer) {
                        ERROR << "File write opening failed at \"" + read_path.string() + "\"";
                        set_time_end_sequence();
                        return false;
                    }
                    //File should have been opened or created here
                    std::fseek(writer, static_cast<long>(offset), SEEK_SET);

                    if (std::ferror(writer)) {
                        FATAL << "I/O error when seeking \"" + read_path.string() + "\"";
                        set_time_end_sequence();
                        return false;
                    }
                    //File should have been opened or created here
                    std::fwrite(bin_time.data(), bin_time.size(), sizeof(unsigned char), writer);

                    if (std::ferror(writer)) {
                        FATAL << "I/O error when writing \"" + read_path.string() + "\"";
                        set_time_end_sequence();
                        return false;
                    }
                    set_time_end_sequence();

                    return true;
                }
            }
        }

        //TODO: integrate delete blocks, maintain valid time, introduce block max age
        //after deletion some blocks are de-fragmented in descending order. Behind the deleted block(s) all blocks need to be re-mapped
        //returns the deleted total size and a list of tuple<old_block_reference with first element tree reference, new_block_reference with first element tree reference,reference to tree_storage of change>
        //maintaining the system can be done in 2 steps: delete_blocks_copy and after that we need to re-map all local block references
        //after function delete_blocks_copy was called the atomic_flag securing write on the chunk will be active and we need to re-map all blocks of a chunk before setting the write flag back to false
        //it is wise to call a delete on blocks on the same chunk, one at a time
        ///WARNING: deleting is not fully thread safe since references are re-mapped, so make sure no reading is scheduled on the chunks carrying the blocks while calling delete!!
        ///Recommended: delete as many blocks as you have CPU cores so one core handles one block delete at a time
        /*
         * The function can delete multiple blocks at once, giving back changed offsets of other blocks it contained
         * which need to be internally reconfigured to be read
         */
        std::tuple<std::size_t, std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>>>>
        delete_blocks(
                std::vector<std::vector<unsigned char>> &block_codes,
                unsigned short num_threads = std::thread::hardware_concurrency()) {
            if (block_codes.empty() || !num_threads)return {};
            std::atomic_flag error_flag(ATOMIC_FLAG_INIT);
            std::atomic<std::size_t> active_threads{};
            std::atomic<std::size_t> out_size{};
            std::shared_mutex out_change_list_protect{};
            std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>>> out_change_list{};
            //sort for lexicographic to find blocks within the same chunks that all need to be deleted
            std::sort(std::execution::par, block_codes.begin(), block_codes.end(), [](auto &a, auto &b) {
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
                if (first && block_codes.size() > 1) {
                    first = false;
                    current = (*end)[0];
                } else {
                    if (block_codes.size() == 1)current = (*end)[0];
                    if (current != (*end)[0] || end == block_codes.end() - 1) {
                        //first filter all blocks with size 5 from the incoming sequence and delete them within this tree level
                        auto error_thread_sequence = [&]() {
                            while (std::atomic_flag_test_and_set_explicit(&error_flag,
                                                                          std::memory_order_acquire)) {
                                return;
                            }
                        };
                        std::mutex m1{};
                        std::unique_lock lock(m1, std::defer_lock);
                        auto first_index_exe_function = [&]() {
                            lock.lock();
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
                                        std::scoped_lock read_path(combined_path_protect);
                                        FATAL << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                                                 " was too short for storage tree \"" +
                                                 combined_path->string() + "\".";
                                        error_thread_sequence();
                                        if (error_flag.test())return;
                                    } else delete_here_codes.emplace_back(a.begin(), a.end());
                                }
                            });
                            std::unique_lock size_read(size_protect, std::defer_lock);
                            size_read.lock();
                            if (!delete_here_codes.empty() && (*cur_tmp)[0] < size->size() &&
                                std::get<0>(size->at((*cur_tmp)[0])) > 0) {
                                auto maintain_ptr = &(*std::get<4>(size->at((*cur_tmp)[0])));
                                auto read_ptr = &(*std::get<3>(size->at((*cur_tmp)[0])));
                                auto write_ptr = &(*std::get<2>(size->at((*cur_tmp)[0])));
                                size_read.unlock();
                                lock.unlock();

                                while (std::atomic_flag_test_and_set_explicit(&(*maintain_ptr),
                                                                              std::memory_order_acquire)) {
                                    maintain_ptr->wait(true);
                                }

                                *read_ptr += 1;

                                while (write_ptr->test()) {
                                    write_ptr->wait(true);
                                }

                                //sort blocks to be deleted here after their offset on the chunk
                                std::string ref_name{boost::algorithm::hex(std::string{(char) (*cur_tmp)[0]})};
                                std::shared_lock path_read(combined_path_protect);
                                std::filesystem::path chunk = *combined_path / ref_name;
                                path_read.unlock();
                                std::filesystem::path chunk_maintain =
                                        chunk.parent_path() / (chunk.filename().string() + "_maintain");

                                std::sort(delete_here_codes.begin(), delete_here_codes.end(), [](auto &a, auto &b) {
                                    std::vector<unsigned char> sub_block_code1{a.cbegin() + 1,
                                                                               a.cend()}, sub_block_code2{
                                            b.cbegin() + 1, b.cend()};
                                    std::size_t offset1{}, offset2{};
                                    for (unsigned short i = 0; i < (unsigned short) sizeof(unsigned int); i++) {
                                        offset1 += (((std::size_t) sub_block_code1[i]) << (i * 8));
                                        offset2 += (((std::size_t) sub_block_code2[i]) << (i * 8));
                                    }
                                    return offset1 < offset2;
                                });

                                auto block_step_beg = delete_here_codes.cbegin();

                                auto offset_calc = [](const auto &a_ref, const auto &b_ref) {
                                    std::vector<unsigned char> sub_block_code{a_ref + 1,
                                                                              b_ref}; //copy offset code and rebuild
                                    std::size_t offset{};
                                    for (unsigned short i = 0; i < (unsigned short) sizeof(unsigned int); i++) {
                                        offset += (((std::size_t) sub_block_code[i]) << (i * 8));
                                    }
                                    return offset;
                                };

                                //read chunk at index (*cur_tmp)[0]
                                FILE *reader = std::fopen(chunk.make_preferred().c_str(), "rb");
                                std::unique_lock filesystem_lock(std_filesystem_protect, std::defer_lock);
                                filesystem_lock.lock();
                                std::size_t total_file_size = std::filesystem::exists(chunk.make_preferred().c_str())
                                                              ? std::filesystem::file_size(
                                                chunk.make_preferred().c_str()) : 0;
                                filesystem_lock.unlock();
                                std::atomic_flag write_control(ATOMIC_FLAG_INIT);
                                auto read_end_sequence = [&]() {
                                    std::fclose(reader);
                                    *read_ptr -= 1;
                                    std::atomic_flag_clear_explicit(&write_control, std::memory_order_release);
                                };
                                if (!reader) {
                                    ERROR << "File read opening failed at \"" + chunk.string() + "\"";
                                    read_end_sequence();
                                    error_thread_sequence();
                                    if (error_flag.test())return;
                                }
                                std::size_t cur_pos = offset_calc(block_step_beg->cbegin(), block_step_beg->cend());

                                std::fseek(reader, static_cast<long>(cur_pos), SEEK_SET);
                                if (std::ferror(reader)) {
                                    FATAL << "I/O error when seeking \"" + chunk.string() + "\"";
                                    read_end_sequence();
                                    error_thread_sequence();
                                    if (error_flag.test())return;
                                }

                                std::size_t trunc_at = cur_pos;
                                std::size_t delete_size{};
                                //producer consumer queue for reading and writing; contains RAM pointer; size of RAM pointer; local tree storage reference of;
                                // *this tree where the referring chunk is stored; truncate size for the chunk; new storage reference after append
                                std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, std::size_t>> multithreading_factory{};
                                std::mutex multithreading_factory_protect{};

                                while (std::atomic_flag_test_and_set_explicit(&write_control,
                                                                              std::memory_order_acquire)) {
                                    write_control.wait(true);
                                }
                                filesystem_lock.lock();
                                if (std::filesystem::exists(chunk_maintain))std::filesystem::remove(chunk_maintain);
                                filesystem_lock.unlock();
                                FILE *writer = std::fopen(chunk_maintain.make_preferred().c_str(), "ab");
                                auto io_end_sequence = [&]() {
                                    std::fclose(reader);
                                    std::fclose(writer);
                                    *read_ptr -= 1;
                                    std::atomic_flag_clear_explicit(&write_control, std::memory_order_release);
                                };
                                if (!writer) {
                                    ERROR << "File write opening failed at \"" + chunk_maintain.string() + "\"";
                                    io_end_sequence();
                                    error_thread_sequence();
                                    if (error_flag.test())return;
                                }

                                auto write_once_to_maintain_file = [&]() {
                                    std::unique_lock multithread_f_read(multithreading_factory_protect,
                                                                        std::defer_lock);
                                    multithread_f_read.lock();
                                    auto new_offset = std::get<2>(*multithreading_factory.cbegin());
                                    auto old_block_code = std::get<1>(*multithreading_factory.cbegin());
                                    auto current_storage_ptr = std::get<0>(*multithreading_factory.cbegin());
                                    multithread_f_read.unlock();
                                    std::vector<unsigned char> out_vec{};
                                    out_vec.reserve(sizeof(unsigned int));
                                    //offset of blocks have been changed, take the first byte for chunk ordering and the last 4 bytes for offset;
                                    for (unsigned char i = 0;
                                         i <
                                         (unsigned char) sizeof(unsigned int); i++) {//STORE_MAX will fit in 4 bytes
                                        out_vec.push_back((unsigned char) (new_offset) >> (i * 8));
                                    }
                                    out_vec.insert(out_vec.cbegin(), old_block_code[0]);

                                    std::fwrite(current_storage_ptr.data(), current_storage_ptr.size(), sizeof(unsigned char),
                                                writer);
                                    if (std::ferror(writer)) {
                                        FATAL << "I/O error when deleting block writing \"" +
                                                 chunk_maintain.string() + "\"";
                                        io_end_sequence();
                                        error_thread_sequence();
                                        if (error_flag.test()) {
                                            return;
                                        }
                                    }
                                    //tmp_buf,write_back_size,out_vec,this,trunc_at
                                    //std::vector<unsigned char>, std::vector<unsigned char>, tree_storage *, std::size_t>>>
                                    std::unique_lock lock_output(out_change_list_protect, std::defer_lock);
                                    lock_output.lock();
                                    out_change_list.emplace_back(old_block_code, out_vec);
                                    lock_output.unlock();
                                    std::unique_lock write_multithreading_f(multithreading_factory_protect,
                                                                            std::defer_lock);
                                    write_multithreading_f.lock();
                                    multithreading_factory.pop_front();
                                    write_multithreading_f.unlock();
                                    if (error_flag.test()) {
                                        FATAL << "I/O extern error when deleting block writing \"" +
                                                 chunk_maintain.string() + "\"";
                                        io_end_sequence();
                                        error_thread_sequence();
                                        return;
                                    }

                                };

                                auto consumer_function = [&]() {
                                    while (write_control.test()) {
                                        std::unique_lock multithread_f_read(multithreading_factory_protect,
                                                                            std::defer_lock);
                                        multithread_f_read.lock();
                                        while (!multithreading_factory.empty()) {
                                            multithread_f_read.unlock();
                                            write_once_to_maintain_file();
                                            multithread_f_read.lock();
                                        }
                                        multithread_f_read.unlock();
                                        if (error_flag.test()) {
                                            FATAL << "I/O extern error when deleting block writing \"" +
                                                     chunk_maintain.string() + "\"";
                                            io_end_sequence();
                                            error_thread_sequence();
                                            return;
                                        }
#ifdef _WIN32
                                        Sleep(10);
#else
                                        usleep(10 * 1000);
#endif // _WIN32
                                    }
                                    std::fclose(writer);
                                };
                                std::thread w1;
                                if (num_threads > 1)w1 = std::thread(consumer_function);

                                while (!std::feof(reader) && cur_pos < total_file_size) {
                                    if (error_flag.test())break;//break thread in case error is there
                                    //File should have been opened or created here, seek for first block
                                    //start position of the block for seeking it later on is the old size
                                    std::vector<unsigned char> out_vec{};
                                    out_vec.reserve(sizeof(unsigned int));//reconstruct current local reference
                                    for (unsigned short i = 0;
                                         i < (unsigned short) sizeof(unsigned int); i++) {//STORE_MAX will fit in 4 bytes
                                        out_vec.push_back((unsigned char) (cur_pos >> (i * 8)));
                                    }
                                    out_vec.insert(out_vec.cbegin(), (*cur_tmp)[0]);

                                    unsigned char time_buf[sizeof(unsigned long)];
                                    std::size_t count = std::fread(&time_buf, sizeof(unsigned char),
                                                                   sizeof(unsigned long),
                                                                   reader);

                                    auto factory_io_sequence_end = [&]() {
                                        std::atomic_flag_test_and_set_explicit(&error_flag,
                                                                               std::memory_order_acquire);//put error flag on
                                        if (num_threads > 1) {
                                            if (w1.joinable())w1.join();//write out remaining blocks to prevent data loss
                                        } else{
                                            if(!multithreading_factory.empty())write_once_to_maintain_file();
                                            std::fclose(writer);
                                        }

                                        read_end_sequence();//reset reader flags
                                    };

                                    if (count != sizeof(unsigned long)) {
                                        FATAL
                                            << "I/O time first 8 bytes reading was not completed on path \"" +
                                               chunk.string() +
                                               "\"";
                                        factory_io_sequence_end();
                                        if (error_flag.test())return;//break thread in case error is there
                                    }
                                    cur_pos += count;

                                    if (std::ferror(reader)) {
                                        FATAL << "I/O error when reading time of block at path \"" + chunk.string() +
                                                 "\"";
                                        factory_io_sequence_end();
                                        if (error_flag.test())return;//break thread in case error is there
                                    }

                                    unsigned char buf_size = 0;
                                    count = std::fread(&buf_size, sizeof(unsigned char), 1, reader);
                                    if (count != 1) {
                                        FATAL
                                            << "I/O prefix first byte reading was not completed on path \"" +
                                               chunk.string() + "\"";
                                        factory_io_sequence_end();
                                        if (error_flag.test())return;//break thread in case error is there
                                    }
                                    cur_pos += count;

                                    if (std::ferror(reader)) {
                                        FATAL << "I/O error when reading prefix at path \"" + chunk.string() + "\"";
                                        factory_io_sequence_end();
                                        if (error_flag.test())return;//break thread in case error is there
                                    }
                                    unsigned char buffer_in[buf_size + 1];
                                    count = std::fread(&buffer_in, sizeof(unsigned char), buf_size + 1, reader);
                                    if (count != buf_size + 1) {
                                        FATAL
                                            << "I/O prefix first byte reading was not completed on path \"" +
                                               chunk.string() + "\"";
                                        factory_io_sequence_end();
                                        if (error_flag.test())return;//break thread in case error is there
                                    }
                                    cur_pos += count;
                                    std::size_t output_size{};
                                    for (unsigned char buf_count = 0; buf_count <= buf_size; buf_count++) {
                                        output_size += (((std::size_t) buffer_in[buf_count]) << (buf_count * 8));
                                    }

                                    std::size_t write_back_size =
                                            sizeof(time_buf) + 1 + sizeof(buffer_in) + output_size;

                                    if (block_step_beg < delete_here_codes.cend() &&
                                        std::equal(out_vec.cbegin(), out_vec.cend(), block_step_beg->cbegin(),
                                                   block_step_beg->cend())) {
                                        //skip block copy and seek over it
                                        std::fseek(reader, static_cast<long>(output_size), SEEK_CUR);
                                        if (std::ferror(reader)) {
                                            FATAL << "I/O error when seeking \"" + chunk.string() + "\"";
                                            factory_io_sequence_end();
                                            if (error_flag.test())return;//break thread in case error is there
                                        }
                                        cur_pos += output_size;
                                        delete_size += write_back_size;
                                        out_size += write_back_size;
                                        block_step_beg++;
                                    } else {
                                        //copy block to maintain file
                                        auto tmp_buf = mem_wait<unsigned char>(write_back_size);
                                        unsigned short i = 0;
                                        for (; i < (unsigned short) sizeof(time_buf); i++) {
                                            tmp_buf[i] = time_buf[i];
                                        }
                                        tmp_buf[i] = buf_size;
                                        i++;
                                        unsigned short meta_offset = i;
                                        i = 0;
                                        for (; i < (unsigned short) sizeof(buffer_in); i++) {
                                            tmp_buf[i + meta_offset] = buffer_in[i];
                                        }

                                        count = std::fread(tmp_buf.data() + (write_back_size - output_size),
                                                           sizeof(unsigned char),
                                                           output_size, reader);

                                        if (count != output_size) {
                                            FATAL << "I/O was not completed on path \"" + chunk.string() + "\"";
                                            factory_io_sequence_end();
                                            if (error_flag.test())return;//break thread in case error is there
                                        }
                                        if (std::ferror(reader)) {
                                            FATAL << "I/O error when reading prefix at path \"" + chunk.string() + "\"";
                                            factory_io_sequence_end();
                                            if (error_flag.test())return;//break thread in case error is there
                                        }
                                        cur_pos += count;
                                        std::scoped_lock write_multithreading_f(multithreading_factory_protect);
                                        multithreading_factory.emplace_back(tmp_buf, out_vec,
                                                                             cur_pos - write_back_size - delete_size);
                                    }
                                }
                                read_end_sequence();
                                if (num_threads > 1) {
                                    if (w1.joinable())w1.join();
                                } else {
                                    write_once_to_maintain_file();
                                    std::fclose(writer);
                                }

                                if (error_flag.test()) {
                                    FATAL << "Block deleting reader thread quit unexpectedly!";
                                    return;
                                }

                                for (auto it = block_step_beg; it < delete_here_codes.cend(); it++) {
                                    DEBUG << "Block code \"" +
                                             boost::algorithm::hex(std::string{it->cbegin(), it->cend()}) +
                                             "\" could not be deleted. Please check for bugs.";
                                }

                                //after the truncate position are known and the following blocks are filtered, truncation and append need to take place under atomic flag protection
                                //write and truncate
                                while (std::atomic_flag_test_and_set_explicit(&(*write_ptr),
                                                                              std::memory_order_acquire)) {
                                    write_ptr->wait(true);
                                }

                                while (read_ptr->load() > 0) {
#ifdef _WIN32
                                    Sleep(10);
#else
                                    usleep(10 * 1000);
#endif // _WIN32
                                }

                                std::size_t maintain_size_append = cur_pos - delete_size;
                                auto buf = mem_wait<unsigned char>(maintain_size_append);

                                FILE *source = fopen(chunk_maintain.make_preferred().c_str(), "rb");
                                auto ptr_release_sequence = [&]() {
                                    fclose(source);
                                    std::atomic_flag_clear_explicit(write_ptr, std::memory_order_release);
                                    if (!write_ptr->test())write_ptr->notify_one();
                                    std::atomic_flag_clear_explicit(maintain_ptr, std::memory_order_release);
                                    if (!maintain_ptr->test())maintain_ptr->notify_one();
                                };
                                if (!source) {
                                    ERROR << "File read opening failed at \"" + chunk_maintain.string() + "\"";
                                    error_thread_sequence();
                                    ptr_release_sequence();
                                    return;
                                }
                                if (fread(buf.data(), sizeof(unsigned char), buf.size(), source) != maintain_size_append) {
                                    ERROR << "File read opening for append failed at \"" + chunk_maintain.string() + "\"";
                                    error_thread_sequence();
                                    ptr_release_sequence();
                                    return;
                                }
                                if (std::ferror(source)) {
                                    FATAL << "I/O error when reading \"" + chunk_maintain.string() + "\"";
                                    error_thread_sequence();
                                    ptr_release_sequence();
                                    return;
                                }
                                if (truncate64(chunk.c_str(), static_cast<long>(trunc_at)) != 0) {
                                    ERROR << "File truncate failed at \"" + chunk_maintain.string() + "\"";
                                    error_thread_sequence();
                                    ptr_release_sequence();
                                    return;
                                }
                                FILE *dest = fopen(chunk.make_preferred().c_str(), "ab");
                                if (!dest) {
                                    ERROR << "File append opening failed at \"" + chunk_maintain.string() + "\"";
                                    fclose(dest);
                                    ptr_release_sequence();
                                    error_thread_sequence();
                                    return;
                                }
                                fwrite(buf.data(), sizeof(unsigned char), buf.size(), dest);
                                if (std::ferror(dest)) {
                                    FATAL << "I/O error when appending \"" + chunk_maintain.string() + "\"";
                                    fclose(dest);
                                    ptr_release_sequence();
                                    error_thread_sequence();
                                    return;
                                }

                                fclose(dest);
                                fclose(source);

                                filesystem_lock.lock();
                                //std::filesystem::remove(chunk_maintain);
                                filesystem_lock.unlock();

                                size_read.lock();
                                std::get<0>(size->at((*cur_tmp)[0])) -= delete_size;
                                size_read.unlock();

                                std::atomic_flag_clear_explicit(write_ptr, std::memory_order_release);
                                if (!write_ptr->test())write_ptr->notify_one();
                                std::atomic_flag_clear_explicit(maintain_ptr, std::memory_order_release);
                                if (!maintain_ptr->test())maintain_ptr->notify_one();
                            } else {
                                size_read.unlock();
                                lock.unlock();
                            }

                            //delete deeper codes
                            std::unique_lock children_read(children_protect, std::defer_lock);
                            children_read.lock();
                            if (!deeper_codes.empty() && (*cur_tmp)[0] < children->size() &&
                                std::get<0>(children->at((*cur_tmp)[0])) > 0) {
                                auto tmp_deeper_tree_ptr = std::get<1>(children->at((*cur_tmp)[0]));
                                children_read.unlock();
                                //parallel start
                                auto deeper_delete = tmp_deeper_tree_ptr->delete_blocks(deeper_codes, 2);
                                children_read.lock();
                                std::get<0>(children->at((*cur_tmp)[0])) -= std::get<0>(
                                        deeper_delete);//subtract deleted size from deeper node
                                children_read.unlock();
                                out_size += std::get<0>(deeper_delete);//add total deleted size
                                for (auto &it: std::get<1>(deeper_delete)) {
                                    //deeper elements have their first position filtered so that we need to rebuild it
                                    std::get<0>(it).insert(std::get<0>(it).cbegin(), (*cur_tmp)[0]);
                                    std::get<1>(it).insert(std::get<1>(it).cbegin(), (*cur_tmp)[0]);
                                }

                                std::scoped_lock lock_splice(out_change_list_protect);
                                out_change_list.splice(out_change_list.cend(), std::get<1>(deeper_delete));
                            } else children_read.unlock();

                            std::lock(size_read, children_read);
                            if ((*cur_tmp)[0] >= size->size() || (*cur_tmp)[0] >= children->size()) {
                                children_read.unlock();
                                size_read.unlock();
                                std::string not_found((const char *) cur_tmp->data(), cur_tmp->size());
                                std::scoped_lock path_read(combined_path_protect);
                                DEBUG << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                                         " was exceeding limits of storage tree \"" +
                                         combined_path->string() +
                                         "\" and was skipped!.";
                            } else {
                                children_read.unlock();
                                size_read.unlock();
                            }
                            //parallel end
                            active_threads -= 2;
                        };
                        if (num_threads == 1)first_index_exe_function();
                        else {
                            //threading manager
                            while (active_threads.load() >= num_threads) {
                                if (error_flag.test()) {
                                    FATAL
                                        << "Delete_blocks threading engine crashed unexpectedly while waiting for CPU cores!";
                                    return {};
                                }
#ifdef _WIN32
                                Sleep(10);
#else
                                usleep(10 * 1000);
#endif // _WIN32
                            }
                            active_threads += 2;
                            std::thread(first_index_exe_function).detach();
                            if (error_flag.test()) {
                                FATAL << "Delete_blocks threading engine crashed unexpectedly!";
                                return {};
                            }
                        }
                        std::scoped_lock lock2(m1);
                        beg = end;
                        current = (*end)[0];
                    }
                }
                cur = end;
            }
            std::scoped_lock out_return_lock(out_change_list_protect);
            return {out_size.load(), out_change_list};
        }

        /*
         * the deconstructor should also let all writing processes and reading processes end before it deletes the tree node
         * After the current writing and reading process is finished, all threads that come later will get a Segfault
         * for their operation since the struct will disassemble, but no dataloss can occur
         */
        ~tree_storage() {
            std::unique_lock lock_size(size_protect, std::defer_lock);
            for(auto &s:*size){
                lock_size.lock();
                //stop all reading and writing operations on this tree node and reserve all rights before destroying itself
                auto write_ptr = &(*std::get<2>(s));
                auto read_ptr = &(*std::get<3>(s));
                auto maintain_ptr = &(*std::get<4>(s));
                lock_size.unlock();

                while (std::atomic_flag_test_and_set_explicit(maintain_ptr, std::memory_order_acquire)) {
                    maintain_ptr->wait(true);
                }
                while (std::atomic_flag_test_and_set_explicit(write_ptr, std::memory_order_acquire)) {
                    write_ptr->wait(true);
                }

                while (read_ptr->load() > 0) {
#ifdef _WIN32
                    Sleep(10);
#else
                    usleep(10 * 1000);
#endif // _WIN32
                }
            }
            std::unique_lock child_write(children_protect);
            for (auto &i: *children) {
                child_write.unlock();

                if (std::get<1>(i) != nullptr) {
                    delete std::get<1>(i);
                }
                child_write.lock();
            }
            child_write.unlock();
        }
    };
}

#endif //UHLIBCOMMON_TREE_STORAGE_H
