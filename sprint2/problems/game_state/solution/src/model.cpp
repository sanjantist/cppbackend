#include "model.h"

#include <stdexcept>

#include "session.h"

namespace model {
using namespace std::literals;

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

std::shared_ptr<GameSession> Game::GetOrCreateSession(const Map::Id& map_id) {
    const Map* map = FindMap(map_id);
    if (!map) return {};

    if (auto it = sessions_by_map_.find(map_id); it != sessions_by_map_.end()) return it->second;

    auto session = std::make_shared<GameSession>(map);
    sessions_by_map_.emplace(map_id, session);
    return session;
}

}  // namespace model
