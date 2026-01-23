#pragma once
#include <sys/stat.h>

#include <filesystem>
#include <system_error>

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <cctype>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "http_server.h"
#include "json_keys.h"
#include "model.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;

using namespace std::literals;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view HTML = "text/html"sv;
    constexpr static std::string_view JSON = "application/json"sv;
    constexpr static std::string_view CSS = "text/css"sv;
    constexpr static std::string_view TEXT = "text/plain"sv;
    constexpr static std::string_view JS = "text/javascript"sv;
    constexpr static std::string_view XML = "application/xml"sv;
    constexpr static std::string_view PNG = "image/png"sv;
    constexpr static std::string_view JPEG = "image/jpeg"sv;
    constexpr static std::string_view GIF = "image/gif"sv;
    constexpr static std::string_view BMP = "image/bmp"sv;
    constexpr static std::string_view ICO = "image/vnd.microsoft.icon"sv;
    constexpr static std::string_view TIFF = "image/tiff"sv;
    constexpr static std::string_view SVG = "image/svg+xml"sv;
    constexpr static std::string_view MP3 = "audio/mpeg"sv;
    constexpr static std::string_view OCTET = "application/octet-stream"sv;
};

struct Endpoint {
    Endpoint() = delete;
    constexpr static std::string_view MAPS = "/api/v1/maps"sv;
    constexpr static std::string_view API = "/api/"sv;
};

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive, std::string_view content_type);

FileResponse MakeFileResponse(http::status status, const std::string &file_path,
                              unsigned http_version, bool keep_alive,
                              std::string_view content_type);

namespace detail {
inline StringResponse MakeJsonError(http::status status, std::string_view code,
                                    std::string_view message, unsigned version, bool keep_alive) {
    boost::json::object obj;
    obj["code"] = std::string(code);
    obj["message"] = std::string(message);
    const std::string body = boost::json::serialize(obj);

    return MakeStringResponse(status, body, version, keep_alive, ContentType::JSON);
}

inline bool IsApi(std::string_view target) { return target.starts_with(Endpoint::API); }

inline std::optional<std::string_view> ExtractMapId(std::string_view target) {
    const auto pos = target.find_last_of('/');
    if (pos == std::string_view::npos || pos + 1 >= target.size()) {
        return std::nullopt;
    }

    return target.substr(pos + 1);
}

inline boost::json::array GetRoadsFromMap(const model::Map *map) {
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

inline boost::json::array GetBuildingsFromMap(const model::Map *map) {
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

inline boost::json::array GetOfficesFromMap(const model::Map *map) {
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

inline int HexVal(unsigned char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

inline bool DecodeSymbol(std::string_view sym, char &out) {
    if (sym.size() != 3 || sym[0] != '%') return false;

    int hi = HexVal(static_cast<unsigned char>(sym[1]));
    int lo = HexVal(static_cast<unsigned char>(sym[2]));
    if (hi < 0 || lo < 0) return false;

    out = static_cast<char>((hi << 4) | lo);
    return true;
}

inline std::string DecodeURI(std::string_view uri) {
    std::string result;
    result.reserve(uri.size());

    std::size_t prev_pos = 0;
    std::size_t current_pos = 0;

    while ((current_pos = uri.find('%', current_pos)) != std::string_view::npos) {
        result.append(uri.substr(prev_pos, current_pos - prev_pos));

        char decoded = 0;
        if (DecodeSymbol(uri.substr(current_pos, 3), decoded)) {
            result += decoded;
            current_pos += 3;
            prev_pos = current_pos;
        } else {
            result += '%';
            current_pos += 1;
            prev_pos = current_pos;
        }
    }

    result.append(uri.substr(prev_pos));
    return result;
}

inline std::string_view GetContentType(std::string_view target) {
    auto dot = target.find_last_of('.');
    if (dot == std::string_view::npos || dot + 1 >= target.size()) {
        return ContentType::OCTET;
    }

    std::string ext(target.substr(dot + 1));
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (ext == "html" || ext == "htm") return ContentType::HTML;
    if (ext == "css") return ContentType::CSS;
    if (ext == "txt") return ContentType::TEXT;
    if (ext == "js") return ContentType::JS;
    if (ext == "json") return ContentType::JSON;
    if (ext == "xml") return ContentType::XML;
    if (ext == "png") return ContentType::PNG;
    if (ext == "jpg" || ext == "jpeg" || ext == "jpe") return ContentType::JPEG;
    if (ext == "gif") return ContentType::GIF;
    if (ext == "bmp") return ContentType::BMP;
    if (ext == "ico") return ContentType::ICO;
    if (ext == "tiff" || ext == "tif") return ContentType::TIFF;
    if (ext == "svg" || ext == "svgz") return ContentType::SVG;
    if (ext == "mp3") return ContentType::MP3;

    return ContentType::OCTET;
}

bool IsWithinRoot(const fs::path &path, const fs::path &root);
}  // namespace detail

class RequestHandler {
   public:
    explicit RequestHandler(model::Game &game, const fs::path &root) : game_{game}, root_(root) {}

    RequestHandler(const RequestHandler &) = delete;
    RequestHandler &operator=(const RequestHandler &) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send) {
        const std::string_view target = req.target();

        if (detail::IsApi(target)) {
            HandleApiRequest(std::move(req), std::forward<decltype(send)>(send));
        } else {
            HandleFileRequest(std::move(req), std::forward<decltype(send)>(send));
        }
    }

   private:
    template <typename Body, typename Allocator, typename Send>
    void HandleApiRequest(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send) {
        const std::string_view target = req.target();
        const auto version = req.version();
        const bool keep_alive = req.keep_alive();

        if (req.method() != http::verb::get) {
            send(detail::MakeJsonError(http::status::method_not_allowed, "methodNotAllowed"sv,
                                       "Only GET is supported"sv, version, keep_alive));
            return;
        }

        if (target == Endpoint::MAPS) {
            auto maps = game_.GetMaps();
            boost::json::array array;
            for (const auto &map : maps) {
                boost::json::object map_obj;
                std::string id = *map.GetId();
                std::string name = map.GetName();
                map_obj[std::string(keys::kId)] = id;
                map_obj[std::string(keys::kName)] = name;
                array.push_back(map_obj);
            }
            send(MakeStringResponse(http::status::ok, boost::json::serialize(array), version,
                                    keep_alive, ContentType::JSON));
            return;
        }

        if (target.starts_with(Endpoint::MAPS)) {
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
                response_body[std::string(keys::kId)] = *map->GetId();
                response_body[std::string(keys::kName)] = map->GetName();

                boost::json::array roads = detail::GetRoadsFromMap(map);
                response_body[std::string(keys::kRoads)] = std::move(roads);

                boost::json::array buildings = detail::GetBuildingsFromMap(map);
                response_body[std::string(keys::kBuildings)] = std::move(buildings);

                boost::json::array offices = detail::GetOfficesFromMap(map);
                response_body[std::string(keys::kOffices)] = std::move(offices);

                send(MakeStringResponse(http::status::ok, boost::json::serialize(response_body),
                                        version, keep_alive, ContentType::JSON));
                return;
            }
        }

        send(detail::MakeJsonError(http::status::bad_request, "badRequest"sv, "Bad request"sv,
                                   version, keep_alive));
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleFileRequest(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send) {
        const std::string_view target = req.target();
        const auto version = req.version();
        const bool keep_alive = req.keep_alive();

        std::string decoded_target = detail::DecodeURI(target);

        if (!decoded_target.empty() && decoded_target.front() == '/') {
            decoded_target.erase(decoded_target.begin());
        }

        if (decoded_target.empty()) {
            decoded_target = "index.html"s;
        }

        fs::path requested = root_ / fs::path(decoded_target);

        std::error_code ec;
        fs::path requested_canonical = fs::weakly_canonical(requested, ec);

        if (ec || !detail::IsWithinRoot(requested_canonical, root_)) {
            send(MakeStringResponse(http::status::bad_request, "Out of root", version, keep_alive,
                                    ContentType::TEXT));
            return;
        }

        if (!fs::exists(requested_canonical)) {
            send(MakeStringResponse(http::status::not_found, "File not found", version, keep_alive,
                                    ContentType::TEXT));
            return;
        }

        send(MakeFileResponse(http::status::ok, requested_canonical.string(), version, keep_alive,
                              detail::GetContentType(requested_canonical.string())));
    }

   private:
    model::Game &game_;
    const fs::path &root_;
};

}  // namespace http_handler
