#include "common_logger.h"

#include <boost/log/core/core.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions/formatters/named_scope.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
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

BOOST_LOG_GLOBAL_LOGGER_INIT(logger, boost::log::sources::severity_logger_mt) {
    boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level> logger;

    // add attributes
    boost::log::add_common_attributes(); // adds LineID, TimeStamp, ProcessID and ThreadID

    // add "console" output sink
    typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend> text_sink;
    boost::shared_ptr<text_sink> sink_console = boost::make_shared<text_sink>();
    sink_console->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));

    // add "file" output sink
    typedef boost::log::sinks::asynchronous_sink<boost::log::sinks::text_file_backend> file_sink;
    boost::shared_ptr< file_sink > sink_file = boost::make_shared< file_sink >(
            boost::log::keywords::file_name = "./logs/%Y-%m-%d.%N.log", // File name pattern
            boost::log::keywords::rotation_size = 1 * 1024 * 1024,      // Rotate files every 10 MiB
            boost::log::keywords::auto_flush = true,                    // Decrease the chance of lost logging in case of program crash (degrades performance)
            boost::log::keywords::open_mode = std::ios_base::app        // Appends to last log file
    );

    // specify the format of the log message
    boost::log::formatter formatter = boost::log::expressions::stream
                            << std::setw(7) << std::setfill('0') << boost::log::expressions::attr< unsigned int >("LineID") << std::setfill(' ') << " | "
                            << boost::log::expressions::attr< boost::posix_time::ptime >("TimeStamp") << " "
                            << "[" << boost::log::trivial::severity << "]"
                            << ": " << boost::log::expressions::smessage;
    sink_console->set_formatter(formatter);
    sink_file->set_formatter(formatter);

    // set the filter level
    sink_console->set_filter(boost::log::trivial::severity >= SEVERITY_THRESHOLD);
    sink_file->set_filter(boost::log::trivial::severity >= SEVERITY_THRESHOLD);
    
    boost::log::core::get()->add_sink(sink_console);
    boost::log::core::get()->add_sink(sink_file);

    return logger;
}