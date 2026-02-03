#pragma once
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <string_view>

#include "application.h"
#include "http_types.h"

namespace http_handler {
class ApiHandler {
   public:
    explicit ApiHandler(app::Application& application) : application_(application) {}

    template <typename Body, typename Allocator>
    StringResponse Handle(http::request<Body, http::basic_fields<Allocator>>&& req) {
        return HandleImpl(req.method(), req.target(), req.body(), req.base(), req.version(),
                          req.keep_alive());
    }

   private:
    StringResponse HandleImpl(http::verb method, std::string_view target, std::string_view body,
                              const http::fields& headers, unsigned version, bool keep_alive);
    StringResponse HandleJoinRequest(http::verb method, std::string_view body,
                                     const http::fields& headers, unsigned version,
                                     bool keep_alive);
    StringResponse HandleMapsRequest(unsigned version, bool keep_alive);
    StringResponse HandleMapDataRequest(std::string_view target, unsigned version, bool keep_alive);
    StringResponse HandlePlayersRequest(http::verb method, const http::fields& headers,
                                        unsigned version, bool keep_alive);
    StringResponse HandleStateRequest(http::verb method, const http::fields& headers,
                                      unsigned version, bool keep_alive);
    StringResponse HandleActionRequest(http::verb method, std::string_view body,
                                       const http::fields& headers, unsigned version,
                                       bool keep_alive);
    StringResponse HandleTickRequest(http::verb method, std::string_view body,
                                     const http::fields& headers, unsigned version, bool keep_alive);

   private:
    app::Application& application_;
};
}  // namespace http_handler