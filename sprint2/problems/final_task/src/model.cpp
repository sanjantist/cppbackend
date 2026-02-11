#include "model.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "session.h"

namespace model {
using namespace std::literals;

const Road* Map::FindRoadAt(DogPos pos) const {
    auto h_lower =
        std::lower_bound(horizontal_roads_.begin(), horizontal_roads_.end(), pos.y - Road::WIDTH,
                         [](const RoadSegment& seg, double y) { return seg.fixed_coord < y; });

    for (auto it = h_lower; it != horizontal_roads_.end(); ++it) {
        if (it->fixed_coord > pos.y + Road::WIDTH) break;
        if (pos.x >= it->start - Road::WIDTH && pos.x <= it->end + Road::WIDTH) {
            return it->road;
        }
    }

    auto v_lower =
        std::lower_bound(vertical_roads_.begin(), vertical_roads_.end(), pos.x - Road::WIDTH,
                         [](const RoadSegment& seg, double x) { return seg.fixed_coord < x; });

    for (auto it = v_lower; it != vertical_roads_.end(); ++it) {
        if (it->fixed_coord > pos.x + Road::WIDTH) break;
        if (pos.y >= it->start - Road::WIDTH && pos.y <= it->end + Road::WIDTH) {
            return it->road;
        }
    }

    return nullptr;
}

const MoveResult Map::ProjectMove(DogPos cur_pos, DogVelocity velocity, double time_delta) const {
    DogPos target{cur_pos.x + velocity.vx * time_delta, cur_pos.y + velocity.vy * time_delta};

    std::vector<const Road*> current_roads;
    const double tolerance = Road::WIDTH;

    for (const auto& road : roads_) {
        if (road.IsHorizontal()) {
            int road_y = road.GetStart().y;
            int min_x = std::min(road.GetStart().x, road.GetEnd().x);
            int max_x = std::max(road.GetStart().x, road.GetEnd().x);

            if (std::abs(cur_pos.y - road_y) <= tolerance && cur_pos.x >= min_x - tolerance &&
                cur_pos.x <= max_x + tolerance) {
                current_roads.push_back(&road);
            }
        } else {
            int road_x = road.GetStart().x;
            int min_y = std::min(road.GetStart().y, road.GetEnd().y);
            int max_y = std::max(road.GetStart().y, road.GetEnd().y);

            if (std::abs(cur_pos.x - road_x) <= tolerance && cur_pos.y >= min_y - tolerance &&
                cur_pos.y <= max_y + tolerance) {
                current_roads.push_back(&road);
            }
        }
    }

    if (current_roads.empty()) {
        return {cur_pos, true};
    }

    const Road* target_road = FindRoadAt(target);
    if (target_road) {
        return {target, false};
    }

    DogPos best_pos = cur_pos;
    double best_distance = 0.0;

    for (const Road* road : current_roads) {
        DogPos constrained = target;

        if (road->IsHorizontal()) {
            int road_y = road->GetStart().y;
            int min_x = std::min(road->GetStart().x, road->GetEnd().x);
            int max_x = std::max(road->GetStart().x, road->GetEnd().x);

            constrained.x = std::clamp(target.x, min_x - Road::WIDTH, max_x + Road::WIDTH);
            constrained.y = std::clamp(target.y, road_y - Road::WIDTH, road_y + Road::WIDTH);
        } else {
            int road_x = road->GetStart().x;
            int min_y = std::min(road->GetStart().y, road->GetEnd().y);
            int max_y = std::max(road->GetStart().y, road->GetEnd().y);

            constrained.y = std::clamp(target.y, min_y - Road::WIDTH, max_y + Road::WIDTH);
            constrained.x = std::clamp(target.x, road_x - Road::WIDTH, road_x + Road::WIDTH);
        }

        double dx = constrained.x - cur_pos.x;
        double dy = constrained.y - cur_pos.y;
        double distance = dx * dx + dy * dy;

        if (distance > best_distance) {
            best_distance = distance;
            best_pos = constrained;
        }
    }

    bool stopped = (best_pos.x != target.x || best_pos.y != target.y);
    return {best_pos, stopped};
}

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
    for (const auto& road : roads_) {
        RoadSegment segment{&road, 0, 0, 0};

        if (road.IsHorizontal()) {
            segment.fixed_coord = road.GetStart().y;
            segment.start = std::min(road.GetStart().x, road.GetEnd().x);
            segment.end = std::max(road.GetStart().x, road.GetEnd().x);
            horizontal_roads_.push_back(segment);
        } else {
            segment.fixed_coord = road.GetStart().x;
            segment.start = std::min(road.GetStart().y, road.GetEnd().y);
            segment.end = std::max(road.GetStart().y, road.GetEnd().y);
            vertical_roads_.push_back(segment);
        }
    }

    auto h_comparator = [](const RoadSegment& a, const RoadSegment& b) {
        return a.fixed_coord < b.fixed_coord ||
               (a.fixed_coord == b.fixed_coord && a.start < b.start);
    };
    std::sort(horizontal_roads_.begin(), horizontal_roads_.end(), h_comparator);
    std::sort(vertical_roads_.begin(), vertical_roads_.end(), h_comparator);
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
