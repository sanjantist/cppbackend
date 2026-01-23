#pragma once
#include <string>
#include <string_view>

#include "http_types.h"

namespace http_handler {
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

StringResponse MakeStringResponse(boost::beast::http::status status, std::string_view body,
                                  unsigned http_version, bool keep_alive,
                                  std::string_view content_type);

FileResponse MakeFileResponse(boost::beast::http::status status, const std::string& file_path,
                              unsigned http_version, bool keep_alive,
                              std::string_view content_type);

StringResponse MakeJsonError(boost::beast::http::status status, std::string_view code,
                             std::string_view message, unsigned version, bool keep_alive);

std::string_view GetContentType(std::string_view target);

}  // namespace http_handler