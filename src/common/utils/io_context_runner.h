#ifndef IO_CONTEXT_RUNNER_H
#define IO_CONTEXT_RUNNER_H
#include "common/telemetry/log.h"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include <thread>

namespace uh::cluster {

class io_context_runner {
public:
    io_context_runner(boost::asio::io_context& ioc, std::size_t thread_count)
        : m_ioc(ioc),
          m_work_guard(boost::asio::make_work_guard(m_ioc)) {

        m_threads.reserve(thread_count);
        for (std::size_t i = 0; i < thread_count; ++i) {
            m_threads.emplace_back([this]() {
                try {
                    m_ioc.run();
                } catch (const std::exception&) {
                    m_excp_ptr = std::current_exception();
                }
            });
        }
    }

    void stop() {
        m_ioc.stop();

        for (auto& thread : m_threads) {
            thread.join();
        }
        if (m_excp_ptr) {
            std::rethrow_exception(m_excp_ptr);
        }
    }

    ~io_context_runner() {
        try {
            stop();
        } catch (const std::exception& e) {
            LOG_ERROR() << "exception thrown in io_context_runner destructor: "
                        << e.what();
        }
    }

private:
    boost::asio::io_context& m_ioc;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        m_work_guard;
    std::vector<std::thread> m_threads;
    std::exception_ptr m_excp_ptr;
};

} // end namespace uh::cluster

#endif // IO_CONTEXT_RUNNER_H
