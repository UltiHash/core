#pragma once

#include <common/etcd/namespace.h>
#include <common/etcd/subscriber.h>
#include <common/etcd/utils.h>
#include <common/telemetry/log.h>
#include <common/utils/strings.h>

namespace uh::cluster {

class candidate {
public:
    // TODO: Use callback of subscriber, rather than using vector_observer's.
    using callback_t = candidate_observer::callback_t;
    using id_t = candidate_observer::id_t;
    candidate(etcd_manager& etcd, const std::string& key, id_t id,
              callback_t callback = nullptr)
        : m_callback{std::move(callback)},
          m_candidate{etcd, key, id,
                      [this](bool is_leader) { this->callback(is_leader); }},
          m_subscriber{"candidate",
                       etcd,
                       key,
                       {
                           m_candidate,
                       }} {}

    auto is_leader() -> bool const { return m_candidate.is_leader(); }

private:
    void callback(bool is_leader) {
        if (m_callback) {
            m_callback(is_leader);
        }
        m_candidate.proclaim();
    }

    callback_t m_callback;
    candidate_observer m_candidate;
    subscriber m_subscriber;
};

} // namespace uh::cluster
