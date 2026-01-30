#include "application.h"

#include "model.h"

namespace app {
Application::Application(model::Game game) : game_(std::move(game)) {}

Application::JoinResult Application::JoinGame(std::string user_name, std::string map_id_str) {
    model::Map::Id map_id{map_id_str};

    const auto* map = game_.FindMap(map_id);
    if (!map) {
        throw std::runtime_error("mapNotFound");
    }

    auto session = game_.GetOrCreateSession(map_id);
    int player_id = players_.AddPlayer(session, std::move(user_name));

    std::string token = *tokens_.Issue(player_id);

    return JoinResult{std::move(token), player_id};
}

std::optional<int> Application::Authorize(std::string_view token) const {
    return tokens_.FindPlayerId(token);
}

boost::json::object Application::GetPlayersJson(int player_id) const {
    boost::json::object out;

    auto* me = players_.Find(player_id);
    if (!me) return out;

    auto session = me->LockSession();
    if (!session) return out;

    const auto* ids = players_.ListInSession(session);
    if (!ids) return out;

    for (int id : *ids) {
        if (auto* p = players_.Find(id)) {
            boost::json::object player;
            player["name"] = p->GetName();
            out[std::to_string(id)] = std::move(player);
        }
    }
    return out;
}

const std::vector<model::Map>& Application::GetMaps() const { return game_.GetMaps(); }

const model::Map* Application::FindMap(const model::Map::Id& id) const { return game_.FindMap(id); }

const model::Player* Application::GetPlayer(int player_id) const {
    return players_.Find(player_id);
}

model::Player* Application::GetPlayer(int player_id) { return players_.Find(player_id); }

// application.cpp
std::optional<std::vector<int>> Application::GetPlayers(std::string_view token) const {
    auto maybe_player_id = tokens_.FindPlayerId(token);
    if (!maybe_player_id) {
        return std::nullopt;
    }

    const int player_id = *maybe_player_id;
    auto* me = players_.Find(player_id);
    if (!me) {
        return std::nullopt;
    }

    auto session = me->LockSession();
    if (!session) {
        return std::nullopt;
    }

    const auto* ids = players_.ListInSession(session);
    if (!ids) {
        return std::nullopt;
    }

    return std::vector<int>(ids->begin(), ids->end());
}

void Application::Move(double dt) {
    auto maps = GetMaps();
    for (const auto& map : maps) {
        auto session = game_.GetSession(map.GetId());
        if (session) {
            auto* players = players_.ListInSession(session);
            for (const int player_id : *players) {
                auto& dog = players_.Find(player_id)->GetDog();

                model::DogPos curr_pos = dog.GetPosition();
                model::DogPos desired;

                desired.x = curr_pos.x + dt * dog.GetVelocity().vx;
                desired.y = curr_pos.y + dt * dog.GetVelocity().vy;

                model::DogPos projected = map.ProjectMovement(curr_pos, desired);
                dog.SetPosition(projected);
                if (projected.x != desired.x || projected.y != desired.y) {
                    dog.SetVelocity({0.0, 0.0});
                }
            }
        }
    }
}

}  // namespace app