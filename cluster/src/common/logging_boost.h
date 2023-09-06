//
// Created by benjamin-elias on 12.10.22.
//

#ifndef UH_LOGGING_LOGGING_BOOST_H
#define UH_LOGGING_LOGGING_BOOST_H

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>

#include <filesystem>
#include <list>
#include <optional>


#ifdef DEBUG
# define HAPPEND_WHERE "(" << __FILE__ << ", " << __LINE__ << ", " << __FUNCTION__ << ") "
#else
# define HAPPEND_WHERE ""
#endif

#define TRACE BOOST_LOG_SEV(uh::log::lg, boost::log::trivial::trace) << HAPPEND_WHERE
#define DEBUG BOOST_LOG_SEV(uh::log::lg, boost::log::trivial::debug) << HAPPEND_WHERE
#define INFO BOOST_LOG_SEV(uh::log::lg, boost::log::trivial::info) << HAPPEND_WHERE
#define WARNING BOOST_LOG_SEV(uh::log::lg, boost::log::trivial::warning) << HAPPEND_WHERE
#define ERROR BOOST_LOG_SEV(uh::log::lg, boost::log::trivial::error) << HAPPEND_WHERE
#define FATAL BOOST_LOG_SEV(uh::log::lg, boost::log::trivial::fatal) << HAPPEND_WHERE


namespace uh::log
{

// ---------------------------------------------------------------------

enum class sink_type
{
    file,
    clog,
    cerr,
    cout
};

// ---------------------------------------------------------------------

std::string to_string(sink_type type);

// ---------------------------------------------------------------------

boost::log::trivial::severity_level severity_from_string(const std::string& s);

// ---------------------------------------------------------------------

std::string to_string(boost::log::trivial::severity_level level);

// ---------------------------------------------------------------------

struct sink_config
{
    sink_type type;
    std::optional<std::filesystem::path> filename;

    boost::log::trivial::severity_level level = boost::log::trivial::info;

    bool operator==(const sink_config&) const = default;
};

// ---------------------------------------------------------------------

struct config
{
    std::list<sink_config> sinks;
};

// ---------------------------------------------------------------------

std::ostream& operator<<(std::ostream& out, const sink_config& c);

// ---------------------------------------------------------------------

/**
 * Initialize application logging.
 *
 * @param logfilename path to log file
 */
void init_logging(const config& cfg);

// ---------------------------------------------------------------------

static boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;

// ---------------------------------------------------------------------

} // namespace uh::log

#endif
