#pragma once

namespace model {

class Map;

class GameSession {
   public:
    explicit GameSession(const Map* map);
    const Map& GetMap() const noexcept;

   private:
    const Map* map_;
};
}  // namespace model