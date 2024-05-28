#include "connection.h"

namespace uh::cluster::db {

connection::connection(const connstr& cs)
    : m_ptr(PQconnectdb(cs), PQfinish) {
    if (PQstatus(m_ptr.get()) != CONNECTION_OK) {
        throw std::runtime_error("cannot connect: " + cs.printable());
    }
}

result connection::exec(const std::string& query) {
    auto res = std::unique_ptr<PGresult, void (*)(PGresult*)>(
        PQexec(m_ptr.get(), query.c_str()), PQclear);

    check_result(res.get());
    return result(std::move(res));
}

void connection::check_result(const PGresult* result) {
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

} // namespace uh::cluster::db
