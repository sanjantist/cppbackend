#include "json_loader.h"

#include <boost/json.hpp>
#include <fstream>

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

    boost::json::value json = boost::json::parse(json_str);
    auto maps = json.at("maps").as_array();
    for (const auto& map : maps) {
        // создание объекта карты для дальнейшего наполнения
        std::string id = map.at("id").as_string().c_str();
        std::string name = map.at("name").as_string().c_str();
        model::Map::Id map_id(id);
        model::Map map_obj(map_id, name);

        // добавление дорог в карту
        auto roads = map.at("roads").as_array();
        for (const auto& road : roads) {
            auto road_as_map = road.as_object();

            bool is_horisontal_road = (road_as_map.find("x1") != road_as_map.end());

            model::Coord x0 = road_as_map.at("x0").as_int64();
            model::Coord y0 = road_as_map.at("y0").as_int64();
            model::Point start{x0, y0};

            if (is_horisontal_road) {
                model::Coord x1 = road_as_map.at("x1").as_int64();

                model::Road road_obj(model::Road::HORIZONTAL, {x0, y0}, x1);
                map_obj.AddRoad(road_obj);
            } else {
                model::Coord y1 = road_as_map.at("y1").as_int64();

                model::Road road_obj(model::Road::VERTICAL, start, y1);
                map_obj.AddRoad(road_obj);
            }
        }

        // добавление зданий в карту
        auto buildings = map.at("buildings").as_array();
        for (const auto& building : buildings) {
            auto building_as_map = building.as_object();

            model::Coord x = building_as_map.at("x").as_int64();
            model::Coord y = building_as_map.at("y").as_int64();
            model::Point pos = {x, y};
            model::Dimension w = building_as_map.at("w").as_int64();
            model::Dimension h = building_as_map.at("h").as_int64();
            model::Size size = {w, h};
            model::Rectangle rect = {pos, size};

            model::Building building_obj(rect);
            map_obj.AddBuilding(building_obj);
        }

        // добавление офисов в карту
        auto offices = map.at("offices").as_array();
        for (const auto& office : offices) {
            auto office_as_map = office.as_object();
            std::string id = office_as_map.at("id").as_string().c_str();
            model::Office::Id office_id(id);
            model::Coord x = office_as_map.at("x").as_int64();
            model::Coord y = office_as_map.at("y").as_int64();
            model::Point pos = {x, y};
            model::Dimension offset_x = office_as_map.at("offsetX").as_int64();
            model::Dimension offset_y = office_as_map.at("offsetY").as_int64();
            model::Offset offset = {offset_x, offset_y};

            model::Office office_obj(office_id, pos, offset);
            map_obj.AddOffice(office_obj);
        }

        game.AddMap(map_obj);
    }

    return game;
}

}  // namespace json_loader