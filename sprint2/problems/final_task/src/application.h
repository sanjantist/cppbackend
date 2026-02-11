#pragma once

#include <boost/json.hpp>
#include <chrono>

#include "model.h"
#include "player.h"
#include "player_tokens.h"
#include "players.h"

namespace app {
class Application {
   public:
    explicit Application(model::Game game, bool randomize_dog_spawn, bool autotick);

    struct JoinResult {
        std::string auth_token;
        int player_id;
    };

    JoinResult JoinGame(std::string user_name, std::string map_id);

    std::optional<int> Authorize(std::string_view token) const;

    boost::json::object GetPlayersJson(int player_id) const;

    const std::vector<model::Map>& GetMaps() const;
    const model::Map* FindMap(const model::Map::Id& id) const;
    std::optional<std::vector<int>> GetPlayers(std::string_view token) const;
    const model::Player* GetPlayer(int player_id) const;
    model::Player* GetPlayer(int player_id);

    model::Game& GetGame() noexcept { return game_; }
    const model::Game& GetGame() const noexcept { return game_; }

    const bool IsAutotick() const { return autotick_; }

    void Tick(std::chrono::milliseconds delta);

   private:
    model::Game game_;
    Players players_;
    PlayerTokens tokens_;
    bool randomize_dog_spawn_;
    bool autotick_;
};
}  // namespace app