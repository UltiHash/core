#pragma once

#include <atomic>
#include <common/etcd/utils.h>
#include <functional>
#include <stop_token>
#include <thread>

namespace uh::cluster {

class candidate {
public:
    candidate(etcd_manager& etcd, const std::string& name, std::string value,
              std::function<void(void)> callback = nullptr)
        : m_etcd{etcd},
          m_name{name},
          m_value{value},
          m_callback{std::move(callback)},
          worker_thread{&candidate::worker, this} {}

    ~candidate() {
        auto resp = m_campaign_response.load(std::memory_order_acquire);
        if (resp != nullptr) {
            auto val = resp->value();
            m_etcd.resign(m_name, val.key(), val.created_index());
        }
        // NOTE: Destructor can consume ETCD_GRPC_TIMEOUT because campaign is a
        // blocking operation.
    }

    auto is_leader() const {
        return m_campaign_response.load(std::memory_order_acquire) != nullptr;
    }

private:
    void worker(std::stop_token token) {
        while (token.stop_requested() == false) {
            std::cout << "waiting for election" << std::endl;
            auto resp = m_etcd.campaign(m_name, m_value);
            // NOTE: While conducting a new election (waiting for the function
            // call above), we do not need to be concerned about increasing the
            // offset on write requests to other services, as any additional
            // write requests will fail during their allocation call.

            if (resp.is_grpc_timeout()) {
                std::cout << "candidate experienced timeout" << std::endl;
                continue;
            }

            if (!resp.is_ok()) {
                std::cout << "candidate failed" << std::endl;
                continue;
            }

            std::cout << "candidate won the election" << std::endl;
            if (m_callback)
                std::move(m_callback)();
            std::cout << "key: " << resp.value().key()
                      << " value: " << resp.value().as_string() << std::endl;
            m_campaign_response.store(std::make_shared<etcd::Response>(resp),
                                      std::memory_order_release);
            break; // no re-election
        }
    }

    etcd_manager& m_etcd;
    std::string m_name;
    std::string m_value;
    std::function<void(void)> m_callback;
    std::jthread worker_thread;
    std::atomic<std::shared_ptr<etcd::Response>> m_campaign_response{nullptr};
};

} // namespace uh::cluster
