//
// Created by max on 24.10.23.
//

#ifndef CORE_TEST_GLOBAL_DATA_VIEW_FIXTURE_H
#define CORE_TEST_GLOBAL_DATA_VIEW_FIXTURE_H

#include "common/cluster_config.h"
#include "network/cluster_map.h"
#include "data_node/data_store_service.h"
#include "dedupe_node/deduplication_service.h"
#include "directory_service/directory_service.h"

namespace uh::cluster {

    template <typename T>
    using coro =  boost::asio::awaitable <T>;

    class cluster_fixture
    {
    public:

        std::vector <std::shared_ptr <service_interface>> m_nodes;
        std::vector <std::shared_ptr <service_interface>> m_dedupe_nodes;
        std::vector <std::shared_ptr <service_interface>> m_directory_nodes;
        std::vector <std::shared_ptr <service_interface>> m_data_nodes;
        std::vector <std::thread> m_threads;
        boost::asio::io_context m_ioc;

        static std::unordered_map <uh::cluster::role, std::map <int, std::string>>
        get_cluster_roles(int data_nodes, int dedupe_nodes, int directory_nodes) {
            std::unordered_map <uh::cluster::role, std::map <int, std::string>> roles;
            for (int i = 0; i < data_nodes; ++i) {
                roles[DATASTORE_SERVICE].emplace(i, "127.0.0.1");
            }
            for (int i = 0; i < dedupe_nodes; ++i) {
                roles[DEDUPLICATION_SERVICE].emplace(i, "127.0.0.1");
            }
            for (int i = 0; i < directory_nodes; ++i) {
                roles[DIRECTORY_SERVICE].emplace(i, "127.0.0.1");
            }
            return roles;
        }

        deduplication_service& get_dedupe_node (int i) {
            return dynamic_cast <deduplication_service&> (*m_dedupe_nodes.at(i));
        }

        directory_service_handler& get_directory_node (int i) {
            return dynamic_cast <directory_service_handler&> (*m_directory_nodes.at(i));
        }

        data_store_service& get_data_node (int i) {
            return dynamic_cast <data_store_service&> (*m_data_nodes.at(i));
        }

        void setup (int data_nodes, int dedupe_nodes, int directory_nodes, ec_type ec) {

            teardown();

            turn_on(data_nodes, dedupe_nodes, directory_nodes, ec);
        }

        void shut_down () {
            for (auto& node: m_dedupe_nodes) {
                auto& dedupe = dynamic_cast <deduplication_service&> (*node);
                if (dedupe.get_global_data_view().get_data_node_count() > 0) {
                    try {
                        dedupe.get_global_data_view().stop();
                    } catch (std::exception& e) {
                        std::cerr << e.what() << std::endl;
                    }
                }
            }

            for (const auto& node: m_nodes) {
                node->stop();
            }

            for (auto& thread: m_threads) {
                thread.join();
            }

            m_threads.clear();
            m_nodes.clear();
            m_dedupe_nodes.clear();
            m_data_nodes.clear();
            m_directory_nodes.clear();

            m_ioc.stop();
            m_ioc.restart();
            sleep(1);
        }

        void turn_on (int data_nodes, int dedupe_nodes, int directory_nodes, ec_type ec) {
            std::exception_ptr excp_ptr;

            const auto cluster_roles = get_cluster_roles(data_nodes, dedupe_nodes, directory_nodes);

            for (const auto &role: cluster_roles) {
                for (const auto &role_nodes: role.second) {
                    auto config = make_cluster_config(role_nodes.first, ec);
                    auto cmap = cluster_map(std::move (config), cluster_roles);
                    switch (role.first) {
                        case DATASTORE_SERVICE:
                            m_nodes.emplace_back(std::make_shared<data_store_service>(role_nodes.first, std::move(cmap)));
                            m_data_nodes.emplace_back(m_nodes.back());
                            break;
                        case DEDUPLICATION_SERVICE:
                            m_nodes.emplace_back(std::make_shared<deduplication_service>(role_nodes.first, std::move(cmap), true));
                            m_dedupe_nodes.emplace_back(m_nodes.back());
                            break;
                        case DIRECTORY_SERVICE:
                            m_nodes.emplace_back(std::make_shared<directory_service_handler>(role_nodes.first, std::move(cmap), true));
                            m_directory_nodes.emplace_back(m_nodes.back());
                            break;
                        default:
                            throw std::runtime_error("no support for the role type");
                    }
                }
            }

            for (const auto &node: m_data_nodes) {
                m_ioc.post([&node] { node->run(); });
                m_threads.emplace_back([&] {
                    try { m_ioc.run(); } catch (std::exception& e) {
                        excp_ptr = std::current_exception();
                    }
                });
            }

            std::this_thread::sleep_for(std::chrono::seconds (4));

            for (const auto &node: m_dedupe_nodes) {
                m_ioc.post([&node] { node->run(); });
                m_threads.emplace_back([&] {
                    try { m_ioc.run(); } catch (std::exception& e) {
                        excp_ptr = std::current_exception();
                    }
                });
            }

            std::this_thread::sleep_for(std::chrono::seconds (1));

            for (const auto &node: m_directory_nodes) {
                m_ioc.post([&node] { node->run(); });
                m_threads.emplace_back([&] {
                    try { m_ioc.run(); } catch (std::exception& e) {
                        excp_ptr = std::current_exception();
                    }
                });
            }

            std::this_thread::sleep_for(std::chrono::seconds (3));

            if (excp_ptr) {
                try {
                    std::rethrow_exception(excp_ptr);
                }
                catch (std::exception& e) {
                    shut_down();
                    throw e;
                }
            }
        }

        void teardown () {

            shut_down();
            sleep(1);

            std::filesystem::remove_all(get_root_path());
        }

        static void destroy_data_node (int i) {
            std::string dn_dir = "dn" + std::to_string(i);

            std::filesystem::remove_all(get_root_path() / dn_dir);
        }

        static std::filesystem::path get_root_path() {
            return "/tmp/uh/";
        }

        static uh::cluster::entry_node_config make_entry_node_config(int i) {
            return {
                    .rest_server_conf = {
                            .threads = 4,
                            .port = static_cast<uint16_t>(8500 + i),
                    },
                    .dedupe_node_connection_count = 2,
                    .directory_connection_count = 2,
            };
        }

        static uh::cluster::data_node_config make_data_node_config(int i) {
            std::string dn_dir = "dn" + std::to_string(i);
            return {
                    .directory = get_root_path() / dn_dir,
                    .hole_log = get_root_path() / dn_dir / "log",
                    .min_file_size = 1ul * 1024ul,
                    .max_file_size = 2ul * 1024ul,
                    .max_data_store_size = 64ul * 1024ul,
                    .server_conf = {
                            .threads = 4,
                            .port = static_cast<uint16_t>(8600 + i),
                            .metrics_bind_address = "0.0.0.0:" + std::to_string(static_cast<uint16_t>(9600 + i)),
                            .metrics_threads = 2,
                            .metrics_path = "/metrics"
                    },
            };
        }

        static uh::cluster::directory_node_config make_directory_node_config(int i) {
            std::string dr_dir = "dr" + std::to_string(i);

            return {
                    .server_conf = {
                            .threads = 4,
                            .port = static_cast<uint16_t>(8700 + i),
                            .metrics_bind_address = "0.0.0.0:" + std::to_string(static_cast<uint16_t>(9700 + i)),
                            .metrics_threads = 2,
                            .metrics_path = "/metrics"

                    },
                    .directory_conf = {
                            .root = get_root_path() / dr_dir,
                            .bucket_conf = {
                                    .min_file_size = 1024ul * 2,
                                    .max_file_size = 1024ul * 64,
                                    .max_storage_size = 1024ul * 256,
                                    .max_chunk_size = std::numeric_limits <uint32_t>::max(),
                            },
                    },
                    .data_node_connection_count = 2,
            };
        }

        static uh::cluster::dedupe_config make_dedupe_node_config(int i) {
            std::string dd_dir = "dd" + std::to_string(i);

            return {
                    .min_fragment_size = 32,
                    .max_fragment_size = 8 * 1024,
                    .server_conf = {
                            .threads = 1,
                            .port = static_cast <uint16_t> (8800 + i),
                            .metrics_bind_address = "0.0.0.0:" + std::to_string(static_cast<uint16_t>(9800 + i)),
                            .metrics_threads = 2,
                            .metrics_path = "/metrics"
                    },
                    .data_node_connection_count = 2,
                    .set_log_path = get_root_path() / "dd_set",
                    .dedupe_worker_minimum_data_size = 128ul * 1024ul,
            };
        }

        static uh::cluster::global_data_view_config make_global_data_view_config(ec_type ec) {

            return {
                    .read_cache_capacity_l1 = 200,
                    .read_cache_capacity_l2 = 100,
                    .l1_sample_size = 128,
                    .ec_algorithm = ec,
                    .recovery_chunk_size = 1 * 1024ul,
            };
        }

        static uh::cluster::cluster_config make_cluster_config(int i, ec_type ec) {
            return {
                    .init_process_count = 4,
                    .data_node_conf = make_data_node_config(i),
                    .dedupe_node_conf = make_dedupe_node_config(i),
                    .directory_node_conf = make_directory_node_config(i),
                    .entry_node_conf = make_entry_node_config(i),
                    .global_data_view_conf = make_global_data_view_config(ec)
            };
        }

        static void fill_random(char* buf, size_t size) {
            for (int i = 0; i < size; ++i) {
                buf[i] = rand()&0xff;
            }
        }

    };


} // end namespace uh::cluster
#endif //CORE_TEST_GLOBAL_DATA_VIEW_FIXTURE_H
