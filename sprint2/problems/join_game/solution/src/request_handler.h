#pragma once
#include <sys/stat.h>

#include <filesystem>
#include <utility>
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <cctype>
#include <string_view>

#include "api_handler.h"
#include "file_handler.h"
#include "http_server.h"
#include "model.h"

namespace http_handler {

using namespace std::literals;

class RequestHandler {
   public:
    explicit RequestHandler(model::Game &game, const fs::path &root)
        : game_{game},
          root_{root},
          players_{},
          tokens_{},
          api_handler_{game_, players_, tokens_},
          file_handler_{game_, root_} {}

    RequestHandler(const RequestHandler &) = delete;
    RequestHandler &operator=(const RequestHandler &) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send) {
        const std::string_view target = req.target();

        if (url::IsApi(target)) {
            send(api_handler_.Handle(std::move(req)));
        } else {
            file_handler_.Handle(std::move(req), std::forward<Send>(send));
        }
    }

   private:
    model::Game &game_;
    const fs::path &root_;

    app::Players players_;
    app::PlayerTokens tokens_;

    ApiHandler api_handler_;
    FileHandler file_handler_;
};

}  // namespace http_handler
