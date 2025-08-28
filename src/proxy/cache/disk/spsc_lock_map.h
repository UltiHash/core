#pragma once

#include <boost/asio/experimental/channel.hpp>
#include <common/utils/strings.h>
#include <memory>

namespace uh::cluster {

template <typename Key, typename Value>
requires requires(Value v) { serialize(v); }
class lock_map {
public:
    coro<std::shared_ptr<Value>> acquire(const Key& key) {
        std::unique_lock lock(m_mutex);
        auto& chan = m_umap[key];
        if (!chan) {
            chan = std::make_shared<boost::asio::experimental::channel<void(
                boost::system::error_code, std::shared_ptr<Value>)>>(1);
            co_return nullptr;
        }

        std::shared_ptr<Value> value;
        boost::system::error_code ec;
        co_await chan->async_receive(ec, value);
        if (ec) {
            throw std::runtime_error("Failed to acquire entry lock: " +
                                     ec.message());
        }
        co_return value;
    }

    coro<void> release(const std::string& key,
                       std::shared_ptr<Value> value = nullptr) {
        std::unique_lock lock(m_mutex);
        auto it = m_umap.find(key);
        if (it == m_umap.end()) {
            throw std::runtime_error("No inflight entry for key: " +
                                     serialize(key));
        }

        boost::system::error_code ec;
        co_await it->second->async_send(ec, value);
        if (ec) {
            throw std::runtime_error("Failed to release entry lock: " +
                                     ec.message());
        }

        if (it->second->size() == 0) {
            m_umap.erase(it);
        }
    }

private:
    std::mutex m_mutex;
    using channel = std::shared_ptr<boost::asio::experimental::channel<void(
        boost::system::error_code, std::shared_ptr<Value>)>>;
    std::unordered_map<Key, channel> m_umap;
};

} // namespace uh::cluster
