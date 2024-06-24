#include "connection.h"
#include "common/debug/debug.h"

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
}

coro<std::optional<row>> connection::exec(const std::string& query) {
    LOG_CORO_CONTEXT();

    co_await cancel();

    if (!PQsendQuery(m_ptr.get(), query.c_str())) {
        throw_error_message();
    }

    co_return co_await next();
}

std::optional<row> connection::raw_exec(const std::string& query) {
    LOG_CORO_CONTEXT();
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
    LOG_CORO_CONTEXT();
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
    LOG_CORO_CONTEXT();
    m_result.reset();

    PGresult* result = nullptr;
    do {
        co_await wait();
        result = PQgetResult(m_ptr.get());
        PQclear(result);
    } while (result != nullptr);
}

coro<void> connection::wait() {
    LOG_CORO_CONTEXT();
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

} // namespace uh::cluster::db
