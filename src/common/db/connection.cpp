#include "connection.h"
#include "common/debug/debug.h"
#include <boost/asio/posix/descriptor.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace uh::cluster::db {

namespace {

void check_result(const PGresult* result) {
    switch (PQresultStatus(result)) {
    case PGRES_EMPTY_QUERY:
    case PGRES_COMMAND_OK:
    case PGRES_TUPLES_OK:
    case PGRES_COPY_OUT:
    case PGRES_COPY_IN:
    case PGRES_COPY_BOTH:
    case PGRES_SINGLE_TUPLE:
    case PGRES_PIPELINE_SYNC:
    case PGRES_PIPELINE_ABORTED:
        break;

    case PGRES_BAD_RESPONSE:
    case PGRES_NONFATAL_ERROR:
    case PGRES_FATAL_ERROR:
        throw std::runtime_error(PQresultErrorMessage(result));
    }
}

void log_raised_message(void* arg, const char* message) {
    connection* c = reinterpret_cast<connection*>(arg);
    std::string msg = message;

    if (msg.find("EXCEPTION") != std::string::npos) {
        LOG_ERROR() << c->id() << msg;
    } else if (msg.find("WARNING") != std::string::npos) {
        LOG_WARN() << c->id() << msg;
    } else if (msg.find("NOTICE") != std::string::npos) {
        LOG_WARN() << c->id() << msg;
    } else if (msg.find("INFO") != std::string::npos) {
        LOG_INFO() << c->id() << msg;
    } else if (msg.find("DEBUG") != std::string::npos) {
        LOG_DEBUG() << c->id() << msg;
    }
}

} // namespace

connection::connection(boost::asio::io_context& ioc, const connstr& cs)
    : m_ioc(ioc),
      m_ptr(PQconnectdb(cs), PQfinish),
      m_fd(m_ioc, PQsocket(m_ptr.get())) {
    if (PQstatus(m_ptr.get()) != CONNECTION_OK) {
        throw std::runtime_error("cannot connect: " + cs.printable());
    }

    if (m_fd.native_handle() == -1) {
        throw std::runtime_error("illegal file descriptor for connection");
    }

    PQsetNoticeProcessor(m_ptr.get(), log_raised_message, this);
}

coro<std::optional<row>> connection::exec(const std::string& query) {
    co_await cancel();

    if (!PQsendQuery(m_ptr.get(), query.c_str())) {
        throw_error_message();
    }

    co_return co_await next();
}

std::optional<row> connection::raw_exec(const std::string& query) {
    m_result =
        std::shared_ptr<PGresult>(PQexec(m_ptr.get(), query.c_str()), PQclear);
    m_row = 0;

    if (PQntuples(m_result.get()) == 0) {
        m_result.reset();
        return std::nullopt;
    }

    return row(m_result, 0);
}

coro<std::optional<row>> connection::next() {
    if (!m_result || m_row >= PQntuples(m_result.get())) {

        co_await wait();

        auto result = PQgetResult(m_ptr.get());

        if (result == nullptr) {
            m_result.reset();
            m_row = 0;
            co_return std::nullopt;
        }

        check_result(result);
        if (PQntuples(result) == 0) {
            PQclear(result);
            m_result.reset();
            m_row = 0;
            co_return std::nullopt;
        }

        m_result = std::shared_ptr<PGresult>(result, PQclear);
        m_row = 0;
    }

    co_return row(m_result, m_row++);
}

coro<void> connection::cancel() {
    m_result.reset();

    PGresult* result = nullptr;
    do {
        co_await wait();
        result = PQgetResult(m_ptr.get());
        PQclear(result);
    } while (result != nullptr);
}

std::string connection::id() const {
    std::stringstream s;
    s << "[pg:" << std::hex << std::setfill('0') << std::setw(16)
      << reinterpret_cast<std::size_t>(this) << "] ";
    return s.str();
}

coro<void> connection::wait() {
    while (PQisBusy(m_ptr.get())) {
        co_await m_fd.async_wait(
            boost::asio::posix::descriptor::wait_type::wait_read,
            boost::asio::use_awaitable);

        if (PQconsumeInput(m_ptr.get()) == 0) {
            throw_error_message();
        }
    }
}

[[noreturn]] void connection::throw_error_message() {
    throw std::runtime_error(PQerrorMessage(m_ptr.get()));
}

void connection::append_args(std::span<char> s,
                             std::vector<const char*>& values,
                             std::vector<int>& lengths,
                             std::vector<int>& format,
                             std::list<std::string>&) {
    values.push_back(s.data());
    lengths.push_back(s.size());
    format.push_back(1);
}

void connection::append_args(std::string s, std::vector<const char*>& values,
                             std::vector<int>& lengths,
                             std::vector<int>& format,
                             std::list<std::string>& mem) {
    auto& buffer = mem.emplace_back(std::move(s));
    values.push_back(buffer.data());
    lengths.push_back(buffer.size());
    format.push_back(0);
}

void connection::append_args(std::string_view s,
                             std::vector<const char*>& values,
                             std::vector<int>& lengths,
                             std::vector<int>& format,
                             std::list<std::string>&) {
    values.push_back(s.data());
    lengths.push_back(s.size());
    format.push_back(0);
}

void connection::append_args(std::size_t n, std::vector<const char*>& values,
                             std::vector<int>& lengths,
                             std::vector<int>& format,
                             std::list<std::string>& mem) {
    auto& buffer = mem.emplace_back(std::to_string(n));
    values.push_back(buffer.data());
    lengths.push_back(buffer.size());
    format.push_back(0);
}

} // namespace uh::cluster::db
