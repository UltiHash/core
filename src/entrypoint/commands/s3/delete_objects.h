#pragma once

#include <entrypoint/commands/command.h>
#include <entrypoint/directory.h>
#include <entrypoint/limits.h>

#include <storage/global/data_view.h>

#include <common/utils/xml_parser.h>

namespace uh::cluster {

class delete_objects : public command {
public:
    delete_objects(directory&, storage::global::global_data_view&, limits&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

    static coro<std::vector<std::reference_wrapper<const pt::ptree>>>
    get_delete_object_keys(ep::http::request& req);

private:
    directory& m_dir;
    static constexpr std::size_t MAXIMUM_DELETE_KEYS = 1000;
};

} // namespace uh::cluster
