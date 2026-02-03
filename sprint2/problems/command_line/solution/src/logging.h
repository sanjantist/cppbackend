#pragma once

#include <boost/json/value.hpp>
#include <boost/log/expressions/keyword.hpp>

namespace app_logging {

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)

void InitLogging();

}  // namespace app_logging