#include "common_logger.h"

#include <boost/log/core/core.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions/formatters/named_scope.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <fstream>
#include <ostream>

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", boost::log::trivial::severity_level)

BOOST_LOG_GLOBAL_LOGGER_INIT(logger, boost::log::sources::severity_logger_mt) {
    boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level> logger;

    // add attributes
    logger.add_attribute("LineID", boost::log::attributes::counter<unsigned int>(1));     // lines are sequentially numbered
    logger.add_attribute("TimeStamp", boost::log::attributes::local_clock());             // each log line gets a timestamp

    // add a text sink
    typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend> text_sink;
    boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();

    // add "console" output stream to our sink
    sink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));

    boost::log::add_file_log
    (
        boost::log::keywords::file_name = "logs/replicode_%Y-%m-%dT%H-%M-%S.%N.log",    // File name pattern
        boost::log::keywords::rotation_size = 1 * 1024 * 1024,                          // Rotate files every 10 MiB
        boost::log::keywords::open_mode = std::ios_base::app,                           // Appends to last log file
        boost::log::keywords::format = boost::log::expressions::stream                  // Log record format
                            << std::setw(7) << std::setfill('0') << line_id << std::setfill(' ') << " | "
                            << boost::log::expressions::format_date_time(timestamp, "%Y-%m-%dT%H:%M:%S.%f") << " "
                            << "[" << boost::log::trivial::severity << "]"
                            << " - " << boost::log::expressions::smessage
    );

    // only messages with severity >= SEVERITY_THRESHOLD are written
    sink->set_filter(severity >= SEVERITY_THRESHOLD);

    // "register" our sink
    boost::log::core::get()->add_sink(sink);

    return logger;
}
