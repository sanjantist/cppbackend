#include "logging.h"

#include <boost/core/null_deleter.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/json.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <iostream>

namespace app_logging {

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace json = boost::json;
namespace pt = boost::posix_time;

static void JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    json::object obj;

    if (auto ts = logging::extract<pt::ptime>("TimeStamp", rec)) {
        obj["timestamp"] = pt::to_iso_extended_string(ts.get());
    } else {
        obj["timestamp"] = nullptr;
    }

    obj["message"] = rec[expr::smessage].get();

    if (auto data = logging::extract<json::value>("AdditionalData", rec)) {
        obj["data"] = data.get();
    } else {
        obj["data"] = json::object{};
    }

    strm << json::serialize(obj) << '\n';
}

void InitLogging() {
    logging::add_common_attributes();

    using text_sink = sinks::synchronous_sink<sinks::text_ostream_backend>;

    auto sink = boost::make_shared<text_sink>();
    sink->locked_backend()->add_stream(
        boost::shared_ptr<std::ostream>(&std::cout, boost::null_deleter{}));
    sink->locked_backend()->auto_flush(true);

    sink->set_formatter(&JsonFormatter);

    logging::core::get()->remove_all_sinks();
    logging::core::get()->add_sink(sink);

    logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);
}

}  // namespace app_logging