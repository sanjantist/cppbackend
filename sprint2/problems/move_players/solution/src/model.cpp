#include "model.h"

#include <algorithm>
#include <cmath>
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
        offices_.pop_back();
        throw;
    }
}

void Map::BuildRoadIndex() {
    for (const Road& road : roads_) {
        if (road.IsHorizontal()) {
            int y = road.GetStart().y;
            double x1 = road.GetStart().x;
            double x2 = road.GetEnd().x;

            horizontal_roads_[y].push_back({x1, x2});
        } else {
            int x = road.GetStart().x;
            double y1 = road.GetStart().y;
            double y2 = road.GetEnd().y;

            vertical_roads_[x].push_back({y1, y2});
        }
    }

    for (auto& [_, v] : horizontal_roads_) {
        std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.from < b.from; });
    }
    for (auto& [_, v] : vertical_roads_) {
        std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.from < b.from; });
    }
}

DogPos Map::ProjectMovement(DogPos from, DogPos to) const {
    DogPos result = from;
    auto horizontal_seg = FindHorizontalRoad(from);
    if (horizontal_seg) {
        result.x = std::clamp(to.x, horizontal_seg->from - 0.4, horizontal_seg->to + 0.4);
    }

    auto vertical_seg = FindVerticalRoad(from);
    if (vertical_seg) {
        result.y = std::clamp(to.y, vertical_seg->from - 0.4, vertical_seg->to + 0.4);
    }

    return result;
}

const Map::RoadSegment* Map::FindVerticalRoad(DogPos pos) const {
    int x0 = static_cast<int>(std::round(pos.x));

    for (int x : {x0, x0 - 1, x0 + 1}) {
        auto it = vertical_roads_.find(x);
        if (it == vertical_roads_.end()) continue;

        if (std::abs(pos.x - x) > 0.4) continue;

        for (const auto& seg : it->second) {
            if (pos.y >= seg.from - 0.4 && pos.y <= seg.to + 0.4) {
                return &seg;
            }
        }
    }
    return nullptr;
}

const Map::RoadSegment* Map::FindHorizontalRoad(DogPos pos) const {
    int y0 = static_cast<int>(std::round(pos.y));

    for (int y : {y0, y0 - 1, y0 + 1}) {
        auto it = horizontal_roads_.find(y);
        if (it == horizontal_roads_.end()) continue;

        if (std::abs(pos.y - y) > 0.4) continue;

        for (const auto& seg : it->second) {
            if (pos.x >= seg.from - 0.4 && pos.x <= seg.to + 0.4) {
                return &seg;
            }
        }
    }
    return nullptr;
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
