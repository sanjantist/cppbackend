#pragma once

#include <memory>

namespace model {

class Map;

class GameSession {
   public:
    explicit GameSession(const Map* map);
    const Map& GetMap() const noexcept;

   private:
    const Map* map_;
};

class Dog {
   public:
    explicit Dog(std::string name);
    const std::string& GetName() const noexcept;

   private:
    std::string name_;
};

class Player {
   public:
    using Id = int;

    Player(Id id, std::string name, std::weak_ptr<GameSession> session);

    Id GetId() const noexcept;
    const std::string& GetName() const noexcept;

    std::shared_ptr<GameSession> LockSession() const noexcept;

   private:
    Id id_;
    Dog dog_;
    std::weak_ptr<GameSession> session_;
};
}  // namespace model