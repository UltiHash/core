//
// Created by benjamin-elias on 09.01.23.
//
#include "tree_storage.h"

std::vector<unsigned char> uh::trees::tree_storage::prefix_wrap(std::size_t input_size) {
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

std::size_t uh::trees::tree_storage::get_size() {
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

std::vector<unsigned char> uh::trees::tree_storage::write(const std::vector<unsigned char> &input,
                                 unsigned long current_time = (unsigned long) std::chrono::nanoseconds(
                                         std::chrono::high_resolution_clock::now().time_since_epoch()).count()) {
    if (input.empty()){
        ERROR << "No input given to write!";
        return std::vector<unsigned char>{};
    }
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

std::tuple<unsigned long, std::vector<unsigned char>> uh::trees::tree_storage::read(const std::vector<unsigned char> &block_code) {
    if (block_code.empty()) {
        ERROR << "No input given to read!";
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

std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>, unsigned long>>
uh::trees::tree_storage::index(unsigned short num_threads = std::thread::hardware_concurrency()) {
    if (num_threads == 0) {
        ERROR << "Not enough threading resources!";
        return {};
    }
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
                std::size_t total_file_size = std::filesystem::exists(read_path.make_preferred())?std::filesystem::file_size(read_path.make_preferred()):0;
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
                auto append_list = tree_ptr->index((num_threads % 2)?1:2);
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
        for (unsigned short i = 0; (unsigned short) i < num_threads / ((num_threads % 2)?1:2); i++) {
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

std::tuple<unsigned long, std::size_t, std::size_t> uh::trees::tree_storage::get_info(const std::vector<unsigned char> &block_code) {
    if (block_code.empty()) {
        ERROR << "No input given to get info of block!";
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

bool uh::trees::tree_storage::set_block_time(const std::vector<unsigned char> &block_code,
                    unsigned long current_time = (unsigned long) std::chrono::nanoseconds(
                            std::chrono::high_resolution_clock::now().time_since_epoch()).count()) {
    if (block_code.empty()) {
        ERROR << "No input given to set block time!";
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

uh::trees::tree_storage::~tree_storage() {
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
