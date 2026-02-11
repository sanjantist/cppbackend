#pragma once

#include <boost/beast/http.hpp>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;

}  // namespace http_handler