#pragma once

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <chrono>
#include <string>

#include "logging.h"

namespace http_handler {

namespace http = boost::beast::http;
namespace json = boost::json;

inline constexpr const char* kClientIpHeader = "X-Client-IP";

template <class DecoratedHandler>
class LoggingRequestHandler {
   public:
    explicit LoggingRequestHandler(DecoratedHandler& decorated) : decorated_(decorated) {}

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send,
                    const std::string& ip) {
        const auto start = std::chrono::steady_clock::now();

        LogRequest(req, ip);

        using SendT = std::decay_t<Send>;
        SendT send_copy(std::forward<Send>(send));

        auto wrapped_send = [start, ip, send = std::move(send_copy)](auto&& resp) mutable {
            LogResponse(ip, start, resp);
            send(std::forward<decltype(resp)>(resp));
        };

        decorated_(std::move(req), std::move(wrapped_send));
    }

   private:
    template <typename Req>
    static void LogRequest(const Req& req, const std::string& ip) {
        json::object data;
        data["ip"] = ip;
        data["URI"] = std::string(req.target());
        data["method"] = std::string(req.method_string());

        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(app_logging::additional_data,
                                                         json::value(std::move(data)))
                                << "request received";
    }

    template <typename Resp>
    static void LogResponse(const std::string& ip, std::chrono::steady_clock::time_point start,
                            const Resp& resp) {
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - start)
                            .count();

        json::object data;
        data["ip"] = ip;
        data["response_time"] = static_cast<std::int64_t>(ms);
        data["code"] = resp.result_int();

        auto ct_it = resp.find(http::field::content_type);
        if (ct_it == resp.end())
            data["content_type"] = nullptr;
        else
            data["content_type"] = std::string(ct_it->value());

        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(app_logging::additional_data,
                                                         json::value(std::move(data)))
                                << "response sent";
    }

   private:
    DecoratedHandler& decorated_;
};

}  // namespace http_handler