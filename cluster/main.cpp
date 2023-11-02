//
// Created by masi on 7/17/23.
//

#include <system_error>
#include "common/cluster_config.h"
#include "data_node/data_node.h"
#include "dedupe_node/dedupe_node.h"
#include "directory_node/directory_node.h"
#include "entry_node/entry_node.h"
#include "network/cluster_map.h"

#include <config.h>
#include <common/log.h>


uh::cluster::entry_node_config make_entry_node_config () {
    return {
        .internal_server_conf = {
                .threads = 4,
                .port = 8081,
        },
        .rest_server_conf = {
                .threads = 4,
                .port = 8080,
        },
        .dedupe_node_connection_count = 2,
        .directory_connection_count = 2,
    };
}

uh::cluster::data_node_config make_data_node_config () {
    return {
        .directory = "ultihash-root/dn",
        .hole_log = "ultihash-root/dn/log",
        .min_file_size = 1ul * 1024ul * 1024ul * 1024ul,
        .max_file_size = 4ul * 1024ul * 1024ul * 1024ul,
        .max_data_store_size = 64ul * 1024ul * 1024ul * 1024ul,
        .server_conf = {
                .threads = 4,
                .port = 8082,
        },
    };
}

uh::cluster::directory_node_config make_directory_node_config () {
    return {
        .server_conf = {
                .threads = 4,
                .port = 8083,
        },
        .directory_conf = {
                .root = "ultihash-root/dr",
                .bucket_conf = {
                        .min_file_size = 1024ul * 1024ul * 1024ul * 2,
                        .max_file_size = 1024ul * 1024ul * 1024ul * 64,
                        .max_storage_size = 1024ul * 1024ul * 1024ul * 256,
                        .max_chunk_size = std::numeric_limits <uint32_t>::max(),
                },
        },
        .data_node_connection_count = 2,
    };
}

uh::cluster::dedupe_config make_dedupe_node_config () {
    return {
        .min_fragment_size = 32,
        .max_fragment_size = 8 * 1024,
        .server_conf = {
                .threads = 1,
                .port = 8084,
        },
        .data_node_connection_count = 2,
        .set_conf = {
                .set_minimum_free_space = 1ul * 1024ul * 1024ul * 1024ul,
                .max_empty_hole_size = 1ul * 1024ul * 1024ul * 1024ul,
                .key_store_config = {
                        .file  = "ultihash-root/dd/set",
                        .init_size = 1ul * 1024ul * 1024ul * 1024ul,
                }
        },
    };
}

uh::cluster::cluster_config make_cluster_config () {
    return {
            .init_process_count = 4,
            .data_node_conf = make_data_node_config(),
            .dedupe_node_conf = make_dedupe_node_config(),
            .directory_node_conf = make_directory_node_config(),
            .entry_node_conf = make_entry_node_config(),
    };
}

void execute_role (const uh::cluster::role role, const int id, uh::cluster::cluster_map cmap) {

    switch (role) {
        case uh::cluster::DATA_NODE: {
            LOG_INFO() << "starting data node";
            uh::cluster::data_node dn (id, std::move (cmap));
            dn.run();
            break;
        }
        case uh::cluster::DEDUPE_NODE: {
            LOG_INFO() << "starting dedupe node";
            uh::cluster::dedupe_node dd (id, std::move (cmap));
            dd.run();
            break;
        }
        case uh::cluster::DIRECTORY_NODE: {
            LOG_INFO() << "starting directory node";
            uh::cluster::directory_node pb (id, std::move (cmap));
            pb.run();
            break;
        }
        case uh::cluster::ENTRY_NODE: {
            LOG_INFO() << "starting entry node";
            uh::cluster::entry_node en (id, std::move(cmap));
            en.run();
            break;
        }
    }

}

uh::cluster::cluster_map init_cluster_map (const uh::cluster::role role, const int id, const uh::cluster::cluster_config& cluster_conf) {
    uh::cluster::cluster_map cmap (cluster_conf);

    if (role == uh::cluster::ENTRY_NODE and id == 0) { // master
        cmap.broadcast_init();
    }
    else {
        cmap.send_recv_roles(role, id);
    }
    return cmap;
}

uh::cluster::role get_role (const std::string_view& role_str) {
    if (role_str == "dn") {
        return uh::cluster::DATA_NODE;
    }
    else if (role_str == "dd") {
        return uh::cluster::DEDUPE_NODE;
    }
    else if (role_str == "dr") {
        return uh::cluster::DIRECTORY_NODE;
    }
    else if (role_str == "en") {
        return uh::cluster::ENTRY_NODE;
    }
    else {
        throw std::invalid_argument ("Invalid role!");
    }
}

int main (int argc, char* args[]) {
    if (argc != 3) {
        throw std::invalid_argument("Usage: uh-cluster <role> <id>");
    }

    uh::log::config lc {
        .sinks = {
            uh::log::sink_config {
                .type = uh::log::sink_type::cout
            },
            uh::log::sink_config {
                .type = uh::log::sink_type::file,
                .filename = "log.log"
            }
        }
    };

    uh::log::init(lc);
    LOG_INFO() << "starting " << PROJECT_NAME << " " << PROJECT_VERSION;

    const auto role_str = std::string_view(args[1]);   // en, dd, dr, dn
    char* end;
    const auto id = static_cast <int> (std::strtol(args[2], &end, 10));

    const auto role = get_role (role_str);
    const auto cluster_conf = make_cluster_config();

    uh::cluster::cluster_map cmap = init_cluster_map (role, id, cluster_conf);

    execute_role (role, id, std::move (cmap));
}
