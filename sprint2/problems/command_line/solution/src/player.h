#pragma once

#include <memory>
#include <random>
#include <string>

#include "dog.h"
#include "session.h"

namespace model {
class Player {
   public:
    using Id = int;

    Player(Id id, std::string name, std::weak_ptr<GameSession> session, bool randomize_dog_spawn);

    Id GetId() const noexcept;
    const std::string& GetName() const noexcept;
    const Dog& GetDog() const noexcept;
    Dog& GetDog() noexcept;

    std::shared_ptr<GameSession> LockSession() const noexcept;

   private:
    Id id_;
    Dog dog_;
    std::weak_ptr<GameSession> session_;

    std::mt19937 gen_;
};
}  // namespace model