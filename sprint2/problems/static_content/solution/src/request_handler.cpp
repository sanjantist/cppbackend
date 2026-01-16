#include "request_handler.h"

#include <stdexcept>

namespace sys = boost::system;

namespace http_handler {

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive, std::string_view content_type) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
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

namespace detail {
bool IsWithinRoot(const fs::path &path, const fs::path &root) {
    auto p_it = path.begin();
    auto r_it = root.begin();

    for (; r_it != root.end(); ++r_it, ++p_it) {
        if (p_it == path.end() || *p_it != *r_it) {
            return false;
        }
    }
    return true;
}
}  // namespace detail

}  // namespace http_handler
