#pragma once
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <string_view>

#include "http_types.h"
#include "model.h"
#include "player_tokens.h"
#include "players.h"

namespace http_handler {
class ApiHandler {
   public:
    explicit ApiHandler(model::Game& game, app::Players& players, app::PlayerTokens& tokens)
        : game_(game), players_(players), tokens_(tokens) {}

    template <typename Body, typename Allocator>
    StringResponse Handle(http::request<Body, http::basic_fields<Allocator>>&& req) {
        return HandleImpl(req.method(), req.target(), req.body(), req.base(), req.version(),
                          req.keep_alive());
    }

   private:
    StringResponse HandleImpl(http::verb method, std::string_view target, std::string_view body,
                              const http::fields& headers, unsigned int version, bool keep_alive);
    StringResponse HandleJoinRequest(http::verb method, std::string_view body,
                                     const http::fields& headers, unsigned int version,
                                     bool keep_alive);
    StringResponse HandleMapsRequest(unsigned int version, bool keep_alive);
    StringResponse HandleMapDataRequest(std::string_view target, unsigned int version,
                                        bool keep_alive);
    StringResponse HandlePlayersRequest(http::verb method, const http::fields& headers,
                                        unsigned int version, bool keep_alive);

   private:
    model::Game& game_;
    app::Players& players_;
    app::PlayerTokens& tokens_;
};
}  // namespace http_handler