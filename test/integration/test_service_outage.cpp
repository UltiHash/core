#define BOOST_TEST_MODULE "test_service_outage"

#include <boost/test/unit_test.hpp>

#include <boost/process.hpp>
#include <common/etcd/utils.h>
#include <common/utils/common.h>

#include "test_config.h"

namespace bp = boost::process;
using namespace uh::cluster;

void send_signal_to_process(pid_t pid, int signal) {
    if (kill(pid, signal) == 0) {
        std::cout << "Signal " << signal << " sent to process " << pid << ".\n";
    } else {
        perror("Failed to send signal");
    }
}

struct fixture {
    fixture()
        : m_etcd{} {
        m_etcd.clear_all();
        std::this_thread::sleep_for(100ms);
    }
    ~fixture() { m_etcd.clear_all(); }

    etcd_manager m_etcd;
    std::string m_data_path = "/var/lib/uh";
};

BOOST_FIXTURE_TEST_SUITE(a_uh_cluster, fixture)

BOOST_AUTO_TEST_CASE(test_service_outage) {
    bp::environment env = boost::this_process::environment();
    env[ENV_CFG_LICENSE] = test_license_string;
    env[ENV_CFG_LOG_LEVEL] = "DEBUG";
    env["OTEL_RESOURCE_ATTRIBUTES"] = "service.name=coordinator";

    std::vector<std::string> args = {"coordinator"};
    bp::child c(UH_CLUSTER_EXECUTABLE, bp::env = env, bp::args = args,
                bp::std_out > "log.txt");

    std::cout << "Process started with PID: " << c.id() << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Simulating power outage (kill -9)...\n";

    c.terminate();
    c.wait();

    std::cout << "Process terminated.\n";
}

BOOST_AUTO_TEST_CASE(test_signal_handling) {
    bp::child c(UH_CLUSTER_EXECUTABLE);

    std::cout << "Process started with PID: " << c.id() << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Sending SIGTERM to process...\n";
    send_signal_to_process(c.id(), SIGTERM);

    c.wait();
    std::cout << "Process terminated.\n";
}

BOOST_AUTO_TEST_SUITE_END()
