//
// Created by benjamin-elias on 09.01.23.
//
#include "tree_storage.h"

uh::trees::tree_storage::tree_storage(const std::filesystem::path &root, unsigned short num_threads) {
    if (!num_threads || root.empty()) {
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
        std::unique_lock step_lock_init(work_steal_protect, std::defer_lock);
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
            if (th.joinable())th.join();
    }
    std::unique_lock lock_size(size_protect, std::defer_lock);
    lock_size.lock();
    //fill holes of size
    if (!size->empty()) {
        auto size_max = std::max_element(size->begin(), size->end(), [](auto &item1, auto &item2) {
            return std::get<1>(item1) < std::get<1>(item2);
        });
        unsigned char const max_i = std::get<1>(*size_max);
        for (unsigned short i6 = 0; i6 < max_i; i6++) {
            if (!std::any_of(size->begin(), size->end(), [&i6](auto &item) {
                return std::get<1>(item) == i6;
            })) {
                std::shared_ptr<std::atomic_flag> f1 = std::make_shared<std::atomic_flag>(), f3 = std::make_shared<std::atomic_flag>();
                std::shared_ptr<std::atomic<std::size_t>> f2 = std::make_shared<std::atomic<std::size_t>>();
                size->emplace_back(0, i6, f1, f2, f3);
            }
        }
    }
    if (!std::is_sorted(size->begin(), size->end(),
                        [](const auto &a, const auto &b) {
                            return std::get<1>(a) < std::get<1>(b);
                        }))
        std::sort(size->begin(), size->end(),
                  [](const auto &a, const auto &b) {
                      return std::get<1>(a) < std::get<1>(b);
                  });
    lock_size.unlock();

    std::scoped_lock const lock_children(children_protect);
    //fill holes of children
    if (!children->empty()) {
        auto children_max = std::max_element(children->begin(), children->end(), [](auto &item1, auto &item2) {
            return std::get<2>(item1) < std::get<2>(item2);
        });
        unsigned char const max_i = std::get<2>(*children_max);
        for (unsigned short i6 = 0; i6 < max_i; i6++) {
            if (!std::any_of(children->begin(), children->end(), [&i6](auto &item) {
                return std::get<2>(item) == i6;
            })) {
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
    if (!std::is_sorted(children->begin(), children->end(),
                        [](const auto &a, const auto &b) {
                            return std::get<2>(a) < std::get<2>(b);
                        }))
        std::sort(children->begin(), children->end(),
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
    for (unsigned short i = 0; i < byte_count; i++) {
        prefix.push_back((unsigned char) (input_size >> (i * CHAR_BITS)));
    }
    if (prefix.empty())prefix.push_back(0);
    prefix.insert(prefix.cbegin(), byte_count - 1);
    return prefix;
}

std::tuple<std::size_t, std::size_t, std::array<unsigned char,
        SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, bool> uh::trees::tree_storage::write_block_base(
        FILE *writer, const std::filesystem::path &write_at, std::vector<unsigned char> block,
        const std::vector<unsigned char> &local_block_ref,
        std::array<unsigned long, TIME_STAMPS_ON_BLOCK> &times,
        bool update_times,
        bool calc_SHA512,
        std::vector<unsigned char> hash_buf,
        std::size_t placeholder_block_size) {
    //generic functions
    auto block_path = [&] {
        std::vector<unsigned char> sub_block_code{local_block_ref.cbegin() + 1,
                                                  local_block_ref.cend()}; //copy offset code and rebuild
        std::string const ref_name{
                boost::algorithm::hex(std::string{local_block_ref.cbegin() + 1, local_block_ref.cend()})};
        std::shared_lock read_path_lock(combined_path_protect);
        std::filesystem::path read_path =
                *combined_path / boost::algorithm::hex(std::string{(char) local_block_ref[0]}) / ref_name;
        read_path_lock.unlock();
        return read_path.make_preferred();
    };
    auto time_binary_calc = [](auto current_time) {
        std::vector<unsigned char> bin_time;
        bin_time.reserve(sizeof(current_time));
        for (unsigned short i = 0; i < (unsigned short) sizeof(current_time); i++) {
            bin_time.push_back((unsigned char) (current_time >> (i * CHAR_BITS)));
        }
        return bin_time;
    };
    auto error_sequence_empty = [&] {
        std::tuple<std::size_t, std::size_t, std::array<unsigned char,
                SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, bool> empty_tup{};
        std::get<3>(empty_tup) = true;
        return empty_tup;
    };
    //is the file pointer valid?
    if (!writer) {
        ERROR << "File write opening failed at \"" + write_at.string() + "\" at block reference\"" +
                 block_path().string() + "\"";
        return error_sequence_empty();
    }
    //flags
    bool block_input = !block.empty();
    bool placeholder_size_available = placeholder_block_size > 0;
    //action if input creation time is set 0: assume the writer wants to update and skip writing the creation time
    if (times[0] == 0 && !update_times) {
        ERROR << "Creation time was empty even if you did not use update_times mode!";
        return error_sequence_empty();
    }
    //write times, prefix, block, block_hash checksum
    //times
    std::array<std::vector<unsigned char>, TIME_STAMPS_ON_BLOCK> convert_time{};//creation time, last visited time, maximum valid
    auto t_beg = convert_time.begin();
    for (const auto &t: times) {
        *t_beg = time_binary_calc(t);
        t_beg++;
    }
    //write times
    bool first_time = true;
    for (auto &c_t: convert_time) {
        c_t.resize(sizeof(unsigned long), 0);
        if (!((block_input && !update_times && calc_SHA512 && hash_buf.empty()) ||
              (!block_input && update_times && calc_SHA512 && !block.empty()) ||
              (!update_times && !calc_SHA512 && !hash_buf.empty() && times[0] > 0)) &&
            first_time) {//if not the clear writing only case we always only modify the block
            first_time = false;
            //read creation time if skipped to handle global hash creation
            if (times[0] == 0) {
                if (std::fread(c_t.data(), sizeof(unsigned char), sizeof(unsigned long), writer) !=
                    sizeof(unsigned long)) {
                    FATAL
                        << "I/O creation time read to create global hash was not completed while writing to path \"" +
                           write_at.string() +
                           "\" at block reference\"" + block_path().string() + "\"";
                    return error_sequence_empty();
                }
                //restore creation time
                unsigned long block_creation{};
                for (unsigned short i = 0; i < (unsigned short) sizeof(unsigned long); i++) {
                    block_creation += (((std::size_t) c_t[i]) << (i * CHAR_BITS));
                }
                times[0] = block_creation;
            } else {
                if (std::fseek(writer, static_cast<long>(sizeof(unsigned long)), SEEK_CUR)) {//skip prefix and block
                    ERROR << "File seek for skipping creation time failed at \"" + write_at.string() + ", position " +
                             std::to_string(sizeof(unsigned long)) + "\" at block reference\"" + block_path().string() +
                             "\"";
                    return error_sequence_empty();
                }
            }
            continue;
        }
        if (!std::fwrite(c_t.data(), sizeof(unsigned char), sizeof(unsigned long), writer)) {
            ERROR << "File write binary time opening failed writing time at \"" + write_at.string() + "\"";
            return error_sequence_empty();
        }
    }
    //somehow generate prefix; first try to generate from block size, then from placeholder_block_size else try to read it from disk and get block size
    std::vector<unsigned char> prefix{};
    std::size_t block_size{};
    bool seeked_prefix = false;
    if (block_input) {
        prefix = prefix_wrap(block.size());
        block_size = block.size();
    } else {
        if (placeholder_size_available) {
            prefix = prefix_wrap(placeholder_block_size);
            block_size = placeholder_block_size;
        } else {
            //read block size from disk
            unsigned char buf_size = 0;
            if (std::fread(&buf_size, sizeof(unsigned char), BUF_LEN_SIZE_FOR_SIZE_BLOCK, writer) !=
                BUF_LEN_SIZE_FOR_SIZE_BLOCK) {
                FATAL
                    << "I/O prefix first byte reading was not completed while writing to path \"" + write_at.string() +
                       "\" at block reference\"" + block_path().string() + "\"";
                return error_sequence_empty();
            }
            std::vector<unsigned char> buffer_in;
            buffer_in.resize(buf_size + BUF_LEN_SIZE_FOR_SIZE_BLOCK, 0);
            if (std::fread(buffer_in.data(), sizeof(unsigned char), buf_size + BUF_LEN_SIZE_FOR_SIZE_BLOCK, writer) !=
                buf_size + BUF_LEN_SIZE_FOR_SIZE_BLOCK) {
                FATAL
                    << "I/O prefix size buffer reading was not completed while writing to path \"" + write_at.string() +
                       "\" at block reference\"" + block_path().string() + "\"";
                return error_sequence_empty();
            }

            for (unsigned short buf_count = 0; buf_count < buf_size + BUF_LEN_SIZE_FOR_SIZE_BLOCK; buf_count++) {
                block_size += (((std::size_t) buffer_in[buf_count]) << (buf_count * CHAR_BITS));
            }
            prefix = prefix_wrap(block_size);
            seeked_prefix = true;
        }
    }
    if (update_times) {
        //prefix was calculated but not read, so skip it
        if (!seeked_prefix) {
            if (std::fseek(writer, static_cast<long>(prefix.size()), SEEK_CUR)) {//skip prefix and block
                ERROR << "File seek for skipping prefix read at writing operation failed at \"" + write_at.string() +
                         ", position " +
                         std::to_string(prefix.size()) + "\" at block reference\"" + block_path().string() +
                         "\"";
                return error_sequence_empty();
            }
        }
    } else {
        //if we could generate the prefix, we write it down else we will have read over it
        if (!seeked_prefix) {
            if (!std::fwrite(prefix.data(), sizeof(unsigned char), prefix.size(), writer)) {
                ERROR << "File write prefix failed while writing to path \"" + write_at.string() +
                         "\" at block reference\"" + block_path().string() + "\"";
                return error_sequence_empty();
            }
        }
    }

    //total size
    const std::size_t total_block_size = SHA512_DIGEST_LENGTH + TIME_STAMPS_ON_BLOCK * sizeof(unsigned long) +
                                         prefix.size() + block_size +
                                         SHA256_DIGEST_LENGTH;

    //continue to write block or get it from elsewhere
    if (update_times) {
        if (block_input) {
            //seek over block on file and take this block input in
            if (std::fseek(writer, static_cast<long>(block.size()), SEEK_CUR)) {//skip prefix and block
                ERROR << "File seek for skipping block read at writing operation failed at \"" + write_at.string() +
                         ", position " +
                         std::to_string(block.size()) + "\" at block reference\"" + block_path().string() +
                         "\"";
                return error_sequence_empty();
            }
        } else {
            //read block from file
            block.resize(block_size, 0);
            if (std::fread(block.data(), sizeof(unsigned char), block.size(), writer) != block.size()) {
                FATAL
                    << "I/O block reading was not completed while writing to path \"" + write_at.string() +
                       "\" at block reference\"" + block_path().string() + "\"";
                return std::make_tuple(total_block_size, block_size, std::array<unsigned char,
                        SHA512_DIGEST_LENGTH + sizeof(unsigned long)>{}, true);
            }
        }
    } else {
        if (block_input) {
            if (!std::fwrite(block.data(), sizeof(unsigned char), block.size(), writer)) {
                ERROR << "File write block failed while writing to path \"" + write_at.string() +
                         "\" at block reference\"" + block_path().string() + "\"";
                return error_sequence_empty();
            }
        } else {
            ERROR << "File write block failed while writing to path \"" + write_at.string() +
                     "\" at block reference\"" + block_path().string() + "\", because the given block was empty!";
            return error_sequence_empty();
        }
    }

    //generate global hash from reading or calculation or read update it from outside or the file
    std::array<unsigned char, SHA512_DIGEST_LENGTH + sizeof(unsigned long)> global_block_reference{};
    //hash read on updating or try if no block input was given
    bool hash_read_error = false;
    auto hash_read = [&] {
        if (hash_buf.size() == SHA512_DIGEST_LENGTH) {
            if (update_times) {
                //if hash_buf is filled we use it and calculate the global hash with creation time
                if (std::fseek(writer, static_cast<long>(hash_buf.size()), SEEK_CUR)) {//skip prefix and block
                    ERROR << "File seek for skipping hash read at writing operation failed at \"" + write_at.string() +
                             ", position " +
                             std::to_string(hash_buf.size()) + "\" at block reference\"" + block_path().string() +
                             "\"";
                    hash_read_error = true;
                }
            } else {
                //write hash input to file
                if (!std::fwrite(hash_buf.data(), sizeof(unsigned char), hash_buf.size(), writer)) {
                    ERROR << "File write hash from extern source failed while writing to path \"" + write_at.string() +
                             "\" at block reference\"" + block_path().string() + "\"";
                    hash_read_error = true;
                }
            }
        } else {
            //try to read hash_buf and the global_block_reference from disk
            hash_buf.clear();
            hash_buf.resize(SHA512_DIGEST_LENGTH, 0);
            if (std::fread(hash_buf.data(), sizeof(unsigned char), hash_buf.size(), writer) != hash_buf.size()) {
                FATAL
                    << "I/O hash reading was not completed while writing to path \"" + write_at.string() +
                       "\" at block reference\"" + block_path().string() + "\"";
                hash_read_error = true;
            }
        }
    };

    bool calc_SHA_and_write_error = false;
    auto calc_SHA_and_write = [&] {
        //calculate hash
        if (hash_buf.size() != SHA512_DIGEST_LENGTH) {
            hash_buf.clear();
            hash_buf.resize(SHA512_DIGEST_LENGTH, 0);
        }
        SHA512(block.data(), block_size, hash_buf.data());

        if (block_input) {
            //write hash
            if (!std::fwrite(hash_buf.data(), sizeof(unsigned char), hash_buf.size(), writer)) {
                ERROR << "File write hash failed at writing to path \"" + write_at.string() +
                         "\" at block reference\"" + block_path().string() + "\"";
                calc_SHA_and_write_error = true;
            }
        } else {
            if (std::fseek(writer, static_cast<long>(hash_buf.size()), SEEK_CUR)) {//skip prefix and block
                ERROR << "File seek for skipping hash write at writing operation failed at \"" + write_at.string() +
                         ", position " +
                         std::to_string(hash_buf.size()) + "\" at block reference\"" + block_path().string() +
                         "\"";
                hash_read_error = true;
            }
        }

    };
    //decide if to read, seek or write the hash on operation
    //on update_times: get hash from outside or from reading if input block is empty, if input block exists generate SHA and overwrite
    //else: get hash from outside or from calculating SHA block input and write, block input is required

    if (block_input) {
        //HASH GENERATION
        if (calc_SHA512) {
            calc_SHA_and_write();
            if (calc_SHA_and_write_error) {
                return error_sequence_empty();
            }
        } else {
            hash_read();
            if (hash_read_error) {
                return std::make_tuple(total_block_size, block_size, std::array<unsigned char,
                        SHA512_DIGEST_LENGTH + sizeof(unsigned long)>{}, true);
            }
        }
    } else {
        if (update_times) {
            if (calc_SHA512) {
                if (!block.empty()) {
                    calc_SHA_and_write();
                    if (calc_SHA_and_write_error) {
                        return error_sequence_empty();
                    }
                } else {
                    ERROR << "Cannot calculate SHA without block input!";
                    return std::make_tuple(total_block_size, block_size, std::array<unsigned char,
                            SHA512_DIGEST_LENGTH + sizeof(unsigned long)>{}, true);
                }
            } else {
                hash_read();
                if (hash_read_error) {
                    return std::make_tuple(total_block_size, block_size, std::array<unsigned char,
                            SHA512_DIGEST_LENGTH + sizeof(unsigned long)>{}, true);
                }
            }
        } else {
            ERROR << "Block input is needed if not updating!";
            return std::make_tuple(total_block_size, block_size, std::array<unsigned char,
                    SHA512_DIGEST_LENGTH + sizeof(unsigned long)>{}, true);
        }
    }
    //calculate global hash reference
    std::ranges::copy(hash_buf.cbegin(), hash_buf.cend(), global_block_reference.begin());
    std::ranges::copy(convert_time[0].cbegin(), convert_time[0].cend(),
                      global_block_reference.begin() + SHA512_DIGEST_LENGTH);
    //finally always calculate and update checksum
    std::vector<unsigned char> block_buf = mem_wait<unsigned char>(total_block_size);
    std::array<unsigned char, SHA256_DIGEST_LENGTH> checksum{};
    //all values should be complete here so we can give most information on error
    auto error_sequence = [&]() {
        return std::make_tuple(total_block_size, std::max(block.size(), placeholder_block_size), global_block_reference,
                               true);
    };
    //fill block_buf to simulate a complete model of the block on RAM and calculate checksum
    for (const auto &t_c: convert_time) {//times
        block_buf.insert(block_buf.cend(), t_c.cbegin(), t_c.cend());
    }
    block_buf.insert(block_buf.cend(), prefix.cbegin(), prefix.cend());//prefix
    block_buf.insert(block_buf.cend(), block.cbegin(), block.cend());
    block_buf.insert(block_buf.cend(), hash_buf.cbegin(), hash_buf.cend());//SHA512
    SHA256(block_buf.data(), block_buf.size(), checksum.data());

    if (block_buf.size() + SHA256_DIGEST_LENGTH != total_block_size) {
        ERROR << "File write opening failed at \"" + write_at.string() + "\" at block reference\"" +
                 block_path().string() + "\"";
        return error_sequence();
    }
    if (!std::fwrite(checksum.data(), sizeof(unsigned char), checksum.size(), writer)) {
        ERROR << "File write binary time opening failed at \"" + write_at.string() +
                 "\" at block reference\"" + block_path().string() + "\"";
        return error_sequence();
    }
//done
    return std::make_tuple(total_block_size, block_size, global_block_reference, false);
}

std::tuple<std::size_t, std::size_t, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>, std::array<unsigned char,
        SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, bool, bool>
uh::trees::tree_storage::read_block_base(
        FILE *reader, const std::filesystem::path &read_at, const std::vector<unsigned char> &local_block_ref,
        bool skip_read_block, bool check_valid) {

    auto block_path = [&] {
        std::vector<unsigned char> sub_block_code{local_block_ref.cbegin() + 1,
                                                  local_block_ref.cend()}; //copy offset code and rebuild
        std::string const ref_name{
                boost::algorithm::hex(std::string{local_block_ref.cbegin() + 1, local_block_ref.cend()})};
        std::shared_lock read_path_lock(combined_path_protect);
        std::filesystem::path read_path =
                *combined_path / boost::algorithm::hex(std::string{(char) local_block_ref[0]}) / ref_name;
        read_path_lock.unlock();
        return read_path.make_preferred();
    };

    auto error_sequence = [&]() {
        auto err_tuple = std::tuple<std::size_t, std::size_t, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>,
                std::array<unsigned char,
                        SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, bool, bool>{};
        std::get<5>(err_tuple) = true;
        return err_tuple;
    };

    if (!reader) {
        ERROR << "File read opening failed at \"" + read_at.string() + "\" at block reference\"" +
                 block_path().string() + "\"";
        return error_sequence();
    }

    if (std::ferror(reader)) {
        FATAL << "I/O error when reading \"" + read_at.string() + "\" at block reference\"" +
                 block_path().string() + "\"";
        return error_sequence();
    }
    //first try to read the times and the block size, optionally the block itself as the middle of the encoding can be skipped
    const std::size_t read_info_block_size = TIME_STAMPS_ON_BLOCK * sizeof(unsigned long);
    std::vector<unsigned char> first_section = mem_wait<unsigned char>(read_info_block_size);
    first_section.resize(read_info_block_size, 0);

    if (std::fread(first_section.data(), sizeof(unsigned char), first_section.size(), reader) !=
        first_section.size()) {
        FATAL
            << "I/O first info section reading was not completed on path \"" + read_at.string() +
               "\" at block reference\"" + block_path().string() + "\"";
        return error_sequence();
    }
    //read times
    std::array<unsigned long, TIME_STAMPS_ON_BLOCK> times{};
    std::array<std::vector<unsigned char>, TIME_STAMPS_ON_BLOCK> convert_time{};
    long time_shifter{};
    for (auto &t: times) {
        auto beg_time = first_section.cbegin() + time_shifter * (long) sizeof(unsigned long);
        auto end_time = beg_time + sizeof(unsigned long);
        std::vector<unsigned char> vector_time = mem_wait<unsigned char>(sizeof(unsigned long));
        vector_time.assign(beg_time, end_time);
        convert_time[time_shifter] = vector_time;
        unsigned long block_time{};
        for (unsigned short i = 0; i < (unsigned short) sizeof(unsigned long); i++) {
            block_time += (((std::size_t) vector_time[i]) << (i * CHAR_BITS));
        }
        time_shifter++;
        t = block_time;
    }
    //read prefix
    unsigned char buf_size = 0;
    if (std::fread(&buf_size, sizeof(unsigned char), BUF_LEN_SIZE_FOR_SIZE_BLOCK, reader) !=
        BUF_LEN_SIZE_FOR_SIZE_BLOCK) {
        FATAL
            << "I/O prefix first byte reading was not completed on path \"" + read_at.string() +
               "\" at block reference\"" + block_path().string() + "\"";
        return error_sequence();
    }
    std::vector<unsigned char> buffer_in;
    buffer_in.resize(buf_size + BUF_LEN_SIZE_FOR_SIZE_BLOCK, 0);
    if (std::fread(buffer_in.data(), sizeof(unsigned char), buf_size + BUF_LEN_SIZE_FOR_SIZE_BLOCK, reader) !=
        buf_size + BUF_LEN_SIZE_FOR_SIZE_BLOCK) {
        FATAL
            << "I/O prefix size buffer reading was not completed on path \"" + read_at.string() +
               "\" at block reference\"" + block_path().string() + "\"";
        return error_sequence();
    }
    std::size_t block_size{};
    for (unsigned short buf_count = 0; buf_count < buf_size + BUF_LEN_SIZE_FOR_SIZE_BLOCK; buf_count++) {
        block_size += (((std::size_t) buffer_in[buf_count]) << (buf_count * CHAR_BITS));
    }

    std::vector<unsigned char> block = mem_wait<unsigned char>(block_size);
    bool valid = true;
    std::size_t total_block_size = TIME_STAMPS_ON_BLOCK * sizeof(unsigned long) +
                                   (BUF_LEN_SIZE_FOR_SIZE_BLOCK + buffer_in.size()) + SHA512_DIGEST_LENGTH +
                                   block_size +
                                   SHA256_DIGEST_LENGTH;
    //read block optional
    if (skip_read_block) {
        if (check_valid) {
            DEBUG << "Validity could not be checked because block and checksum were skipped at \"" +
                     read_at.string() + ", jump " + std::to_string(block_size + SHA256_DIGEST_LENGTH) +
                     "\" at block reference\"" + block_path().string() + "\"";
            return error_sequence();
        } else {
            if (std::fseek(reader, static_cast<long>(block_size), SEEK_CUR)) {
                ERROR << "File seek for skipping block failed at \"" + read_at.string() + ", jump " +
                         std::to_string(block_size) + "\" at block reference\"" +
                         block_path().string() + "\"";
                return error_sequence();
            }
        }
    } else {
        block.resize(block_size, 0);
        if (std::fread(block.data(), sizeof(unsigned char), block_size, reader) != block_size) {
            FATAL
                << "I/O block reading not completed on path \"" + read_at.string() + "\" at block reference\"" +
                   block_path().string() + "\"";
            return error_sequence();
        }
    }
    //read block hash
    std::array<unsigned char, SHA512_DIGEST_LENGTH> hash_buf{};
    std::array<unsigned char, SHA512_DIGEST_LENGTH + sizeof(unsigned long)> global_block_reference{};
    //read hash
    if (std::fread(hash_buf.data(), sizeof(unsigned char), hash_buf.size(), reader) != hash_buf.size()) {
        FATAL
            << "I/O hash reading not completed on path \"" + read_at.string() + "\" at block reference\"" +
               block_path().string() + "\"";
        return error_sequence();
    }
    if (check_valid) {
        std::array<unsigned char, SHA512_DIGEST_LENGTH> hash_test{};
        SHA512(block.data(), block.size(), hash_test.data());
        valid = std::equal(hash_buf.cbegin(), hash_buf.cend(), hash_test.cbegin(), hash_test.cend());
        if (!valid) {
            TRACE
                << "Block did not fit it's hash on path \"" + read_at.string() + "\" at block reference\"" +
                   block_path().string() + "\"";
        }
        if (valid) {
            std::vector<unsigned char> block_buf = mem_wait<unsigned char>(
                    total_block_size - SHA256_DIGEST_LENGTH);
            for (const auto &t_c: convert_time) {
                block_buf.insert(block_buf.cend(), t_c.cbegin(), t_c.cend());
            }
            block_buf.insert(block_buf.cend(), buf_size);
            block_buf.insert(block_buf.cend(), buffer_in.cbegin(), buffer_in.cend());
            block_buf.insert(block_buf.cend(), block.cbegin(), block.cend());
            block_buf.insert(block_buf.cend(), hash_buf.cbegin(), hash_buf.cend());

            std::array<unsigned char, SHA256_DIGEST_LENGTH> checksum_test{}, checksum_read{};
            SHA256(block_buf.data(), block_buf.size(), checksum_test.data());

            if (std::fread(checksum_read.data(), sizeof(unsigned char), checksum_read.size(), reader) !=
                checksum_read.size()) {
                FATAL
                    << "I/O checksum reading for validity testnot completed on path \"" + read_at.string() +
                       "\" at block reference\"" + block_path().string() + "\"";
                return error_sequence();
            }
            valid = std::equal(checksum_test.cbegin(), checksum_test.cend(), checksum_read.cbegin(),
                               checksum_read.cend());
            if (!valid) {
                TRACE
                    << "Block meta data was invalid on path \"" + read_at.string() +
                       "\" at block reference\"" + block_path().string() + "\"";
            }
        } else {
            if (std::fseek(reader, static_cast<long>(SHA256_DIGEST_LENGTH), SEEK_CUR)) {
                ERROR << "File seek for validity test after broken block failed at \"" + read_at.string() +
                         ", jump " + std::to_string(SHA256_DIGEST_LENGTH) + "\" at block reference\"" +
                         block_path().string() + "\"";
                return error_sequence();
            }
        }
    } else {
        if (std::fseek(reader, static_cast<long>(SHA256_DIGEST_LENGTH), SEEK_CUR)) {
            ERROR << "File seek for validity test after broken block failed at \"" + read_at.string() +
                     ", jump " + std::to_string(SHA256_DIGEST_LENGTH) + "\" at block reference\"" +
                     block_path().string() + "\"";
            return error_sequence();
        }
    }

    //generate global hash
    std::ranges::copy(hash_buf.cbegin(), hash_buf.cend(), global_block_reference.begin());
    std::ranges::copy(convert_time[0].cbegin(), convert_time[0].cend(),
                      global_block_reference.begin() + SHA512_DIGEST_LENGTH);

    //<std::size_t,std::vector<unsigned char>,std::array<unsigned long,TIME_STAMPS_ON_BLOCK>,std::array<unsigned long,SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, bool, bool>
    return std::make_tuple(total_block_size, block_size, block, times, global_block_reference, false, valid);
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

std::tuple<std::array<unsigned char,
        SHA512_DIGEST_LENGTH + sizeof(unsigned long)>, std::vector<unsigned char>, std::size_t, std::size_t>
uh::trees::tree_storage::write(const std::vector<unsigned char> &input,
                               std::array<unsigned long, TIME_STAMPS_ON_BLOCK> times) {
    if (input.empty()) {
        ERROR << "No input given to write!";
        return {};
    }
    if (input.size() > STORE_MAX) {
        FATAL << "A block could not be written because it exceeded maximum size of blocks \"" +
                 std::to_string(STORE_MAX) +
                 "\" with a size of \"" + std::to_string(input.size()) + "\".";
        return {};
    }
    std::vector<unsigned char> prefix = prefix_wrap(input.size());
    std::size_t const total_size = SHA512_DIGEST_LENGTH +
                                   TIME_STAMPS_ON_BLOCK * sizeof(unsigned long) +
                                   prefix.size() + input.size() +
                                   SHA256_DIGEST_LENGTH;

    //check block fill of this node, look for free space
    std::unique_lock lock_size(size_protect, std::defer_lock);

    auto IO_lock = [&](auto &write_ptr, auto &read_ptr) {
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
    };

    auto maintain_lock = [&](auto &maintain_ptr) {
        while (std::atomic_flag_test_and_set_explicit(maintain_ptr, std::memory_order_acquire)) {
            maintain_ptr->wait(true);
        }
    };

    auto IO_unlock = [&](auto &write_ptr) {
        std::atomic_flag_clear_explicit(write_ptr, std::memory_order_release);
        if (!write_ptr->test())write_ptr->notify_all();
    };

    auto maintain_unlock = [&](auto &maintain_ptr) {
        std::atomic_flag_clear_explicit(maintain_ptr, std::memory_order_release);
        if (!maintain_ptr->test())maintain_ptr->notify_one();
    };

    unsigned short min_pos = 0, min_pos_old = 0;
    std::size_t min_val = 0;
    bool deeper{};
    bool count_loop{};
    std::size_t size_tmp{};
    //search loop to find free space and adapt on changes
    do {
        lock_size.lock();
        if (size->size() < N) {
            min_pos = size->size();
            min_val = 0;
            if (min_pos == min_pos_old && count_loop) {
                std::shared_ptr<std::atomic_flag> f1 = std::make_shared<std::atomic_flag>(), f3 = std::make_shared<std::atomic_flag>();
                std::shared_ptr<std::atomic<std::size_t>> const f2 = std::make_shared<std::atomic<std::size_t>>();
                size->emplace_back(min_val, min_pos, f1, f2, f3);
            }
        } else {
            if (count_loop) {
                auto maintain_ptr = &(*std::get<4>(size->at(min_pos_old)));
                maintain_unlock(maintain_ptr);
            }
            auto min_el = *std::min_element(size->begin(), size->end(), [&count_loop](auto &a, auto &b) {
                return std::get<0>(a) < std::get<0>(b) && !std::get<4>(a)->test() &&
                       !std::get<4>(b)->test();//skip maintain chunks for smaller check
            });
            min_pos = std::get<1>(min_el);
            min_val = std::get<0>(size->at(min_pos));
        }
        if (min_pos != min_pos_old && count_loop) {
            //unlock old chunk again because of update
            auto maintain_ptr = &(*std::get<4>(size->at(min_pos_old)));
            maintain_unlock(maintain_ptr);
            count_loop = false;
        }
        deeper = min_val >= STORE_MAX || min_val + total_size >= STORE_HARD_LIMIT || size->size() <= min_pos ||
                 std::get<4>(size->at(min_pos))->test();

        if (size->size() > min_pos) {
            auto maintain_ptr = &(*std::get<4>(size->at(min_pos)));
            maintain_lock(maintain_ptr);
        }

        if (count_loop) {
            if (!deeper) {
                size_tmp = std::get<0>(size->at(min_pos));
                std::get<0>(size->at(min_pos)) += total_size;
            }
            lock_size.unlock();
            break;
        }

        count_loop = true;
        min_pos_old = min_pos;
        lock_size.unlock();
    } while (count_loop);

    lock_size.lock();
    auto maintain_ptr = &(*std::get<4>(size->at(min_pos)));
    auto read_ptr = &(*std::get<3>(size->at(min_pos)));
    auto write_ptr = &(*std::get<2>(size->at(min_pos)));
    lock_size.unlock();

    if (deeper) {
        //no maintain needed
        maintain_unlock(maintain_ptr);
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

        auto out_vec = tree_ptr2->write(input, times);
        std::get<1>(out_vec).insert(std::get<1>(out_vec).cbegin(), min_pos);
        return out_vec;
    } else {
        //store block to this position
        std::string const ref_name{boost::algorithm::hex(std::string{(char) min_pos})};
        std::shared_lock path_lock2(combined_path_protect);
        std::filesystem::path read_chunk = *combined_path / ref_name;
        path_lock2.unlock();

        IO_lock(write_ptr, read_ptr);

        std::vector<unsigned char> local_block_ref{};
        local_block_ref.reserve(sizeof(unsigned int));
        for (unsigned short i = 0;
             i < (unsigned short) sizeof(unsigned int); i++) {//STORE_MAX will fit in 4 bytes
            local_block_ref.push_back((unsigned char) (size_tmp >> (i * CHAR_BITS)));
        }
        local_block_ref.insert(local_block_ref.cbegin(), min_pos);

        auto block_path = [&] {
            std::vector<unsigned char> sub_block_code{local_block_ref.cbegin() + 1,
                                                      local_block_ref.cend()}; //copy offset code and rebuild
            std::string const ref_name{
                    boost::algorithm::hex(std::string{local_block_ref.cbegin() + 1, local_block_ref.cend()})};
            std::shared_lock read_path_lock(combined_path_protect);
            std::filesystem::path read_path =
                    *combined_path / boost::algorithm::hex(std::string{(char) local_block_ref[0]}) / ref_name;
            read_path_lock.unlock();
            return read_path.make_preferred();
        };

        FILE *writer = std::fopen(read_chunk.make_preferred().c_str(), "ab");
        auto end_write_sequence = [&writer, &write_ptr, &IO_unlock, &maintain_ptr, &maintain_unlock]() {
            if (std::fclose(writer))ERROR << "Write stream was not open!";
            IO_unlock(write_ptr);
            maintain_unlock(maintain_ptr);
        };

        auto write_tup = write_block_base(writer, read_chunk, input, local_block_ref, times);
        end_write_sequence();
        if (std::get<3>(write_tup)) {
            ERROR << "File error at \"" + read_chunk.make_preferred().string() + "\" at block reference\"" +
                     block_path().string() + "\"";
            return {};
        }

        return {std::get<2>(write_tup), local_block_ref, std::get<0>(write_tup), std::get<1>(write_tup)};
    }
}

std::tuple<std::list<std::tuple<std::array<unsigned char, SHA512_DIGEST_LENGTH +
                                                          sizeof(unsigned long)>, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>>>,
        std::list<std::tuple<std::array<unsigned char, SHA512_DIGEST_LENGTH +
                                                       sizeof(unsigned long)>, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>>>>
uh::trees::tree_storage::index(unsigned short num_threads) {
    if (num_threads == 0) {
        ERROR << "Not enough threading resources!";
        return {};
    }
    std::list<std::tuple<std::array<unsigned char, SHA512_DIGEST_LENGTH +
                                                   sizeof(unsigned long)>, std::vector<unsigned char>, std::array<unsigned long, TIME_STAMPS_ON_BLOCK>>> search_index{}, damaged_blocks_index{};
    std::shared_mutex search_index_protect{};
    std::atomic<long> i_constructor{-1};
    std::atomic_flag error_flag{ATOMIC_FLAG_INIT};

    auto multithread_index = [&]() {
        std::unique_lock step_lock_init(work_steal_protect, std::defer_lock);
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
                auto maintain_ptr = &(*std::get<4>(size->at(i)));
                size_lock.unlock();

                *read_ptr += 1;

                while (write_ptr->test()) {
                    write_ptr->wait(true);
                }

                while (maintain_ptr->test()) {
                    maintain_ptr->wait(true);
                }

                std::unique_lock filesystem_lock(std_filesystem_protect, std::defer_lock);
                filesystem_lock.lock();
                std::size_t const total_file_size = std::filesystem::exists(read_path.make_preferred())
                                                    ? std::filesystem::file_size(read_path.make_preferred()) : 0;
                filesystem_lock.unlock();
                FILE *reader = std::fopen(read_path.make_preferred().c_str(), "rb");

                auto read_end_sequence = [&reader, &read_ptr, &write_ptr]() {
                    if (std::fclose(reader))ERROR << "Read stream was not open!";
                    *read_ptr -= 1;
                    if (!read_ptr->load())write_ptr->notify_one();
                };
                auto error_thread_sequence = [&error_flag]() {
                    while (std::atomic_flag_test_and_set_explicit(&error_flag, std::memory_order_acquire)) {
                        return;
                    }
                };
                auto total_end_sequence = [&] {
                    read_end_sequence();
                    error_thread_sequence();
                    if (error_flag.test())return;
                };
                std::atomic<std::size_t> cur_pos{};

                std::atomic<std::size_t> output_size{};
                std::atomic<bool> parallel_switch{};
                while (!std::feof(reader) && cur_pos.load() < total_file_size) {
                    std::vector<unsigned char> local_block_ref{};
                    for (unsigned short i1 = 0;
                         i1 < (unsigned short) sizeof(unsigned int); i1++) {//STORE_MAX will fit in 4 bytes
                        local_block_ref.push_back((unsigned char) (cur_pos.load() >> (i1 * CHAR_BITS)));
                    }
                    local_block_ref.insert(local_block_ref.cbegin(), i);
                    auto read_tup = this->read_block_base(reader, read_path.make_preferred(), local_block_ref, false,
                                                          true);

                    if (error_flag.test() || std::get<5>(read_tup)) {
                        total_end_sequence();
                        ERROR << "Unexpected error. Quitting.";
                        return;
                    }

                    if (!std::get<5>(read_tup) && std::get<6>(read_tup) && std::get<0>(read_tup) > 0) {//if valid
                        std::scoped_lock const lock_here(search_index_protect);
                        search_index.emplace_back(std::get<4>(read_tup), local_block_ref,
                                                  std::get<3>(read_tup));
                    } else {
                        std::scoped_lock const lock_here(search_index_protect);//bit flip detect
                        damaged_blocks_index.emplace_back(std::get<4>(read_tup), local_block_ref,
                                                          std::get<3>(read_tup));
                    }
                    cur_pos += std::get<0>(read_tup);
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
                auto append_list = tree_ptr->index(1);
                for (auto &el: std::get<0>(append_list)) {
                    std::get<1>(el).insert(std::get<1>(el).begin(), i);
                }
                for (auto &el: std::get<1>(append_list)) {
                    std::get<1>(el).insert(std::get<1>(el).begin(), i);
                }
                std::scoped_lock const lock_splice(search_index_protect);
                search_index.splice(search_index.cend(), std::get<0>(append_list));
                damaged_blocks_index.splice(damaged_blocks_index.cend(), std::get<1>(append_list));
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

    if (num_threads == 1) {
        multithread_index();
    } else {
        std::vector<std::thread> workers;
        for (unsigned short i = 0; i < num_threads; i++) {
            std::thread w(multithread_index);
            workers.push_back(std::move(w));
        }
        for (auto &th: workers)
            if (th.joinable())th.join();

    }
    if (error_flag.test()) {
        FATAL << "Indexing threading engine crashed unexpectedly!";
        return {};
    }

    std::scoped_lock const lock_return(search_index_protect);
    return {search_index, damaged_blocks_index};
}

std::size_t uh::trees::tree_storage::delete_recursive(unsigned short num_threads) {
    if (!num_threads) {
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
                if (std::filesystem::exists(read_path) || std::filesystem::is_empty(read_path)) {
                    if (!std::filesystem::remove(read_path)) {
                        filesystem_lock.unlock();
                        FATAL << "Removing was not completed on path \"" + read_path.string() +
                                 "\". Check inconsistent maintain files!";
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
                std::shared_lock path_protect(combined_path_protect, std::defer_lock);
                path_protect.lock();
                std::string const fname = combined_path->filename().string();
                ref_name.insert(ref_name.cbegin(), fname.cbegin() + 2, fname.cbegin() + 4);
                std::filesystem::path const read_path = *combined_path / ref_name;
                path_protect.unlock();
                std::unique_lock filesystem_lock(std_filesystem_protect, std::defer_lock);
                filesystem_lock.lock();
                if (std::filesystem::exists(read_path)) {
                    if (!std::filesystem::remove_all(read_path)) {
                        filesystem_lock.unlock();
                        FATAL << "Removing was not completed on path \"" + read_path.string() +
                                 "\". Check inconsistent maintain files!";
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
            if (th.joinable())th.join();
    }
    if (error_flag.test()) {
        FATAL << "Delete_recursive threading engine crashed unexpectedly!";
        return {};
    }
    return out_size.load();
}

bool uh::trees::tree_storage::set_block_time(const std::vector<unsigned char> &local_block_reference,
                                             std::array<unsigned long, TIME_STAMPS_ON_BLOCK - 1> times) {
    if (local_block_reference.empty()) {
        ERROR << "No input given to set block time!";
        return false;
    }
    if (local_block_reference.size() > SMALLEST_LOCAL_BLOCK_SIZE) {
        //size encoding is not reached yet, set_block_time along tree path
        std::unique_lock read_children(children_protect, std::defer_lock);
        read_children.lock();
        if ((short) children->size() - 1 < (short) local_block_reference[0] ||
            !std::get<0>(children->at(local_block_reference[0]))) {
            read_children.unlock();
            std::string const not_found((const char *) local_block_reference.data(), local_block_reference.size());
            std::scoped_lock const read_path(combined_path_protect);
            DEBUG << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                     " could not be found in storage tree \"" +
                     combined_path->string() + "\".";
            return false;
        } else {
            auto tree_ptr = std::get<1>(children->at(local_block_reference[0]));
            read_children.unlock();
            std::vector<unsigned char> const sub_block_code{local_block_reference.cbegin() + 1,
                                                            local_block_reference.cend()};
            auto out_vec = tree_ptr->set_block_time(sub_block_code, times);
            if (!out_vec) {
                std::string const not_found((const char *) local_block_reference.data(), local_block_reference.size());
                std::scoped_lock const read_path(combined_path_protect);
                DEBUG << "<Block error trace on return>: Block code " + boost::algorithm::hex(not_found) +
                         " could not be found in storage tree \"" + combined_path->string() + "\".";
            }
            return out_vec;
        }
    } else {
        if (local_block_reference.size() < SMALLEST_LOCAL_BLOCK_SIZE) {
            std::string const not_found((const char *) local_block_reference.data(), local_block_reference.size());
            std::scoped_lock const read_path(combined_path_protect);
            FATAL << "<Block error trace>: Block code " + boost::algorithm::hex(not_found) +
                     " was too short for storage tree \"" +
                     combined_path->string() + "\".";
            return false;
        }
        //the block code should have a size of 5; one chunk index and 4 bytes of encoding for the offset
        std::unique_lock size_read(size_protect, std::defer_lock);
        size_read.lock();
        if ((short) size->size() - 1 < (short) local_block_reference[0] ||
            !std::get<0>(size->at(local_block_reference[0]))) {
            size_read.unlock();
            std::string const not_found((const char *) local_block_reference.data(), local_block_reference.size());
            std::scoped_lock const read_path(combined_path_protect);
            DEBUG
                << "<Block error trace on return, final tree>: Block code " + boost::algorithm::hex(not_found) +
                   " could not be found in storage tree \"" + combined_path->string() +
                   "\" and size of storage chunk was 0.";
            return false;
        } else {
            size_read.unlock();

            std::vector<unsigned char> sub_block_code{local_block_reference.cbegin() + 1,
                                                      local_block_reference.cend()}; //copy offset code and rebuild
            std::size_t offset{};
            for (unsigned short i = 0; i < (unsigned short) sizeof(unsigned int); i++) {
                offset += (((std::size_t) sub_block_code[i]) << (i * CHAR_BITS));
            }
            std::string const ref_name{boost::algorithm::hex(std::string{(char) local_block_reference[0]})};
            std::shared_lock read_path2(combined_path_protect);
            std::filesystem::path read_path = *combined_path / ref_name;
            read_path2.unlock();

            size_read.lock();
            auto write_ptr = &(*std::get<2>(size->at(local_block_reference[0])));
            auto read_ptr = &(*std::get<3>(size->at(local_block_reference[0])));
            auto maintain_ptr = &(*std::get<4>(size->at(local_block_reference[0])));
            size_read.unlock();

            while (std::atomic_flag_test_and_set_explicit(maintain_ptr, std::memory_order_acquire)) {
                maintain_ptr->wait(true);
            }
            //protect interaction of reading and unlocking before writing the block, also keep track of maintaining the file
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

            auto block_path = [&] {
                std::string const ref_name{
                        boost::algorithm::hex(
                                std::string{local_block_reference.cbegin() + 1, local_block_reference.cend()})};
                std::shared_lock read_path_lock(combined_path_protect);
                std::filesystem::path read_path =
                        *combined_path / boost::algorithm::hex(std::string{(char) local_block_reference[0]}) / ref_name;
                read_path_lock.unlock();
                return read_path.make_preferred();
            };

            FILE *writer = std::fopen(read_path.make_preferred().c_str(), "rb+");
            auto set_time_end_sequence = [&]() {
                if (std::fclose(writer))ERROR << "Write stream was not open!";

                std::atomic_flag_clear_explicit(write_ptr, std::memory_order_release);
                if (!write_ptr->test())write_ptr->notify_all();
                std::atomic_flag_clear_explicit(maintain_ptr, std::memory_order_release);
                if (!maintain_ptr->test())maintain_ptr->notify_one();
            };
            //File should have been opened or created here
            if (std::fseek(writer, static_cast<long>(offset), SEEK_SET)) {
                ERROR << "File seek failed at \"" + read_path.make_preferred().string() + ", position " +
                         std::to_string(offset) + "\"";
                set_time_end_sequence();
                return false;
            }

            std::array<unsigned long, TIME_STAMPS_ON_BLOCK> times_not_creation{};
            times_not_creation[0] = 0;
            times_not_creation[1] = times[0];
            times_not_creation[2] = times[1];

            auto write_tup = write_block_base(writer, read_path.make_preferred(), std::vector<unsigned char>{},
                                              local_block_reference, times_not_creation, true);
            set_time_end_sequence();
            if (std::get<3>(write_tup)) {
                ERROR << "File error at \"" + read_path.make_preferred().string() + "\" at block reference\"" +
                         block_path().string() + "\"";
                return {};
            }

            return true;
        }
    }
}

std::tuple<std::size_t, std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>>>>
uh::trees::tree_storage::delete_blocks(
        std::vector<std::vector<unsigned char>> &block_codes,
        unsigned short num_threads) {
    if (block_codes.empty() || !num_threads) {
        ERROR << "No block codes as input or there was a lack of threading resources!";
        return {};
    }

    auto maintain_lock = [&](auto &maintain_ptr) {
        while (std::atomic_flag_test_and_set_explicit(maintain_ptr, std::memory_order_acquire)) {
            maintain_ptr->wait(true);
        }
    };

    auto maintain_unlock = [](auto &maintain_ptr) {
        std::atomic_flag_clear_explicit(maintain_ptr, std::memory_order_release);
        if (!maintain_ptr->test())maintain_ptr->notify_one();
    };

    auto write_lock = [&](auto &write_ptr) {
        while (std::atomic_flag_test_and_set_explicit(write_ptr, std::memory_order_acquire)) {
            write_ptr->wait(true);
        }
    };

    auto write_unlock = [](auto &write_ptr) {
        std::atomic_flag_clear_explicit(write_ptr, std::memory_order_release);
        if (!write_ptr->test())write_ptr->notify_one();
    };

    std::atomic_flag error_flag(ATOMIC_FLAG_INIT);
    std::atomic<std::size_t> active_threads{};
    std::atomic<std::size_t> out_size{};
    std::shared_mutex out_change_list_protect{};
    std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>>> out_change_list{};
    //sort for lexicographic to find blocks within the same chunks that all need to be deleted
    std::sort(block_codes.begin(), block_codes.end(), [](auto &a, auto &b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    });
    //scan and filter for size == 5 and delete blocks from chunks, deliver deleted size and changed local block codes via chunk level indexing after change spot
    //presort local block reference groups
    std::vector<std::vector<std::vector<unsigned char>>> sorted_block_codes{};
    unsigned char current = (*block_codes.begin())[0];
    std::vector<std::vector<unsigned char>> buffer{};
    for (auto it = block_codes.begin(); it < block_codes.end(); it++) {
        if ((*it)[0] != current) {
            sorted_block_codes.push_back(buffer);
            buffer.clear();
            current = (*it)[0];
        }
        buffer.push_back(*it);
    }
    if (!buffer.empty())sorted_block_codes.push_back(buffer);

    auto error_thread_sequence = [&error_flag]() {
        std::atomic_flag_test_and_set_explicit(&error_flag, std::memory_order_acquire);
    };

    //use multithreading with a thread management system so that threads from deleting go on to deeper delete
    std::shared_mutex worker_protect{};
    std::list<std::tuple<std::size_t, std::shared_ptr<std::atomic_flag>, std::jthread>> workers{};

    auto index_main_func = [&](const std::vector<std::vector<unsigned char>> &item, std::size_t worker_number) {
        //parallel start, sort codes that are at this level and codes that are deeper down the tree
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
            //atomic IO protection lock order: read, write, maintain on read; maintain, write, read on any write
            auto maintain_ptr = &(*std::get<4>(size->at((*item.begin())[0])));
            auto read_ptr = &(*std::get<3>(size->at((*item.begin())[0])));
            auto write_ptr = &(*std::get<2>(size->at((*item.begin())[0])));
            size_read.unlock();

            //start filter-copy, truncate, append algorithm
            auto offset_calc = [](const auto &a_ref, const auto &b_ref) {
                std::vector<unsigned char> sub_block_code{a_ref + 1,
                                                          b_ref}; //copy offset code and rebuild
                std::size_t offset{};
                for (unsigned short i = 0; i < (unsigned short) sizeof(unsigned int); i++) {
                    offset += (((std::size_t) sub_block_code[i]) << (i * CHAR_BITS));
                }
                return offset;
            };

            //sort blocks to be deleted here after their offset on the chunk to delete in order
            std::string const ref_name{boost::algorithm::hex(std::string{(char) (*item.begin())[0]})};
            std::shared_lock path_read(combined_path_protect);
            std::filesystem::path chunk = *combined_path / ref_name;
            path_read.unlock();
            std::filesystem::path chunk_maintain =
                    chunk.parent_path() / (chunk.filename().string() + "_maintain");

            std::sort(delete_here_codes.begin(), delete_here_codes.end(), [&offset_calc](auto &a, auto &b) {
                return offset_calc(a.cbegin(), a.cend()) < offset_calc(b.cbegin(), b.cend());
            });

            auto block_step_beg = delete_here_codes.cbegin();

            std::unique_lock filesystem_lock(std_filesystem_protect, std::defer_lock);
            filesystem_lock.lock();
            std::size_t const total_file_size = std::filesystem::exists(chunk.make_preferred().c_str())
                                                ? std::filesystem::file_size(
                            chunk.make_preferred().c_str()) : 0;
            filesystem_lock.unlock();

            maintain_lock(maintain_ptr);

            write_lock(write_ptr);

            *read_ptr += 1;
            FILE *reader = std::fopen(chunk.make_preferred().c_str(), "rb");

            auto read_end_sequence = [&reader, &read_ptr]() {
                if (std::fclose(reader))ERROR << "Read stream was not open!";
                *read_ptr -= 1;
            };
            //control variables
            std::size_t cur_pos = offset_calc(block_step_beg->cbegin(), block_step_beg->cend());
            std::size_t const trunc_at = cur_pos;
            std::size_t delete_size{};
            //jump to first block of chunk
            if (std::fseek(reader, static_cast<long>(cur_pos), SEEK_SET)) {//lowest offset block is start indicator
                ERROR << "File seek failed at \"" + chunk.string() + ", position " + std::to_string(cur_pos) + "\"";
                read_end_sequence();
                error_thread_sequence();
                if (error_flag.test())return;
            }
            //open new maintain file to write to
            //clear old mainain file
            filesystem_lock.lock();
            if (std::filesystem::exists(chunk_maintain))std::filesystem::remove(chunk_maintain);
            filesystem_lock.unlock();
            //transmission from chunk to maintain file
            FILE *writer = std::fopen(chunk_maintain.make_preferred().c_str(), "ab");
            auto io_end_sequence = [&writer, &read_end_sequence, &write_ptr, &write_unlock]() {
                read_end_sequence();
                if (std::fclose(writer))ERROR << "Write stream was not open!";
                write_unlock(write_ptr);
            };
            auto write_total_end = [&io_end_sequence, &error_thread_sequence, &error_flag, &chunk_maintain, &maintain_ptr, &maintain_unlock, &cur_pos, &delete_size] {
                ERROR << "File write thread failed to put down a line at \"" + chunk_maintain.string() +
                         "\" at position " + std::to_string(cur_pos) + " and deleting postion " +
                         std::to_string(cur_pos - delete_size);
                io_end_sequence();
                error_thread_sequence();
                maintain_unlock(maintain_ptr);
                if (error_flag.test()) {
                    return;
                }
            };

            auto block_code_generate = [](std::size_t pos, unsigned char first_el) {
                std::vector<unsigned char> local_block_test_ref{};
                local_block_test_ref.reserve(sizeof(unsigned int));//reconstruct current local reference
                for (unsigned short i = 0;
                     i < (unsigned short) sizeof(unsigned int); i++) {//STORE_MAX will fit in 4 bytes
                    local_block_test_ref.push_back((unsigned char) (pos >> (i * CHAR_BITS)));
                }
                local_block_test_ref.insert(local_block_test_ref.cbegin(), first_el);
                return local_block_test_ref;
            };

            while (!std::feof(reader) && cur_pos < total_file_size) {
                if (error_flag.test())break;//break thread in case error is there
                //File should have been opened or created here, seek for first block
                //start position of the block for seeking it later on is the old size
                std::vector<unsigned char> local_block_test_ref = block_code_generate(cur_pos, (*item.begin())[0]);
                //read a block from current chunk
                if (block_step_beg < delete_here_codes.cend() &&
                    std::equal(local_block_test_ref.cbegin(), local_block_test_ref.cend(), block_step_beg->cbegin(),
                               block_step_beg->cend())) {
                    //skip block copy and seek over it if to delete
                    auto read_tup = read_block_base(reader, chunk.make_preferred(), local_block_test_ref, true);
                    if (error_flag.test() || std::get<5>(read_tup)) {
                        ERROR << "Unexpected error. Quitting.";
                        write_total_end();
                        return;//break thread in case error is there
                    }
                    cur_pos += std::get<0>(read_tup);
                    delete_size += std::get<0>(read_tup);
                    out_size += std::get<0>(read_tup);
                    block_step_beg++;
                } else {
                    //copy block to maintain file
                    auto read_tup = read_block_base(reader, chunk.make_preferred(), local_block_test_ref);
                    if (error_flag.test() || std::get<5>(read_tup)) {
                        ERROR << "Unexpected error. Quitting.";
                        write_total_end();
                        return;//break thread in case error is there
                    }
                    std::array<unsigned char,
                            SHA512_DIGEST_LENGTH + sizeof(unsigned long)> global_hash = std::get<4>(read_tup);
                    std::vector<unsigned char> hash{};
                    hash.resize(SHA512_DIGEST_LENGTH, 0);
                    std::ranges::copy(global_hash.cbegin(), global_hash.cbegin() + SHA512_DIGEST_LENGTH,
                                      hash.begin());
                    //stores block, old offset vector, new offset number, block SHA512 and times
                    auto write_tup = write_block_base(writer, chunk_maintain.make_preferred(), std::get<2>(read_tup),
                                                      local_block_test_ref, std::get<3>(read_tup), false, false,
                                                      hash);

                    if (std::get<3>(write_tup)) {
                        write_total_end();
                    }
                    std::unique_lock lock_change(out_change_list_protect);
                    out_change_list.emplace_back(local_block_test_ref,
                                                 block_code_generate(cur_pos - delete_size, (*item.begin())[0]));
                    lock_change.unlock();
                    cur_pos += std::get<0>(read_tup);
                }
            }
            io_end_sequence();

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
            write_lock(write_ptr);

            while (read_ptr->load() > 0) {
#ifdef _WIN32
                Sleep(TEN_MS);
#else
                usleep(TEN_MS * ONE_MILLISECOND);
#endif // _WIN32
            }

            std::size_t const maintain_size_append = cur_pos - delete_size;
            auto buf = mem_wait<unsigned char>(maintain_size_append);
            buf.resize(maintain_size_append, 0);

            FILE *source = fopen(chunk_maintain.make_preferred().c_str(), "rb");
            auto ptr_release_sequence = [&]() {
                if (std::fclose(source))ERROR << "Source stream was not open!";
                write_unlock(write_ptr);
                maintain_unlock(maintain_ptr);
            };
            if (!source) {
                ERROR << "File read opening failed at \"" + chunk_maintain.string() + "\"";
                ptr_release_sequence();
                error_thread_sequence();
                return;
            }
            filesystem_lock.lock();
            if (fread(buf.data(), sizeof(unsigned char), buf.size(), source) != maintain_size_append &&
                std::filesystem::exists(chunk_maintain.make_preferred())
                && std::filesystem::file_size(chunk_maintain.make_preferred()) != 0) {
                filesystem_lock.unlock();
                ERROR << "File read opening for append failed at \"" + chunk_maintain.string() + "\"";
                ptr_release_sequence();
                error_thread_sequence();
                return;
            } else filesystem_lock.unlock();
            if (std::ferror(source)) {
                FATAL << "I/O error when reading \"" + chunk_maintain.string() + "\"";
                ptr_release_sequence();
                error_thread_sequence();
                return;
            }
            if (truncate64(chunk.c_str(), static_cast<long>(trunc_at)) != 0) {
                ERROR << "File truncate failed at \"" + chunk_maintain.string() + "\"";
                ptr_release_sequence();
                error_thread_sequence();
                return;
            }
            FILE *dest = fopen(chunk.make_preferred().c_str(), "ab");
            if (!dest) {
                ERROR << "File append opening failed at \"" + chunk_maintain.string() + "\"";
                if (std::fclose(dest))ERROR << "Destination stream was not open!";
                ptr_release_sequence();
                error_thread_sequence();
                return;
            }
            filesystem_lock.lock();
            if (!std::fwrite(buf.data(), buf.size(), sizeof(unsigned char), dest) &&
                std::filesystem::exists(chunk_maintain.make_preferred())
                && std::filesystem::file_size(chunk_maintain.make_preferred()) != 0) {
                filesystem_lock.unlock();
                FATAL << "I/O error when writing binary time sequence at \"" + chunk_maintain.string() + "\"";
                if (std::fclose(dest))ERROR << "Destination stream was not open!";
                ptr_release_sequence();
                error_thread_sequence();
                return;
            } else filesystem_lock.unlock();

            if (std::ferror(dest)) {
                FATAL << "I/O error when appending \"" + chunk_maintain.string() + "\"";
                if (std::fclose(dest))ERROR << "Destination stream was not open!";
                ptr_release_sequence();
                error_thread_sequence();
                return;
            }

            if (std::fclose(dest))ERROR << "Destination stream was not open!";
            if (std::fclose(source))ERROR << "Source stream was not open!";

            filesystem_lock.lock();
            if (!std::filesystem::remove(chunk_maintain)) {
                filesystem_lock.unlock();
                FATAL << "Could not remove old maintainance file \"" + chunk_maintain.string() + "\"";
                write_unlock(write_ptr);
                maintain_unlock(maintain_ptr);
                error_thread_sequence();
                return;
            } else filesystem_lock.unlock();

            size_read.lock();
            std::get<0>(size->at((*item.begin())[0])) -= delete_size;
            size_read.unlock();

            write_unlock(write_ptr);
            maintain_unlock(maintain_ptr);

        } else {
            size_read.unlock();
        }

        //delete deeper codes recursive
        std::unique_lock children_read(children_protect, std::defer_lock);
        children_read.lock();
        if (!deeper_codes.empty() && (*item.begin())[0] < children->size() &&
            std::get<0>(children->at((*item.begin())[0])) > 0) {
            auto tmp_deeper_tree_ptr = std::get<1>(children->at((*item.begin())[0]));
            children_read.unlock();
            //parallel start
            auto deeper_delete = tmp_deeper_tree_ptr->delete_blocks(deeper_codes, 1);
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
        } else {
            children_read.unlock();
        }

        std::lock(size_read, children_read);
        if ((*item.begin())[0] >= size->size() && (*item.begin())[0] >= children->size()) {
            children_read.unlock();
            size_read.unlock();

            std::scoped_lock const path_read(combined_path_protect);
            std::string out_error =
                    "<Block error trace>: The following local block codes were exceeding limits of storage tree \"" +
                    combined_path->string() +
                    "\" and were skipped:\n";

            for (const auto &item1: item) {
                std::string const not_found((const char *) item1.data(), item1.size());
                out_error += boost::algorithm::hex(not_found) + "\n";
            }
            DEBUG << out_error;
        } else {
            children_read.unlock();
            size_read.unlock();
        }
        std::scoped_lock worker_lock2(worker_protect);
        for (auto &it: workers) {
            if (worker_number == std::get<0>(it)) {
                std::atomic_flag_clear_explicit(&(*std::get<1>(it)), std::memory_order_release);
                break;
            }
        }
    };

    std::size_t worker_count{};
    for (auto &item_now: sorted_block_codes) {
        if (item_now.empty()) {
            ERROR << "Internal Error on deleting: empty block!";
            return {};
        }
        if (num_threads == 1) {
            active_threads += 1;
            index_main_func(item_now, worker_count);
            active_threads -= 1;
        } else {
            //threading manager
            std::unique_lock manage_lock1(worker_protect);
            auto it_w = workers.begin();
            while ((workers.size() >= num_threads || sorted_block_codes.size() <= active_threads.load()) &&
                   it_w != workers.end()) {
                manage_lock1.unlock();
                std::unique_lock manage_lock2(worker_protect);
                bool advanced = false;
                while (it_w != workers.end() && !std::get<1>(*it_w)->test() && std::get<2>(*it_w).joinable()) {
                    std::get<2>(*it_w).join();
                    auto tmp_cur = it_w++;
                    workers.erase(tmp_cur);
                    active_threads -= 1;
                    advanced = true;
                }
                if (!advanced)it_w++;
                manage_lock2.unlock();
                if (error_flag.test()) {
                    FATAL
                        << "Delete_blocks threading engine crashed unexpectedly while waiting for CPU cores!";
                    return {};
                }
                manage_lock1.lock();
            }
            manage_lock1.unlock();

            if (error_flag.test()) {
                FATAL
                    << "Delete_blocks threading engine crashed unexpectedly while waiting for CPU cores!";
                return {};
            }

            active_threads += 1;
            std::shared_ptr<std::atomic_flag> flag_worker = std::make_shared<std::atomic_flag>();
            while (std::atomic_flag_test_and_set_explicit(&(*flag_worker), std::memory_order_acquire)) {
                flag_worker->wait(true);
            }
            std::unique_lock manage_lock2(worker_protect);
            workers.emplace_back(worker_count, flag_worker, std::jthread(index_main_func, item_now, worker_count));
            manage_lock2.unlock();
            worker_count++;
            if (error_flag.test()) {
                FATAL << "Delete_blocks threading engine crashed unexpectedly!";
                return {};
            }
        }
    }

    if (num_threads > 1) {
        std::unique_lock manage_lock3(worker_protect);
        auto it_w = workers.begin();
        while (it_w != workers.end()) {
            manage_lock3.unlock();
            std::unique_lock manage_lock4(worker_protect);
            if (std::get<2>(*it_w).joinable()) {
                manage_lock4.unlock();
                std::unique_lock manage_lock5(worker_protect);
                while (std::atomic_flag_test_and_set_explicit(&(*std::get<1>(*it_w)), std::memory_order_acquire)) {
                    std::get<1>(*it_w)->wait(true);
                }
                if (std::get<1>(*it_w)->test())std::get<2>(*it_w).join();
                auto tmp_cur = it_w++;
                workers.erase(tmp_cur);
                manage_lock5.unlock();
                active_threads -= 1;
                if (error_flag.test()) {
                    FATAL
                        << "Delete_blocks threading engine crashed unexpectedly while waiting for CPU cores!";
                    return {};
                }
            } else {
                it_w++;
                manage_lock4.unlock();
            }
            manage_lock3.lock();
        }
        manage_lock3.unlock();
    }

    std::scoped_lock const out_return_lock(out_change_list_protect);
    return {out_size.load(), out_change_list};
}

uh::trees::tree_storage::~tree_storage() {
    std::unique_lock lock_size(size_protect, std::defer_lock);
    for (auto &s: *size) {
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
