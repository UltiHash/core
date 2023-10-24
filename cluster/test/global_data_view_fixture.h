//
// Created by max on 24.10.23.
//

#ifndef CORE_TEST_GLOBAL_DATA_VIEW_FIXTURE_H
#define CORE_TEST_GLOBAL_DATA_VIEW_FIXTURE_H

#include "common/cluster_config.h"
#include "network/cluster_map.h"
#include "data_node/data_node.h"
#include "dedupe_node/dedupe_node.h"

namespace uh::cluster {

    template <typename T>
    using coro =  boost::asio::awaitable <T>;

    class global_data_view_fixture
    {
    public:

        cluster_config config_dn0 = make_cluster_config(0);
        cluster_config config_dn1 = make_cluster_config(1);
        cluster_config config_dn2 = make_cluster_config(2);
        cluster_config config_dd0 = make_cluster_config(0);
        std::optional<cluster_map> cmap_dn0;
        std::optional<cluster_map> cmap_dn1;
        std::optional<cluster_map> cmap_dn2;
        std::optional<cluster_map> cmap_dd0;
        std::thread thread_dn0;
        std::thread thread_dn1;
        std::thread thread_dn2;
        std::thread thread_dd0;
        std::unique_ptr<data_node> dn0;
        std::unique_ptr<data_node> dn1;
        std::unique_ptr<data_node> dn2;
        std::unique_ptr<dedupe_node> dd0;


        void setup() {
            cmap_dn0.emplace(cluster_map(config_dn0, get_cluster_roles()));
            cmap_dn1.emplace(cluster_map(config_dn1, get_cluster_roles()));
            cmap_dn2.emplace(cluster_map(config_dn2, get_cluster_roles()));
            cmap_dd0.emplace(cluster_map(config_dd0, get_cluster_roles()));

            dn0 = std::make_unique<data_node>(0,  std::move(cmap_dn0.value()));
            thread_dn0 = std::thread([&]() { dn0->run(); });
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            dn1 = std::make_unique<data_node>(1,  std::move(cmap_dn1.value()));
            thread_dn1 = std::thread([&]() { dn1->run(); });
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            dn2 = std::make_unique<data_node>(2,  std::move(cmap_dn2.value()));
            thread_dn2 = std::thread([&]() { dn2->run(); });
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            dd0 = std::make_unique<dedupe_node>(0,  std::move(cmap_dd0.value()), true);
            thread_dd0 = std::thread([&]() { dd0->run(); });
        }


        static std::filesystem::path get_root_path() {
            return "/tmp/uh/";
        }

        void teardown() {
            dd0->stop();
            thread_dd0.join();
            dn0->stop();
            thread_dn0.join();
            dn1->stop();
            thread_dn1.join();
            dn2->stop();
            thread_dn2.join();

            dd0.reset(nullptr);
            dn0.reset(nullptr);
            dn1.reset(nullptr);
            dn2.reset(nullptr);

            sleep(2);

            std::filesystem::remove_all(get_root_path());

        }

        static std::unordered_map <uh::cluster::role, std::map <int, std::string>> get_cluster_roles() {
            return {
                    {uh::cluster::role::DATA_NODE, {{0, "127.0.0.1"}, {1, "127.0.0.1"}, {2, "127.0.0.1"}}},
                    {uh::cluster::role::DEDUPE_NODE, {{0, "127.0.0.1"}}},
            };
        }

        static uh::cluster::entry_node_config make_entry_node_config(int i) {
            return {
                    .internal_server_conf = {
                            .threads = 4,
                            .port = static_cast<uint16_t>(9030 + i),
                    },
                    .rest_server_conf = {
                            .threads = 4,
                            .port = static_cast<uint16_t>(9040 + i),
                    },
                    .dedupe_node_connection_count = 2,
                    .directory_connection_count = 2,
            };
        }

        static uh::cluster::data_node_config make_data_node_config(int i) {
            return {
                    .directory = get_root_path() / "dn",
                    .hole_log = get_root_path() / "dn/log",
                    .min_file_size = 1ul * 1024ul * 1024ul * 1024ul,
                    .max_file_size = 4ul * 1024ul * 1024ul * 1024ul,
                    .max_data_store_size = 64ul * 1024ul * 1024ul * 1024ul,
                    .server_conf = {
                            .threads = 4,
                            .port = static_cast<uint16_t>(9000 + i),
                    },
            };
        }

        static uh::cluster::directory_node_config make_directory_node_config(int i) {
            return {
                    .server_conf = {
                            .threads = 4,
                            .port = static_cast<uint16_t>(9020 + i),
                    },
                    .directory_conf = {
                            .root = get_root_path() / "dr",
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

        static uh::cluster::dedupe_config make_dedupe_node_config(int i) {
            return {
                    .min_fragment_size = 32,
                    .max_fragment_size = 8 * 1024,
                    .server_conf = {
                            .threads = 1,
                            .port = static_cast<uint16_t>(9010 + i),
                    },
                    .data_node_connection_count = 2,
                    .set_conf = {
                            .set_minimum_free_space = 1ul * 1024ul * 1024ul * 1024ul,
                            .max_empty_hole_size = 1ul * 1024ul * 1024ul * 1024ul,
                            .key_store_config = {
                                    .file  = get_root_path() / "dd/set",
                                    .init_size = 1ul * 1024ul * 1024ul * 1024ul,
                            }
                    },
            };
        }

        static uh::cluster::cluster_config make_cluster_config(int i) {
            return {
                    .init_process_count = 4,
                    .ec_algorithm = XOR,
                    .recovery_chunk_size = 1 * 1024ul * 1024ul * 1024ul,
                    .data_node_conf = make_data_node_config(i),
                    .dedupe_node_conf = make_dedupe_node_config(i),
                    .directory_node_conf = make_directory_node_config(i),
                    .entry_node_conf = make_entry_node_config(i),
            };
        }

    };

} // end namespace uh::cluster
#endif //CORE_TEST_GLOBAL_DATA_VIEW_FIXTURE_H
