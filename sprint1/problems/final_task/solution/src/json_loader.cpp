#include "json_loader.h"

#include <fstream>

#include "json_keys.h"
#include "model.h"

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    model::Game game;

    std::ifstream ifs;
    std::stringstream ss;

    ifs.open(json_path);

    if (!ifs) {
        throw std::runtime_error("Can't open file");
    }

    while (!ifs.eof()) {
        std::string temp;
        getline(ifs, temp);
        ss << temp << "\n";
    }

    ifs.close();

    std::string json_str = ss.str();

    boost::json::value json;
    try {
        json = boost::json::parse(json_str);
    } catch (const boost::json::system_error& e) {
        throw std::runtime_error("Failed to parse JSON");
    }

    auto maps = json.at("maps").as_array();
    for (const auto& map : maps) {
        // создание объекта карты для дальнейшего наполнения
        std::string id = map.at(keys::kId).as_string().c_str();
        std::string name = map.at(keys::kName).as_string().c_str();
        model::Map::Id map_id(id);
        model::Map map_obj(map_id, name);

        // добавление дорог в карту
        auto roads = map.at(keys::kRoads).as_array();
        for (const auto& road : roads) {
            auto road_obj = LoadRoad(road.as_object());
            map_obj.AddRoad(road_obj);
        }

        // добавление зданий в карту
        auto buildings = map.at(keys::kBuildings).as_array();
        for (const auto& building : buildings) {
            auto building_obj = LoadBuilding(building.as_object());
            map_obj.AddBuilding(building_obj);
        }

        // добавление офисов в карту
        auto offices = map.at(keys::kOffices).as_array();
        for (const auto& office : offices) {
            auto office_obj = LoadOffice(office.as_object());
            map_obj.AddOffice(office_obj);
        }

        game.AddMap(map_obj);
    }

    return game;
}

model::Road LoadRoad(const boost::json::object& map) {
    bool is_horisontal_road = (map.find(keys::kX1) != map.end());

    model::Coord x0 = map.at(keys::kX0).as_int64();
    model::Coord y0 = map.at(keys::kY0).as_int64();
    model::Point start{x0, y0};

    if (is_horisontal_road) {
        model::Coord x1 = map.at(keys::kX1).as_int64();

        model::Road road_obj(model::Road::HORIZONTAL, {x0, y0}, x1);
        return road_obj;
    } else {
        model::Coord y1 = map.at(keys::kY1).as_int64();

        model::Road road_obj(model::Road::VERTICAL, start, y1);
        return road_obj;
    }
}

model::Building LoadBuilding(const boost::json::object& map) {
    model::Coord x = map.at(keys::kX).as_int64();
    model::Coord y = map.at(keys::kY).as_int64();
    model::Point pos = {x, y};
    model::Dimension w = map.at(keys::kW).as_int64();
    model::Dimension h = map.at(keys::kH).as_int64();
    model::Size size = {w, h};
    model::Rectangle rect = {pos, size};

    model::Building building_obj(rect);
    return building_obj;
}

model::Office LoadOffice(const boost::json::object& map) {
    std::string id = map.at(keys::kId).as_string().c_str();
    model::Office::Id office_id(id);
    model::Coord x = map.at(keys::kX).as_int64();
    model::Coord y = map.at(keys::kY).as_int64();
    model::Point pos = {x, y};
    model::Dimension offset_x = map.at(keys::kOffsetX).as_int64();
    model::Dimension offset_y = map.at(keys::kOffsetY).as_int64();
    model::Offset offset = {offset_x, offset_y};

    model::Office office_obj(office_id, pos, offset);
    return office_obj;
}

}  // namespace json_loader
