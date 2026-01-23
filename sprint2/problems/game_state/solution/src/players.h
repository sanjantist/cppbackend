#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "session.h"
#include "player.h"

namespace app {
class Players {
   public:
    model::Player::Id AddPlayer(std::shared_ptr<model::GameSession> session, std::string name);
    const model::Player* Find(model::Player::Id id) const noexcept;
    const std::vector<model::Player::Id>* ListInSession(
        const std::shared_ptr<model::GameSession>& session) const;

   private:
    model::Player::Id next_id_ = 0;
    std::unordered_map<model::Player::Id, model::Player> players_;
    std::unordered_map<std::shared_ptr<model::GameSession>, std::vector<model::Player::Id>>
        session_to_players_;
};
}  // namespace app