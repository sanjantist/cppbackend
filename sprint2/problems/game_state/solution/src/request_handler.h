#pragma once
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <cctype>
#include <filesystem>
#include <string_view>
#include <utility>

#include "api_handler.h"
#include "application.h"
#include "file_handler.h"
#include "http_server.h"

namespace http_handler {

using namespace std::literals;
namespace net = boost::asio;

class RequestHandler {
   public:
    explicit RequestHandler(net::io_context &ioc, app::Application &application,
                            const fs::path &root)
        : application_{application},
          root_{root},
          api_handler_{application},
          file_handler_{root_},
          app_strand_(net::make_strand(ioc)) {}

    RequestHandler(const RequestHandler &) = delete;
    RequestHandler &operator=(const RequestHandler &) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send) {
        std::string_view target = req.target();

        if (url::IsApi(target)) {
            using Req = http::request<Body, http::basic_fields<Allocator>>;
            using SendT = std::decay_t<Send>;
            net::dispatch(app_strand_, [this, req = Req(std::move(req)),
                                        send = SendT(std::forward<Send>(send))]() mutable {
                send(api_handler_.Handle(std::move(req)));
            });
        } else {
            file_handler_.Handle(std::move(req), std::forward<Send>(send));
        }
    }

   private:
    app::Application &application_;
    const fs::path &root_;
    net::strand<net::io_context::executor_type> app_strand_;

    ApiHandler api_handler_;
    FileHandler file_handler_;
};

}  // namespace http_handler
