//
// Created by masi on 7/17/23.
//

#include <system_error>
#include "src/cluster_config.h"
#include "src/data_node.h"
#include "src/dedupe_job.h"
#include "src/phonebook_job.h"
#include "src/entry_job.h"
#include "cluster_map.h"

uh::cluster::cluster_config make_cluster_config () {
    return {
        .init_process_count = 6,
        .broadcast_port = 8079,
        .broadcast_threads = 4,
    };
}

uh::cluster::server_config make_internal_server_config () {
    return {
            .threads = 10,
            .port = 8081,
            .buffer_size = 1024 * 1024,
    };
}

uh::cluster::global_data_config make_global_data_config () {
    return {
            .max_data_store_size =  64ul * 1024ul * 1024ul * 1024ul,
    };
}

// config roles

uh::cluster::entry_node_config make_entry_node_config () {
    return {
        .internal_server_conf = make_internal_server_config(),
        .rest_server_conf = {
                .threads = 10,
                .port = 8080,
                .buffer_size = 1024 * 1024,
        }
    };
}

uh::cluster::data_node_config make_data_node_config () {
    return {
        .directory = "root/dn",
        .hole_log = "root/dn/log",
        .min_file_size = 1ul * 1024ul * 1024ul * 1024ul,
        .max_file_size = 4ul * 1024ul * 1024ul * 1024ul,
        .max_data_store_size = 64ul * 1024ul * 1024ul * 1024ul,
        .server_conf = make_internal_server_config(),
    };
}

uh::cluster::phonebook_node_config make_phonebook_node_config () {
    return {
        .server_conf = make_internal_server_config(),
        .storage_conf = make_global_data_config(),
    };
}

uh::cluster::dedupe_config make_dedupe_node_config () {
    return {
        .min_fragment_size = 1024,
        .max_fragment_size = 4 * 1024,
        .storage_conf = make_global_data_config (),
        .server_conf = make_internal_server_config(),
        .set_conf = {
                .set_minimum_free_space = 1ul * 1024ul * 1024ul * 1024ul,
                .max_empty_hole_size = 1ul * 1024ul * 1024ul * 1024ul,
                .key_store_config = {
                        .file  = "root/set",
                        .init_size = 1ul * 1024ul * 1024ul * 1024ul,
                }
        },
    };
}

void execute_role (const uh::cluster::role role, const int id, uh::cluster::cluster_map cmap) {

    switch (role) {
        case uh::cluster::DATA_NODE: {
            uh::cluster::data_node dn (id, make_data_node_config(), std::move (cmap));
            dn.run();
            break;
        }
        case uh::cluster::DEDUPE_NODE: {
            uh::cluster::dedupe_job dd (id, make_dedupe_node_config (), std::move (cmap));
            dd.run();
            break;
        }
        case uh::cluster::PHONE_BOOK_NODE: {
            uh::cluster::phonebook_job pb (id, make_phonebook_node_config(), std::move (cmap));
            pb.run();
            break;
        }
        case uh::cluster::ENTRY_NODE: {
            uh::cluster::entry_job en (id, make_entry_node_config(), std::move(cmap));
            en.run();
            break;
        }
    }

}

uh::cluster::cluster_map init_cluster_map (const uh::cluster::role role, const int id) {
    uh::cluster::cluster_map cmap (make_cluster_config());
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
    else if (role_str == "pb") {
        return uh::cluster::PHONE_BOOK_NODE;
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
    const auto role_str = std::string_view(args[1]);   // en, dd, ph, dn
    char* end;
    const auto id = static_cast <int> (std::strtol(args[2], &end, 10));

    const auto role = get_role (role_str);

    uh::cluster::cluster_map cmap = init_cluster_map (role, id);

    execute_role (role, id, std::move (cmap));

}