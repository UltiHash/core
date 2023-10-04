//
// Created by max on 04.10.23.
//

#ifndef CORE_BUCKET_H
#define CORE_BUCKET_H

#include "chaining_data_store.h"
#include "transaction_log.h"

namespace uh::cluster{

class bucket {

public:
    bucket (const std::filesystem::path& root, const std::string& bucket_name, bucket_config& conf):
        m_data_store({
            .directory = root/bucket_name/"ds",
            .free_spot_log = root/bucket_name/"ds/free_spot_log",
            .min_file_size = conf.min_file_size,
            .max_file_size = conf.max_file_size,
            .max_storage_size = conf.max_storage_size,
            .max_chunk_size = conf.max_chunk_size,
        }),
        m_transaction_log(root/bucket_name/"transaction_log"),
        m_object_ptrs (m_transaction_log.replay()) {
    }

    std::vector <std::string> list_keys () {
        std::vector <std::string> keys;
        keys.reserve (m_object_ptrs.size());
        for (const auto& obj: m_object_ptrs) {
            keys.emplace_back (obj.first);
        }
        return keys;
    }

    void insert_object (const std::string& key, std::span<char> data) {
        std::unique_lock <std::shared_mutex> lock(m_mutex);
        if (m_object_ptrs.contains(key)) [[unlikely]] {
            throw std::runtime_error("Attempt to insert object ' + key + ' failed: an object with the same name already exists.");
        }
        const auto index = m_data_store.post_write(data);
        m_transaction_log.append(key, index, transaction_log::operation::INSERT_START);

        //TODO: handle rollback in case something goes up in smoke during the transaction
        m_data_store.apply_write();
        m_object_ptrs[key] = index;
        lock.unlock();

        m_transaction_log.append(key, index, transaction_log::operation::INSERT_END);
    }

    ospan<char> get_obj(const std::string& key) {
        std::shared_lock lock(m_mutex);
        if (!m_object_ptrs.contains(key)) [[unlikely]] {
            throw std::runtime_error("Attempt to get object ' + key + ' failed: no such object.");
        }
        const auto index = m_object_ptrs.at(key);
        return m_data_store.read(index);
    }

    void delete_object (const std::string& key) {
        std::unique_lock <std::shared_mutex> lock(m_mutex);
        if (!m_object_ptrs.contains(key)) [[unlikely]] {
            throw std::runtime_error("Attempt to remove object ' + key + ' failed: no such object.");
        }
        const auto index = m_object_ptrs.at(key);
        m_transaction_log.append(key, index, transaction_log::operation::REMOVE_START);

        m_object_ptrs.erase(key);
        m_data_store.remove(index);
        lock.unlock();

        m_transaction_log.append(key, index, transaction_log::operation::REMOVE_END);
    }

    void update_object (const std::string &key, std::span<char> data) {
        std::unique_lock <std::shared_mutex> lock(m_mutex);
        const auto index = m_data_store.post_write(data);
        m_transaction_log.append(key, index, transaction_log::operation::UPDATE_START);

        //TODO: handle rollback in case something goes up in smoke during the transaction
        m_data_store.apply_write();
        const auto old_index = m_object_ptrs.at(key);
        m_data_store.remove(old_index);
        m_object_ptrs[key] = index;
        lock.unlock();

        m_transaction_log.append(key, index, transaction_log::operation::UPDATE_END);
    }

    bool contains_object (const std::string &key) {
        return m_object_ptrs.contains(key);
    }


private:
    std::string m_bucket_id;
    std::filesystem::path m_root;
    chaining_data_store m_data_store;
    transaction_log m_transaction_log;
    std::unordered_map <std::string, uint64_t> m_object_ptrs;
    std::shared_mutex m_mutex;



};

}

#endif //CORE_BUCKET_H