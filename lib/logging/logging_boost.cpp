#include "logging_boost.h"

#include <boost/shared_ptr.hpp>
#include <boost/tti/tti.hpp>
#include <boost/make_shared.hpp>
#include <boost/phoenix.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/core/null_deleter.hpp>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/sources/severity_feature.hpp>

namespace logging  = boost::log;
namespace attrs    = boost::log::attributes;
namespace expr     = boost::log::expressions;

using namespace boost::log;

BOOST_LOG_ATTRIBUTE_KEYWORD(process_id, "ProcessID", attrs::current_process_id::value_type );
BOOST_LOG_ATTRIBUTE_KEYWORD(thread_id, "ThreadID", attrs::current_thread_id::value_type );


namespace uh::log
{

namespace
{

// ---------------------------------------------------------------------

attrs::current_process_id::value_type::native_type get_native_process_id(
    logging::value_ref<attrs::current_process_id::value_type, tag::process_id> const& pid)
{
    if (pid)
    {
        return pid->native_id();
    }

    return 0;
}

// ---------------------------------------------------------------------

attrs::current_thread_id::value_type::native_type get_native_thread_id(
    logging::value_ref<attrs::current_thread_id::value_type, tag::thread_id> const& tid)
{
    if (tid)
    {
        return tid->native_id();
    }

    return 0;
}

// ---------------------------------------------------------------------

void my_formatter(logging::record_view const& rec, logging::formatting_ostream& strm)
{
    strm << "[";
    auto date_time_formatter = expr::stream << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S.%f");
    date_time_formatter(rec, strm);
    strm << "] -->\t";

    strm << logging::extract< unsigned int >("LineID", rec) << " ";
    strm << "<" << rec[logging::trivial::severity] << "> \t";

    strm << "Process/Thread(" ;
    auto process_formatter = expr::stream << boost::phoenix::bind(&get_native_process_id, process_id_type::or_none());
    process_formatter(rec,strm);
    strm << "/";
    auto thread_formatter = expr::stream << boost::phoenix::bind(&get_native_thread_id, thread_id_type::or_none());
    thread_formatter(rec,strm);
    strm << "): ";

    strm << rec[expr::smessage];
}

// ---------------------------------------------------------------------

boost::shared_ptr<std::ostream> open_file(const std::filesystem::path& path)
{
    auto rv = boost::make_shared<std::ofstream>(path);

    if (!*rv)
    {
        throw std::runtime_error(std::string("could not open file: ") + path.string());
    }

    return rv;
}

// ---------------------------------------------------------------------

boost::shared_ptr<std::ostream> open_stream(const sink_config& cfg)
{
    switch (cfg.type)
    {
        case sink_type::file: return open_file(*cfg.filename);
        case sink_type::clog: return boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter());
        case sink_type::cerr: return boost::shared_ptr<std::ostream>(&std::cerr, boost::null_deleter());
        case sink_type::cout: return boost::shared_ptr<std::ostream>(&std::cout, boost::null_deleter());
    }

    throw std::runtime_error("unsupported log sink type");
}

// ---------------------------------------------------------------------

boost::shared_ptr<sinks::sink> make_sink(const sink_config& cfg)
{
    auto sink = boost::make_shared< sinks::synchronous_sink<sinks::text_ostream_backend> >();

    sink->locked_backend()->add_stream(open_stream(cfg));
    sink->set_filter( logging::trivial::severity >= cfg.level );
    sink->set_formatter(&my_formatter);
    sink->locked_backend()->auto_flush();

    return sink;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

std::string to_string(sink_type type)
{
    switch (type)
    {
        case sink_type::file: return "sink_type::file";
        case sink_type::clog: return "sink_type::clog";
        case sink_type::cerr: return "sink_type::cerr";
        case sink_type::cout: return "sink_type::cout";
    }

    throw std::runtime_error("unsupported log sink type");
}

// ---------------------------------------------------------------------

#define RETURN_IF_MATCH(s, string, symbol) if (s == string) { return symbol; }

// ---------------------------------------------------------------------

trivial::severity_level severity_from_string(const std::string& s)
{
    RETURN_IF_MATCH(s, "TRACE", trivial::trace);
    RETURN_IF_MATCH(s, "DEBUG", trivial::debug);
    RETURN_IF_MATCH(s, "INFO", trivial::info);
    RETURN_IF_MATCH(s, "WARN", trivial::warning);
    RETURN_IF_MATCH(s, "ERROR", trivial::error);
    RETURN_IF_MATCH(s, "FATAL", trivial::fatal);

    throw std::runtime_error("unsupported log level type: " + s);
}

// ---------------------------------------------------------------------

std::string to_string(boost::log::trivial::severity_level level)
{
    switch (level)
    {
        case trivial::trace: return "TRACE";
        case trivial::debug: return "DEBUG";
        case trivial::info: return "INFO";
        case trivial::warning: return "WARN";
        case trivial::error: return "ERROR";
        case trivial::fatal: return "FATAL";
    }

    throw std::runtime_error("unsupported log level type");
}

// ---------------------------------------------------------------------

std::ostream& operator<<(std::ostream& out, const sink_config& c)
{
    out << "sink(" << to_string(c.type) << ", "
        << (c.filename ? *c.filename : "<empty>") << ", "
        << uh::log::to_string(c.level) << ")";

    return out;
}

// ---------------------------------------------------------------------

void init_logging(const config& cfg)
{
    logging::add_common_attributes();
    logging::core::get()->add_global_attribute("CountDown", attrs::counter<int>(0, +1));

    for (const auto& sink : cfg.sinks)
    {
        logging::core::get()->add_sink(make_sink(sink));
    }

    INFO << "Logging was configured.";
}

// ---------------------------------------------------------------------

} // namespace uh::log
