//
// Created by masi on 7/24/23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "global data view tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/asio/awaitable.hpp>

#include <filesystem>

#include "common/cluster_config.h"
#include "network/cluster_map.h"
#include "data_node/data_node.h"
#include "dedupe_node/dedupe_node.h"


// ------------- Tests Suites Follow --------------

namespace uh::cluster {

template <typename T>
using coro =  boost::asio::awaitable <T>;

// ---------------------------------------------------------------------

struct config_fixture
{
    static std::filesystem::path get_root_path() {
        return "_tmp_test/dr/";
    }

    static void cleanup () {
        std::filesystem::remove_all("_tmp_test");
    }

    static std::unordered_map <uh::cluster::role, std::map <int, std::string>> get_cluster_roles() {
        return {
                {uh::cluster::role::DATA_NODE, {{0, "127.0.0.1"}, {1, "127.0.0.1"}, {2, "127.0.0.1"}}},
                {uh::cluster::role::DEDUPE_NODE, {{0, "127.0.0.1"}}},
        };
    }

    uh::cluster::entry_node_config make_entry_node_config(int i) {
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

    uh::cluster::data_node_config make_data_node_config(int i) {
        return {
                .directory = "ultihash-root/dn",
                        .hole_log = "ultihash-root/dn/log",
                .min_file_size = 1ul * 1024ul * 1024ul * 1024ul,
                .max_file_size = 4ul * 1024ul * 1024ul * 1024ul,
                .max_data_store_size = 64ul * 1024ul * 1024ul * 1024ul,
                .server_conf = {
                        .threads = 4,
                        .port = static_cast<uint16_t>(9000 + i),
                },
        };
    }

    uh::cluster::directory_node_config make_directory_node_config(int i) {
        return {
                .server_conf = {
                        .threads = 4,
                        .port = static_cast<uint16_t>(9020 + i),
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

    uh::cluster::dedupe_config make_dedupe_node_config(int i) {
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
                                .file  = "ultihash-root/dd/set",
                                .init_size = 1ul * 1024ul * 1024ul * 1024ul,
                        }
                },
        };
    }

    uh::cluster::cluster_config make_cluster_config(int i) {
        return {
                .init_process_count = 4,
                .ec_algorithm = XOR,
                .recovery_chunk_size = 1024ul,
                .data_node_conf = make_data_node_config(i),
                .dedupe_node_conf = make_dedupe_node_config(i),
                .directory_node_conf = make_directory_node_config(i),
                .entry_node_conf = make_entry_node_config(i),
        };
    }

    uh::cluster::cluster_map get_cluster_map() {
        auto conf = make_cluster_config(0);
        std::unordered_map<role, std::map<int, std::string>> roles = get_cluster_roles();
        return {conf, roles};
    }

};


// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE (test_dedup_write_cache_setup, config_fixture)
{
    cluster_config config0= make_cluster_config(0);
    cluster_map cmap0(config0, get_cluster_roles());
    uh::cluster::data_node dn0 (0,  std::move(cmap0));
    std::thread dn0_t([&dn0]() { dn0.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    cluster_config config1= make_cluster_config(1);
    cluster_map cmap1(config1, get_cluster_roles());
    uh::cluster::data_node dn1 (1,  std::move(cmap1));
    std::thread dn1_t([&dn1]() { dn1.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    cluster_config config2= make_cluster_config(2);
    cluster_map cmap2(config2, get_cluster_roles());
    uh::cluster::data_node dn2 (2,  std::move(cmap2));
    std::thread dn2_t([&dn2]() { dn2.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    cluster_config config3 = make_cluster_config(0);
    cluster_map cmap3(config3, get_cluster_roles());
    uh::cluster::dedupe_node dd0 (0,  std::move(cmap3), true);
    std::thread dd0_t([&dd0]() { dd0.run(); });

    // do stuff
    global_data_view& data_view = dd0.get_global_data_view();
    auto addr = co_await data_view.write("Hallo dies ist ein sehr langer Text mit vielen vielen Duplikaten. Hallo dies ist ein sehr langer Text mit vielen vielen Duplikaten.");

    dd0.stop();
    dd0_t.join();
    dn0.stop();
    dn0_t.join();
    dn1.stop();
    dn1_t.join();
    dn2.stop();
    dn2_t.join();



}


// ---------------------------------------------------------------------

} // end namespace uh::cluster

