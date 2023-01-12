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
#include "sys/types.h"
#include "sys/sysinfo.h"

#ifdef _WIN32
#include <windows.h>
#else

#include <unistd.h>

#endif // _WIN32

namespace uh::trees {
    DEFINE_EXCEPTION(out_of_memory);

#define N (unsigned int) (std::numeric_limits<unsigned char>::max() + 1)
#define CHAR_BITS 8
#define ONE_MILLISECOND 1000
#define TEN_MS 10
#define SMALLEST_LOCAL_BLOCK_SIZE 5
#define TIME_STAMPS_ON_BLOCK 3
#define BUF_LEN_SIZE_FOR_SIZE_BLOCK 1

    typedef struct tree_storage tree_storage;

    /*
     * module description: Tree storage is fast tree based interface to write binary information to disk at a given root folder, the tree storage root
     * The module is a thread safe and multithreading optimized approach to manage data on the later system. The storage tree
     * automatically balances load among the used B+-Tree file tree to share load of all hard drives of the RAID controller in an optimal fashion
     * While maintaining the tree storage, deleting blocks with an outdated timestamp, we should prevent the to be deleted blocks from being read (only not thread safe aspect)
     */

    struct tree_storage {

        //returns wrapped string
        static std::vector<unsigned char> prefix_wrap(std::size_t input_size);

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

    private:
        template<typename ALLOC>
        std::vector<ALLOC> mem_wait(std::size_t mem) {
            //long pages = sysconf(_SC_PHYS_PAGES);
            //long page_size = sysconf(_SC_PAGE_SIZE);
            std::size_t count{};
            //auto totalMem = static_cast<unsigned long>(pages * page_size);
            std::size_t freeMem;

            do {
                std::unique_lock lock_mem(memory_protect, std::defer_lock);
                lock_mem.lock();
                struct sysinfo info{};
                sysinfo(&info);
                freeMem = info.freeram;

                if (freeMem >= mem * sizeof(ALLOC)) {
                    auto out_vec = std::vector<ALLOC>();
                    out_vec.reserve(mem);
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
            out_vec.reserve(mem);
            return out_vec;
        }

    public:
        //returns total size, block_size, (optional valid) SHA512 with an ending of creation time as hash extend reordered vector, error occurred
        //for efficiency SHA512 hash calculation can be skipped and an external hash can be written
        //for updating efficiency writing prefix and block again can be skipped
        //writes times,SHA512,prefix,block,checksum
        std::tuple<std::size_t, std::size_t, std::array<unsigned char,
                SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, bool> write_block_base(
                FILE *writer, const std::filesystem::path &write_at, const std::vector<unsigned char> &block,
                const std::vector<unsigned char> &local_block_ref,
                std::array<unsigned long, TIME_STAMPS_ON_BLOCK> times,
                bool update_times = false,
                bool calc_SHA512 = true,
                std::array<unsigned char, SHA512_DIGEST_LENGTH> SHA_input_optional = std::array<unsigned char, SHA512_DIGEST_LENGTH>{},
                std::size_t placeholder_block_size = 0);

        //returns total size of block plus information, the received block as vector, the total block with information as vector, the times in normal unsigned long form,
        //the global hash of SHA512 with creation time extend, bool error occurred, bool block description valid, block size
        std::tuple<std::size_t, std::size_t, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char, SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, bool, bool>
        read_block_base(
                FILE *reader, const std::filesystem::path &read_at, const std::vector<unsigned char> &local_block_ref,
                bool skip_read_block = false, bool check_valid = false);

        /*
         * tell a root folder, if another root is detected it is loaded into a loose substructure that is not indexed yet
         * the module will not know the blocks within the storage chunks, reading will fail
         * call the index function after crating the tree_storage to proceed
         */
        explicit tree_storage(const std::filesystem::path &root,
                              unsigned short num_threads = std::thread::hardware_concurrency());

        /*
         * get the size of the entire tree, so the size of all contained information in total, the tree always maintains total size
         * call get_info() to get the real block size and the total size as well as the block time stamp in comparison
         */
        std::size_t get_size();

        //write a binary block to a certain creation date and get a local block reference in return
        std::tuple<std::array<unsigned char,
                SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::vector<unsigned char>, std::size_t, std::size_t>
        write(const std::vector<unsigned char> &input,
              std::array<unsigned long, TIME_STAMPS_ON_BLOCK> times);

        /*
         * returns a tuple with binary block, local binary block reference, block times (create,valid duration,last touch), global hash
         * if "only_info" flag is set, it will not return the block, only all the other information as a "get_info" function,
         * pushing up all results left by 1
         */
        //returns block,local block reference, timestamps, global hash reference,block size, block is valid on valid_check
        template<const bool only_info = false>
        auto read(const std::vector<unsigned char> &block_code, bool valid_check = false) {
            if (block_code.empty()) {
                ERROR << "No input given to read!";
                if constexpr (only_info) {
                    return std::tuple<std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                            SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::size_t, bool>{};
                } else {
                    return std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                            SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::size_t, bool>{};
                }
            }
            if (block_code.size() > SMALLEST_LOCAL_BLOCK_SIZE) {
                //size encoding is not reached yet, read along tree path
                std::unique_lock read_children(children_protect, std::defer_lock);
                read_children.lock();
                if ((short) children->size() - 1 < (short) block_code[0] ||
                    !std::get<0>(children->at(block_code[0]))) {
                    read_children.unlock();
                    std::string const not_found((const char *) block_code.data(), block_code.size());
                    std::scoped_lock const read_path(combined_path_protect);
                    DEBUG << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " could not be found in storage tree \"" +
                             combined_path->string() + "\".";
                    if constexpr (only_info) {
                        return std::tuple<std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                                SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::size_t, bool>{};
                    } else {
                        return std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                                SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::size_t, bool>{};
                    }
                } else {
                    auto tree_ptr3 = std::get<1>(children->at(block_code[0]));
                    read_children.unlock();
                    std::vector<unsigned char> const sub_block_code{block_code.cbegin() + 1, block_code.cend()};
                    auto out_vec = tree_ptr3->read<only_info>(sub_block_code, valid_check);
                    if (std::get<1 - only_info>(out_vec).empty()) {
                        std::string const not_found((const char *) block_code.data(), block_code.size());
                        std::scoped_lock const read_path(combined_path_protect);
                        DEBUG << "<Block error trace on return>: Block code " + boost::algorithm::hex(not_found) +
                                 " could not be found in storage tree \"" +
                                 combined_path->string() + "\".";
                    }
                    return out_vec;
                }
            } else {
                if (block_code.size() < SMALLEST_LOCAL_BLOCK_SIZE) {
                    std::string const not_found((const char *) block_code.data(), block_code.size());
                    std::scoped_lock const read_path(combined_path_protect);
                    FATAL << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                             " was too short for storage tree \"" +
                             combined_path->string() + "\".";
                    if constexpr (only_info) {
                        return std::tuple<std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                                SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::size_t, bool>{};
                    } else {
                        return std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                                SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::size_t, bool>{};
                    }
                }
                //the block code should have a size of 5; one chunk index and 4 bytes of encoding for the offset
                std::unique_lock size_lock(size_protect, std::defer_lock);
                size_lock.lock();
                if ((short) size->size() - 1 < (short) block_code[0] ||
                    !std::get<0>(size->at(block_code[0]))) {
                    size_lock.unlock();
                    std::string const not_found((const char *) block_code.data(), block_code.size());
                    std::scoped_lock const read_path(combined_path_protect);
                    DEBUG
                        << "<Block error trace on return, final tree>: Block code " + boost::algorithm::hex(not_found) +
                           " could not be found in storage tree \"" + combined_path->string() +
                           "\" and size of storage chunk was 0.";
                    if constexpr (only_info) {
                        return std::tuple<std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                                SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::size_t, bool>{};
                    } else {
                        return std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                                SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::size_t, bool>{};
                    }
                } else {
                    size_lock.unlock();
                    std::vector<unsigned char> sub_block_code{block_code.cbegin() + 1,
                                                              block_code.cend()}; //copy offset code and rebuild
                    std::size_t offset{};
                    for (unsigned short i = 0; i < (unsigned short) sizeof(unsigned int); i++) {
                        offset += (((std::size_t) sub_block_code[i]) << (i * CHAR_BITS));
                    }
                    std::string const ref_name{boost::algorithm::hex(std::string{(char) block_code[0]})};
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
                    auto read_end_sequence = [&reader, &read_ptr, &write_ptr, &read_path]() {
                        if (std::fclose(reader)) {
                            ERROR << "Read stream was not open on " + read_path.make_preferred().string() + "!";
                        }
                        *read_ptr -= 1;
                        if (!read_ptr->load())write_ptr->notify_one();
                    };
                    //File should have been opened or created here
                    if (std::fseek(reader, static_cast<long>(offset), SEEK_SET)) {
                        ERROR << "File seek failed at \"" + read_path.make_preferred().string() + ", position " +
                                 std::to_string(offset) + "\"";
                        read_end_sequence();
                        if constexpr (only_info) {
                            return std::tuple<std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                                    SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::size_t, bool>{};
                        } else {
                            return std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                                    SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::size_t, bool>{};
                        }
                    }

                    const auto block_read_tup = this->read_block_base(reader, read_path.make_preferred(), block_code, only_info, valid_check);
                    read_end_sequence();

                    if (std::get<5>(block_read_tup)) {
                        ERROR << "File error at \"" + read_path.make_preferred().string() + ", position " +
                                 std::to_string(offset) + "\"";
                        if constexpr (only_info) {
                            return std::tuple<std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                                    SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::size_t, bool>{};
                        } else {
                            return std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                                    SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::size_t, bool>{};
                        }
                    }

                    if (!std::get<6>(block_read_tup)) {
                        ERROR << "File block is broken at \"" + read_path.make_preferred().string() + ", position " +
                                 std::to_string(offset) +
                                 "\"";
                        if constexpr (only_info) {
                            return std::tuple<std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                                    SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::size_t, bool>{};
                        } else {
                            return std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                                    SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::size_t, bool>{};
                        }
                    }
                    //std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
                    //        SHA512_DIGEST_LENGTH + sizeof(unsigned long)>>
                    if constexpr (only_info) {
                        return std::make_tuple(block_code, std::get<3>(block_read_tup), std::get<4>(block_read_tup),
                                               std::get<1>(block_read_tup), std::get<6>(block_read_tup));
                    } else {
                        return std::make_tuple(std::get<2>(block_read_tup), block_code, std::get<3>(block_read_tup),
                                               std::get<4>(block_read_tup), std::get<1>(block_read_tup),
                                               std::get<6>(block_read_tup));
                    }
                }
            }
        }

        /*
         * The index reads the entire information of the tree storage and calculates SHA512 hashes with creation time extension for every single block
         * matching a local storage reference and delivering the time stamps of the block
         * The first list carries valid blocks, the second one carries corrupted blocks that need to be deleted or repaired
         * in each list: returns global_hash, local_block_reference and block timestamps
         */
        std::tuple<std::list<std::tuple<std::array<unsigned char, SHA512_DIGEST_LENGTH +
                                                                  sizeof(unsigned long)>, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>>>,
                std::list<std::tuple<std::array<unsigned char, SHA512_DIGEST_LENGTH +
                                                               sizeof(unsigned long)>, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>>>>
        index(unsigned short num_threads = std::thread::hardware_concurrency());

        /*
         * deletes all storage recursively within its root file and unregisters it on RAM
         * In case of failure the node can clean itself off and reload blocks from different locations where the now
         * missing blocks still persist
         */
        std::size_t delete_recursive(unsigned short num_threads = std::thread::hardware_concurrency());

        /*
         * returns success on changing storage duration and touch time (NOT creation time)
         * WARNING: Wrong block references will cause data loss!!!
         */
        bool set_block_time(const std::vector<unsigned char> &local_block_reference,
                            std::array<unsigned long, TIME_STAMPS_ON_BLOCK - 1> times);

        //TODO: maintain valid time, introduce block max age
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
                unsigned short num_threads = std::thread::hardware_concurrency());

        /*
         * the deconstructor should also let all writing processes and reading processes end before it deletes the tree node
         * After the current writing and reading process is finished, all threads that come later will get a Segfault
         * for their operation since the struct will disassemble, but no dataloss can occur
         */
        ~tree_storage();
    };


}

#endif //UHLIBCOMMON_TREE_STORAGE_H
