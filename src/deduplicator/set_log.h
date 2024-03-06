#ifndef UH_CLUSTER_SET_LOG_H
#define UH_CLUSTER_SET_LOG_H

#include "common/types/common_types.h"
#include "fragment_element.h"
#include <common/global_data/global_data_view.h>
#include <cstring>
#include <fcntl.h>
#include <filesystem>

namespace uh::cluster {

enum set_operation : char {
    INSERT,
    REMOVE,
};

class set_log {

    std::filesystem::path m_log_path;
    int m_log_file;

    struct entry {
        set_operation op{};
        uint128_t pointer;
        uint16_t size{};
        uint128_t prefix{0};
    };

public:
    explicit set_log(std::filesystem::path log_path);
    ~set_log();

    void append(const entry& e) const;
    void replay(std::set<fragment_element>& set, global_data_view& storage,
                std::shared_mutex& m);

private:
    static int get_log_file(const std::filesystem::path& path);
    static void serialize(const entry& entry, char* buf);
    static void serialize(const fragment_element& frag, char* buf);
    [[nodiscard]] std::pair<set_operation, set_log::entry> deserialize() const;
    void recreate(std::set<fragment_element>& fragment_set);
};

} // end namespace uh::cluster

#endif // UH_CLUSTER_SET_LOG_H
