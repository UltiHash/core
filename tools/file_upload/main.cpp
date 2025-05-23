#include "common/network/messenger.h"
#include "common/types/common_types.h"
#include "common/utils/random.h"
#include <CLI/CLI.hpp>
#include <filesystem>
#include <future>

using namespace uh::cluster;
namespace asio = boost::asio;

struct config {
    std::filesystem::path path;

    std::string host = "127.0.0.1";
    uint16_t port = 9300;
    unsigned connections = 4;
    std::size_t buffer_size = 4 * MEBI_BYTE;
};

class uploader {
public:
    uploader(const config& c)
        : m_total_size(0ull),
          m_effective_size(0ull),
          m_conf(c),
          m_running(0) {
        for (unsigned t = 0; t < m_conf.connections; ++t) {
            m_threads.emplace_back([this]() { worker(); });
        }
    }

    ~uploader() {
        try {
            wait();
        } catch (...) {
        }
    }

    void push(const std::filesystem::path& file) {
        std::cout << "pushing " << file << "\n";
        m_jobs.push_back(file);
    }

    auto start() {
        m_start = true;
        m_cv.notify_all();
    }

    void wait() {
        for (auto& t : m_threads) {
            t.join();
        }
    }

    std::size_t effective_size() const { return m_effective_size; }
    std::size_t total_size() const { return m_total_size; }

    std::size_t running() const { return m_running; }

private:
    void worker() {
        asio::ip::tcp::endpoint endpoint(asio::ip::make_address(m_conf.host),
                                         m_conf.port);
        asio::ip::tcp::socket sock(m_ioc);
        sock.connect(endpoint);

        ++m_running;

        {
            std::unique_lock<std::mutex> l(m_m);
            m_cv.wait(l, [this]() { return m_start; });
        }

        while (true) {
            std::filesystem::path job;

            {
                std::unique_lock<std::mutex> l(m_mtx_jobs);
                if (m_jobs.empty()) {
                    break;
                }

                job = m_jobs.front();
                m_jobs.pop_front();
            }

            try {
                upload(job, sock);
            } catch (const std::exception& e) {
                std::cerr << "failure uploading file " << job << ": "
                          << e.what() << "\n";
            }
        }

        --m_running;
    }

    void upload(const std::filesystem::path& file,
                asio::ip::tcp::socket& sock) {

        std::ifstream in(file);
        std::vector<char> buffer(m_conf.buffer_size);

        while (!in.eof()) {
            in.read(buffer.data(), buffer.size());
            std::size_t count = in.gcount();
            if (count == 0) {
                break;
            }

            message_type type = DEDUPLICATOR_REQ;
            std::vector<asio::const_buffer> send_buffers{
                {&type, sizeof(type)},
                {&count, sizeof(count)},
                {buffer.data(), count}};
            write(sock, send_buffers);

            messenger_core::header h{};
            std::vector<asio::mutable_buffer> recv_buffers{
                {&h.type, sizeof h.type}, {&h.size, sizeof h.size}};
            read(sock, recv_buffers);

            dedupe_response dedupe_resp;
            dedupe_resp.addr = address(address::allocated_elements(
                h.size - sizeof(dedupe_resp.effective_size)));
            std::vector<asio::mutable_buffer> buffers{
                boost::asio::buffer(&dedupe_resp.effective_size,
                                    sizeof dedupe_resp.effective_size),
                boost::asio::buffer(dedupe_resp.addr.fragments),
            };

            read(sock, buffers);
            if (h.type != SUCCESS) [[unlikely]] {
                throw std::runtime_error("unsuccessful integration");
            }

            m_total_size += count;
            m_effective_size += dedupe_resp.effective_size;
        }
    }

    asio::io_context m_ioc;
    std::mutex m_mtx_jobs;
    std::list<std::filesystem::path> m_jobs;
    std::atomic<size_t> m_total_size;
    std::atomic<size_t> m_effective_size;
    config m_conf;

    std::mutex m_m;
    std::condition_variable m_cv;
    bool m_start = false;
    std::vector<std::thread> m_threads;
    std::atomic<unsigned> m_running;
};

std::optional<config> read_config(int argc, char** argv) {
    CLI::App app("Upload test");
    argv = app.ensure_utf8(argv);

    config rv;

    app.add_option("--host,-H", rv.host, "host of deduplicator service");
    app.add_option("--port,-p", rv.port, "port of deduplicator service");
    app.add_option("--connections,-C", rv.connections, "number of connections");
    app.add_option("--buffer,-B", rv.buffer_size, "size of buffer");
    app.add_option("path", rv.path, "path to upload");

    try {
        app.parse(argc, argv);
    } catch (const CLI::Success& e) {
        app.exit(e);
        return {};
    }

    return rv;
}

int main(int argc, char** argv) {

    try {
        auto cfg = read_config(argc, argv);
        if (!cfg) {
            return 0;
        }

        uploader up(*cfg);

        std::size_t total = 0ull;

        for (const auto& entry :
             std::filesystem::recursive_directory_iterator(cfg->path)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            up.push(entry);
            total += std::filesystem::file_size(entry);
        }

        const auto start = std::chrono::steady_clock::now();
        up.start();

        auto end = std::chrono::steady_clock::now();
        while (up.running() > 0) {
            std::chrono::duration<double> time = end - start;

            std::size_t total_mb = up.total_size() / MEBI_BYTE;

            if (time.count() > 0) {
                std::cout << "\ruploading ... " << up.total_size() << "/"
                          << total << " bytes ("
                          << (100 * up.total_size() / total) << "%)"
                          << ", throughput: " << total_mb / time.count()
                          << " MB/s"
                          << ", time " << time.count() << " s";
            } else {
                std::cout << "\ruploading ... " << up.total_size() << "/"
                          << total << " bytes";
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            end = std::chrono::steady_clock::now();
        }

        up.wait();

        std::chrono::duration<double> time = end - start;
        auto total_mb = up.total_size() / MEBI_BYTE;

        std::cout << "total size: " << up.total_size() << "\n";
        std::cout << "effective size: " << up.effective_size() << "\n";
        std::cout << "time required: " << time.count() << " s\n";
        std::cout << "bandwidth: " << total_mb / time.count() << " MB/s\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
