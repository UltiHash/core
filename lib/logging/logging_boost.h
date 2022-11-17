//
// Created by benjamin-elias on 12.10.22.
//

#ifndef INC_18_LOGGING_LOGGING_BOOST_H
#define INC_18_LOGGING_LOGGING_BOOST_H

#pragma once

//boost libraries general
#include <boost/shared_ptr.hpp>
#include <boost/tti/tti.hpp>
#include <boost/make_shared.hpp>
#include <boost/phoenix.hpp>
#include <boost/filesystem/fstream.hpp>

//logging libraries within boost
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/support/date_time.hpp>
#include <filesystem>


//namespaces used by module conceptTypes
namespace logging  = boost::log;
namespace attrs    = boost::log::attributes;
namespace expr     = boost::log::expressions;
namespace src      = boost::log::sources;
namespace keywords = boost::log::keywords;

//initializing boost logging system
static boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;

//definition for errors to retrieve CustomThrowException information about file and line where it happend
#define HAPPEND_WHERE "(" << __FILE__ << ", " << __LINE__ << ", " << __FUNCTION__ << ") "

//log severity flags
#define TRACE  BOOST_LOG_SEV(lg,boost::log::trivial::trace) << HAPPEND_WHERE
#define DEBUG  BOOST_LOG_SEV(lg,boost::log::trivial::debug) << HAPPEND_WHERE
#define INFO  BOOST_LOG_SEV(lg,boost::log::trivial::info) << HAPPEND_WHERE
#define WARNING  BOOST_LOG_SEV(lg,boost::log::trivial::warning) << HAPPEND_WHERE
#define ERROR  BOOST_LOG_SEV(lg,boost::log::trivial::error) << HAPPEND_WHERE
#define FATAL  BOOST_LOG_SEV(lg,boost::log::trivial::fatal) << HAPPEND_WHERE


BOOST_LOG_ATTRIBUTE_KEYWORD(process_id, "ProcessID", attrs::current_process_id::value_type )
BOOST_LOG_ATTRIBUTE_KEYWORD(thread_id, "ThreadID", attrs::current_thread_id::value_type )

// Get Process native ID
static attrs::current_process_id::value_type::native_type get_native_process_id(
        logging::value_ref<attrs::current_process_id::value_type,
                tag::process_id> const& pid)
{
    if (pid)
        return pid->native_id();
    return 0;
}

// Get Thread native ID
static attrs::current_thread_id::value_type::native_type get_native_thread_id(
        logging::value_ref<attrs::current_thread_id::value_type,
                tag::thread_id> const& tid)
{
    if (tid)
        return tid->native_id();
    return 0;
}


/*
 * boost logging formatter with custom style
 * copy all following to main
 */

static void my_formatter(logging::record_view const& rec, logging::formatting_ostream& strm){
    //timestamp
    strm << "[";
    auto date_time_formatter = expr::stream << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S.%f");
    date_time_formatter(rec, strm);
    strm << "] -->\t";
    //line
    strm << logging::extract< unsigned int >("LineID", rec) << " ";
    //severity
    strm << "<" << rec[logging::trivial::severity] << "> \t";
    //thread
    strm << "Process/Thread(" ;
    auto process_formatter = expr::stream << boost::phoenix::bind(&get_native_process_id, process_id_type::or_none());
    process_formatter(rec,strm);
    strm << "/";
    auto thread_formatter = expr::stream << boost::phoenix::bind(&get_native_thread_id, thread_id_type::or_none());
    thread_formatter(rec,strm);
    strm << "): ";
    //message
    strm << rec[expr::smessage];
}

/*
 * Copy init logging to main or main_base and use basic input pipe info to configure
*/

static void init_logging(const std::string& logfilename){
    logging::add_common_attributes();
    typedef logging::sinks::asynchronous_sink<logging::sinks::text_ostream_backend> text_sink;
    boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();
    sink->locked_backend()->add_stream(boost::make_shared<std::ofstream>(logfilename.c_str()));//TODO: fix missing login from concept types
    logging::core::get()->add_global_attribute("CountDown", attrs::counter<int>(0, +1));
    sink->set_formatter(&my_formatter);

    sink->locked_backend()->auto_flush();

    logging::core::get()->add_sink(sink);
    INFO << "Logging was configured.";
}


/*
config logging
init_logging(std::filesystem::path{argv[0]}.parent_path().append("27_Include_Manager.log").string());
*/

#endif //INC_18_LOGGING_LOGGING_BOOST_H
