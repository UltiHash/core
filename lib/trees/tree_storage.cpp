//
// Created by benjamin-elias on 09.01.23.
//
#include "tree_storage.h"

uh::trees::tree_storage::tree_storage(const std::filesystem::path &root, unsigned short num_threads) {
    if (!num_threads || root.empty()){
        ERROR << "No root as input or there was a lack of threading resources!";
        return;
    }
    std::atomic<long> i_constructor{-1};

    std::unique_lock lock5(combined_path_protect, std::defer_lock);
    lock5.lock();
    if (combined_path->empty()) {
        std::string const parent_name = root.filename().string();
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
        std::scoped_lock const filesystem_lock(std_filesystem_protect);
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
            std::filesystem::path const chunk = *combined_path / s_tmp;
            lock4.unlock();

            std::unique_lock filesystem_lock(std_filesystem_protect, std::defer_lock);
            filesystem_lock.lock();
            if (std::filesystem::exists(chunk)) {
                filesystem_lock.unlock();
                std::shared_ptr<std::atomic_flag> f1 = std::make_shared<std::atomic_flag>(), f3 = std::make_shared<std::atomic_flag>();
                std::shared_ptr<std::atomic<std::size_t>> f2 = std::make_shared<std::atomic<std::size_t>>();
                std::scoped_lock const lock1(size_protect);
                size->emplace_back(std::filesystem::file_size(chunk), i, f1, f2, f3);
            } else filesystem_lock.unlock();

            lock4.lock();
            std::string const fname = combined_path->filename().string();
            lock4.unlock();
            s_tmp.insert(s_tmp.cbegin(), fname.cbegin() + 2, fname.cbegin() + 4);
            lock4.lock();
            std::filesystem::path const deeper_tree = *combined_path / s_tmp;
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
                    std::scoped_lock const lock2(children_protect);
                    children->emplace_back(tmp_tree->get_size(), tmp_tree, i);
                }
            } else filesystem_lock.unlock();
            std::scoped_lock const step_lock(work_steal_protect);
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
            if(th.joinable())th.join();
    }
    std::unique_lock lock_size(size_protect, std::defer_lock);
    lock_size.lock();
    //fill holes of size
    if(!size->empty()){
        auto size_max = std::max_element(size->begin(),size->end(),[](auto &item1,auto& item2){
            return std::get<1>(item1)<std::get<1>(item2);
        });
        unsigned char const max_i = std::get<1>(*size_max);
        for(unsigned short i6=0; i6<max_i;i6++){
            if(!std::any_of(std::execution::par, size->begin(),size->end(),[&i6](auto &item){
                return std::get<1>(item) == i6;
            })){
                std::shared_ptr<std::atomic_flag> f1 = std::make_shared<std::atomic_flag>(), f3 = std::make_shared<std::atomic_flag>();
                std::shared_ptr<std::atomic<std::size_t>> f2 = std::make_shared<std::atomic<std::size_t>>();
                size->emplace_back(0, i6, f1, f2, f3);
            }
        }
    }
    if (!std::is_sorted(std::execution::par_unseq, size->begin(), size->end(),
                        [](const auto &a, const auto &b) {
                            return std::get<1>(a) < std::get<1>(b);
                        }))
        std::sort(std::execution::par, size->begin(), size->end(),
                  [](const auto &a, const auto &b) {
                      return std::get<1>(a) < std::get<1>(b);
                  });
    lock_size.unlock();

    std::scoped_lock const lock_children(children_protect);
    //fill holes of children
    if(!children->empty()){
        auto children_max = std::max_element(children->begin(),children->end(),[](auto &item1,auto& item2){
            return std::get<2>(item1)<std::get<2>(item2);
        });
        unsigned char const max_i = std::get<2>(*children_max);
        for(unsigned short i6=0; i6<max_i;i6++){
            if(!std::any_of(std::execution::par, children->begin(),children->end(),[&i6](auto &item){
                return std::get<2>(item) == i6;
            })){
                std::string ref_name{boost::algorithm::hex(std::string{(char) i6})};
                std::shared_lock lock_path1(combined_path_protect);
                std::string const fname = combined_path->filename().string();
                lock_path1.unlock();
                ref_name.insert(ref_name.cbegin(), fname.cbegin() + 2, fname.cbegin() + 4);
                lock_path1.lock();
                std::filesystem::path const deeper_tree = *combined_path / ref_name;
                lock_path1.unlock();
                auto *tmp_tree = new tree_storage(deeper_tree);
                children->emplace_back(0, tmp_tree, i6);
            }
        }
    }
    if (!std::is_sorted(std::execution::par_unseq, children->begin(), children->end(),
                        [](const auto &a, const auto &b) {
                            return std::get<2>(a) < std::get<2>(b);
                        }))
        std::sort(std::execution::par, children->begin(), children->end(),
                  [](const auto &a, const auto &b) {
                      return std::get<2>(a) < std::get<2>(b);
                  });
}

std::vector<unsigned char> uh::trees::tree_storage::prefix_wrap(std::size_t input_size) {
    std::size_t h_bit{input_size};
    unsigned char total_bit_count{};
    //counting max bits
    while (h_bit != 0)[[likely]] {
        total_bit_count++;
        h_bit >>= 1;
    }
    unsigned char const byte_count = total_bit_count / CHAR_BITS + std::min(1, total_bit_count % CHAR_BITS);
    std::vector<unsigned char> prefix{};
    for (unsigned char i = 0; i < byte_count; i++) {
        prefix.push_back((unsigned char) (input_size >> (i * CHAR_BITS)));
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
                                 unsigned long current_time) {
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
    std::size_t const total_size = input.size() + prefix.size() + sizeof(unsigned long);
    //check block fill of this node, look for free space

    std::unique_lock lock_size(size_protect, std::defer_lock);
    lock_size.lock();
    unsigned short min_pos = 0;
    std::size_t min_val = 0;
    if (size->size() < N) {
        min_pos = size->size();
        lock_size.unlock();
        min_val = 0;
        std::shared_ptr<std::atomic_flag> f1 = std::make_shared<std::atomic_flag>(), f3 = std::make_shared<std::atomic_flag>();
        std::shared_ptr<std::atomic<std::size_t>> const f2 = std::make_shared<std::atomic<std::size_t>>();
        std::scoped_lock const size_own1(size_protect);
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
    bool const no_deeper = min_val < STORE_MAX && min_val + total_size < STORE_HARD_LIMIT &&
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
            std::string const fname = combined_path->filename().string();
            lock_path1.unlock();
            ref_name.insert(ref_name.cbegin(), fname.cbegin() + 2, fname.cbegin() + 4);
            lock_path1.lock();
            std::filesystem::path const deeper_tree = *combined_path / ref_name;
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
        std::string const ref_name{boost::algorithm::hex(std::string{(char) min_pos})};
        std::shared_lock path_lock2(combined_path_protect);
        std::filesystem::path read_chunk = *combined_path / ref_name;
        path_lock2.unlock();

        //calculate binary of timestamp
        std::vector<unsigned char> bin_time;
        for (unsigned short i = 0; i < (unsigned short) sizeof(current_time); i++) {
            bin_time.push_back((unsigned char) (current_time >> (i * CHAR_BITS)));
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
            Sleep(TEN_MS);
#else
            usleep(TEN_MS * ONE_MILLISECOND);
#endif // _WIN32
        }
        lock_size.lock();
        std::size_t const size_tmp = std::get<0>(size->at(min_pos));
        lock_size.unlock();
        std::unique_lock write_size(size_protect, std::defer_lock);
        write_size.lock();
        std::get<0>(size->at(min_pos)) += total_size;
        write_size.unlock();

        FILE *writer = std::fopen(read_chunk.make_preferred().c_str(), "ab");
        auto end_write_sequence = [&]() {
            if(std::fclose(writer))ERROR << "Write stream was not open!";

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
        if (!std::fwrite(bin_time.data(), bin_time.size(), sizeof(unsigned char), writer)) {
            ERROR << "File write binary time opening failed at \"" + read_chunk.string() + "\"";
            end_write_sequence();
            return std::vector<unsigned char>{};
        }
        if (!std::fwrite(prefix.data(), prefix.size(), sizeof(unsigned char), writer)) {
            ERROR << "File write prefix data opening failed at \"" + read_chunk.string() + "\"";
            end_write_sequence();
            return std::vector<unsigned char>{};
        }
        if (!std::fwrite(input.data(), input.size(), sizeof(unsigned char), writer)) {
            ERROR << "File write input data opening failed at \"" + read_chunk.string() + "\"";
            end_write_sequence();
            return std::vector<unsigned char>{};
        }

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
            local_block_ref.push_back((unsigned char) (size_tmp >> (i * CHAR_BITS)));
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
            return {0, std::vector<unsigned char>{}};
        } else {
            auto tree_ptr3 = std::get<1>(children->at(block_code[0]));
            read_children.unlock();
            std::vector<unsigned char> const sub_block_code{block_code.cbegin() + 1, block_code.cend()};
            auto out_vec = tree_ptr3->read(sub_block_code);
            if (std::get<1>(out_vec).empty()) {
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
            return {0, std::vector<unsigned char>{}};
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
            return {0, std::vector<unsigned char>{}};
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
            auto read_end_sequence = [&]() {
                if(std::fclose(reader))ERROR << "Read stream was not open on "+read_path.string()+"!";
                *read_ptr -= 1;
                if (!read_ptr->load())write_ptr->notify_one();
            };
            if (!reader) {
                ERROR << "File read opening failed at \"" + read_path.string() + "\"";
                read_end_sequence();
                return {0, std::vector<unsigned char>{}};
            }
            //File should have been opened or created here
            if (std::fseek(reader, static_cast<long>(offset), SEEK_SET)) {
                ERROR << "File seek failed at \"" + read_path.string() + ", position "+std::to_string(offset)+"\"";
                read_end_sequence();
                return {0, std::vector<unsigned char>{}};
            }

            if (std::ferror(reader)) {
                FATAL << "I/O error when seeking \"" + read_path.string() + "\"";
                read_end_sequence();
                return {0, std::vector<unsigned char>{}};
            }

            std::array<unsigned char, sizeof(unsigned long)> time_buf;
            std::size_t total_read{};
            std::size_t count = std::fread(time_buf.data(), sizeof(unsigned char), sizeof(unsigned long), reader);
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
                block_time += (((std::size_t) time_buf[i]) << (i * CHAR_BITS));
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
            std::vector<unsigned char> buffer_in;
            buffer_in.resize(buf_size + 1, 0);
            count = std::fread(buffer_in.data(), sizeof(unsigned char), buf_size + 1, reader);
            total_read += count;
            if (count != buf_size + 1) {
                FATAL
                    << "I/O prefix first byte reading was not completed on path \"" + read_path.string() + "\"";
                read_end_sequence();
                return {0, std::vector<unsigned char>{}};
            }
            std::size_t output_size{};
            for (unsigned char buf_count = 0; buf_count <= buf_size; buf_count++) {
                output_size += (((std::size_t) buffer_in[buf_count]) << (buf_count * CHAR_BITS));
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
uh::trees::tree_storage::index(unsigned short num_threads) {
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
                std::string const ref_name{boost::algorithm::hex(std::string{(char) i})};
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
                std::size_t const total_file_size = std::filesystem::exists(read_path.make_preferred())?std::filesystem::file_size(read_path.make_preferred()):0;
                filesystem_lock.unlock();
                auto read_end_sequence = [&]() {
                    if(std::fclose(reader))ERROR << "Read stream was not open!";
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

                std::array<std::vector<unsigned char>,2> local_block_ref;
                std::atomic<unsigned long> block_time_current{};
                std::array<std::vector<unsigned char>,2> tmp_buf;

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
                            local_block_ref[parallel_switch_cpy].push_back((unsigned char) (cur_pos.load() >> (i1 * CHAR_BITS)));
                        }
                        local_block_ref[parallel_switch_cpy].insert(local_block_ref[parallel_switch_cpy].cbegin(), i);

                        std::array<unsigned char, sizeof(unsigned long)> time_buf;
                        std::size_t count = std::fread(time_buf.data(), sizeof(unsigned char), sizeof(unsigned long),
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
                            block_time += (((std::size_t) time_buf[i4]) << (i4 * CHAR_BITS));
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
                        std::vector<unsigned char> buffer_for_size;
                        buffer_for_size.resize(buf_size + 1, 0);
                        count = std::fread(buffer_for_size.data(), sizeof(unsigned char), buf_size + 1, reader);
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
                            output_size += (((std::size_t) buffer_for_size[buf_count]) << (buf_count * CHAR_BITS));
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
                        std::array<unsigned char, SHA512_DIGEST_LENGTH> hash_buf;//HASH GENERATION
                        auto block_time_cpy = block_time_current.load();
                        auto parallel_switch_cpy = parallel_switch.load();
                        global_thread_lock.unlock();
                        SHA512(tmp_buf[!parallel_switch_cpy].data(), tmp_buf[!parallel_switch_cpy].size(), hash_buf.data());

                        std::vector<unsigned char> hash{hash_buf.cbegin(), hash_buf.cend()};
                        std::scoped_lock const lock_emplace(search_index_protect);
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
                std::scoped_lock const lock_splice(search_index_protect);
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
            std::scoped_lock const step_lock(work_steal_protect);
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
        for (auto &th: workers)
            if(th.joinable())th.join();

    }
    if (error_flag.test()) {
        FATAL << "Indexing threading engine crashed unexpectedly!";
        return {};
    }

    std::scoped_lock const lock_return(search_index_protect);
    return *search_index;
}

std::size_t uh::trees::tree_storage::delete_recursive(unsigned short num_threads) {
    if (!num_threads){
        ERROR << "No input given to write!";
        return {};
    }
    std::atomic<long> i_constructor{-1};
    std::atomic<std::size_t> out_size{};
    std::atomic_flag error_flag{ATOMIC_FLAG_INIT};


    auto multithread_index = [&]() {
        std::unique_lock step_lock_init(work_steal_protect);
        std::size_t i = (i_constructor += 1);
        step_lock_init.unlock();

        if (i >= N)return;
        for (; i < N;) {
            std::unique_lock size_lock(size_protect, std::defer_lock);
            size_lock.lock();
            if (i < size->size() && std::get<0>(size->at(i)) > 0) {
                std::string const ref_name{boost::algorithm::hex(std::string{(char) i})};
                std::shared_lock lock_path(combined_path_protect);
                std::filesystem::path const read_path = *combined_path / ref_name;
                lock_path.unlock();
                std::unique_lock filesystem_lock(std_filesystem_protect, std::defer_lock);
                filesystem_lock.lock();
                std::size_t const vanish_size = std::filesystem::exists(read_path) ? std::filesystem::file_size(
                        read_path) : 0;
                filesystem_lock.unlock();
                out_size += vanish_size;
                std::get<0>(size->at(i)) -= vanish_size;
                size_lock.unlock();
                filesystem_lock.lock();
                if(std::filesystem::exists(read_path)){
                    if (!std::filesystem::remove(read_path)) {
                        filesystem_lock.unlock();
                        FATAL << "Removing was not completed on path \"" + read_path.string() + "\". Check inconsistent maintain files!";
                        while (std::atomic_flag_test_and_set_explicit(&error_flag, std::memory_order_acquire)) {
                            return;
                        }
                        if (error_flag.test())return;
                    } else filesystem_lock.unlock();
                }
            } else {
                size_lock.unlock();
            }
            //splice indexes of children plus the min_pos to decide local_block_ref of children array
            std::unique_lock children_lock(children_protect, std::defer_lock);
            children_lock.lock();
            if (i < children->size() && std::get<0>(children->at(i)) > 0) {
                auto tree_ptr = std::get<1>(children->at(i));
                children_lock.unlock();
                std::size_t const vanish_size = tree_ptr->delete_recursive(1);
                std::string ref_name{boost::algorithm::hex(std::string{(char) i})};
                std::shared_lock path_protect(combined_path_protect,std::defer_lock);
                path_protect.lock();
                std::string const fname = combined_path->filename().string();
                ref_name.insert(ref_name.cbegin(), fname.cbegin() + 2, fname.cbegin() + 4);
                std::filesystem::path const read_path = *combined_path / ref_name;
                path_protect.unlock();
                std::unique_lock filesystem_lock(std_filesystem_protect, std::defer_lock);
                filesystem_lock.lock();
                if(std::filesystem::exists(read_path)){
                    if (!std::filesystem::remove(read_path)) {
                        filesystem_lock.unlock();
                        FATAL << "Removing was not completed on path \"" + read_path.string() + "\". Check inconsistent maintain files!";
                        while (std::atomic_flag_test_and_set_explicit(&error_flag, std::memory_order_acquire)) {
                            return;
                        }
                        if (error_flag.test())return;
                    } else filesystem_lock.unlock();
                }
                out_size += vanish_size;
                std::scoped_lock const lg(size_protect);
                std::get<0>(size->at(i)) -= vanish_size;
            } else {
                children_lock.unlock();
            }

            std::scoped_lock const step_lock(work_steal_protect);
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
            if(th.joinable())th.join();
    }
    if (error_flag.test()) {
        FATAL << "Delete_recursive threading engine crashed unexpectedly!";
        return {};
    }
    return out_size.load();
}

std::tuple<unsigned long, std::size_t, std::size_t> uh::trees::tree_storage::get_info(const std::vector<unsigned char> &block_code) {
    if (block_code.empty()) {
        ERROR << "No input given to get info of block!";
        return {};
    }
    if (block_code.size() > SMALLEST_LOCAL_BLOCK_SIZE) {
        //size encoding is not reached yet, get_info along tree path
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
            return {};
        } else {
            auto tree_ptr = std::get<1>(children->at(block_code[0]));
            read_children.unlock();
            std::vector<unsigned char> const sub_block_code{block_code.cbegin() + 1, block_code.cend()};
            auto out_vec = tree_ptr->get_info(sub_block_code);
            if (std::get<1>(out_vec) == 0) {
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
            return {};
        }
        //the block code should have a size of 5; one chunk index and 4 bytes of encoding for the offset
        std::unique_lock read_size(size_protect, std::defer_lock);
        read_size.lock();
        if ((short) size->size() - 1 < (short) block_code[0] ||
            !std::get<0>(size->at(block_code[0]))) {
            read_size.unlock();
            std::string const not_found((const char *) block_code.data(), block_code.size());
            std::scoped_lock const read_path(combined_path_protect);
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
                offset += (((std::size_t) sub_block_code[i]) << (i * CHAR_BITS));
            }
            std::string const ref_name{boost::algorithm::hex(std::string{(char) block_code[0]})};
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
                if(std::fclose(reader))ERROR << "Read stream was not open!";
                *read_ptr -= 1;
                if (!read_ptr->load())write_ptr->notify_one();
            };
            if (!reader) {
                ERROR << "File get_info opening failed at \"" + read_path.string() + "\"";
                read_end_sequence();
                return {};
            }
            //File should have been opened or created here
            if (std::fseek(reader, static_cast<long>(offset), SEEK_SET)) {
                ERROR << "File seek failed at \"" + read_path.string() + ", position "+std::to_string(offset)+"\"";
                read_end_sequence();
                return {};
            }

            if (std::ferror(reader)) {
                FATAL << "I/O error when seeking \"" + read_path.string() + "\"";
                read_end_sequence();
                return {};
            }

            std::array<unsigned char, sizeof(unsigned long)> time_buf;
            std::size_t count = std::fread(time_buf.data(), sizeof(unsigned char), sizeof(unsigned long), reader);
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
                block_time += (((std::size_t) time_buf[i]) << (i * CHAR_BITS));
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
            std::vector<unsigned char> buffer_in;
            buffer_in.resize(buf_size+1,0);
            count = std::fread(buffer_in.data(), sizeof(unsigned char), buf_size + 1, reader);
            if (count != buf_size + 1) {
                FATAL
                    << "I/O prefix first byte reading was not completed on path \"" + read_path.string() + "\"";
                read_end_sequence();
                return {};
            }
            std::size_t output_size{};
            for (unsigned char buf_count = 0; buf_count <= buf_size; buf_count++) {
                output_size += (((std::size_t) buffer_in[buf_count]) << (buf_count * CHAR_BITS));
            }

            read_end_sequence();

            return {block_time, output_size, time_buf.size()+ 1 + buffer_in.size() + output_size};
        }
    }
}

bool uh::trees::tree_storage::set_block_time(const std::vector<unsigned char> &block_code,
                    unsigned long current_time) {
    if (block_code.empty()) {
        ERROR << "No input given to set block time!";
        return false;
    }
    if (block_code.size() > SMALLEST_LOCAL_BLOCK_SIZE) {
        //size encoding is not reached yet, set_block_time along tree path
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
            return false;
        } else {
            auto tree_ptr = std::get<1>(children->at(block_code[0]));
            read_children.unlock();
            std::vector<unsigned char> const sub_block_code{block_code.cbegin() + 1, block_code.cend()};
            auto out_vec = tree_ptr->set_block_time(sub_block_code, current_time);
            if (!out_vec) {
                std::string const not_found((const char *) block_code.data(), block_code.size());
                std::scoped_lock const read_path(combined_path_protect);
                DEBUG << "<Block error trace on return>: Block code " + boost::algorithm::hex(not_found) +
                         " could not be found in storage tree \"" + combined_path->string() + "\".";
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
            return false;
        }
        //the block code should have a size of 5; one chunk index and 4 bytes of encoding for the offset
        std::unique_lock size_read(size_protect, std::defer_lock);
        size_read.lock();
        if ((short) size->size() - 1 < (short) block_code[0] ||
            !std::get<0>(size->at(block_code[0]))) {
            size_read.unlock();
            std::string const not_found((const char *) block_code.data(), block_code.size());
            std::scoped_lock const read_path(combined_path_protect);
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
                offset += (((std::size_t) sub_block_code[i]) << (i * CHAR_BITS));
            }
            std::string const ref_name{boost::algorithm::hex(std::string{(char) block_code[0]})};
            std::shared_lock read_path2(combined_path_protect);
            std::filesystem::path read_path = *combined_path / ref_name;
            read_path2.unlock();

            //calculate binary of timestamp
            std::vector<unsigned char> bin_time;
            for (unsigned short i = 0; i < (unsigned short) sizeof(current_time); i++) {
                bin_time.push_back((unsigned char) (current_time >> (i * CHAR_BITS)));
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
                Sleep(TEN_MS);
#else
                usleep(TEN_MS * ONE_MILLISECOND);
#endif // _WIN32
            }

            FILE *writer = std::fopen(read_path.make_preferred().c_str(), "rb+");
            auto set_time_end_sequence = [&]() {
                if(std::fclose(writer))ERROR << "Write stream was not open!";
                std::atomic_flag_clear_explicit(write_ptr, std::memory_order_release);
                if (!write_ptr->test())write_ptr->notify_all();
            };
            if (!writer) {
                ERROR << "File write opening failed at \"" + read_path.string() + "\"";
                set_time_end_sequence();
                return false;
            }
            //File should have been opened or created here
            if (std::fseek(writer, static_cast<long>(offset), SEEK_SET)) {
                ERROR << "File seek failed at \"" + read_path.string() + ", position "+std::to_string(offset)+"\"";
                set_time_end_sequence();
                return false;
            }

            if (std::ferror(writer)) {
                FATAL << "I/O error when seeking \"" + read_path.string() + "\"";
                set_time_end_sequence();
                return false;
            }
            //File should have been opened or created here
            if (!std::fwrite(bin_time.data(), bin_time.size(), sizeof(unsigned char), writer)) {
                FATAL << "I/O error when writing binary time sequence at \"" + read_path.string() + "\"";
                set_time_end_sequence();
                return false;
            }

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

std::tuple<std::size_t, std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>>>>
uh::trees::tree_storage::delete_blocks(
        std::vector<std::vector<unsigned char>> &block_codes,
        unsigned short num_threads) {
    if (block_codes.empty() || !num_threads){
        ERROR << "No block codes as input or there was a lack of threading resources!";
        return {};
    }
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

    //presort local block reference groups
    std::vector<std::vector<std::vector<unsigned char>>> sorted_block_codes{};
    unsigned char current = (*block_codes.begin())[0];
    std::vector<std::vector<unsigned char>> buffer{};
    for(auto it = block_codes.begin(); it < block_codes.end();it++){
        if((*it)[0] != current){
            sorted_block_codes.push_back(buffer);
            buffer.clear();
            current = (*it)[0];
        }
        buffer.push_back(*it);
    }
    if(!buffer.empty())sorted_block_codes.push_back(buffer);

    std::mutex m1;
    std::unique_lock item_lock(m1,std::defer_lock);

    for(auto &item_now:sorted_block_codes){
        if(item_now.empty()){
            ERROR << "Internal Error on deleting: empty block!";
            return {};
        }
        auto error_thread_sequence = [&]() {
            while (std::atomic_flag_test_and_set_explicit(&error_flag,
                                                          std::memory_order_acquire)) {
                return;
            }
        };

        auto first_index_exe_function = [&]() {
            auto item = item_now;
            item_lock.unlock();
            //parallel start
            std::vector<std::vector<unsigned char>> deeper_codes{}, delete_here_codes{};
            //take a branch to delete multiple blocks within
            std::for_each(item.begin(), item.end(), [&](auto &a) {
                if (a.size() > SMALLEST_LOCAL_BLOCK_SIZE)deeper_codes.emplace_back(a.begin() + 1, a.end());
                else {
                    if (a.size() < SMALLEST_LOCAL_BLOCK_SIZE) {
                        std::string const not_found((const char *) a.data(), a.size());
                        std::scoped_lock const read_path(combined_path_protect);
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
            if (!delete_here_codes.empty() && (*item.begin())[0] < size->size() &&
                std::get<0>(size->at((*item.begin())[0])) > 0) {
                auto maintain_ptr = &(*std::get<4>(size->at((*item.begin())[0])));//the first element of sorted local blocks references the security mechanism
                auto read_ptr = &(*std::get<3>(size->at((*item.begin())[0])));
                auto write_ptr = &(*std::get<2>(size->at((*item.begin())[0])));
                size_read.unlock();

                while (std::atomic_flag_test_and_set_explicit(&(*maintain_ptr),
                                                              std::memory_order_acquire)) {
                    maintain_ptr->wait(true);
                }

                *read_ptr += 1;

                while (write_ptr->test()) {
                    write_ptr->wait(true);
                }

                //sort blocks to be deleted here after their offset on the chunk
                std::string const ref_name{boost::algorithm::hex(std::string{(char) (*item.begin())[0]})};
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
                        offset1 += (((std::size_t) sub_block_code1[i]) << (i * CHAR_BITS));
                        offset2 += (((std::size_t) sub_block_code2[i]) << (i * CHAR_BITS));
                    }
                    return offset1 < offset2;
                });

                auto block_step_beg = delete_here_codes.cbegin();

                auto offset_calc = [](const auto &a_ref, const auto &b_ref) {
                    std::vector<unsigned char> sub_block_code{a_ref + 1,
                                                              b_ref}; //copy offset code and rebuild
                    std::size_t offset{};
                    for (unsigned short i = 0; i < (unsigned short) sizeof(unsigned int); i++) {
                        offset += (((std::size_t) sub_block_code[i]) << (i * CHAR_BITS));
                    }
                    return offset;
                };

                //read chunk at index (*cur_tmp)[0]
                FILE *reader = std::fopen(chunk.make_preferred().c_str(), "rb");
                std::unique_lock filesystem_lock(std_filesystem_protect, std::defer_lock);
                filesystem_lock.lock();
                std::size_t const total_file_size = std::filesystem::exists(chunk.make_preferred().c_str())
                                              ? std::filesystem::file_size(
                                chunk.make_preferred().c_str()) : 0;
                filesystem_lock.unlock();
                std::atomic_flag write_control(ATOMIC_FLAG_INIT);
                auto read_end_sequence = [&]() {
                    if(std::fclose(reader))ERROR << "Read stream was not open!";
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

                if (std::fseek(reader, static_cast<long>(cur_pos), SEEK_SET)) {
                    ERROR << "File seek failed at \"" + chunk.string() + ", position "+std::to_string(cur_pos)+"\"";
                    read_end_sequence();
                    error_thread_sequence();
                    if (error_flag.test())return;
                }

                if (std::ferror(reader)) {
                    FATAL << "I/O error when seeking \"" + chunk.string() + "\"";
                    read_end_sequence();
                    error_thread_sequence();
                    if (error_flag.test())return;
                }

                std::size_t const trunc_at = cur_pos;
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
                    if(std::fclose(reader))ERROR << "Read stream was not open!";
                    if(std::fclose(writer))ERROR << "Write stream was not open!";
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
                        out_vec.push_back((unsigned char) (new_offset) >> (i * CHAR_BITS));
                    }
                    out_vec.insert(out_vec.cbegin(), old_block_code[0]);

                    if (!std::fwrite(current_storage_ptr.data(), current_storage_ptr.size(), sizeof(unsigned char), writer)) {
                        ERROR << "File write thread failed to put down a line at \"" + chunk_maintain.string() + "\"";
                        io_end_sequence();
                        error_thread_sequence();
                        if (error_flag.test()) {
                            return;
                        }
                    }
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
                        Sleep(TEN_MS);
#else
                        usleep(TEN_MS * ONE_MILLISECOND);
#endif // _WIN32
                    }
                    if(std::fclose(writer))ERROR << "Write stream was not open!";
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
                        out_vec.push_back((unsigned char) (cur_pos >> (i * CHAR_BITS)));
                    }
                    out_vec.insert(out_vec.cbegin(), (*item.begin())[0]);

                    std::array<unsigned char, sizeof(unsigned long)> time_buf;
                    std::size_t count = std::fread(time_buf.data(), sizeof(unsigned char),
                                                   sizeof(unsigned long),
                                                   reader);

                    auto factory_io_sequence_end = [&]() {
                        std::atomic_flag_test_and_set_explicit(&error_flag,
                                                               std::memory_order_acquire);//put error flag on
                        if (num_threads > 1) {
                            if (w1.joinable())w1.join();//write out remaining blocks to prevent data loss
                        } else{
                            if(!multithreading_factory.empty())write_once_to_maintain_file();
                            if(std::fclose(writer))ERROR << "Write stream was not open!";
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
                    std::vector<unsigned char> buffer_in;
                    buffer_in.resize(buf_size+1,0);
                    count = std::fread(buffer_in.data(), sizeof(unsigned char), buf_size + 1, reader);
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
                        output_size += (((std::size_t) buffer_in[buf_count]) << (buf_count * CHAR_BITS));
                    }

                    std::size_t const write_back_size =
                            time_buf.size() + 1 + buffer_in.size() + output_size;

                    if (block_step_beg < delete_here_codes.cend() &&
                        std::equal(out_vec.cbegin(), out_vec.cend(), block_step_beg->cbegin(),
                                   block_step_beg->cend())) {
                        //skip block copy and seek over it
                        if (std::fseek(reader, static_cast<long>(output_size), SEEK_CUR)) {
                            ERROR << "File seek failed at \"" + chunk.string() + ", position "+std::to_string(output_size)+"\"";
                            factory_io_sequence_end();
                            if (error_flag.test())return;//break thread in case error is there
                        }

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
                        for (; i < (unsigned short) time_buf.size(); i++) {
                            tmp_buf[i] = time_buf[i];
                        }
                        tmp_buf[i] = buf_size;
                        i++;
                        unsigned short const meta_offset = i;
                        i = 0;
                        for (; i < (unsigned short) buffer_in.size(); i++) {
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
                        std::scoped_lock const write_multithreading_f(multithreading_factory_protect);
                        multithreading_factory.emplace_back(tmp_buf, out_vec,
                                                            cur_pos - write_back_size - delete_size);
                    }
                }
                read_end_sequence();
                if (num_threads > 1) {
                    if (w1.joinable())w1.join();
                } else {
                    if(!multithreading_factory.empty())write_once_to_maintain_file();
                    if(std::fclose(writer))ERROR << "Write stream was not open!";
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
                    Sleep(TEN_MS);
#else
                    usleep(TEN_MS * ONE_MILLISECOND);
#endif // _WIN32
                }

                std::size_t const maintain_size_append = cur_pos - delete_size;
                auto buf = mem_wait<unsigned char>(maintain_size_append);

                FILE *source = fopen(chunk_maintain.make_preferred().c_str(), "rb");
                auto ptr_release_sequence = [&]() {
                    if(std::fclose(source))ERROR << "Source stream was not open!";
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
                    if(std::fclose(dest))ERROR << "Destination stream was not open!";
                    ptr_release_sequence();
                    error_thread_sequence();
                    return;
                }
                filesystem_lock.lock();
                if (!std::fwrite(buf.data(), buf.size(), sizeof(unsigned char), writer) && std::filesystem::exists(chunk_maintain.make_preferred())
                && std::filesystem::file_size(chunk_maintain.make_preferred()) != 0) {
                    filesystem_lock.unlock();
                    FATAL << "I/O error when writing binary time sequence at \"" + chunk_maintain.string() + "\"";
                    if(std::fclose(dest))ERROR << "Destination stream was not open!";
                    ptr_release_sequence();
                    error_thread_sequence();
                    return;
                }
                else filesystem_lock.unlock();

                if (std::ferror(dest)) {
                    FATAL << "I/O error when appending \"" + chunk_maintain.string() + "\"";
                    if(std::fclose(dest))ERROR << "Destination stream was not open!";
                    ptr_release_sequence();
                    error_thread_sequence();
                    return;
                }

                if(std::fclose(dest))ERROR << "Destination stream was not open!";
                if(std::fclose(source))ERROR << "Source stream was not open!";

                filesystem_lock.lock();
                if (!std::filesystem::remove(chunk_maintain)) {
                    filesystem_lock.unlock();
                    FATAL << "Could not remove old maintainance file \"" + chunk_maintain.string() + "\"";
                    error_thread_sequence();
                    return;
                }
                else filesystem_lock.unlock();

                size_read.lock();
                std::get<0>(size->at((*item.begin())[0])) -= delete_size;
                size_read.unlock();

                std::atomic_flag_clear_explicit(write_ptr, std::memory_order_release);
                if (!write_ptr->test())write_ptr->notify_one();
                std::atomic_flag_clear_explicit(maintain_ptr, std::memory_order_release);
                if (!maintain_ptr->test())maintain_ptr->notify_one();
            } else {
                size_read.unlock();
            }

            //delete deeper codes
            std::unique_lock children_read(children_protect, std::defer_lock);
            children_read.lock();
            if (!deeper_codes.empty() && (*item.begin())[0] < children->size() &&
                std::get<0>(children->at((*item.begin())[0])) > 0) {
                auto tmp_deeper_tree_ptr = std::get<1>(children->at((*item.begin())[0]));
                children_read.unlock();
                //parallel start
                auto deeper_delete = tmp_deeper_tree_ptr->delete_blocks(deeper_codes, (num_threads % 2)?1:2);
                children_read.lock();
                std::get<0>(children->at((*item.begin())[0])) -= std::get<0>(
                        deeper_delete);//subtract deleted size from deeper node
                children_read.unlock();
                out_size += std::get<0>(deeper_delete);//add total deleted size
                for (auto &it: std::get<1>(deeper_delete)) {
                    //deeper elements have their first position filtered so that we need to rebuild it
                    std::get<0>(it).insert(std::get<0>(it).cbegin(), (*item.begin())[0]);
                    std::get<1>(it).insert(std::get<1>(it).cbegin(), (*item.begin())[0]);
                }

                std::scoped_lock const lock_splice(out_change_list_protect);
                out_change_list.splice(out_change_list.cend(), std::get<1>(deeper_delete));
            } else{
                children_read.unlock();
            }

            std::lock(size_read, children_read);
            if ((*item.begin())[0] >= size->size() || (*item.begin())[0] >= children->size()) {
                children_read.unlock();
                size_read.unlock();

                std::scoped_lock const path_read(combined_path_protect);
                std::string out_error = "<Block error trace>: The following local block codes were exceeding limits of storage tree \"" +
                                        combined_path->string() +
                                        "\" and were skipped:\n";

                for(const auto &item1:item){
                    std::string const not_found((const char *) item1.data(), item1.size());
                    out_error += boost::algorithm::hex(not_found) + "\n";
                }
                DEBUG << out_error;
            } else {
                children_read.unlock();
                size_read.unlock();
            }
            active_threads -= (num_threads % 2)?1:2;
        };

        if (num_threads == 1){
            item_lock.lock();
            active_threads += (num_threads % 2)?1:2;
            first_index_exe_function();
        }
        else {
            //threading manager
            while (active_threads.load() >= num_threads) {
                if (error_flag.test()) {
                    FATAL
                        << "Delete_blocks threading engine crashed unexpectedly while waiting for CPU cores!";
                    return {};
                }
#ifdef _WIN32
                Sleep(TEN_MS);
#else
                usleep(TEN_MS * ONE_MILLISECOND);
#endif // _WIN32
            }
            active_threads += (num_threads % 2)?1:2;
            item_lock.lock();
            std::thread(first_index_exe_function).detach();
            if (error_flag.test()) {
                FATAL << "Delete_blocks threading engine crashed unexpectedly!";
                return {};
            }
        }
    }

    while (active_threads.load() >= 0) {
        if (error_flag.test()) {
            FATAL
                << "Delete_blocks threading engine crashed unexpectedly while waiting for CPU cores!";
            return {};
        }
#ifdef _WIN32
        Sleep(TEN_MS);
#else
        usleep(TEN_MS * ONE_MILLISECOND);
#endif // _WIN32
    }

    std::scoped_lock const out_return_lock(out_change_list_protect);
    return {out_size.load(), out_change_list};
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
            Sleep(TEN_MS);
#else
            usleep(TEN_MS * ONE_MILLISECOND);
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
