#include "json_serializer.h"

#include "json_keys.h"

namespace json_serialization {
boost::json::array GetRoadsFromMap(const model::Map *map) {
    boost::json::array roads;
    for (const auto &road : map->GetRoads()) {
        boost::json::object road_obj;
        road_obj[std::string(keys::kX0)] = road.GetStart().x;
        road_obj[std::string(keys::kY0)] = road.GetStart().y;

        if (road.IsHorizontal()) {
            road_obj[std::string(keys::kX1)] = road.GetEnd().x;
        } else {
            road_obj[std::string(keys::kY1)] = road.GetEnd().y;
        }
        roads.push_back(road_obj);
    }
    return roads;
}

boost::json::array GetBuildingsFromMap(const model::Map *map) {
    boost::json::array buildings;
    for (const auto &building : map->GetBuildings()) {
        boost::json::object building_obj;
        building_obj[std::string(keys::kX)] = building.GetBounds().position.x;
        building_obj[std::string(keys::kY)] = building.GetBounds().position.y;
        building_obj[std::string(keys::kW)] = building.GetBounds().size.width;
        building_obj[std::string(keys::kH)] = building.GetBounds().size.height;

        buildings.push_back(building_obj);
    }
    return buildings;
}

boost::json::array GetOfficesFromMap(const model::Map *map) {
    boost::json::array offices;
    for (const auto &office : map->GetOffices()) {
        boost::json::object office_obj;
        office_obj[std::string(keys::kId)] = *office.GetId();
        office_obj[std::string(keys::kX)] = office.GetPosition().x;
        office_obj[std::string(keys::kY)] = office.GetPosition().y;
        office_obj[std::string(keys::kOffsetX)] = office.GetOffset().dx;
        office_obj[std::string(keys::kOffsetY)] = office.GetOffset().dy;

        offices.push_back(office_obj);
    }
    return offices;
}
}  // namespace json_serialization