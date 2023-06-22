//
// Created by masi on 6/22/23.
//
#include "structured_queries.h"

namespace uh::util {

void handle_labels(auto q, size_t shift_offset_size, ospan<std::string_view> &labels, char *const data_ptr) {

    const auto label_count = std::get <0> (q.m_req.get().label_counts).data [q.index];
    const auto wq_label_index = q.label_index;

    q.offset += shift_offset_size;
    q.index ++;

    labels = util::ospan <std::string_view> (label_count);
    for (q.label_index = wq_label_index; q.label_index < label_count; ++q.label_index) {
        const auto label_size = std::get <0> (q.m_req.get().label_sizes).data [q.label_index];
        labels.data [q.label_index] = {data_ptr + q.offset, label_size};
        q.offset += label_size;
    }
    if (q.offset > std::get <0> (q.m_req.get().data).size) {
        throw std::overflow_error ("overflow when parsing key value request");
    }
}

read_query::read_query(structured_queries<protocol::read_key_value::request> &rq) {
    const auto start_key_size = std::get <0> (rq.m_req.get().start_key_sizes).data [rq.index];
    const auto end_key_size = std::get <0> (rq.m_req.get().end_key_sizes).data [rq.index];
    const auto single_key_size = std::get <0> (rq.m_req.get().single_key_sizes).data [rq.index];
    const auto data_ptr = std::get <0> (rq.m_req.get().data).data.get () + rq.offset;

    start_key = {data_ptr, start_key_size};
    end_key = {data_ptr + start_key_size, end_key_size};
    single_key = {data_ptr + start_key_size + end_key_size, single_key_size};

    handle_labels(rq, start_key_size + end_key_size + single_key_size, labels, data_ptr);

}

write_query::write_query(structured_queries<protocol::write_key_value::request> &wq) {
    const auto key_size = std::get <0> (wq.m_req.get().key_sizes).data [wq.index];
    const auto val_size = std::get <0> (wq.m_req.get().value_sizes).data [wq.index];
    const auto data_ptr = std::get <0> (wq.m_req.get().data).data.get () + wq.offset;

    key = {data_ptr, key_size};
    value = {data_ptr + key_size, val_size};

    handle_labels(wq, key_size + val_size, labels, data_ptr);

}

} // namespace uh::util
