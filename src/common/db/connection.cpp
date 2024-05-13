#include "connection.h"

namespace uh::cluster::db {

connection::connection(const std::string& connstr)
    : m_ptr(PQconnectdb(connstr.c_str()), PQfinish) {
    if (PQstatus(m_ptr.get()) != CONNECTION_OK) {
        throw std::runtime_error("cannot connect: " + connstr);
    }
}

result connection::exec(const std::string& query) {
    auto res = std::unique_ptr<PGresult, void (*)(PGresult*)>(
        PQexec(m_ptr.get(), query.c_str()), PQclear);

    check_result(res.get());
    return result(std::move(res));
}

result connection::execp(const std::string& query,
                         const std::vector<std::string>& args) {
    std::vector<const char*> values;
    std::vector<int> lengths;
    std::vector<int> format;
    for (const auto& a : args) {
        values.push_back(a.c_str());
        lengths.push_back(a.size());
        format.push_back(1);
    }

    auto res = std::unique_ptr<PGresult, void (*)(PGresult*)>(
        PQexecParams(m_ptr.get(), query.c_str(), args.size(), nullptr,
                     values.data(), lengths.data(), format.data(), 0),
        PQclear);

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
