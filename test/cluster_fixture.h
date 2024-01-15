//
// Created by max on 24.10.23.
//

#ifndef CORE_TEST_GLOBAL_DATA_VIEW_FIXTURE_H
#define CORE_TEST_GLOBAL_DATA_VIEW_FIXTURE_H

#include "common/utils/cluster_config.h"
#include "storage/storage.h"
#include "deduplicator/deduplicator.h"
#include "directory/directory.h"
#include "common/utils/common.h"

namespace uh::cluster {

    template <typename T>
    using coro =  boost::asio::awaitable <T>;

    class cluster_fixture
    {
    public:


        cluster_fixture() :
                m_registry_url("http://127.0.0.1:2379"),
                m_registry(uh::cluster::STORAGE_SERVICE, UINT64_MAX, m_registry_url)
        {
            m_registration = m_registry.register_testing();
        }


        const std::string m_registry_url = "http://127.0.0.1:2379";
        service_registry m_registry;
        std::unique_ptr<service_registry::registration> m_registration;

        std::vector <std::shared_ptr <service_interface>> m_deduplicator_instances;
        std::vector <std::shared_ptr <service_interface>> m_directory_instances;
        std::vector <std::shared_ptr <service_interface>> m_storage_instances;
        std::vector <std::thread> m_threads;
        boost::asio::io_context m_ioc;

        deduplicator& get_deduplicator_service (int i) {
            return dynamic_cast <deduplicator&> (*m_deduplicator_instances.at(i));
        }

        directory& get_directory_service (int i) {
            return dynamic_cast <directory&> (*m_directory_instances.at(i));
        }

        storage& get_storage_service (int i) {
            return dynamic_cast <storage&> (*m_storage_instances.at(i));
        }

        void setup (int data_nodes, int dedupe_nodes, int directory_nodes, ec_type ec) {

            teardown();

            turn_on(data_nodes, dedupe_nodes, directory_nodes, ec);
            sleep(15);
        }

        void shut_down () {
            for (auto& node: m_deduplicator_instances) {
                auto& dedupe = dynamic_cast <deduplicator&> (*node);
                if (dedupe.get_global_data_view().get_data_node_count() > 0) {
                    try {
                        dedupe.get_global_data_view().stop();
                    } catch (std::exception& e) {
                        std::cerr << e.what() << std::endl;
                    }
                }
            }

            for (const auto& node: m_deduplicator_instances) {
                node->stop();
            }

            for (const auto& node: m_directory_instances) {
                node->stop();
            }

            for (const auto& node: m_storage_instances) {
                node->stop();
            }


            for (auto& thread: m_threads) {
                thread.join();
            }

            m_threads.clear();
            m_deduplicator_instances.clear();
            m_storage_instances.clear();
            m_directory_instances.clear();

            m_ioc.stop();
            m_ioc.restart();
            sleep(1);
        }

        void turn_on (int data_nodes, int dedupe_nodes, int directory_nodes, ec_type ec) {
            std::exception_ptr excp_ptr;

            for (int i = 0; i < data_nodes; ++i) {
                m_storage_instances.emplace_back(std::make_shared<storage>(i, m_registry_url));
            }
            for (int i = 0; i < dedupe_nodes; ++i) {
                m_deduplicator_instances.emplace_back(std::make_shared<deduplicator>(i, m_registry_url));
            }
            for (int i = 0; i < directory_nodes; ++i) {
                m_directory_instances.emplace_back(std::make_shared<directory>(i, m_registry_url));
            }


            for (const auto &node: m_storage_instances) {
                m_ioc.post([&node] { node->run(); });
                m_threads.emplace_back([&] {
                    try { m_ioc.run(); } catch (std::exception& e) {
                        excp_ptr = std::current_exception();
                    }
                });
            }

            for (const auto &node: m_deduplicator_instances) {
                m_ioc.post([&node] { node->run(); });
                m_threads.emplace_back([&] {
                    try { m_ioc.run(); } catch (std::exception& e) {
                        excp_ptr = std::current_exception();
                    }
                });
            }

            for (const auto &node: m_directory_instances) {
                m_ioc.post([&node] { node->run(); });
                m_threads.emplace_back([&] {
                    try { m_ioc.run(); } catch (std::exception& e) {
                        excp_ptr = std::current_exception();
                    }
                });
            }

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

        static std::filesystem::path get_root_path() {
            return "/tmp/uh/";
        }

        static void fill_random(char* buf, size_t size) {
            for (int i = 0; i < size; ++i) {
                buf[i] = rand()&0xff;
            }
        }

    };


} // end namespace uh::cluster
#endif //CORE_TEST_GLOBAL_DATA_VIEW_FIXTURE_H
