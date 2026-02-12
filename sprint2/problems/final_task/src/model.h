#pragma once
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "tagged.h"

namespace model {

using DogCoord = double;
using Velocity = double;

enum class DogDirection { North, South, East, West };

struct DogPos {
    DogCoord x;
    DogCoord y;
};

struct DogVelocity {
    Velocity vx = 0.0;
    Velocity vy = 0.0;
};

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

struct MoveResult {
    DogPos new_pos;
    bool stopped_by_boundary = false;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

   public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    static constexpr double WIDTH = 0.4;

    Road(HorizontalTag, Point start, Coord end_x) noexcept : start_{start}, end_{end_x, start.y} {}

    Road(VerticalTag, Point start, Coord end_y) noexcept : start_{start}, end_{start.x, end_y} {}

    bool IsHorizontal() const noexcept { return start_.y == end_.y; }

    bool IsVertical() const noexcept { return start_.x == end_.x; }

    Point GetStart() const noexcept { return start_; }

    Point GetEnd() const noexcept { return end_; }

   private:
    Point start_;
    Point end_;
};

class Building {
   public:
    explicit Building(Rectangle bounds) noexcept : bounds_{bounds} {}

    const Rectangle& GetBounds() const noexcept { return bounds_; }

   private:
    Rectangle bounds_;
};

class Office {
   public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}, position_{position}, offset_{offset} {}

    const Id& GetId() const noexcept { return id_; }

    Point GetPosition() const noexcept { return position_; }

    Offset GetOffset() const noexcept { return offset_; }

   private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
   public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept : id_(std::move(id)), name_(std::move(name)) {}

    const Id& GetId() const noexcept { return id_; }

    const std::string& GetName() const noexcept { return name_; }

    const Buildings& GetBuildings() const noexcept { return buildings_; }

    const Roads& GetRoads() const noexcept { return roads_; }

    const Offices& GetOffices() const noexcept { return offices_; }

    const std::optional<double> GetDogSpeed() const noexcept { return dog_speed_; }

    const Road* FindRoadAt(DogPos pos) const;

    const MoveResult ProjectMove(DogPos cur_pos, DogVelocity velocity, double time_delta) const;

    void AddRoad(const Road& road) { roads_.emplace_back(road); }

    void AddBuilding(const Building& building) { buildings_.emplace_back(building); }

    void AddOffice(Office office);

    void BuildRoadIndex();

    void SetDogSpeed(double speed) { dog_speed_ = speed; }

   private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    struct RoadSegment {
        const Road* road;
        int fixed_coord;
        int start, end;
    };

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    std::optional<double> dog_speed_;

    std::vector<RoadSegment> horizontal_roads_;
    std::vector<RoadSegment> vertical_roads_;
};

class GameSession;

class Game {
   public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept { return maps_; }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    const double GetDefaultDogSpeed() const noexcept { return default_dog_speed_; }

    void SetDefaultDogSpeed(double speed) { default_dog_speed_ = speed; }

    std::shared_ptr<GameSession> GetOrCreateSession(const Map::Id& map_id);
    std::shared_ptr<GameSession> GetSession(const Map::Id& map_id) {
        if (auto it = sessions_by_map_.find(map_id); it != sessions_by_map_.end()) {
            return it->second;
        }
        return nullptr;
    }

   private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    std::unordered_map<Map::Id, std::shared_ptr<GameSession>, MapIdHasher> sessions_by_map_;

    double default_dog_speed_ = 1.0;
};

}  // namespace model
