#define BOOST_BEAST_USE_STD_STRING_VIEW
#include "api_handler.h"

#include <string>
#include <string_view>

#include "boost/json/object.hpp"
#include "http_response.h"
#include "json_keys.h"
#include "json_serializer.h"
#include "url_utils.h"

namespace http_handler {
namespace json = boost::json;
namespace {
inline bool IsHex(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

inline bool IsValidTokenFormat(std::string_view token) {
    if (token.size() != 32) return false;
    for (char c : token) {
        if (!IsHex(c)) return false;
    }
    return true;
}

inline std::optional<std::string_view> ExtractBearerToken(const http::fields& headers) {
    auto it = headers.find(http::field::authorization);
    if (it == headers.end()) return std::nullopt;

    std::string_view v = it->value();
    constexpr std::string_view kPrefix = "Bearer "sv;
    if (!v.starts_with(kPrefix)) return std::nullopt;

    v.remove_prefix(kPrefix.size());
    if (v.empty()) return std::nullopt;

    return v;
}

}  // namespace

StringResponse ApiHandler::HandleImpl(http::verb method, std::string_view target,
                                      std::string_view body, const http::fields& headers,
                                      unsigned int version, bool keep_alive) {
    if (target == Endpoint::JOIN) {
        return HandleJoinRequest(method, body, headers, version, keep_alive);
    }
    if (target == Endpoint::MAPS) {
        return HandleMapsRequest(version, keep_alive);
    }
    if (target.starts_with(Endpoint::MAPS)) {
        return HandleMapDataRequest(target, version, keep_alive);
    }
    if (target == Endpoint::PLAYERS) {
        return HandlePlayersRequest(method, headers, version, keep_alive);
    }
    if (target == Endpoint::STATE) {
        return HandleStateRequest(method, body, headers, version, keep_alive);
    }

    return MakeJsonError(http::status::bad_request, "badRequest"sv, "Bad request"sv, version,
                         keep_alive);
}

StringResponse ApiHandler::HandleJoinRequest(http::verb method, std::string_view body,
                                             const http::fields& headers, unsigned int version,
                                             bool keep_alive) {
    if (method != http::verb::post) {
        auto response = MakeJsonError(http::status::method_not_allowed, "invalidMethod"sv,
                                      "Only POST method is expected"sv, version, keep_alive);
        response.set(http::field::allow, "POST");
        return response;
    }

    auto it = headers.find(http::field::content_type);
    if (it == headers.end()) {
        return MakeJsonError(http::status::bad_request, "invalidArgument"sv,
                             "Invalid content type"sv, version, keep_alive);
    }

    std::string_view ct = it->value();
    auto pos = ct.find(';');
    auto media = (pos == std::string_view::npos) ? ct : ct.substr(0, pos);
    while (!media.empty() && media.back() == ' ') media.remove_suffix(1);

    if (media != "application/json"sv) {
        return MakeJsonError(http::status::bad_request, "invalidArgument"sv,
                             "Invalid content type"sv, version, keep_alive);
    }

    boost::json::value parsed;
    try {
        parsed = boost::json::parse(boost::json::string_view(body.data(), body.size()));
    } catch (...) {
        return MakeJsonError(http::status::bad_request, "invalidArgument"sv,
                             "Join game request parse error"sv, version, keep_alive);
    }

    if (!parsed.is_object()) {
        return MakeJsonError(http::status::bad_request, "invalidArgument"sv,
                             "Join game request parse error"sv, version, keep_alive);
    }

    const auto& obj = parsed.as_object();
    auto it_name = obj.find("userName");
    auto it_map = obj.find("mapId");
    if (it_name == obj.end() || it_map == obj.end() || !it_name->value().is_string() ||
        !it_map->value().is_string()) {
        return MakeJsonError(http::status::bad_request, "invalidArgument"sv,
                             "Join game request parse error"sv, version, keep_alive);
    }

    std::string user_name = std::string(it_name->value().as_string());
    std::string map_id_str = std::string(it_map->value().as_string());
    if (user_name.empty()) {
        return MakeJsonError(http::status::bad_request, "invalidArgument"sv, "Invalid name"sv,
                             version, keep_alive);
    }

    model::Map::Id map_id{map_id_str};
    try {
        auto result = application_.JoinGame(user_name, *map_id);

        boost::json::object out;
        out["authToken"] = result.auth_token;
        out["playerId"] = result.player_id;

        return MakeStringResponse(http::status::ok, boost::json::serialize(out), version,
                                  keep_alive, ContentType::JSON);
    } catch (...) {
        return MakeJsonError(http::status::not_found, "mapNotFound"sv, "Map not found"sv, version,
                             keep_alive);
    }
}

StringResponse ApiHandler::HandleMapsRequest(unsigned int version, bool keep_alive) {
    const auto& maps = application_.GetMaps();
    boost::json::array array;
    for (const auto& map : maps) {
        boost::json::object map_obj;
        std::string id = *map.GetId();
        std::string name = map.GetName();
        map_obj[std::string(keys::kId)] = id;
        map_obj[std::string(keys::kName)] = name;
        array.push_back(map_obj);
    }
    return MakeStringResponse(http::status::ok, boost::json::serialize(array), version, keep_alive,
                              ContentType::JSON);
}

StringResponse ApiHandler::HandleMapDataRequest(std::string_view target, unsigned int version,
                                                bool keep_alive) {
    auto id = url::ExtractMapId(target);

    if (!id) {
        return MakeJsonError(http::status::bad_request, "badRequest"sv, "Bad request"sv, version,
                             keep_alive);
    }

    auto map_id = model::Map::Id(std::string(*id));
    const auto& map = application_.FindMap(map_id);
    if (!map) {
        return MakeJsonError(http::status::not_found, "mapNotFound"sv, "Map not found"sv, version,
                             keep_alive);
    }

    boost::json::object response_body;
    response_body[std::string(keys::kId)] = *map->GetId();
    response_body[std::string(keys::kName)] = map->GetName();

    boost::json::array roads = json_serialization::GetRoadsFromMap(map);
    response_body[std::string(keys::kRoads)] = std::move(roads);

    boost::json::array buildings = json_serialization::GetBuildingsFromMap(map);
    response_body[std::string(keys::kBuildings)] = std::move(buildings);

    boost::json::array offices = json_serialization::GetOfficesFromMap(map);
    response_body[std::string(keys::kOffices)] = std::move(offices);

    return MakeStringResponse(http::status::ok, boost::json::serialize(response_body), version,
                              keep_alive, ContentType::JSON);
}

StringResponse ApiHandler::HandlePlayersRequest(http::verb method, const http::fields& headers,
                                                unsigned int version, bool keep_alive) {
    if (method != http::verb::get && method != http::verb::head) {
        auto response = MakeJsonError(http::status::method_not_allowed, "invalidMethod"sv,
                                      "Invalid method", version, keep_alive);
        response.set(http::field::allow, "GET, HEAD");
        return response;
    }

    auto token = ExtractBearerToken(headers);
    if (!token) {
        return MakeJsonError(http::status::unauthorized, "invalidToken"sv,
                             "Authorization header is missing"sv, version, keep_alive);
    }

    std::string_view token_sv = *token;
    if (!IsValidTokenFormat(token_sv)) {
        return MakeJsonError(http::status::unauthorized, "invalidToken"sv, "Invalid token"sv,
                             version, keep_alive);
    }

    auto players = application_.GetPlayers(token_sv);
    if (!players) {
        return MakeJsonError(http::status::unauthorized, "unknownToken"sv,
                             "Player token has not been found"sv, version, keep_alive);
    }

    json::object out;
    for (int id : *players) {
        if (auto* p = application_.GetPlayer(id)) {
            out[std::to_string(id)] = {{"name", p->GetName()}};
        }
    }

    if (method == http::verb::head) {
        return MakeStringResponse(http::status::ok, ""sv, version, keep_alive, ContentType::JSON);
    }

    return MakeStringResponse(http::status::ok, json::serialize(out), version, keep_alive,
                              ContentType::JSON);
}

StringResponse ApiHandler::HandleStateRequest(http::verb method, std::string_view body,
                                              const http::fields& headers, unsigned int version,
                                              bool keep_alive) {
    if (method != http::verb::get && method != http::verb::head) {
        auto response = MakeJsonError(http::status::method_not_allowed, "invalidMethod"sv,
                                      "Invalid method"sv, version, keep_alive);
        response.set(http::field::allow, "GET, HEAD");
        return response;
    }

    auto token = ExtractBearerToken(headers);
    if (!token) {
        return MakeJsonError(http::status::unauthorized, "invalidToken"sv,
                             "Authorization header is missing"sv, version, keep_alive);
    }

    std::string_view token_sv = *token;
    if (!IsValidTokenFormat(token_sv)) {
        return MakeJsonError(http::status::unauthorized, "invalidToken"sv, "Invalid token"sv,
                             version, keep_alive);
    }

    auto players = application_.GetPlayers(token_sv);
    if (!players) {
        return MakeJsonError(http::status::unauthorized, "unknownToken"sv,
                             "Player token has not been found"sv, version, keep_alive);
    }

    json::object out;
    json::object players_info;
    for (int id : *players) {
        json::object player_info;
        if (auto* p = application_.GetPlayer(id)) {
            auto dog = p->GetDog();

            json::array dog_pos;
            dog_pos.push_back(dog.GetPosition().x);
            dog_pos.push_back(dog.GetPosition().y);
            player_info["pos"] = dog_pos;

            json::array dog_speed;
            dog_speed.push_back(dog.GetVelocity().vx);
            dog_speed.push_back(dog.GetVelocity().vy);
            player_info["speed"] = dog_speed;

            std::string dir_str;
            auto direction = dog.GetDirection();
            switch (direction) {
                case model::DogDirection::East:
                    dir_str = "R";
                    break;
                case model::DogDirection::West:
                    dir_str = "L";
                    break;
                case model::DogDirection::North:
                    dir_str = "U";
                    break;
                case model::DogDirection::South:
                    dir_str = "D";
                    break;
            }
            player_info["dir"] = dir_str;
        }
        players_info[std::to_string(id)] = player_info;
    }
    out["players"] = players_info;

    if (method == http::verb::head) {
        return MakeStringResponse(http::status::ok, ""sv, version, keep_alive, ContentType::JSON);
    }

    return MakeStringResponse(http::status::ok, json::serialize(out), version, keep_alive,
                              ContentType::JSON);
}
}  // namespace http_handler