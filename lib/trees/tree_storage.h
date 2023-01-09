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
        static std::vector<unsigned char> prefix_wrap(std::size_t input_size);

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
                              unsigned short num_threads = std::thread::hardware_concurrency());

        /*
         * get the size of the entire tree, so the size of all contained information in total, the tree always maintains total size
         * call get_info() to get the real block size and the total size as well as the block time stamp in comparison
         */
        std::size_t get_size();

        //write a binary block to a certain creation date and get a local block reference in return
        std::vector<unsigned char> write(const std::vector<unsigned char> &input,
                                         unsigned long current_time = (unsigned long) std::chrono::nanoseconds(
                                                 std::chrono::high_resolution_clock::now().time_since_epoch()).count());

        /*
         * returns a tuple with block time and binary vector of block when entering a block reference (as far as it exists)
         * WARNING: not existing blocks will still crash the procedure, a radix tree for block existence is needed
         */
        std::tuple<unsigned long, std::vector<unsigned char>> read(const std::vector<unsigned char> &block_code);

        /*
         * The index reads the entire information of the tree storage and calculates SHA512 hashes for every single block
         * matching a local storage reference
         * By running index on startup of a DB node it can scans information about the existence and validity of all blocks
         * returns hash, local block reference and the block last visited time
         */
        std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, unsigned long>>
        index(unsigned short num_threads = std::thread::hardware_concurrency());

        /*
         * deletes all storage recursively within its root file and unregisters it on RAM
         * In case of failure the node can clean itself off and reload blocks from different locations where the now
         * missing blocks still persist
         */
        std::size_t delete_recursive(unsigned short num_threads = std::thread::hardware_concurrency());

        //returns a tuple with block time and block size from disk and total size on disk on request of local block reference
        std::tuple<unsigned long, std::size_t, std::size_t> get_info(const std::vector<unsigned char> &block_code);

        /*
         * returns success on changing touch time of a block, giving the function a valid block code
         * WARNING: Wrong block references will cause data loss!!!
         */
        bool set_block_time(const std::vector<unsigned char> &block_code,
                            unsigned long current_time = (unsigned long) std::chrono::nanoseconds(
                                    std::chrono::high_resolution_clock::now().time_since_epoch()).count());

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
