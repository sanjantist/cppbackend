#include "players.h"

namespace app {
model::Player::Id Players::AddPlayer(std::shared_ptr<model::GameSession> session,
                                     std::string name) {
    const auto id = next_id_++;
    players_.emplace(id, model::Player{id, std::move(name), session});
    session_to_players_[std::move(session)].push_back(id);
    return id;
}
const model::Player* Players::Find(model::Player::Id id) const noexcept {
    if (auto it = players_.find(id); it != players_.end()) return &it->second;
    return nullptr;
}
const std::vector<model::Player::Id>* Players::ListInSession(
    const std::shared_ptr<model::GameSession>& session) const {
    if (auto it = session_to_players_.find(session); it != session_to_players_.end())
        return &it->second;
    return nullptr;
}
}  // namespace app