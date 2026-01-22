#include "http_response.h"

#include <boost/json.hpp>

namespace http_handler {
namespace sys = boost::system;

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive, std::string_view content_type) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.set(http::field::cache_control, "no-cache");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

FileResponse MakeFileResponse(http::status status, const std::string &file_path,
                              unsigned http_version, bool keep_alive,
                              std::string_view content_type) {
    FileResponse response(status, http_version);
    response.set(http::field::content_type, content_type);

    http::file_body::value_type file;
    if (sys::error_code ec; file.open(file_path.c_str(), beast::file_mode::read, ec), ec) {
        throw std::runtime_error("Can't open file"s);
    }

    response.body() = std::move(file);
    response.keep_alive(keep_alive);
    response.prepare_payload();

    return response;
}

StringResponse MakeJsonError(boost::beast::http::status status, std::string_view code,
                             std::string_view message, unsigned version, bool keep_alive) {
    boost::json::object obj;
    obj["code"] = std::string(code);
    obj["message"] = std::string(message);
    const std::string body = boost::json::serialize(obj);

    return MakeStringResponse(status, body, version, keep_alive, ContentType::JSON);
}

std::string_view GetContentType(std::string_view target) {
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
}  // namespace http_handler