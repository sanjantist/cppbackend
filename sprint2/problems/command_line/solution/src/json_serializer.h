#pragma once
#include <boost/json.hpp>

#include "model.h"

namespace json_serialization {
boost::json::array GetRoadsFromMap(const model::Map *map);
boost::json::array GetBuildingsFromMap(const model::Map *map);
boost::json::array GetOfficesFromMap(const model::Map *map);
}  // namespace json_serialization