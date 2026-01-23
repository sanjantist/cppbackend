#pragma once

#include <boost/json.hpp>
#include <filesystem>

#include "model.h"

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path);
model::Road LoadRoad(const boost::json::object& map);
model::Building LoadBuilding(const boost::json::object& map);
model::Office LoadOffice(const boost::json::object& map);

}  // namespace json_loader
