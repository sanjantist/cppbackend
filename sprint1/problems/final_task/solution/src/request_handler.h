#pragma once
#include "boost/json/array.hpp"
#include "boost/json/object.hpp"
#include "boost/json/serialize.hpp"
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <optional>
#include <string_view>

#include "http_server.h"
#include "model.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

using namespace std::literals;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view JSON = "application/json"sv;
};

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive, std::string_view content_type);

namespace detail {
inline StringResponse MakeJsonError(http::status status, std::string_view code,
                                    std::string_view message, unsigned version, bool keep_alive) {
    boost::json::object obj;
    obj["code"] = std::string(code);
    obj["message"] = std::string(message);
    const std::string body = boost::json::serialize(obj);

    return MakeStringResponse(status, body, version, keep_alive, ContentType::JSON);
}

inline bool IsApi(std::string_view target) { return target.starts_with("/api/"sv); }

inline std::optional<std::string_view> ExtractMapId(std::string_view target) {
    const auto pos = target.find_last_of('/');
    if (pos == std::string_view::npos || pos + 1 >= target.size()) {
        return std::nullopt;
    }

    return target.substr(pos + 1);
}

inline boost::json::array GetRoadsFromMap(const model::Map* map) {
    boost::json::array roads;
    for (const auto& road : map->GetRoads()) {
        boost::json::object road_obj;
        road_obj["x0"] = road.GetStart().x;
        road_obj["y0"] = road.GetStart().y;

        if (road.IsHorizontal()) {
            road_obj["x1"] = road.GetEnd().x;
        } else {
            road_obj["y1"] = road.GetEnd().y;
        }
        roads.push_back(road_obj);
    }
    return roads;
}

inline boost::json::array GetBuildingsFromMap(const model::Map* map) {
    boost::json::array buildings;
    for (const auto& building : map->GetBuildings()) {
        boost::json::object building_obj;
        building_obj["x"] = building.GetBounds().position.x;
        building_obj["y"] = building.GetBounds().position.y;
        building_obj["w"] = building.GetBounds().size.width;
        building_obj["h"] = building.GetBounds().size.height;

        buildings.push_back(building_obj);
    }
    return buildings;
}

inline boost::json::array GetOfficesFromMap(const model::Map* map) {
    boost::json::array offices;
    for (const auto& office : map->GetOffices()) {
        boost::json::object office_obj;
        office_obj["id"] = *office.GetId();
        office_obj["x"] = office.GetPosition().x;
        office_obj["y"] = office.GetPosition().y;
        office_obj["offsetX"] = office.GetOffset().dx;
        office_obj["offsetY"] = office.GetOffset().dy;

        offices.push_back(office_obj);
    }
    return offices;
}
}  // namespace detail

class RequestHandler {
   public:
    explicit RequestHandler(model::Game& game) : game_{game} {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send

        const std::string_view target = req.target();
        const auto version = req.version();
        const bool keep_alive = req.keep_alive();

        if (!detail::IsApi(target)) {
            send(detail::MakeJsonError(http::status::bad_request, "badRequest"sv, "Bad request"sv,
                                       version, keep_alive));
            return;
        }

        if (req.method() != http::verb::get) {
            send(detail::MakeJsonError(http::status::method_not_allowed, "methodNotAllowed"sv,
                                       "Only GET is supported"sv, version, keep_alive));
            return;
        }

        if (target == "/api/v1/maps"sv) {
            auto maps = game_.GetMaps();
            boost::json::array array;
            for (const auto& map : maps) {
                boost::json::object map_obj;
                std::string id = *map.GetId();
                std::string name = map.GetName();
                map_obj["id"] = id;
                map_obj["name"] = name;
                array.push_back(map_obj);
            }
            send(MakeStringResponse(http::status::ok, boost::json::serialize(array), version,
                                    keep_alive, ContentType::JSON));
            return;
        }

        if (target.starts_with("/api/v1/maps/"sv)) {
            auto id = detail::ExtractMapId(target);

            if (id) {
                auto map_id = model::Map::Id(std::string(*id));
                auto map = game_.FindMap(map_id);
                if (!map) {
                    send(detail::MakeJsonError(http::status::not_found, "mapNotFound"sv,
                                               "Map not found"sv, version, keep_alive));
                    return;
                }

                boost::json::object response_body;
                response_body["id"] = *map->GetId();
                response_body["name"] = map->GetName();

                boost::json::array roads = detail::GetRoadsFromMap(map);
                response_body["roads"] = std::move(roads);

                boost::json::array buildings = detail::GetBuildingsFromMap(map);
                response_body["buildings"] = std::move(buildings);

                boost::json::array offices = detail::GetOfficesFromMap(map);
                response_body["offices"] = std::move(offices);

                send(MakeStringResponse(http::status::ok, boost::json::serialize(response_body),
                                        version, keep_alive, ContentType::JSON));
                return;
            }
        }

        send(detail::MakeJsonError(http::status::bad_request, "badRequest"sv, "Bad request"sv,
                                   version, keep_alive));
        return;
    }

   private:
    model::Game& game_;
};

}  // namespace http_handler
