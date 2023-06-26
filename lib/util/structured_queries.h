//
// Created by masi on 6/22/23.
//

#ifndef CORE_STRUCTURED_QUERIES_H
#define CORE_STRUCTURED_QUERIES_H

#include <span>
#include <util/ospan.h>
#include <protocol/messages.h>

namespace uh::util {


struct read_query;
struct write_query;

template <typename RequestType>
requires (std::is_same_v <RequestType, protocol::read_key_value::request> or
        (std::is_same_v <RequestType, protocol::write_key_value::request>) )
struct structured_queries {

    explicit structured_queries (const RequestType& req):
            m_req (req) {
    }

    auto next () {
        if constexpr (std::is_same_v <RequestType, protocol::read_key_value::request>)
            return (offset == std::get <0> (m_req.get().data).size) ? nullptr: std::make_unique<read_query>(*this);
        else
            return (offset == std::get <0> (m_req.get().data).size) ? nullptr: std::make_unique<write_query>(*this);
    }

    const std::reference_wrapper <const RequestType> m_req;
    std::size_t offset {};
    int index {};
    int label_index {};
};

void handle_labels (auto& q, size_t shift_offset_size, util::ospan <std::string_view>& labels, char* const data_ptr);

struct read_query {
    std::span <char> start_key;
    std::span <char> end_key;
    std::span <char> single_key;
    util::ospan <std::string_view> labels;
    explicit read_query (structured_queries <protocol::read_key_value::request>& rq);
};


struct write_query {
    std::span <char> key;
    std::span <char> value;
    util::ospan <std::string_view> labels;

    explicit write_query (structured_queries <protocol::write_key_value::request>& wq);
};

} // namespace uh::util

#endif //CORE_STRUCTURED_QUERIES_H
