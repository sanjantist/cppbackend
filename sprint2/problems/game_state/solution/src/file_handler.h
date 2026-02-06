#pragma once
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <filesystem>

#include "http_response.h"
#include "url_utils.h"

namespace http_handler {
namespace fs = std::filesystem;

class FileHandler {
   public:
    explicit FileHandler(const fs::path& root) : root_(root) {}

    template <typename Body, typename Allocator, typename Send>
    void Handle(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        const std::string_view target = req.target();
        const auto version = req.version();
        const bool keep_alive = req.keep_alive();

        std::string decoded_target = url::DecodeURI(target);

        if (!decoded_target.empty() && decoded_target.front() == '/') {
            decoded_target.erase(decoded_target.begin());
        }

        if (decoded_target.empty()) {
            decoded_target = "index.html"s;
        }

        fs::path requested = root_ / fs::path(decoded_target);

        std::error_code ec;
        fs::path requested_canonical = fs::weakly_canonical(requested, ec);

        if (ec || !IsWithinRoot(requested_canonical)) {
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
                              GetContentType(requested_canonical.string())));
    }

   private:
    bool IsWithinRoot(const fs::path& path) {
        auto p_it = path.begin();
        auto r_it = root_.begin();

        for (; r_it != root_.end(); ++r_it, ++p_it) {
            if (p_it == path.end() || *p_it != *r_it) {
                return false;
            }
        }
        return true;
    }

   private:
    const fs::path& root_;
};
}  // namespace http_handler